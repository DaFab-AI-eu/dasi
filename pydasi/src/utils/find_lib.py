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
import errno

from platform import system as p_system


class FindLib:
    """
    Finds the library given the name and path to search for.
    """

    def __init__(self, name: str, dir: str):
        import logging

        self._log = logging.getLogger(__name__)

        self._log.info("Searching library '%s' ...", name)

        platform = p_system()

        if platform == "Windows":
            raise NotImplementedError("Windows OS is not supported!")

        self.__name = "lib" + name

        self._log.debug("library name: %s", self.__name)

        paths = [os.path.join(dir, "libs", platform)]
        for env_var in ("DASI_DIR", "dasi_DIR", "LD_LIBRARY_PATH"):
            env_path = os.getenv(env_var)
            if env_path:
                for epath_ in env_path.split(":"):
                    paths.append(epath_)

        paths.append("/usr/local/lib64")
        paths.append("/usr/local/lib")

        def scan_paths(paths):
            def list_files(dirs):
                result = []
                for dir in dirs:
                    for root, _, files in os.walk(dir):
                        for file in files:
                            result.append(os.path.join(root, file))
                return result

            files = list_files(paths)
            for file in files:
                if self.__name in file:
                    return file
            return ""

        self.__path = scan_paths(paths)

        if not os.path.exists(self.__path):
            raise FileNotFoundError(
                errno.ENOENT, os.strerror(errno.ENOENT), self.__path
            )

        self._log.info("found: '%s'", self.__path)

    @property
    def path(self):
        return self.__path
