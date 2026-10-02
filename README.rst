Data Access and Storage Interface (DASI)
========================================

.. image:: https://readthedocs.org/projects/dasi/badge/?version=latest
    :target: https://dasi.readthedocs.io/en/latest/?badge=latest
    :alt: Documentation Status

.. image:: https://github.com/DaFab-AI-eu/dasi/actions/workflows/dev-image.yml/badge.svg
   :target: https://github.com/DaFab-AI-eu/dasi/actions/workflows/dev-image.yml
   :alt: Dev Image

.. image:: https://github.com/DaFab-AI-eu/dasi/actions/workflows/ci.yml/badge.svg
   :target: https://github.com/DaFab-AI-eu/dasi/actions/workflows/ci.yml
   :alt: CI

**DASI** is a metadata-driven data store. It is a semantic interface for data, where the data is indexed and uniquely identified by sets of scientifically-meaningful metadata keys.
DASI is modular and is compatible with multiple backends (i.e., object stores or POSIX) through diverse frontends (Python, C++, C).

**Warning**
This project is BETA and will be experimental for the foreseeable future. Interfaces and functionality are likely to change. DO NOT use this software in any project/software that is operational.


Introduction
============

DASI is built on top of FDB [1]_, which has been developed
at ECMWF [2]_ for previous EU projects (see `Acknowledgements <docs/source/acknowledgements.rst>`_) and
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


Containerized Installation
--------------------------

A containerized installation using `Docker <https://www.docker.com/>`_ is also available.
The Dockerfile has three stages: ``build-dependencies`` installs the toolchain,
``dev-env`` adds development tools, and ``dasi-runtime`` packages a tested installation.
Neither the toolchain nor the dev image contains DASI source code.

Local Development
~~~~~~~~~~~~~~~~~

Open this repository in VS Code and select **Dev Containers: Reopen in Container**.
The VS Code workspace is the bundle directory ``/workspace/bundle``, opened as the
non-root ``vscode`` user:

.. code-block:: text

   /workspace/bundle/
   ├── CMakeLists.txt, Linux.cmake, Dependencies.cmake  -> dasi/bundle/*
   ├── eckit/  metkit/  fdb/   (cloned from the branches in Dependencies.cmake)
   └── dasi/                   (this checkout, bind-mounted)

The bundle files are symlinks into the checkout, so editing them edits the
repository. ``eckit``, ``metkit`` and ``fdb`` are separate Git repositories kept in
a persistent volume; commit changes to them in their own repositories. Startup
configures the bundle and never switches any checkout's branch. Build trees,
installation files and compiler caches also use persistent volumes. The Python
interpreter is ``/workspace/bundle/dasi/.venv/bin/python``.

Optional overrides belong in ``.devcontainer/.env``, which is ignored by Git;
see ``.devcontainer/.env.example``. ``DEV_IMAGE`` selects the development image.

Pipeline scripts live in ``scripts/`` and are shared by the devcontainer and CI:
``configure.sh`` configures the bundle, ``build-and-test.sh`` builds, tests and
exports artifacts, and ``smoke-test.sh`` checks a runtime image.

Inside the devcontainer:

.. code-block:: shell

   cmake --build /tmp/build/dasi-bundle --parallel 2 --target all pydasi_develop

The devcontainer also starts Rucio and MinIO (``docker-compose.local.yml``
enables the ``integration`` profile). To start them for an already running
devcontainer, run from a host terminal:

.. code-block:: shell

   docker compose -p dasi -f .devcontainer/docker-compose.yml \
     -f .devcontainer/docker-compose.local.yml up -d rucio-setup

The shared initializers create the MinIO bucket and Rucio catalogue entries.
Local-only port bindings expose Rucio on ``localhost:8080`` and MinIO on
``https://localhost:9000``; CI does not publish host ports. The checked-in TLS
certificates and default credentials are for development only.

Once integration services are ready, run all tests and package runtime artifacts
inside the devcontainer:

.. code-block:: shell

   bash dasi/scripts/build-and-test.sh

This builds once in Release mode in ``/tmp/build/dasi-release`` (separate from
the Debug tree in ``/tmp/build/dasi-bundle``), runs all CTest suites and Python
tests, and exports the successful installation and wheel to ``.artifacts/``.
``fdb_move_auxiliary.sh`` and ``test_fdb5_s3_store`` (an upstream FDB S3 wipe
bug) are excluded. ``BUILD_JOBS`` and ``TEST_JOBS`` control parallelism; both
default to two.

After successful tests, a host terminal can package and check the same artifacts:

.. code-block:: shell

   docker build --target dasi-runtime --build-arg DASI_VERSION=0.3.1 -t dasi:local .
   docker run --rm -e DASI_EXPECTED_VERSION=0.3.1 \
     -v "$PWD/scripts/smoke-test.sh:/smoke.sh:ro" dasi:local bash /smoke.sh

The runtime does not clone or compile DASI again. Its smoke check verifies both
versions and a filesystem archive/retrieve round trip.

CI And Releases
~~~~~~~~~~~~~~~

``ci.yml`` and ``cd.yml`` use the same reusable ``build-test.yml`` pipeline.
Every normal CI run builds the requested checkout once and runs C++ and Python
tests. Environment images are reused by recipe hash and resolved to immutable
digests. Missing PR environments are built locally, without pushing PR images
or transferring image tarballs between jobs. BuildKit and compiler caches avoid
unnecessary work on reruns.

On trusted branch pushes, publication occurs only after tests and the installed
runtime smoke check succeed. Runtime cache tags identify the source commit and
environment digest. Test reports use the repository's normal artifact retention.

For a ``v*`` release tag, the pipeline first verifies that the tag matches the
checked-out DASI version. It can reuse a matching previously tested runtime;
otherwise it performs the full build and tests. CD promotes that tested digest
to release tags instead of doing an independent source checkout and rebuild.

CI builds and publishes the dev image whenever its recipe changes.
``dev-image.yml`` additionally rebuilds it weekly (or on demand) without layer
caching, to pick up upstream OS package updates.

eckit, metkit and fdb track the branches set in ``bundle/Dependencies.cmake``.
CI clones their current heads on every run. An existing clone, such as the one
in the devcontainer volume, stays at its commit until you run
``cmake --build /tmp/build/dasi-bundle --target update``. libaec and the AWS SDK
are pinned in the Dockerfile. Standalone bundle builds select the parent DASI
checkout automatically, or accept an explicit ``DASI_SOURCE_DIR``. Remote bundle
builds require ``DASI_BRANCH`` and optionally ``DASI_REPOSITORY``; there is no
implicit checkout of ``develop``.



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

Past and current funding and support for this project are listed in the `Acknowledgements <docs/source/acknowledgements.rst>`_.


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
