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


from backend import ffi, lib, new_dasi

from .key import Key
from .wipe import Wipe
from .list import List
from .retrieve import Retrieve


class Dasi:
    """
    The Python interface to DASI.

    .. code-block:: python

        # Import dasi
        from dasi import Dasi

        # Create new DASI session
        dasi = Dasi("config.yaml")
    """

    def __init__(self, config: str):
        """
        Creates a DASI session.

        :param str config: the configuration file.
        """
        import logging

        lib.load()

        self._log = logging.getLogger(__name__)

        self._log.debug("Initialize Dasi...")

        self._cdata = new_dasi(config)

    def archive(self, key, data):
        """
        Write data to be stored according to Dasi configuration.

        :param key: The metadata description of the data to store and index
        :param data: A pointer to a (read-only) copy of the data
        """

        self._log.debug("Archiving...")

        dbuffer = ffi.from_buffer(data)

        lib.dasi_archive(self._cdata, Key(key).cdata, dbuffer, len(data))

    def wipe(self, query, doit: bool = False, all: bool = False) -> Wipe:
        """Wipe data from dasi

        :param query: A description of the span of metadata to wipe
        :return: An iterable wipe output.
        :rtype: Wipe
        """

        self._log.debug("Wiping...")

        return Wipe(self._cdata, query, doit, all)

    def list(self, query) -> List:
        """List data present and retrievable from the archive

        :param query: A description of the span of metadata to list within
        :return: An iterable details of the objects describing data.
        :rtype: List
        """

        self._log.debug("Listing...")

        return List(self._cdata, query)

    def retrieve(self, query) -> Retrieve:
        """Retrieve data objects from the archive

        :param query: A description of the span of data to retrieve
        :return: A generic data handle, that will retrieve the data
        :rtype: Retrieve
        """

        self._log.debug("Retrieving...")

        return Retrieve(self._cdata, query)

    def flush(self):
        """
        Flushes all buffers and ensures internal state is safe (wrt failure).
        """

        self._log.debug("Flushing...")

        lib.dasi_flush(self._cdata)
