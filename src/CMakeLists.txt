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

configure_file( lib/dasi_version.h.in lib/dasi_version.h @ONLY )

set( HEADER_FILE_LIST
    ${PROJECT_SOURCE_DIR}/include/dasi/api/detail/Generators.h
    ${PROJECT_SOURCE_DIR}/include/dasi/api/detail/ListDetail.h
    ${PROJECT_SOURCE_DIR}/include/dasi/api/detail/PolicyDetail.h
    ${PROJECT_SOURCE_DIR}/include/dasi/api/detail/PurgeDetail.h
    ${PROJECT_SOURCE_DIR}/include/dasi/api/detail/RetrieveDetail.h
    ${PROJECT_SOURCE_DIR}/include/dasi/api/detail/WipeDetail.h
    ${PROJECT_SOURCE_DIR}/include/dasi/api/Key.h
    ${PROJECT_SOURCE_DIR}/include/dasi/api/Dasi.h
    ${PROJECT_SOURCE_DIR}/include/dasi/api/Query.h
    ${PROJECT_SOURCE_DIR}/include/dasi/api/dasi_c.h
    ${PROJECT_SOURCE_DIR}/include/dasi/lib/LibDasi.h
    ${PROJECT_SOURCE_DIR}/include/dasi/tools/DASITool.h
)

set( SOURCE_FILE_LIST
    api/detail/RetrieveDetail.cc
    api/detail/ListDetail.cc
    api/detail/PolicyDetail.cc
    api/dasi_c.cc
    api/Query.cc
    api/Key.cc
    api/Dasi.cc
    impl/ListGeneratorImpl.cc
    impl/ListGeneratorImpl.h
    impl/RetrieveResultImpl.h
    impl/RetrieveResultImpl.cc
    impl/PurgeGeneratorImpl.h
    impl/PolicyStatusGeneratorImpl.h
    impl/PolicyStatusGeneratorImpl.cc
    impl/WipeGeneratorImpl.h
    lib/LibDasi.cc
    tools/DASITool.cc
)

add_library( ${PROJECT_NAME} ${HEADER_FILE_LIST} ${SOURCE_FILE_LIST} )

target_include_directories( ${PROJECT_NAME}
    PRIVATE
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/src>
        $<BUILD_INTERFACE:${PROJECT_BINARY_DIR}/src>
    PUBLIC
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
    INTERFACE
        $<INSTALL_INTERFACE:include/${PROJECT_NAME}>
)

set_target_properties( ${PROJECT_NAME} PROPERTIES VERSION ${PROJECT_VERSION} )
set_target_properties( ${PROJECT_NAME} PROPERTIES POSITION_INDEPENDENT_CODE ${BUILD_SHARED_LIBS} )

target_link_libraries( ${PROJECT_NAME} PUBLIC eckit fdb5 )

add_subdirectory( tools )
