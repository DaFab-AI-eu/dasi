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

project( dasi VERSION 0.2.5
              DESCRIPTION "Data Access and Storage Interface (DASI)"
              LANGUAGES C CXX )

set( CMAKE_CXX_STANDARD 17 )
set( CMAKE_CXX_EXTENSIONS OFF )
set( CMAKE_CXX_STANDARD_REQUIRED ON )

########################################################################################################################
# dependencies

find_package( eckit 1.28 REQUIRED )
find_package( fdb5 5.13 REQUIRED )

########################################################################################################################
# options

option( BUILD_SHARED_LIBS "Build using shared libraries" ON )

option( BUILD_PYTHON "Build the python package" OFF )

option( BUILD_EXAMPLES "Build the examples" OFF )

option( BUILD_TESTING "Build the tests" OFF )

########################################################################################################################

list( APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake" )

include( cmake/ProjectSettings.cmake )

include( CTest )

########################################################################################################################

add_subdirectory( src )

if( BUILD_PYTHON )
    add_subdirectory( pydasi )
endif()

if( BUILD_TESTING )
    add_subdirectory( tests )
endif()

if( BUILD_EXAMPLES )
    add_subdirectory( examples )
endif()

# install package

include( cmake/install_project.cmake )

########################################################################################################################
