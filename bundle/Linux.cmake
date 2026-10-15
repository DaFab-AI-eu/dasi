########################################################################################################################
#
# Copyright 2024 European Centre for Medium-Range Weather Forecasts (ECMWF)
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
#
########################################################################################################################

macro( ecbuild_set_verbose )
    set( ${ARGV} )
    message( STATUS "setting: ${ARGV0} = ${ARGV1}" )
endmacro()

ecbuild_set_verbose( ENABLE_NETCDF           OFF  CACHE  BOOL "NetCDF" )
ecbuild_set_verbose( ENABLE_DUMMY_TAPES      OFF  CACHE  BOOL "Build dummy tape interface" )
ecbuild_set_verbose( ENABLE_JPG              OFF  CACHE  BOOL "no JPG" )
ecbuild_set_verbose( ENABLE_AEC              ON   CACHE  BOOL "AEC" )
ecbuild_set_verbose( ENABLE_POINTDB          ON   CACHE  BOOL "PointDB" )
ecbuild_set_verbose( ENABLE_PYTHON           ON   CACHE  BOOL "python" )
ecbuild_set_verbose( ENABLE_FORTRAN          ON   CACHE  BOOL "no Fortran" )
ecbuild_set_verbose( ENABLE_MPI              OFF  CACHE  BOOL "no MPI" )
ecbuild_set_verbose( ENABLE_EXAMPLES         OFF  CACHE  BOOL "no examples" )
ecbuild_set_verbose( ENABLE_ECCODES          OFF  CACHE  BOOL "eccodes" )
ecbuild_set_verbose( ENABLE_AWSSDK_S3        ON   CACHE  BOOL "eckit s3 support" )
ecbuild_set_verbose( ENABLE_S3FDB            ON   CACHE  BOOL "fdb s3 backend" )
ecbuild_set_verbose( ENABLE_TESTS            ON   CACHE  BOOL "test" )

# DASI options
ecbuild_set_verbose( BUILD_PYTHON      ON  CACHE   BOOL "Enable build pydasi" )
ecbuild_set_verbose( BUILD_EXAMPLES    ON  CACHE   BOOL "Enable build examples" )
ecbuild_set_verbose( BUILD_TESTING     ON  CACHE   BOOL "Enable build tests" )
