#!/usr/bin/env python3
# Copyright 2026 European Centre for Medium-Range Weather Forecasts (ECMWF)
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

"""
Migrate a DASI database written by an old pydasi/libdasi into a new (v0.3.3) one.

Two libdasi versions cannot coexist in one process, so each worker runs as:

    reader (old python + old libdasi) --pipe--> writer (new python + new libdasi)

Data only travels through in-memory pipes; nothing is written to temporary files.

- Resumable: objects already listed in the new DB are skipped before their data is read.
- Bounded memory: FDB keeps a 64 MiB buffered data handle open per product until the session is
  closed, so the writer reopens its session once those buffers reach --reopen-mib.
- Throughput for small products is bound by FDB syncing data, index and toc once per product.
- Parallel: --workers N; each new database (e.g. one month) is owned by exactly one worker,
  since FDB does not guard concurrent creation of the same database across processes.

Example:

    python3 migrate_dasi.py run --transform dafab --workers 4 \\
        --old-pythonpath /tmp/build/bundle/old/data-tools-lib/python3.11/site-packages \\
        --old-lib-dir /tmp/build/old-deps-install/lib64 \\
        --new-pythonpath /tmp/build/pydasi-0.3.3 \\
        --new-lib-dir /tmp/build/bundle/0.3.3/dasi/lib:/tmp/build/bundle/0.3.3/lib \\
        --old-config assets.yml --new-config dasi.yaml \\
        --query source=CDSE --query source=MPC
"""

import argparse
import json
import os
import resource
import struct
import subprocess
import sys
import time
import zlib

HEADER_LEN = struct.Struct(">I")
DATA_LEN = struct.Struct(">Q")
FDB_BUFFER = 64 * 2**20  # fdbBufferSize: per-product write buffer in TocStore

LOG_TAG = ""


def log(msg: str) -> None:
    print(f"[migrate{LOG_TAG}:{os.getpid()}] {msg}", file=sys.stderr, flush=True)


def parse_query(text: str) -> dict:
    """'k1=v1/v2,k2=v3' -> {'k1': ['v1', 'v2'], 'k2': ['v3']}"""
    query = {}
    for part in filter(None, (p.strip() for p in text.split(","))):
        keyword, _, values = part.partition("=")
        if not keyword or not values:
            raise ValueError(f"invalid query element: '{part}'")
        query[keyword.strip()] = [v.strip() for v in values.split("/")]
    return query


def key_to_dict(key) -> dict:
    # repr() is "<k1:v1><k2:v2>..." in all pydasi versions; avoids relying on newer Key APIs
    out = {}
    for item in repr(key)[1:-1].split("><"):
        if item:
            keyword, _, value = item.partition(":")
            out[keyword] = value
    return out


def canonical(key: dict) -> str:
    return json.dumps(key, sort_keys=True, separators=(",", ":"))


# ----------------------------------------------------------------------------------------------------------------------
# Key transforms: old key -> new key, plus how new keys group into products and databases


class Transform:
    """Identity mapping; every object is its own product and database."""

    def key(self, old: dict) -> dict:
        return old

    def query(self, query: dict) -> dict:
        """Old-schema query -> query usable on the new DB."""
        return query

    def product(self, new: dict) -> str:
        return canonical(new)

    def database(self, new: dict) -> str:
        return canonical(new)


class DafabTransform(Transform):
    """Old schema.metadata/schema.assets -> schema.dasi."""

    PER_ASSET = ("asset_name", "mediatype")

    def key(self, old: dict) -> dict:
        new = {k: v.lower() for k, v in old.items()}
        if "asset_name" not in new:
            new["asset_name"] = "metadata"
            new["mediatype"] = "application_json"
        if "collection" not in new:
            source, mission, level = new.get("source"), new.get("mission", ""), new.get("level", "")
            if source == "cdse" and mission.startswith("s2") and level.startswith("msi"):
                new["collection"] = "sentinel-2-" + level[3:]
            elif source == "mpc" and mission.startswith("s1"):
                new["collection"] = "sentinel-1-rtc"
            else:
                raise ValueError(f"cannot derive 'collection' for {old}")
        return new

    def query(self, query: dict) -> dict:
        return {k: [v.lower() for v in values] for k, values in query.items()}

    def product(self, new: dict) -> str:
        return canonical({k: v for k, v in new.items() if k not in self.PER_ASSET})

    def database(self, new: dict) -> str:
        # schema.dasi first level; takedate is YearMonth there
        if new["mission"].startswith("s2"):
            names = ("source", "mission", "collection", "level")
        else:
            names = ("source", "mission", "collection", "mode", "type")
        return ":".join([new[n] for n in names] + [new["takedate"][:7]])


