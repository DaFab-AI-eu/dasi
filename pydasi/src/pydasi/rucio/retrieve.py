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
from typing import Any, Dict, Optional, Sequence
from logging import getLogger as _getLogger

from rucio.client import Client
from rucio.client.downloadclient import DownloadClient
from rucio.common.exception import NoFilesDownloaded

logger = _getLogger(__name__)


class RucioRetrieveItem:

    __slots__ = ("_scope", "_name", "_data", "_metadata")

    def __init__(
        self,
        scope: str,
        name: str,
        data: bytes,
        metadata: dict[str, Any],
    ) -> None:
        self._scope = scope
        self._name = name
        self._data = data
        self._metadata = metadata

    # ------------------------------------------------------------------
    # Properties that mirror the pydasi.Retrieve interface
    # ------------------------------------------------------------------

    @property
    def key(self) -> dict[str, Any]:
        """Metadata key-value pairs attached to the DID."""
        return self._metadata

    @property
    def data(self) -> bytes:
        """Raw file content as bytes."""
        return self._data

    @property
    def length(self) -> int:
        return len(self._data)

    @property
    def timestamp(self) -> Any:
        return self._metadata.get("created_at", None)

    @property
    def offset(self) -> int:
        return 0

    @property
    def scope(self) -> str:
        return self._scope

    @property
    def name(self) -> str:
        return self._name

    @property
    def uri(self) -> str:
        return f"{self._scope}:{self._name}"

    def __str__(self) -> str:
        return f"RucioRetrieveItem(uri={self.uri!r}, length={self.length}, " f"metadata={self._metadata})"

    def __repr__(self) -> str:
        return self.__str__()


class RucioRetrieve:
    """Download files matching a metadata query and expose their content."""

    def __init__(
        self,
        client: Client,
        download_client: DownloadClient,
        scope: str,
        rse: str,
        protocol: str,
        query: Sequence[dict[str, Any]],
    ) -> None:
        self._client = client
        self._dl_client = download_client
        self._rse = rse
        self._scope = scope
        self._protocol = protocol
        self._filters: Sequence[dict[str, Any]] = query

        self._did_list: list[dict[str, Any]] = []
        self._index: int = 0
        self._current: Optional[RucioRetrieveItem] = None

    def __iter__(self) -> "RucioRetrieve":
        logger.debug("RucioRetrieve: scope=%r filters=%r rse=%r", self._scope, self._filters, self._rse)
        self._did_list = list(
            self._client.list_dids(scope=self._scope, filters=self._filters, did_type="file", long=True)
        )
        self._index = 0
        return self

    def __next__(self) -> RucioRetrieveItem:
        while self._index < len(self._did_list):
            entry = self._did_list[self._index]
            self._index += 1

            name = entry.get("name", entry.get("did", ""))

            try:
                data = self._download(name)
            except (NoFilesDownloaded, FileNotFoundError) as exc:
                logger.warning("Skipping %s:%s because it is not downloadable (%s)", self._scope, name, exc)
                continue

            metadata: dict[str, Any] = {}
            try:
                metadata = self._client.get_metadata(scope=self._scope, name=name)
            except Exception as exc:
                logger.warning("get_metadata failed for %s:%s — %s", self._scope, name, exc)

            item = RucioRetrieveItem(scope=self._scope, name=name, data=data, metadata=metadata)
            self._current = item
            return item

        raise StopIteration

    def __len__(self) -> int:
        return len(self._did_list)

    @property
    def key(self) -> dict[str, Any]:
        return self._current.key if self._current else {}

    @property
    def data(self) -> bytes:
        return self._current.data if self._current else b""

    @property
    def timestamp(self) -> Any:
        return self._current.timestamp if self._current else None

    @property
    def offset(self) -> int:
        return 0

    @property
    def length(self) -> int:
        return self._current.length if self._current else 0

    def _download(self, name: str) -> bytes:
        with tempfile.TemporaryDirectory() as tmpdir:
            item_spec: dict[str, Any] = {
                "did": f"{self._scope}:{name}",
                "base_dir": tmpdir,
                "no_subdir": True,
                "rse": self._rse,
                "force_scheme": self._protocol,
            }

            results = self._dl_client.download_dids([item_spec])

            # Locate the downloaded file
            dest = os.path.join(tmpdir, name)
            if not os.path.exists(dest):
                # Some download clients write to a sub-directory
                for root, _dirs, files in os.walk(tmpdir):
                    for fname in files:
                        dest = os.path.join(root, fname)
                        break
                    else:
                        continue
                    break

            if results and results[0].get("clientState") not in (
                "DONE",
                "ALREADY_DONE",
                None,
            ):
                state = results[0].get("clientState", "UNKNOWN")
                raise RuntimeError(f"Download of {self._scope}:{name} did not complete — state={state!r}")

            if not os.path.exists(dest):
                raise FileNotFoundError(f"Downloaded file not found for DID {self._scope}:{name} in {tmpdir}")

            with open(dest, "rb") as fh:
                return fh.read()
