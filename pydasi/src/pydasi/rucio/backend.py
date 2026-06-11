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

"""Rucio — pydasi-compatible backend that reads/queries data from a Rucio server.

This module provides a drop-in replacement for the standard DASI C-library
backend for *read* and *query* operations.  Archive (write) support uses
Rucio's upload mechanism and attaches the supplied key dict as DID metadata.

Usage example
-------------
.. code-block:: python

    from pydasi import Rucio

    # All settings are read from a single YAML config file
    dasi = Rucio("pydasi.yml")

    # --- List matching DIDs -------------------------------------------------
    for item in dasi.list({"experiment": "dasi-dev", "run": "42"}):
        print(item.uri, item.key, item.length)

    # --- Retrieve file data -------------------------------------------------
    for item in dasi.retrieve({"experiment": "dasi-dev", "run": "42"}):
        print(item.uri, len(item.data))

    # --- Archive (upload + attach metadata) ---------------------------------
    with open("result.nc", "rb") as fh:
        dasi.archive(
            key={"experiment": "dasi-dev", "run": "42"},
            data=fh.read(),
            filename="result.nc",
        )
"""

from __future__ import annotations

import logging
import os
import tempfile
from typing import Any, Dict, Optional

import yaml

from .list import RucioList
from .retrieve import RucioRetrieve


