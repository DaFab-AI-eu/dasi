# Copyright 2023 European Centre for Medium-Range Weather Forecasts (ECMWF)
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

"""Rucio S3/MinIO RSE protocol implementation using boto3.

This is a standalone, top-level module (it is intentionally NOT part of the
``pydasi`` package) so that it can be imported by ``rucio-server`` for
deterministic PFN construction (``lfns2pfns``) without pulling in the rest of
the pydasi client stack. The module must be on the server's ``PYTHONPATH``.

Register it as the RSE protocol impl:
    rucio_s3boto3.Default
"""

import json
import logging
import os
import tempfile
from typing import TYPE_CHECKING

# boto3/botocore are imported lazily so this module can be loaded server-side
# (e.g. by rucio-server for deterministic PFN construction via lfns2pfns) in
# environments where boto3 is not installed. Only the data-transfer methods
# (connect/get/put/exists/stat/rename/delete) require boto3; lfns2pfns and
# pfn2path are pure path computations that do not.
try:
    import boto3
    from botocore.config import Config
except ImportError:  # pragma: no cover - server-side without boto3
    boto3 = None
    Config = None

from rucio.common import exception
from rucio.common.checksum import adler32
from rucio.rse.protocols import protocol

if TYPE_CHECKING:
    from botocore.exceptions import ClientError as _ClientError
else:
    try:
        from botocore.exceptions import ClientError as _ClientError
    except ImportError:  # pragma: no cover - server-side without boto3

        class _ClientError(Exception):
            """Placeholder so transfer methods can be defined without botocore."""


