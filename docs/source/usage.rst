Usage
=====

S3 Usage
--------

DASI can be configured to use any object stores that support S3 API.

A complete example can be found in example `Simple S3 <https://github.com/ecmwf-projects/dasi/tree/0.2.4/examples/simple_s3>`_.


Configuration
~~~~~~~~~~~~~

.. code-block:: yaml

   schema: ./schema
   catalogue: toc
   spaces:
     - roots:
         - path: ./database
   store: s3
   s3:
     endpoint: "127.0.0.1:9000"
     credential: ./s3credentials.yml
     bucketPrefix: "dasi-"


Python Usage
------------

The Python interface to DASI is called **pydasi**.


.. note::

   Please refer to :ref:`install_pydasi` on how to install **pydasi**.


Simple Archive
~~~~~~~~~~~~~~

We use the :py:meth:`pydasi.dasi.Dasi.archive` function to save example data ``SIMPLE_DATA`` using the metadata ``key`` as:

.. code-block:: python

   from dasi import Dasi
   dasi = Dasi("config.yaml")
   key=["User":"muster", "Project":"simple", "Date":"16-03-23", "Type":"text"]
   data="SIMPLE_DATA"
   dasi.archive(key, data)

where, the example schema is:

.. code-block:: yaml

   [ User, Project
      [ Date
         [ Type ]]]

See :ref:`dasi-config` for config yaml..

Reference
~~~~~~~~~

.. automodule:: pydasi.dasi.Key

.. automethod:: pydasi.dasi.Dasi.archive
