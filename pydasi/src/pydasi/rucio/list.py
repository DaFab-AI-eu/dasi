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

from typing import Any, Dict, Iterator, Optional, Sequence
from logging import getLogger as _getLogger
from rucio.client import Client

logger = _getLogger(__name__)


class RucioListItem:
    __slots__ = (
        "_scope",
        "_name",
        "_metadata",
    )

    def __init__(
        self,
        scope: str,
        name: str,
        metadata: dict[str, Any],
    ) -> None:
        self._scope = scope
        self._name = name
        self._metadata = metadata

    @property
    def key(self) -> dict[str, Any]:
        return self._metadata

    @property
    def uri(self) -> str:
        return f"{self._scope}:{self._name}"

    @property
    def scope(self) -> str:
        return self._scope

    @property
    def name(self) -> str:
        return self._name

    @property
    def did_type(self) -> str:
        return self._metadata.get("did_type", "")

    @property
    def length(self) -> int:
        return self._metadata.get("bytes", 0)

    # timestamp is not available from Rucio list_dids; expose creation time
    @property
    def timestamp(self) -> Any:
        return self._metadata.get("created_at", None)

    # offset is a DASI/FDB concept; not applicable here
    @property
    def offset(self) -> int:
        return 0

    def __str__(self) -> str:
        return f"RucioListItem(uri={self.uri!r}, type={self.did_type!r}, metadata={self._metadata})"

    def __repr__(self) -> str:
        return self.__str__()


class RucioList:
    """Iterable list of DIDs matching a metadata query.

    Parameters
    ----------
    client:
        An authenticated rucio client instance.
    query:
        Sequence of filter dicts used as Rucio DID metadata filters.
    scope:
        Fallback scope when *query* contains no ``"scope"`` key.
    """

    def __init__(
        self,
        client: Client,
        scope: str,
        query: Sequence[dict[str, Any]],
    ) -> None:
        self._client = client
        self._scope = scope
        self._filters: Sequence[dict[str, Any]] = query

        self._iter: Optional[Iterator[Any]] = None
        self._current: Optional[RucioListItem] = None

    def __str__(self) -> str:
        return f"RucioList(scope={self._scope!r}, filters={self._filters!r})"

    def __repr__(self) -> str:
        return self.__str__()

    def __iter__(self) -> RucioList:
        self._iter = self._client.list_dids(
            scope=self._scope,
            filters=self._filters,
            did_type="file",
            long=True,
        )
        return self

    def __next__(self) -> RucioListItem:
        if self._iter is None:
            raise StopIteration

        entry = next(self._iter)

        scope = entry.get("scope", self._scope)
        name = entry.get("name", entry.get("did", ""))

        metadata: dict[str, Any] = {}
        try:
            metadata = self._client.get_metadata(scope=scope, name=name)
        except Exception as exc:
            logger.warning("get_metadata failed for %s:%s — %s", scope, name, exc)

        item = RucioListItem(
            scope=scope,
            name=name,
            metadata=metadata,
        )
        self._current = item
        return item

    @property
    def key(self) -> dict[str, Any]:
        return self._current.key if self._current else {}

    @property
    def uri(self) -> str:
        return self._current.uri if self._current else ""

    @property
    def timestamp(self) -> Any:
        return self._current.timestamp if self._current else None

    @property
    def offset(self) -> int:
        return 0

    @property
    def length(self) -> int:
        return self._current.length if self._current else 0

    @property
    def name(self) -> str:
        return self._current.name if self._current else ""