class Default(protocol.RSEProtocol):
    """Rucio RSE protocol for S3-compatible storage using boto3."""

    def __init__(self, protocol_attr, rse_settings, logger=logging.log):
        super().__init__(protocol_attr, rse_settings, logger=logger)

        # Prefer env vars first; fallback to protocol extended attributes.
        ext = self.attributes.get("extended_attributes") or {}
        if isinstance(ext, str):
            try:
                ext = json.loads(ext)
            except Exception:
                ext = {}

        access_key = os.environ.get("AWS_ACCESS_KEY_ID") or ext.get("s3_access_key") or "minioadmin"
        secret_key = os.environ.get("AWS_SECRET_ACCESS_KEY") or ext.get("s3_secret_key") or "minioadmin"

        hostname = self.attributes["hostname"]
        port = self.attributes.get("port", 9000)
        scheme = "https" if port == 443 else "http"
        endpoint_url = ext.get("s3_endpoint_url") or os.environ.get("MINIO_HOST") or f"{scheme}://{hostname}:{port}"
        region_name = os.environ.get("MINIO_REGION") or os.environ.get("AWS_DEFAULT_REGION")

        # TLS verification. The local dev MinIO serves HTTPS with a self-signed
        # certificate, so verification is disabled by default for https
        # endpoints. Set MINIO_VERIFY_TLS=true (or a CA bundle path) to enforce.
        verify_env = os.environ.get("MINIO_VERIFY_TLS")
        if verify_env is not None and verify_env.strip():
            value = verify_env.strip()
            if value.lower() in ("0", "false", "no", "off"):
                verify = False
            elif value.lower() in ("1", "true", "yes", "on"):
                verify = True
            else:
                verify = value  # treat as a CA bundle path
        elif endpoint_url.lower().startswith("https"):
            verify = False
        else:
            verify = None

        # Defer boto3 client creation until a transfer is actually requested so
        # that PFN construction works even where boto3 is unavailable.
        self._s3_access_key = access_key
        self._s3_secret_key = secret_key
        self._s3_endpoint_url = endpoint_url
        self._s3_region_name = region_name
        self._s3_verify = verify
        self._s3_client = None

        # Prefix convention is /<bucket>[/optional/root/prefix]/
        prefix = self.attributes.get("prefix", "/").strip("/")
        if not prefix:
            prefix = os.environ.get("MINIO_BUCKET", "").strip("/")

        parts = prefix.split("/", 1)
        self._bucket = parts[0]
        if not self._bucket:
            raise exception.RSEAccessDenied(
                "No S3 bucket configured. Set protocol prefix to /<bucket>/ or define MINIO_BUCKET."
            )
        self._root = parts[1].rstrip("/") + "/" if len(parts) > 1 and parts[1] else ""

    @property
    def _s3(self):
        """Lazily create and cache the boto3 S3 client.

        boto3 is only required for actual data transfers, which run client-side.
        """
        if self._s3_client is None:
            if boto3 is None or Config is None:
                raise exception.RSEAccessDenied(
                    "boto3 is required for S3 transfers but is not installed in this environment."
                )
            self._s3_client = boto3.client(
                "s3",
                endpoint_url=self._s3_endpoint_url,
                aws_access_key_id=self._s3_access_key,
                aws_secret_access_key=self._s3_secret_key,
                region_name=self._s3_region_name,
                verify=self._s3_verify,
                config=Config(signature_version="s3v4"),
            )
        return self._s3_client

    def _pfn_to_key(self, pfn: str) -> str:
        """Convert a PFN like s3://host/bucket/path/file to an S3 object key."""
        parsed = list(self.parse_pfns(pfn).values())[0]
        path = parsed.get("path", "").strip("/")
        name = parsed.get("name", "")
        parts = [p for p in [self._root.rstrip("/"), path, name] if p]
        return "/".join(parts)

    def _lfns_to_pfns_impl(self, lfns):
        prefix = "/" + self._bucket + "/" + self._root
        pfns = {}
        for lfn in lfns:
            scope, name = str(lfn["scope"]), lfn["name"]
            if "path" in lfn and lfn.get("path"):
                rel = lfn["path"].lstrip("/")
            else:
                rel = self._get_path(scope=scope, name=name)
            pfns[f"{scope}:{name}"] = (
                f"s3://{self.attributes['hostname']}:{self.attributes.get('port', 9000)}" f"{prefix}{rel}"
            )
        return pfns

    def connect(self):
        """Verify bucket access (create bucket if missing)."""
        try:
            self._s3.head_bucket(Bucket=self._bucket)
        except _ClientError as exc:
            code = exc.response["Error"]["Code"]
            if code in ("404", "NoSuchBucket"):
                try:
                    self._s3.create_bucket(Bucket=self._bucket)
                    self.logger(logging.INFO, f"s3boto3: created bucket {self._bucket}")
                except Exception as create_exc:
                    raise exception.RSEAccessDenied(
                        f"Cannot create bucket {self._bucket}: {create_exc}"
                    ) from create_exc
            else:
                raise exception.RSEAccessDenied(exc) from exc

    def close(self):
        return None

    def exists(self, path):
        if path is None:
            return False
        key = self._pfn_to_key(path)
        try:
            self._s3.head_object(Bucket=self._bucket, Key=key)
            return True
        except _ClientError as exc:
            if exc.response["Error"]["Code"] in ("404", "NoSuchKey"):
                return False
            raise exception.ServiceUnavailable(exc) from exc

    def lfns2pfns(self, lfns):
        lfns = [lfns] if isinstance(lfns, dict) else lfns
        return self._lfns_to_pfns_impl(lfns)

    def pfn2path(self, pfn):
        return self._pfn_to_key(pfn)

    def get(self, path, dest, transfer_timeout=None):
        key = self._pfn_to_key(path)
        try:
            self._s3.download_file(self._bucket, key, dest)
        except _ClientError as exc:
            if exc.response["Error"]["Code"] in ("404", "NoSuchKey"):
                raise exception.SourceNotFound(exc) from exc
            raise exception.ServiceUnavailable(exc) from exc

    def put(self, source, target, source_dir=None, transfer_timeout=None):
        source_file = os.path.join(source_dir, source) if source_dir else source
        if not os.path.exists(source_file):
            raise exception.SourceNotFound(f"Source file not found: {source_file}")
        key = self._pfn_to_key(target)
        try:
            self._s3.upload_file(source_file, self._bucket, key)
        except _ClientError as exc:
            raise exception.ServiceUnavailable(exc) from exc

    def rename(self, path, new_path):
        old_key = self._pfn_to_key(path)
        new_key = self._pfn_to_key(new_path)
        try:
            self._s3.copy_object(
                Bucket=self._bucket,
                CopySource={"Bucket": self._bucket, "Key": old_key},
                Key=new_key,
            )
            self._s3.delete_object(Bucket=self._bucket, Key=old_key)
        except _ClientError as exc:
            raise exception.ServiceUnavailable(exc) from exc

    def delete(self, path):
        key = self._pfn_to_key(path)
        try:
            self._s3.delete_object(Bucket=self._bucket, Key=key)
        except _ClientError as exc:
            raise exception.ServiceUnavailable(exc) from exc

    def stat(self, path):
        key = self._pfn_to_key(path)
        try:
            resp = self._s3.head_object(Bucket=self._bucket, Key=key)
            size = resp["ContentLength"]
            with tempfile.NamedTemporaryFile(delete=False) as tmp:
                self._s3.download_file(self._bucket, key, tmp.name)
                checksum = adler32(tmp.name)
            os.unlink(tmp.name)
            return {"filesize": size, "adler32": checksum}
        except _ClientError as exc:
            if exc.response["Error"]["Code"] in ("404", "NoSuchKey"):
                raise exception.SourceNotFound(exc) from exc
            raise exception.ServiceUnavailable(exc) from exc
