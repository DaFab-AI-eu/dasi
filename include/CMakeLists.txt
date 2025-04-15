########################################################################################################################
#
# Copyright 1996- European Centre for Medium-Range Weather Forecasts (ECMWF)
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     https://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
########################################################################################################################

list( APPEND HEADER_FILE_LIST
    # ${PROJECT_SOURCE_DIR}/include/dasi/api/detail/Generators.h
    # ${PROJECT_SOURCE_DIR}/include/dasi/api/detail/ListDetail.h
    # ${PROJECT_SOURCE_DIR}/include/dasi/api/detail/PolicyDetail.h
    # ${PROJECT_SOURCE_DIR}/include/dasi/api/detail/PurgeDetail.h
    # ${PROJECT_SOURCE_DIR}/include/dasi/api/detail/RetrieveDetail.h
    # ${PROJECT_SOURCE_DIR}/include/dasi/api/detail/WipeDetail.h
    ${CMAKE_CURRENT_SOURCE_DIR}/dasi/api/Key.h
    ${CMAKE_CURRENT_SOURCE_DIR}/dasi/api/Dasi.h
    ${CMAKE_CURRENT_SOURCE_DIR}/dasi/api/Query.h
    ${CMAKE_CURRENT_SOURCE_DIR}/dasi/api/dasi_c.h
    ${CMAKE_CURRENT_SOURCE_DIR}/dasi/lib/LibDasi.h
    ${CMAKE_CURRENT_SOURCE_DIR}/dasi/tools/DASITool.h
)

set( HEADER_FILE_LIST ${HEADER_FILE_LIST} PARENT_SCOPE )