TRANSFORMS = {"none": Transform(), "dafab": DafabTransform()}


def shard(database: str, workers: int) -> int:
    return zlib.crc32(database.encode()) % workers


# ----------------------------------------------------------------------------------------------------------------------


def protect_stdout():
    """libdasi/eckit may log to fd 1: move it to stderr and keep a private fd for data."""
    sys.stdout.flush()
    data_fd = os.dup(1)
    os.dup2(2, 1)
    return data_fd


def open_dasi(config: str, quiet: bool = False):
    import pydasi

    if os.path.isfile(config):
        # libdasi resolves a relative 'schema:' against the cwd, not the config file
        config = os.path.abspath(config)
        os.chdir(os.path.dirname(config))
    dasi = pydasi.Dasi(config)
    if not quiet:
        with open("/proc/self/maps") as maps:
            libs = sorted({line.split()[-1] for line in maps if "libdasi" in line})
        log(f"pydasi {getattr(pydasi, 'Version', '?')} from {os.path.dirname(pydasi.__file__)}, libdasi: {libs}")
        log(f"FDB_INDEX_TYPE={os.environ.get('FDB_INDEX_TYPE', '<unset>')}, config: {config}")
    return dasi


def readinto_exact(stream, size: int) -> bytearray:
    buf = bytearray(size)
    view = memoryview(buf)
    got = 0
    while got < size:
        n = stream.readinto(view[got:])
        if not n:
            raise EOFError(f"unexpected end of stream ({got}/{size} bytes)")
        got += n
    return buf


# ----------------------------------------------------------------------------------------------------------------------


def cmd_scan(args) -> int:
    """(new env) print canonical keys already present in the new DB, one per line."""
    data_fd = protect_stdout()
    dasi = open_dasi(args.config)
    transform = TRANSFORMS[args.transform]
    total = 0
    with os.fdopen(data_fd, "w") as out:
        for qtext in args.query:
            query = transform.query(parse_query(qtext))
            for item in dasi.list(query):
                out.write(canonical(key_to_dict(item.key)) + "\n")
                total += 1
    log(f"scan: {total} objects already in new DB")
    return 0


def cmd_read(args) -> int:
    data_fd = protect_stdout()
    transform = TRANSFORMS[args.transform]

    skip = set()
    if args.skip_from_stdin:
        skip = {line.rstrip("\n") for line in sys.stdin}
        log(f"{len(skip)} objects already migrated in this shard")

    dasi = open_dasi(args.config, quiet=args.worker > 0)

    todo = []
    for qtext in args.query:
        query = parse_query(qtext)
        listed = skipped = 0
        for item in dasi.list(query):
            old = key_to_dict(item.key)
            new = transform.key(old)
            database = transform.database(new)
            if shard(database, args.workers) != args.worker:
                continue
            listed += 1
            if canonical(new) in skip:
                skipped += 1
                continue
            todo.append((database, transform.product(new), canonical(new), old, new))
        log(f"{query}: {listed} objects in shard, {skipped} already migrated")

    # Contiguous products let the writer bound its open handles; contiguous DBs keep few DBs open
    todo.sort(key=lambda t: t[:3])
    log(f"to migrate: {len(todo)} objects, {len({t[1] for t in todo})} products, {len({t[0] for t in todo})} DBs")

    total = total_bytes = 0
    with os.fdopen(data_fd, "wb", buffering=0) as out:
        for _, product, _, old, new in todo:
            if total < args.show_keys:
                log(f"key {old} -> {new}")
            found = False
            for item in dasi.retrieve({k: [v] for k, v in old.items()}):
                data = item.data
                header = json.dumps({"key": new, "product": product}).encode()
                out.write(HEADER_LEN.pack(len(header)) + header + DATA_LEN.pack(len(data)))
                out.write(data)
                total += 1
                total_bytes += len(data)
                found = True
                break
            if not found:
                log(f"WARNING: listed but not retrievable: {old}")

        out.write(HEADER_LEN.pack(0))

    log(f"read {total} objects, {total_bytes} bytes")
    return 0