class Rucio:
    """pydasi-compatible interface backed by a Rucio data-management system.

    All settings are read from a single YAML config file passed as the only
    argument, e.g. ``Rucio("pydasi.yml")``.  The file holds the backend
    settings (optionally nested under a top-level ``rucio:`` key)::

        rucio:
          config_path: rucio.cfg    # path to rucio.cfg (credentials + server)
          scope: user.root          # scope for new/queried DIDs
          rse: MINIORSE             # default Replica Storage Element
          protocol: s3              # preferred transfer protocol

    Connection details (server URL, account, authentication, TLS) live in the
    ``rucio.cfg`` referenced by ``config_path``; that file is exposed to the
    Rucio client via the ``RUCIO_CONFIG`` environment variable.  A relative
    ``config_path`` is resolved relative to the YAML file's directory.

    Parameters
    ----------
    config:
        Path to the YAML configuration file.
    """

    def __init__(self, config: str) -> None:
        self._log = logging.getLogger(__name__)

        cfg = self._load_config_file(config)

        config_path = cfg.get("config_path")
        self._scope = cfg.get("scope") or "user.root"
        self._rse = cfg.get("rse") or "default"
        self._protocol = cfg.get("protocol") or "s3"

        # only as a fallback
        if config_path:
            os.environ.setdefault("RUCIO_CONFIG", config_path)

        try:
            from rucio.client import Client
            from rucio.client.downloadclient import DownloadClient
            from rucio.client.uploadclient import UploadClient
        except ImportError as exc:
            raise ImportError(
                "rucio-clients is required for the Rucio pydasi backend. " "Install it with: pip install rucio-clients"
            ) from exc

        rucio_log = logging.getLogger("pydasi.rucio.client")
        if rucio_log.level == logging.NOTSET:
            rucio_log.setLevel(logging.WARNING)

        self._client = Client()
        self._dl_client = DownloadClient(client=self._client, logger=rucio_log)
        self._ul_client = UploadClient(self._client, logger=rucio_log)

    # ------------------------------------------------------------------
    # Public API — mirrors pydasi.Dasi
    # ------------------------------------------------------------------

    def list(
        self,
        query: Dict[str, Any],
    ) -> RucioList:
        """List DIDs matching *query*.

        Parameters
        ----------
        query:
            Key-value filter dict.  A ``"scope"`` key overrides the instance
            default scope.  All other keys are passed as Rucio metadata
            filters.

        Returns
        -------
        RucioList
            An iterable whose items expose ``.key``, ``.uri``, ``.length``,
            and ``.timestamp`` — matching the :class:`pydasi.List` interface.
        """
        self._log.debug("list query=%r", query)
        result = RucioList(
            client=self._client,
            scope=self._scope,
            query=query,
        )
        return iter(result)

    def retrieve(
        self,
        query: Dict[str, Any],
    ) -> RucioRetrieve:
        """Download files matching *query* and expose their raw content.

        Parameters
        ----------
        query:
            Key-value filter dict.  A ``"scope"`` key overrides the instance
            default scope.  All other keys are passed as Rucio metadata
            filters.

        Returns
        -------
        RucioRetrieve
            An iterable whose items expose ``.key``, ``.data``, ``.length``,
            and ``.timestamp`` — matching the :class:`pydasi.Retrieve`
            interface.
        """
        self._log.debug("retrieve query=%r", query)
        result = RucioRetrieve(
            client=self._client,
            download_client=self._dl_client,
            scope=self._scope,
            rse=self._rse,
            protocol=self._protocol,
            query=query,
        )
        return iter(result)

    def archive(
        self,
        key: Dict[str, Any],
        data: bytes,
        filename: Optional[str] = None,
        lifetime: Optional[int] = None,
    ) -> str:
        """Upload *data* to Rucio and attach *key* as DID metadata.

        Parameters
        ----------
        key:
            Metadata dict that describes the data.  Written as Rucio DID
            metadata attributes after the upload.
        data:
            Raw bytes to upload.
        filename:
            DID name (file name).  Generated from *key* when not provided.
        lifetime:
            Replication rule lifetime in seconds (``None`` = permanent).

        Returns
        -------
        str
            The resulting DID identifier ``scope:filename``.
        """
        if not self._rse:
            raise ValueError("RSE must be configured!")

        if not filename:
            # Build a deterministic filename from sorted key items
            parts = "_".join(f"{k}-{v}" for k, v in sorted(key.items()))
            filename = f"{parts}.bin"

        self._log.debug("archive DID=%s:%s rse=%s", self._scope, filename, self._rse)

        with tempfile.TemporaryDirectory() as tmpdir:
            local_path = os.path.join(tmpdir, filename)
            with open(local_path, "wb") as fh:
                fh.write(data)

            upload_item: Dict[str, Any] = {
                "path": local_path,
                "rse": self._rse,
                "did_scope": self._scope,
                "did_name": filename,
                "no_register": False,
            }
            if lifetime is not None:
                upload_item["lifetime"] = lifetime

            try:
                self._ul_client.upload([upload_item])
            except Exception as exc:  # noqa: BLE001
                # Rucio raises NoFilesUploaded when the replica already
                # exists on the RSE; treat that as an idempotent success and
                # continue to (re)attach metadata.
                from rucio.common.exception import NoFilesUploaded

                if isinstance(exc, NoFilesUploaded):
                    self._log.debug(
                        "%s:%s already present on %s — skipping upload",
                        self._scope,
                        filename,
                        self._rse,
                    )
                else:
                    raise

        # Attach metadata attributes
        for attr_key, attr_val in key.items():
            try:
                self._client.set_metadata(
                    scope=self._scope,
                    name=filename,
                    key=attr_key,
                    value=str(attr_val),
                )
            except Exception as exc:  # noqa: BLE001
                self._log.warning(
                    "Could not set metadata %r=%r on %s:%s — %s",
                    attr_key,
                    attr_val,
                    self._scope,
                    filename,
                    exc,
                )

        did = f"{self._scope}:{filename}"
        self._log.debug("Archived %s", did)
        return did

    def ping(self) -> Dict[str, Any]:
        """Return Rucio server version info."""
        return self._client.ping()

    def whoami(self) -> Dict[str, Any]:
        """Return information about the authenticated account."""
        return self._client.whoami()

    # ------------------------------------------------------------------
    # Internal helpers
    # ------------------------------------------------------------------

    @staticmethod
    def _load_config_file(path: str) -> Dict[str, Any]:
        """Load Rucio backend settings from a YAML file.

        Settings may sit at the top level or nested under a ``rucio:`` key.
        A relative ``config_path`` is resolved against the file's directory.
        """
        with open(path, "r") as fh:
            raw = yaml.safe_load(fh) or {}

        if not isinstance(raw, dict):
            raise ValueError(f"Invalid Rucio config file {path!r}: expected a mapping, " f"got {type(raw).__name__}.")

        section = raw.get("rucio", raw)
        if not isinstance(section, dict):
            raise ValueError(f"Invalid 'rucio' section in {path!r}: expected a mapping.")

        cfg: Dict[str, Any] = dict(section)

        config_path = cfg.get("config_path")
        if isinstance(config_path, str) and not os.path.isabs(config_path):
            base = os.path.dirname(os.path.abspath(path))
            cfg["config_path"] = os.path.join(base, config_path)

        return cfg
