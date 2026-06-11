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
import tempfile

from pathlib import Path
from typing import Any, Iterable, Optional, Sequence
from logging import getLogger as _getLogger
from rucio.common.types import FileToUploadDict

from .list import RucioList
from .retrieve import RucioRetrieve

logger = _getLogger(__name__)


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

    def list(
        self,
        query: Sequence[dict[str, Any]],
    ) -> RucioList:
        logger.debug("list query=%r", query)
        result = RucioList(
            client=self._client,
            scope=self._scope,
            query=query,
        )
        return result

    def retrieve(
        self,
        query: Sequence[dict[str, Any]],
    ) -> RucioRetrieve:
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

    def archive(
        self,
        key: dict[str, Any],
        data: bytes,
        filename: Optional[str] = None,
        lifetime: Optional[int] = None,
    ) -> str:
        if not self._rse:
            raise ValueError("RSE must be configured!")

        if not filename:
            # TODO check logic; Build a deterministic filename from sorted key items
            parts = "_".join(f"{k}-{v}" for k, v in sorted(key.items()))
            filename = f"{parts}.bin"

        logger.debug(f"Archiving [{self._scope}:{filename}] on rse[{self._rse}]")

        with tempfile.TemporaryDirectory() as tmpdir:
            local_path = Path(tmpdir) / filename
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
            if lifetime is not None:
                upload_item[0]["lifetime"] = lifetime

            try:
                self._ul_client.upload(upload_item)
            except Exception as exc:
                from rucio.common.exception import NoFilesUploaded

                if isinstance(exc, NoFilesUploaded):
                    logger.debug(f"{self._scope}:{filename} already present on {self._rse} — skipping upload")
                else:
                    raise

        # Attach metadata attributes
        for attr_key, attr_val in key.items():
            try:
                self._client.set_metadata(scope=self._scope, name=filename, key=attr_key, value=str(attr_val))
            except Exception as exc:
                logger.warning(f"Could not set metadata {attr_key!r}={attr_val!r} on {self._scope}:{filename} — {exc}")

        did = f"{self._scope}:{filename}"
        logger.debug(f"Archived {did}")
        return did

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