def cmd_write(args) -> int:
    protect_stdout()
    stream = sys.stdin.buffer
    dasi = open_dasi(args.config, quiet=args.worker > 0)

    total = total_bytes = products = 0
    current = None
    since_reopen = 0
    product_bytes = 0
    buffered = 0  # estimated resident FDB write buffers since the last reopen
    reopen_bytes = args.reopen_mib * 2**20
    start = time.monotonic()

    def report(final=False):
        secs = max(time.monotonic() - start, 1e-9)
        prefix = "(dry-run) " if args.dry_run else ""
        log(f"{prefix}{'done: ' if final else ''}archived {total} objects, {products} products, "
            f"{total_bytes / 2**30:.1f} GiB, {total_bytes / 2**20 / secs:.0f} MiB/s")

    try:
        while True:
            (hlen,) = HEADER_LEN.unpack(readinto_exact(stream, HEADER_LEN.size))
            if hlen == 0:
                break
            header = json.loads(readinto_exact(stream, hlen))
            (dlen,) = DATA_LEN.unpack(readinto_exact(stream, DATA_LEN.size))
            data = readinto_exact(stream, dlen)

            if header["product"] != current:
                if current is not None:
                    products += 1
                    since_reopen += 1
                    buffered += min(product_bytes, FDB_BUFFER)
                    if products % args.report_every == 0:
                        report()
                current = header["product"]
                product_bytes = 0
                if (buffered >= reopen_bytes or since_reopen >= args.reopen_every) and not args.dry_run:
                    # flush() keeps data handles open; only closing the session frees their buffers
                    dasi.flush()
                    dasi = None
                    dasi = open_dasi(args.config, quiet=True)
                    since_reopen = 0
                    buffered = 0

            if not args.dry_run:
                dasi.archive(header["key"], data)
            total += 1
            total_bytes += dlen
            product_bytes += dlen
            del data
    finally:
        if dasi is not None and not args.dry_run:
            dasi.flush()
        if current is not None:
            products += 1
        report(final=True)
    return 0


def cmd_run(args) -> int:
    script = os.path.abspath(__file__)
    transform = TRANSFORMS[args.transform]

    def env_for(lib_dir, pythonpath, index_type):
        env = dict(os.environ)
        # Drop inherited settings so each side only sees its own pydasi/libdasi
        for var in ("DASI_DIR", "dasi_DIR", "PYTHONPATH", "FDB_INDEX_TYPE"):
            env.pop(var, None)
        if lib_dir:
            env["DASI_DIR"] = lib_dir
            env["LD_LIBRARY_PATH"] = lib_dir + os.pathsep + os.environ.get("LD_LIBRARY_PATH", "")
        if pythonpath:
            env["PYTHONPATH"] = pythonpath
        if index_type:
            env["FDB_INDEX_TYPE"] = index_type
        return env

    old_env = env_for(args.old_lib_dir, args.old_pythonpath, args.old_index_type)
    new_env = env_for(args.new_lib_dir, args.new_pythonpath, args.new_index_type)
    queries = [a for q in args.query for a in ("--query", q)]
    common = ["--transform", args.transform, "--workers", str(args.workers)] + queries

    # Already migrated keys, split per worker so each reader only holds its own shard
    skip = [[] for _ in range(args.workers)]
    if not args.no_skip:
        scan = subprocess.run(
            [args.new_python, script, "scan", "--config", args.new_config, "--transform", args.transform] + queries,
            stdout=subprocess.PIPE, env=new_env, text=True, check=True,
        )
        for line in scan.stdout.splitlines():
            skip[shard(transform.database(json.loads(line)), args.workers)].append(line.encode() + b"\n")
        del scan

    workers = []
    for w in range(args.workers):
        reader_cmd = [args.old_python, script, "read", "--config", args.old_config, "--worker", str(w),
                      "--show-keys", str(args.show_keys)] + common
        if not args.no_skip:
            reader_cmd.append("--skip-from-stdin")
        writer_cmd = [args.new_python, script, "write", "--config", args.new_config, "--worker", str(w),
                      "--reopen-mib", str(args.reopen_mib), "--reopen-every", str(args.reopen_every),
                      "--report-every", str(args.report_every)]
        if args.dry_run:
            writer_cmd.append("--dry-run")

        reader = subprocess.Popen(reader_cmd, stdin=subprocess.PIPE, stdout=subprocess.PIPE, env=old_env)
        writer = subprocess.Popen(writer_cmd, stdin=reader.stdout, env=new_env)
        reader.stdout.close()  # writer owns the read end; lets reader get SIGPIPE if writer dies
        workers.append((w, reader, writer))

    for w, reader, _ in workers:
        reader.stdin.writelines(skip[w])
        reader.stdin.close()
    del skip

    failed = 0
    for w, reader, writer in workers:
        wrc = writer.wait()
        rrc = reader.wait()
        log(f"worker {w}: reader exit={rrc}, writer exit={wrc}")
        failed += rrc != 0 or wrc != 0
    log(f"peak RSS of largest child: {resource.getrusage(resource.RUSAGE_CHILDREN).ru_maxrss / 2**10:.0f} MiB")
    return 1 if failed else 0


