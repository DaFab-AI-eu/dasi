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

ecbuild_set_verbose( ENABLE_NETCDF           OFF  CACHE  BOOL "NetCDF" FORCE)
ecbuild_set_verbose( ENABLE_DUMMY_TAPES      OFF  CACHE  BOOL "Build dummy tape interface" FORCE)
ecbuild_set_verbose( ENABLE_JPG              OFF  CACHE  BOOL "no JPG" FORCE)
ecbuild_set_verbose( ENABLE_AEC              ON   CACHE  BOOL "AEC" FORCE)
ecbuild_set_verbose( ENABLE_POINTDB          ON   CACHE  BOOL "PointDB" FORCE)
ecbuild_set_verbose( ENABLE_PYTHON           ON   CACHE  BOOL "python" FORCE)
ecbuild_set_verbose( ENABLE_FORTRAN          ON   CACHE  BOOL "no Fortran" FORCE )
ecbuild_set_verbose( ENABLE_MPI              OFF  CACHE  BOOL "no MPI" FORCE )
ecbuild_set_verbose( ENABLE_EXAMPLES         OFF  CACHE  BOOL "no examples" FORCE )
ecbuild_set_verbose( ENABLE_ECCODES          OFF  CACHE  BOOL "eccodes" FORCE )
ecbuild_set_verbose( ENABLE_AWSSDK_S3        ON   CACHE  BOOL "eckit s3 support" FORCE )
ecbuild_set_verbose( ENABLE_S3FDB            ON   CACHE  BOOL "fdb s3 backend" FORCE )

# DASI options
ecbuild_set_verbose( BUILD_PYTHON      ON  CACHE   BOOL "Enable build pydasi"   FORCE )
ecbuild_set_verbose( BUILD_EXAMPLES    ON  CACHE   BOOL "Enable build examples" FORCE )
ecbuild_set_verbose( BUILD_TESTING     ON  CACHE   BOOL "Enable build tests"    FORCE )
