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

configure_file( lib/dasi_version.h.in ${CMAKE_CURRENT_BINARY_DIR}/lib/dasi_version.h @ONLY )

list( APPEND SOURCE_FILE_LIST
    ${CMAKE_CURRENT_SOURCE_DIR}/api/detail/RetrieveDetail.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/api/detail/ListDetail.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/api/detail/PolicyDetail.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/api/dasi_c.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/api/Query.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/api/Key.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/api/Dasi.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/impl/ListGeneratorImpl.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/impl/ListGeneratorImpl.h
    ${CMAKE_CURRENT_SOURCE_DIR}/impl/RetrieveResultImpl.h
    ${CMAKE_CURRENT_SOURCE_DIR}/impl/RetrieveResultImpl.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/impl/PurgeGeneratorImpl.h
    ${CMAKE_CURRENT_SOURCE_DIR}/impl/PolicyStatusGeneratorImpl.h
    ${CMAKE_CURRENT_SOURCE_DIR}/impl/PolicyStatusGeneratorImpl.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/impl/WipeGeneratorImpl.h
    ${CMAKE_CURRENT_SOURCE_DIR}/lib/LibDasi.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/tools/DASITool.cc
)

set( SOURCE_FILE_LIST ${SOURCE_FILE_LIST} PARENT_SCOPE )
