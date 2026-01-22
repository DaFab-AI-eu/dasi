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

if( NOT DEFINED PROJECT_NAME )
    message( FATAL_ERROR "This CMakeLists.txt file should not be included directly!" )
endif()

configure_file( src/utils/version.py.in
                ${CMAKE_CURRENT_SOURCE_DIR}/src/utils/version.py @ONLY )

find_package( Python3 REQUIRED COMPONENTS Interpreter )

if( DEFINED ENV{CONDA_PREFIX} )
    message( FATAL_ERROR "BUILD_PYTHON set but Conda environment is not supported!" )
endif()

if( NOT DEFINED ENV{VIRTUAL_ENV} )

    set( _venv_path ${PROJECT_SOURCE_DIR}/.venv )

    message( STATUS "Creating virtual environment: ${_venv_path}" )

    execute_process( COMMAND ${Python3_EXECUTABLE} -m venv ${_venv_path} )

    set( ENV{VIRTUAL_ENV} ${PROJECT_SOURCE_DIR}/.venv )

    set( Python3_FIND_VIRTUALENV FIRST )

    unset( Python3_EXECUTABLE )

    # now find venv python
    find_package( Python3 REQUIRED COMPONENTS Interpreter )

    execute_process( COMMAND ${Python3_EXECUTABLE} -m pip install --upgrade pip )

endif( )

# generate the cffi header from the dasi_c.h

set( _c_header ${PROJECT_SOURCE_DIR}/include/dasi/api/dasi_c.h )
set( _cffi_header ${CMAKE_CURRENT_SOURCE_DIR}/src/backend/include/dasi_c_cffi.h )

add_custom_command(
    OUTPUT
        ${_cffi_header}
    DEPENDS
        ${_c_header}
    WORKING_DIRECTORY
        ${CMAKE_CURRENT_SOURCE_DIR}
    COMMAND
        ${Python3_EXECUTABLE} -m pip install pycparser
    COMMAND
        ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/generate_cffi.py
        ${_c_header} ${_cffi_header}
    COMMENT "Generating ${_cffi_header} ..."
    VERBATIM
)

# copy libdasi.so to pydasi backend

set( _backend_lib ${CMAKE_CURRENT_SOURCE_DIR}/src/backend/libs/${CMAKE_SYSTEM_NAME} )

add_custom_target( pydasi_backend
    DEPENDS
        ${PROJECT_NAME} ${_cffi_header}
    WORKING_DIRECTORY
        ${CMAKE_CURRENT_SOURCE_DIR}
    COMMAND ${CMAKE_COMMAND} -E echo "Copying libdasi to pydasi backend ..."
    COMMAND ${CMAKE_COMMAND} -E make_directory ${_backend_lib}
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:${PROJECT_NAME}> ${_backend_lib}
    VERBATIM
)

# install pydasi package in editable mode with dev dependencies

add_custom_target( pydasi_develop
    DEPENDS
        pydasi_backend
    WORKING_DIRECTORY
        ${CMAKE_CURRENT_SOURCE_DIR}
    COMMAND
        ${Python3_EXECUTABLE} -m pip install -e "${CMAKE_CURRENT_SOURCE_DIR}[dev,tests]"
    COMMENT "Installing pydasi in editable mode with dev dependencies ..."
    VERBATIM
)

# package pydasi

add_custom_target( pydasi_package
    DEPENDS
        pydasi_backend
    WORKING_DIRECTORY
        ${CMAKE_CURRENT_SOURCE_DIR}
    COMMAND
        ${Python3_EXECUTABLE} -m pip install --upgrade build
    COMMAND
        ${Python3_EXECUTABLE} -m build --wheel
    COMMENT "Packaging pydasi ..."
)

# run pydasi tests

add_custom_target( pydasi_test
    DEPENDS
        pydasi_develop
    WORKING_DIRECTORY
        ${CMAKE_CURRENT_SOURCE_DIR}
    COMMAND
        ${CMAKE_COMMAND} -E env LD_LIBRARY_PATH=${CMAKE_BINARY_DIR}/lib:$ENV{LD_LIBRARY_PATH}
            ${Python3_EXECUTABLE} -m pytest tests/ -v
    COMMENT "Running pydasi tests ..."
    VERBATIM
)

# format pydasi code

add_custom_target( pydasi_format
    WORKING_DIRECTORY
        ${CMAKE_CURRENT_SOURCE_DIR}
    COMMAND
        ${Python3_EXECUTABLE} -m black src/ tests/
    COMMAND
        ${Python3_EXECUTABLE} -m isort src/ tests/
    COMMENT "Formatting pydasi code with black and isort ..."
    VERBATIM
)

# lint pydasi code

add_custom_target( pydasi_lint
    WORKING_DIRECTORY
        ${CMAKE_CURRENT_SOURCE_DIR}
    COMMAND
        ${Python3_EXECUTABLE} -m ruff check src/ tests/
    COMMAND
        ${Python3_EXECUTABLE} -m mypy src/pydasi/
    COMMENT "Linting pydasi code with ruff and mypy ..."
    VERBATIM
)

# run tests with coverage

add_custom_target( pydasi_coverage
    WORKING_DIRECTORY
        ${CMAKE_CURRENT_SOURCE_DIR}
    COMMAND
        ${Python3_EXECUTABLE} -m pytest tests/ --cov=pydasi --cov-report=html --cov-report=term
    COMMENT "Running pydasi tests with coverage ..."
    VERBATIM
)
