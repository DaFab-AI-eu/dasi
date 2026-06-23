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

import os
import re
import tempfile
from logging import getLogger as _getLogger
from pathlib import Path
from typing import Any, Iterable, Optional, Sequence

from .list import RucioList
from .retrieve import RucioRetrieve

logger = _getLogger(__name__)

_SAFE_FILENAME_CHARS = re.compile(r"[^A-Za-z0-9._-]+")


class Rucio:
    """pydasi-compatible interface backed by a Rucio data-management system.

    The following additional configuration options under a top-level ``rucio:`` key in the YAML config file:
    rucio:
        config_path: rucio.cfg    # path to rucio.cfg (credentials + server)
        scope: user.root          # scope for new/queried DIDs
        rse: MINIORSE             # default Replica Storage Element
        protocol: s3              # preferred transfer protocol
    """

    def __init__(self, config: str) -> None:

        cfg = self._read_rucio_config(config)

        self._scope = cfg.get("scope", "user.root")
        self._rse = cfg.get("rse", "MINIORSE")
        self._protocol = cfg.get("protocol", "s3")

        config_path = cfg.get("config_path")
        if config_path:
            os.environ.setdefault("RUCIO_CONFIG", config_path)

        try:
            from rucio.client import Client
            from rucio.client.downloadclient import DownloadClient
            from rucio.client.uploadclient import UploadClient
        except ImportError as exc:
            raise ImportError(
                "rucio-clients is required for the pydasi Rucio backend: pip install rucio-clients"
            ) from exc

        self._client = Client()
        self._dl_client = DownloadClient(client=self._client, logger=logger)
        self._ul_client = UploadClient(_client=self._client, logger=logger)

    def list(self, query: Sequence[dict[str, Any]]) -> RucioList:
        logger.debug("list query=%r", query)
        result = RucioList(
            client=self._client,
            scope=self._scope,
            query=query,
        )
        return result

    def retrieve(self, query: Sequence[dict[str, Any]]) -> RucioRetrieve:
        logger.debug("retrieve query=%r", query)
        result = RucioRetrieve(
            client=self._client,
            download_client=self._dl_client,
            scope=self._scope,
            rse=self._rse,
            protocol=self._protocol,
            query=query,
        )
        return result

    # uploads the given data to Rucio, using a filename derived from the key.
    # It then attaches metadata attributes corresponding to the key items.
    # Note: Rucio's upload API is designed for files on disk,
    # so we need to write the data to a temporary file first.
    def archive(self, key: dict[str, Any], data: bytes):
        from rucio.common.types import FileToUploadDict

        # Build a deterministic, filesystem-safe filename from key items.
        parts = "_".join(
            f"{self._safe_filename_component(k)}-{self._safe_filename_component(v)}" for k, v in sorted(key.items())
        )
        filename = f"{parts}.data"

        logger.debug(f"Archiving [{self._scope}:{filename}] on rse[{self._rse}]")

        with tempfile.TemporaryDirectory() as tmpdir:
            tmpdir_path = Path(tmpdir).resolve()
            local_path = (tmpdir_path / filename).resolve()
            if local_path.parent != tmpdir_path:
                raise ValueError(f"Unsafe filename generated from key: {filename!r}")

            with open(local_path, "wb") as fh:
                fh.write(data)

            upload_item: Iterable[FileToUploadDict] = [
                {
                    "path": local_path,
                    "rse": self._rse,
                    "did_scope": self._scope,
                    "did_name": filename,
                    "no_register": False,
                }
            ]

            uploaded = False
            try:
                upload_result = self._ul_client.upload(upload_item)
                self._validate_upload_result(upload_result, filename)
                uploaded = True
            except Exception as exc:
                from rucio.common.exception import NoFilesUploaded

                if isinstance(exc, NoFilesUploaded):
                    logger.warning(f"{self._scope}:{filename} already present on {self._rse}")
                else:
                    raise

        # Attach metadata attributes
        if uploaded:
            metadata: dict[str, str] = {str(attr_key): str(attr_val) for attr_key, attr_val in key.items()}

            metadata_errors: list[str] = []
            for attr_key, attr_val in metadata.items():
                try:
                    self._client.set_metadata(scope=self._scope, name=filename, key=attr_key, value=attr_val)
                except Exception as exc:
                    metadata_errors.append(f"{attr_key!r}={attr_val!r}: {exc}")

            if metadata_errors:
                raise RuntimeError(
                    f"Upload succeeded but metadata update failed for {self._scope}:{filename}: "
                    + "; ".join(metadata_errors)
                )

        logger.debug(f"Archived {self._scope}:{filename} on {self._rse}.")

    @staticmethod
    def _safe_filename_component(value: Any) -> str:
        text = str(value).strip()
        if not text:
            return "empty"

        text = _SAFE_FILENAME_CHARS.sub("-", text)
        text = text.strip(".-_")
        return text or "empty"

    @staticmethod
    def _validate_upload_result(upload_result: Any, filename: str) -> None:
        if upload_result is None:
            return

        if isinstance(upload_result, bool):
            if not upload_result:
                raise RuntimeError(f"Upload returned unsuccessful status for DID name {filename!r}")
            return

        if isinstance(upload_result, dict):
            error = upload_result.get("error") or upload_result.get("exception")
            if error:
                raise RuntimeError(f"Upload returned an error for DID name {filename!r}: {error}")
            return

        if isinstance(upload_result, Sequence) and not isinstance(upload_result, (str, bytes, bytearray)):
            if len(upload_result) == 0:
                raise RuntimeError(f"Upload returned an empty result for DID name {filename!r}")

            errors = []
            for item in upload_result:
                if isinstance(item, dict):
                    error = item.get("error") or item.get("exception")
                    if error:
                        errors.append(str(error))

            if errors:
                raise RuntimeError(f"Upload reported errors for DID name {filename!r}: {'; '.join(errors)}")

    @staticmethod
    def _read_rucio_config(path: str) -> dict[str, Any]:
        import yaml

        with open(path, "r") as fh:
            raw = yaml.safe_load(fh) or {}

        rucio = raw.get("rucio", raw)
        if not isinstance(rucio, dict):
            raise ValueError(f"Invalid 'rucio' section in {path!r}: expected a mapping.")

        cfg: dict[str, Any] = dict(rucio)

        config_path = cfg.get("config_path")
        if isinstance(config_path, str) and not os.path.isabs(config_path):
            base = os.path.dirname(os.path.abspath(path))
            cfg["config_path"] = os.path.join(base, config_path)

        logger.debug(f"Rucio configuration: {cfg}")

        return cfg
