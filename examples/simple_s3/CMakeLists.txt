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

cmake_minimum_required( VERSION 3.15 FATAL_ERROR )

project( dasi-example-s3 VERSION 0.1 LANGUAGES CXX )

set( CMAKE_CXX_STANDARD 17 )

find_package( dasi 0.2.3 REQUIRED )

include_directories( include )

# archive
add_executable( archive src/Archive.cc )
target_link_libraries( archive PUBLIC dasi::dasi )
set_target_properties( archive
    PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/bin
)

# retrieve
add_executable( retrieve src/Retrieve.cc )
target_link_libraries( retrieve PUBLIC dasi::dasi )
set_target_properties( retrieve
    PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/bin
)
