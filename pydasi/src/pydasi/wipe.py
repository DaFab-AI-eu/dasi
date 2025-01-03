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

from backend import FFI, ffi, lib, ffi_decode, new_wipe

# from dasi.dasi import Dasi
from .query import Query


class Wipe:
    def __init__(self, dasi: FFI.CData, query, doit: bool, all: bool):
        from utils.log import getLogger

        self._log = getLogger(__name__)

        self._log.debug("Initialize Wipe...")

        cdoit = ffi.new("dasi_bool_t *", doit)
        call = ffi.new("dasi_bool_t *", all)

        self.__value = ffi.new("const char **", ffi.NULL)
        self._cdata = new_wipe(dasi, Query(query).cdata, cdoit, call)

    def __str__(self) -> str:
        return "wipe: {}".format(self.value)

    def __iter__(self):
        return self

    def __next__(self):
        if lib.dasi_wipe_next(self._cdata) == lib.DASI_ITERATION_COMPLETE:
            raise StopIteration
        lib.dasi_wipe_get_value(self._cdata, self.__value)
        return self

    def __len__(self) -> int:
        self._log.debug("not implemented in Dasi C lib!")
        return 0

    @property
    def value(self) -> str:
        val: FFI.CData = self.__value[0]
        return ffi_decode(val) if val != ffi.NULL else "unknown"