# ----------------------------------------------------------------------------------------------------------------------


def main() -> int:
    global LOG_TAG

    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    sub = parser.add_subparsers(dest="cmd", required=True)

    query_help = "old-schema query, e.g. 'source=CDSE' or 'User=alice,Project=p1/p2' (repeatable)"

    p_scan = sub.add_parser("scan", help="(new env) print keys already in the new DB")
    p_scan.add_argument("--config", required=True)
    p_scan.add_argument("--query", action="append", required=True, help=query_help)
    p_scan.add_argument("--transform", choices=TRANSFORMS, default="none")
    p_scan.set_defaults(func=cmd_scan)

    p_read = sub.add_parser("read", help="(old env) stream one shard of objects to stdout")
    p_read.add_argument("--config", required=True)
    p_read.add_argument("--query", action="append", required=True, help=query_help)
    p_read.add_argument("--transform", choices=TRANSFORMS, default="none")
    p_read.add_argument("--workers", type=int, default=1)
    p_read.add_argument("--worker", type=int, default=0)
    p_read.add_argument("--skip-from-stdin", action="store_true")
    p_read.add_argument("--show-keys", type=int, default=0)
    p_read.set_defaults(func=cmd_read)

    p_write = sub.add_parser("write", help="(new env) archive objects from stdin")
    p_write.add_argument("--config", required=True)
    p_write.add_argument("--worker", type=int, default=0)
    p_write.add_argument("--reopen-mib", type=int, default=256)
    p_write.add_argument("--reopen-every", type=int, default=1000)
    p_write.add_argument("--report-every", type=int, default=100)
    p_write.add_argument("--dry-run", action="store_true")
    p_write.set_defaults(func=cmd_write)

    p_run = sub.add_parser("run", help="scan new DB, then spawn N x (reader (old) | writer (new))")
    p_run.add_argument("--old-python", default=sys.executable)
    p_run.add_argument("--new-python", default=sys.executable)
    p_run.add_argument("--old-lib-dir", help="old libdasi + eckit/metkit/fdb lib dirs (':'-separated)")
    p_run.add_argument("--new-lib-dir", help="new (v0.3.3) libdasi + eckit/metkit/fdb lib dirs (':'-separated)")
    p_run.add_argument("--old-pythonpath", help="PYTHONPATH for the old pydasi")
    p_run.add_argument("--new-pythonpath", help="PYTHONPATH for the new pydasi")
    p_run.add_argument("--old-index-type", help="FDB_INDEX_TYPE for the reader (default: unset)")
    p_run.add_argument("--new-index-type", default="BTreeIndex64", help="FDB_INDEX_TYPE for the writer")
    p_run.add_argument("--old-config", required=True)
    p_run.add_argument("--new-config", required=True)
    p_run.add_argument("--query", action="append", required=True, help=query_help)
    p_run.add_argument("--transform", choices=TRANSFORMS, default="none", help="key mapping old -> new schema")
    p_run.add_argument("--workers", type=int, default=1, help="parallel reader|writer pairs, sharded by new DB")
    p_run.add_argument("--reopen-mib", type=int, default=256,
                       help="reopen the writer session once its FDB write buffers reach ~N MiB (memory per writer)")
    p_run.add_argument("--reopen-every", type=int, default=1000,
                       help="also reopen after N products (caps open files and in-memory indexes)")
    p_run.add_argument("--report-every", type=int, default=100, help="log progress every N products")
    p_run.add_argument("--no-skip", action="store_true", help="do not skip objects already in the new DB")
    p_run.add_argument("--dry-run", action="store_true", help="read everything but do not archive")
    p_run.add_argument("--show-keys", type=int, default=0, help="log the first N old -> new key mappings")
    p_run.set_defaults(func=cmd_run)

    args = parser.parse_args()
    for name in ("workers", "reopen_mib", "reopen_every", "report_every"):
        if getattr(args, name, 1) < 1:
            parser.error(f"--{name.replace('_', '-')} must be >= 1")
    if args.cmd in ("read", "write"):
        LOG_TAG = f"/w{args.worker}"
    return args.func(args)


if __name__ == "__main__":
    sys.exit(main())
