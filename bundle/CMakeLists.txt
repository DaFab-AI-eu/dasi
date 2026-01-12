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

########################################################################################################################

ecbuild_bundle_initialize()

# Disable features not needed for bundle build
set( ENABLE_TESTS OFF CACHE BOOL "Disable tests for dependencies" )
set( ENABLE_ECKIT_SQL OFF CACHE BOOL "Disable eckit SQL" )

# ECMWF dependencies
ecbuild_bundle( PROJECT eckit   GIT "https://github.com/ecmwf/eckit"   TAG 1.28.0 )
ecbuild_bundle( PROJECT metkit  GIT "https://github.com/ecmwf/metkit"  TAG 1.11.0 )
ecbuild_bundle( PROJECT fdb     GIT "https://github.com/ecmwf/fdb"     TAG 5.13.0 )

# DASI
ecbuild_bundle( PROJECT dasi SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/.." )

ecbuild_bundle_finalize()
