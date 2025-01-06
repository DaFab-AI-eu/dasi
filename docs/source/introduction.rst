Introduction
============

DASI is built on top of FDB [#]_, which has been developed at ECMWF for previous EU projects (NEXTGenIO) and has been adapted to be highly configurable for different domains.
Using FDB allows DASI to use many backends, such as high-performance POSIX backend, Ceph object-storage backend, and NVRAM backend.

.. figure:: /_static/dasi-with-wps.png

   Relationship of DASI and the other applications in the IOSEA project.


.. _dasi-config:

Configuration
-------------

An example configuration in YAML format:

.. code-block:: yaml

    ---
    schema: /path/to/schema/file
    catalogue: toc
    store: file
    spaces:
        - roots:
            - path: /path/to/output/data


Schema
------

.. _dasi-schema:

DASI is domain-agnostic, and is configured for each scientific domain using a schema.
The schema defines the metadata keys which index and identify the data within a domain.

In a *schema* file, we define *rule(s)* using attributes in 3-levels, as shown below.

.. code-block:: yaml

   # rule 1
   [ class, experiment
      [ type, date?
         [step, name ] ] ]
   # rule 2
   [a1, a2, a3 ...[b1, b2, b3... [c1, c2, c3...]]]

- Attributes can be set *optional* by using "?", which will be replaced internally by an empty value.
- The first level (a) defines which attributes are used to name the top level directory
- The second level (b) defines which attributes are used to name the data files
- The third level (c) defines which attributes are used as index keys



.. rubric:: Footnotes

.. [#] `Fields DataBase (FDB) is a domain-specific object store <https://github.com/ecmwf/fdb>`_
