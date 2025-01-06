Data Access and Storage Interface (DASI)
========================================

.. image:: https://readthedocs.org/projects/dasi/badge/?version=latest
    :target: https://dasi.readthedocs.io/en/latest/?badge=latest
    :alt: Documentation Status


**DASI** is a metadata-driven data store. It is a semantic interface for data, where the data is indexed and uniquely identified by sets of scientifically-meaningful metadata keys.
DASI is modular and is compatible with multiple backends (i.e., object stores or POSIX) through diverse frontends (Python, C++, C).

**Warning**
This project is BETA and will be experimental for the foreseeable future. Interfaces and functionality are likely to change. DO NOT use this software in any project/software that is operational.


Introduction
============

DASI is built on top of FDB [1]_, which has been developed
at ECMWF [2]_ for previous EU projects (see `Acknowledgements <https://github.com/ecmwf-projects/dasi/blob/master/docs/acknowledgements.rst>`_) and
has been adapted to be highly configurable for different domains.
Using FDB allows DASI to use various backends, such as POSIX, Ceph and Cortx-Motr object-storage, and NVRAM backend.

Configuration
-------------

An example configuration:

.. code-block:: yaml

   ---
   schema: path/to/schema/file
   catalogue: toc
   store: file
   spaces:
      - roots:
        - path: path/to/data/output1
        - path: path/to/data/output2


Schema
------

The schema defines the metadata keys which index and identify the data within a domain.

An example schema describing a hierarchical taxonomy of metadata keys:

.. code-block:: yaml

   [ User, Laboratory?, Project
      [ DateTime, Processing
         [ Type, Object ]]]

Installation
============

DASI Library (C/C++)
--------------------

The supported way to install DASI library is by building from source.

Dependencies
~~~~~~~~~~~~

* C/C++ compiler
* `CMake`_
* `eckit`_
* `fdb`_
* `AWS SDK C++<https://github.com/aws/aws-sdk-cpp>`_ (only if S3 enabled for FDB)

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


**Note** To enable S3 support, use the following cmake options:
`-DENABLE_AWS_S3:BOOL=TRUE -DAWSSDK_ROOT:STRING=/path/to/AWSSDK`

Install pydasi
--------------

The Python interface to DASI is called **pydasi**.
It uses the `cffi`_ Python package for interfacing with the DASI C API.

Dependencies
~~~~~~~~~~~~

* `DASI Library (C/C++)`_
* `cffi`_


Optional: Python Environment Setup
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

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



Contributions
=============

Have any feedback / questions / comments / issues ? You can post them `here <https://github.com/ecmwf-projects/dasi/issues>`_.

The main repository is hosted on GitHub; testing, bug reports and contributions are highly welcomed and appreciated.

See also the `contributors <https://github.com/ecmwf-projects/dasi/contributors>`_ for a more complete list.

Contacts:

- James Hawkes [2]_
- Simon Smart [2]_
- Tiago Quintino [2]_

Acknowledgements
================

Past and current funding and support for this project are listed in the `Acknowledgements <https://github.com/ecmwf-projects/dasi/docs/acknowledgements.rst>`_.


License
=======

This software is licensed under the terms of the Apache License Version 2.0 which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.

In applying this license, ECMWF does not waive the privileges and immunities granted to it by virtue of its status as an intergovernmental organisation nor does it submit to any jurisdiction.

.. |License| image:: https://img.shields.io/badge/License-Apache%202.0-blue.svg
   :target: https://github.com/ecmwf/dasi/blob/develop/LICENSE
   :alt: Apache License


Footnotes
=========

.. [1] "Fields DataBase (`FDB <https://github.com/ecmwf/fdb>`_) is a domain-specific object store"
.. [2] "European Centre for Medium-Range Weather Forecasts (`ECMWF <https://www.ecmwf.int>`_)"
