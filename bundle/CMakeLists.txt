# (C) Copyright 2024- European Centre for Medium-Range Weather Forecasts (ECMWF)
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

cmake_minimum_required( VERSION 3.18 FATAL_ERROR )

find_package( ecbuild 3.12 REQUIRED HINTS ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/../ecbuild )

project( dasi-bundle VERSION 0.0.1 LANGUAGES C CXX )

set( CMAKE_CXX_STANDARD 17 )
set( CMAKE_CXX_STANDARD_REQUIRED ON )

# Load platform-specific configuration
include( "./${CMAKE_SYSTEM_NAME}.cmake" )

########################################################################################################################

ecbuild_bundle_initialize()

ecbuild_bundle( PROJECT eckit   GIT "https://github.com/ecmwf/eckit"      BRANCH project/dafab UPDATE )
ecbuild_bundle( PROJECT metkit  GIT "https://github.com/ecmwf/metkit"     BRANCH project/dafab UPDATE )
ecbuild_bundle( PROJECT fdb     GIT "https://github.com/ecmwf/fdb"        BRANCH project/dafab UPDATE )
ecbuild_bundle( PROJECT dasi SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/.." )

ecbuild_bundle_finalize()
