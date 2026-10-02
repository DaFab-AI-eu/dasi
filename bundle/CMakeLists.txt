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

set( DASI_VERSION 0.3.1 )

project( dasi-bundle VERSION ${DASI_VERSION} LANGUAGES C CXX )

set( CMAKE_CXX_STANDARD 17 )
set( CMAKE_CXX_STANDARD_REQUIRED ON )
set( CMAKE_CXX_EXTENSIONS OFF )
# position-independent code (PIC) for all
set( CMAKE_POSITION_INDEPENDENT_CODE ON )

# Load platform-specific configuration
include( "${CMAKE_CURRENT_LIST_DIR}/${CMAKE_SYSTEM_NAME}.cmake" )
include( "${CMAKE_CURRENT_LIST_DIR}/Dependencies.cmake" )

########################################################################################################################

ecbuild_bundle_initialize()

ecbuild_bundle( PROJECT eckit  GIT "https://github.com/ecmwf/eckit"  BRANCH ${ECKIT_BRANCH} UPDATE )
ecbuild_bundle( PROJECT metkit GIT "https://github.com/ecmwf/metkit" BRANCH ${METKIT_BRANCH} UPDATE )
ecbuild_bundle( PROJECT fdb    GIT "https://github.com/ecmwf/fdb"    BRANCH ${FDB_BRANCH} UPDATE )

get_filename_component( _dasi_parent "${CMAKE_CURRENT_LIST_DIR}/.." ABSOLUTE )
set( _dasi_source "" )
if( EXISTS "${_dasi_parent}/include/dasi/api/Dasi.h" )
	set( _dasi_source "${_dasi_parent}" )
endif()
set( DASI_SOURCE_DIR "${_dasi_source}" CACHE PATH "Local DASI checkout" )
set( DASI_REPOSITORY "https://github.com/DaFab-AI-eu/dasi" CACHE STRING "DASI Git repository" )
set( DASI_BRANCH "" CACHE STRING "Remote DASI branch" )

if( EXISTS "${PROJECT_SOURCE_DIR}/dasi/CMakeLists.txt" AND NOT IS_SYMLINK "${PROJECT_SOURCE_DIR}/dasi" )
	# Checkout placed directly in the bundle (devcontainer and CI).
	ecbuild_bundle( PROJECT dasi )
elseif( DASI_SOURCE_DIR )
	if( EXISTS "${PROJECT_SOURCE_DIR}/dasi" )
		get_filename_component( _existing_dasi "${PROJECT_SOURCE_DIR}/dasi" REALPATH )
		get_filename_component( _requested_dasi "${DASI_SOURCE_DIR}" REALPATH )
		if( NOT _existing_dasi STREQUAL _requested_dasi )
			message( FATAL_ERROR "Bundle contains another DASI checkout; use a clean bundle directory" )
		endif()
	endif()
	ecbuild_bundle( PROJECT dasi SOURCE "${DASI_SOURCE_DIR}" )
elseif( DASI_BRANCH )
	ecbuild_bundle( PROJECT dasi GIT "${DASI_REPOSITORY}" BRANCH "${DASI_BRANCH}" )
else()
	message( FATAL_ERROR "Set DASI_SOURCE_DIR for local development or DASI_BRANCH for a remote checkout" )
endif()

ecbuild_bundle_finalize()
