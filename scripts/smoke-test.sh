#!/usr/bin/env bash
set -euo pipefail

test "$(dasi version)" = "$DASI_EXPECTED_VERSION"
python - <<'PY'
import os
from pathlib import Path
from tempfile import TemporaryDirectory

from pydasi import Config, Dasi, Version

assert Version == os.environ["DASI_EXPECTED_VERSION"]
with TemporaryDirectory() as directory:
    root = Path(directory)
    schema = root / "schema"
    schema.write_text("[a [b [c]]]\n")
    config = Config().default(schema, root).dump
    key = {"a": "one", "b": "two", "c": "three"}
    writer = Dasi(config)
    writer.archive(key, b"runtime smoke test")
    del writer
    reader = Dasi(config)
    results = reader.retrieve({name: [value] for name, value in key.items()})
    assert len(results) == 1
    assert next(iter(results)).data == b"runtime smoke test"
print("Runtime version and archive/retrieve checks passed")
PY