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

from __future__ import annotations

import logging
import os
import tempfile
from typing import Any, Dict, Optional


class RucioRetrieveItem:
    """A single data item returned by :class:`RucioRetrieve` iteration."""

    __slots__ = ("_scope", "_name", "_metadata", "_data", "_created_at")

    def __init__(
        self,
        scope: str,
        name: str,
        metadata: Dict[str, Any],
        data: bytes,
        created_at: Any,
    ) -> None:
        self._scope = scope
        self._name = name
        self._metadata = metadata
        self._data = data
        self._created_at = created_at

    # ------------------------------------------------------------------
    # Properties that mirror the pydasi.Retrieve interface
    # ------------------------------------------------------------------

    @property
    def key(self) -> Dict[str, Any]:
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
        return self._created_at

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
        return f"RucioRetrieveItem(uri={self.uri!r}, bytes={self.length}, " f"metadata={self._metadata})"

    def __repr__(self) -> str:
        return self.__str__()


class RucioRetrieve:
    """Download files matching a metadata query and expose their content.

    Parameters
    ----------
    client:
        An authenticated ``rucio.client.Client`` instance.
    download_client:
        An authenticated ``rucio.client.downloadclient.DownloadClient``
        instance used for the actual data transfer.
    query:
        Dict of key→value (or key→[values]) pairs used as Rucio DID metadata
        filters.  A ``"scope"`` key selects the Rucio scope; all other keys
        become ``filters`` passed to ``list_dids``.
    scope:
        Fallback scope when *query* contains no ``"scope"`` key.
    rse:
        Restrict downloads to this RSE (optional).
    protocol:
        Preferred protocol (e.g. ``"s3"``).  ``None`` lets Rucio choose.
    """

    def __init__(
        self,
        client: Any,
        download_client: Any,
        rse: str,
        scope: str,
        protocol: str,
        query: Dict[str, Any],
    ) -> None:
        self._log = logging.getLogger(__name__)
        self._client = client
        self._dl_client = download_client
        self._rse = rse
        self._scope = scope
        self._protocol = protocol

        filters = dict(query)

        self._filters: Dict[str, Any] = {}
        for k, v in filters.items():
            if isinstance(v, (list, tuple)) and len(v) == 1:
                self._filters[k] = v[0]
            else:
                self._filters[k] = v

        self._did_list: list[Dict[str, Any]] = []
        self._index: int = 0
        self._current: Optional[RucioRetrieveItem] = None

    # ------------------------------------------------------------------
    # Iterator protocol
    # ------------------------------------------------------------------

    def __iter__(self) -> "RucioRetrieve":
        self._log.debug(
            "RucioRetrieve: listing scope=%r filters=%r rse=%r",
            self._scope,
            self._filters,
            self._rse,
        )
        self._did_list = list(
            self._client.list_dids(
                scope=self._scope,
                filters=self._filters,
                did_type="file",
                long=True,
            )
        )
        self._index = 0
        return self

    def __next__(self) -> RucioRetrieveItem:
        if self._index >= len(self._did_list):
            raise StopIteration

        entry = self._did_list[self._index]
        self._index += 1

        name = entry.get("name", entry.get("did", ""))
        created_at = entry.get("created_at")

        # Fetch DID metadata
        metadata: Dict[str, Any] = {}
        try:
            metadata = self._client.get_metadata(scope=self._scope, name=name) or {}
        except Exception as exc:  # noqa: BLE001
            self._log.debug("get_metadata failed for %s:%s — %s", self._scope, name, exc)

        # Download the file into a temporary directory and read it back
        data = self._download_did(name)

        item = RucioRetrieveItem(
            scope=self._scope,
            name=name,
            metadata=metadata,
            data=data,
            created_at=created_at,
        )
        self._current = item
        return item

    def __len__(self) -> int:
        return len(self._did_list)

    # ------------------------------------------------------------------
    # Internal helpers
    # ------------------------------------------------------------------

    def _download_did(self, name: str) -> bytes:
        """Download a single file DID and return its content as bytes."""
        with tempfile.TemporaryDirectory() as tmpdir:
            item_spec: Dict[str, Any] = {
                "did": f"{self._scope}:{name}",
                "base_dir": tmpdir,
                "no_subdir": True,
            }
            if self._rse:
                item_spec["rse"] = self._rse
            if self._protocol:
                item_spec["force_scheme"] = self._protocol

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

    # ------------------------------------------------------------------
    # Convenience properties (reflect the *last* yielded item)
    # ------------------------------------------------------------------

    @property
    def key(self) -> Dict[str, Any]:
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
