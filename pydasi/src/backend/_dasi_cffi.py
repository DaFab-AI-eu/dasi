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

from os import path as ospath
from cffi import FFI

from utils import FindLib, log, version


ffi = FFI()

__file_dir__ = ospath.dirname(__file__)
"""current file directory path"""

__source_file__ = ospath.join(__file_dir__, "include", "dasi_c_cffi.h")
"""source file directory path"""


def ffi_encode(data) -> bytes:
    if isinstance(data, bytes):
        return data

    if not isinstance(data, str):
        data = str(data)

    return data.encode(encoding="utf-8", errors="surrogateescape")


def ffi_decode(data: FFI.CData) -> str:
    buf = ffi.string(data)
    if isinstance(buf, str):
        return buf
    else:
        return buf.decode(encoding="utf-8", errors="surrogateescape")


def read_lib_version(lib) -> str:
    tmp = ffi.new("char**")
    lib.dasi_version(tmp)
    return ffi_decode(tmp[0])


def new_dasi(config: str) -> FFI.CData:
    cdasi = ffi.new("dasi_t **")
    lib.dasi_open(cdasi, ffi_encode(config))
    return ffi.gc(cdasi[0], lib.dasi_close)


def new_key(key=None) -> FFI.CData:
    ckey = ffi.new("dasi_key_t **")
    if isinstance(key, str) and key.strip() != "":
        lib.dasi_new_key_from_string(ckey, ffi_encode(key))
    else:
        lib.dasi_new_key(ckey)
    return ffi.gc(ckey[0], lib.dasi_free_key)


def new_query(query=None) -> FFI.CData:
    cquery = ffi.new("dasi_query_t **")
    if isinstance(query, str):
        lib.dasi_new_query_from_string(cquery, ffi_encode(query))
    else:
        lib.dasi_new_query(cquery)
    return ffi.gc(cquery[0], lib.dasi_free_query)


def new_wipe(
    cdasi: FFI.CData, cquery: FFI.CData, cdoit: FFI.CData, call: FFI.CData
) -> FFI.CData:
    check_type(cdasi, "dasi_t *")
    check_type(cquery, "dasi_query_t *")
    check_type(cdoit, "int *")
    check_type(call, "int *")
    cwipe = ffi.new("dasi_wipe_t **")
    lib.dasi_wipe(cdasi, cquery, cdoit, call, cwipe)
    return ffi.gc(cwipe[0], lib.dasi_free_wipe)


def new_list(cdasi: FFI.CData, cquery: FFI.CData) -> FFI.CData:
    check_type(cdasi, "dasi_t *")
    check_type(cquery, "dasi_query_t *")
    clist = ffi.new("dasi_list_t **")
    lib.dasi_list(cdasi, cquery, clist)
    return ffi.gc(clist[0], lib.dasi_free_list)


def new_retrieve(cdasi: FFI.CData, cquery: FFI.CData) -> FFI.CData:
    check_type(cdasi, "dasi_t *")
    check_type(cquery, "dasi_query_t *")
    cret = ffi.new("dasi_retrieve_t **")
    lib.dasi_retrieve(cdasi, cquery, cret)
    return ffi.gc(cret[0], lib.dasi_free_retrieve)


def check_type(cobj: FFI.CData, name: str):
    cname = ffi.typeof(cobj).cname
    if cname != name:
        raise DASIException("Type error!", cname + " != " + name)


class DASIException(RuntimeError):
    """Raised when dasi library throws exception"""

    def __init__(self, message: str, error: str = ""):
        super().__init__(message)
        self.error = error

    def __str__(self) -> str:
        return super().__str__() + ": " + self.error


class CFFIModuleLoadFailed(ImportError):
    """Raised when the shared library fails to load"""

    pass


class PatchedLib:
    """
    Patch a CFFI library with error handling

    Finds the header file associated with the DASI C API and parses it,
    loads the shared library, and patches the accessors with
    automatic python-C error handling.
    """

    def __init__(self):
        from os import environ as osenv

        osenv["ECKIT_EXCEPTION_IS_SILENT"] = "1"
        osenv["ECKIT_ASSERT_FAILED_IS_SILENT"] = "1"

        self.__loaded = False
        self._log = log.getLogger(__package__)
        self._log.info("version: %s", version.__version__)

    def load(self):
        if self.__loaded:
            return

        self._log.info("Loading DASI C library...")

        # parse the C source; types, functions, globals, etc.
        with open(__source_file__) as sf:
            ffi.cdef(sf.read())

        try:
            libdasi = FindLib("dasi", __file_dir__).path
            self.__lib = ffi.dlopen(libdasi)
        except Exception as e:
            self._log.error(str(e))
            raise CFFIModuleLoadFailed from e

        # All of the executable members of the CFFI-loaded library are
        # functions in the DASI C API. These should be wrapped with the correct
        # error handling. Otherwise forward these on directly.

        for f in dir(self.__lib):
            try:
                attr = getattr(self.__lib, f)
                setattr(
                    self,
                    f,
                    self.__check_error(attr, f) if callable(attr) else attr,
                )
            except Exception as e:
                self._log.error(str(e))
                self._log.error("Cannot set attribute ", f, " from library")

        self.check_version()

        # Initialise and setup for python-appropriate behaviour
        self.__lib.dasi_initialise_api()

        self.__loaded = True

    def check_version(self):
        version_ = read_lib_version(self.__lib)
        if version.is_newer(version_):
            self._log.info("- version: %s", version_)
        else:
            msg = "The library version [{}] is old.".format(version_)
            self._log.error(msg)
            raise CFFIModuleLoadFailed(msg)

    def __check_error(self, fn, name: str):
        """
        If calls into the DASI library return errors, ensure that they get
        detected and reported by throwing an appropriate python exception.
        """

        def wrapped_fn(*args, **kwargs):
            retval = fn(*args, **kwargs)
            if retval not in (
                self.__lib.DASI_SUCCESS,
                self.__lib.DASI_ITERATION_COMPLETE,
            ):
                err = ffi_decode(self.__lib.dasi_get_error_string())
                msg = "Error in function '{}': {}".format(name, err)
                raise DASIException(msg, err)
            return retval

        return wrapped_fn


try:
    lib = PatchedLib()
except CFFIModuleLoadFailed as e:
    raise ImportError() from e
