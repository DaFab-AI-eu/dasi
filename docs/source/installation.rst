.. _installation:

Installation
============

.. _install_libdasi:

DASI Library (C/C++)
--------------------

The supported way to install DASI library is by building from source.

Dependencies
~~~~~~~~~~~~

* C/C++ compiler
* `CMake`_
* `ecbuild`_
* `eckit`_
* `metkit`_
* `fdb`_


Build and Install
~~~~~~~~~~~~~~~~~

.. code-block:: shell

   git clone https://github.com/ecmwf-projects/dasi
   cd dasi

   # Setup environment variables (edit as needed)
   SRC_DIR=$(pwd)
   BUILD_DIR=build
   INSTALL_DIR=$HOME/local
   export eckit_DIR=$ECKIT_DIR
   export metkit_DIR=$METKIT_DIR
   export fdb5_DIR=$FDB_DIR

   # Create the the build directory
   mkdir $BUILD_DIR
   cd $BUILD_DIR

   # Run ecbuild (CMake)
   ecbuild --prefix=$INSTALL_DIR -- $SRC_DIR

   # Build and install
   make -j10
   make test      # optional
   make install

   # Check installation
   $INSTALL_DIR/bin/dasi --version


.. _install_pydasi:

DASI Python
-----------

The Python interface to DASI is called **pydasi**.
It uses the `cffi`_ Python package for interfacing with the DASI C API
(see :ref:`install_libdasi`).


Dependencies
~~~~~~~~~~~~

* `cffi`_
* :ref:`install_libdasi`


Python Environment Setup
~~~~~~~~~~~~~~~~~~~~~~~~

It is advised to create a virtual Python environment:

.. code-block:: console

   $ cd project_dir
   $ python -m venv .venv
   $ source .venv/bin/activate


Installation
~~~~~~~~~~~~

**pydasi** can be installed using **pip** command:

.. code-block:: console

   $ cd project_dir
   $ source .venv/bin/activate
   (.venv) $ pip install cffi
   (.venv) $ pip install pydasi



.. _`CMake`: https://cmake.org
.. _`ecbuild`: https://github.com/ecmwf/ecbuild
.. _`eckit`: https://github.com/ecmwf/eckit
.. _`metkit`: https://github.com/ecmwf/metkit
.. _`fdb`: https://github.com/ecmwf/fdb
.. _`cffi`: https://pypi.org/project/cffi/
