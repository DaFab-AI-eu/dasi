## 0.2.7 (2026-01-29)

### Feat

- **docker**: add containerized install to readme
- **docker**: add basic runtime test
- **pydasi**: read version from package

## 0.2.2 (2023-09-07)

### Feat

- **pydasi.list**: added __str__ and fixed null error
- **pydasi**: added __str__
- **pydasi**: handle empty list and retrieve

### Fix

- **doc**: broken img caused by git lfs
- **doc**: remove images subfolder
- **doc**: RTD broken images
- **doc**: broken image on RTD
- **pydasi**: remove import side effects
- **doc**: added requirements.txt
- **doc**: sphinx api documentation
- **doc**: remove pdf format
- **doc**: don't fail on warning
- **histogram**: missing file error

## 0.2.1 (2023-08-23)

### Feat

- **pydasi**: load config via str or file path
- **pydasi**: allows config as string
- **dasi-schema**: added --list option
- **dasi-tool**: added dasi-schema tool
- **api**: added dump schema
- **doc**: add draft using sphinx
- **pydasi**: key compare, repr, and more tests
- **pydasi**: retrieve reads all attributes
- **pydasi**: add list and refactor C API time type
- **pydasi**: add get_value for query
- **pydasi**: add Query, Retrieve, logger, and tests
- **pydasi**: add plot.py to cryoem example
- **pydasi**: update example cryoem
- **pydasi**: add example case - cryoem
- **pydasi**: add dasi session and improve key.py
- **pydasi**: add ffi encode/decode
- **pydasi**: add logger
- **pydasi**: add dasi cffi library, c header, header builder, and Key class
- **c api**: add dasi_initialise_api and version functions
- **pydasi**: initial commit
- **c api**: add dasi_list_delete
- **c api**: add list and tests
- start new c api
- **retrieve**: c api: retrieve result
- add c api

### Fix

- **pydasi**: added packaging dependency
- **pydasi**: remove deprecated distutils
- **examples**: simplified code, config uses yml extension
- **pydasi**: fixes editable installs, moved new_ to backend, restructured directories
- **weather**: removed dependency on ecbuild
- **test**: zero size array
- **dasi**: add missing headers
- **c api**: add throw not found key
- **c api**: add missing cstring headers
- **pydasi**: example cryoem uses retrieve with multiple Objects in single query
- **pydasi**: corrected query to work w/ list(value) add test for query
- **c_api**: replace time_t with dasi_time_t (opaque time type)
- **weather**: working example
- **weather**: adapt to new c api and add retrieve
- **weather**: adapt to c api changes
- **c_api**: invalid bool type
- **c api**: delete prev element while traversing
- **c api**: add const
- **c api**: set key to null after delete
- **c api**: set query null after delete
- **c example**: fix c example: weather
- missing includes

### Refactor

- **pydasi**: renamed dirs, fixes name collision issues
- **pydasi**: folder structure and find library
- **pydasi**: remove unused bytearray in ffi_encode
- **c api**: incorporated PR comments for dasi_new_retrieve
- **c api**: incorporated PR comments
- **pydasi**: incorporated some of the PR comments
- **pydasi**: retrieve.read
- **c api**: add _new_ to retrieve and list api
- **c api**: inheritance model
- **c api**: rm pointer from dasi_error_t
- **c api**: rm pointer from dasi_t
- **c api**: rm pointer from dasi_key_t
- **c api**: query typedef
- cosmetic

## 0.1.0 (2022-03-29)
