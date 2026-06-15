# DASI Example: Image Processing (Histogram)

This example demonstrates an image processing workflow.

## Requirements

* Python (3.6 or higher),
* [pydasi](https://pypi.org/project/pydasi/) (the python interface to DASI),
* `matplotlib`.

## How-to

### 1. Setup:

Replace the paths in `dasi.yml` such that they point to your local repository:

```yaml
schema: SET_PATH_TO_DASI_SCHEMA_FILE
- path: SET_PATH_TO_OUTPUT
```

Make sure the DASI library can be found:

**Option 1.** update `LD_LIBRARY_PATH` to point to `libdasi.so` (linux) or `libdasi.dylib` (macos).

```bash
export LD_LIBRARY_PATH="PATH_TO_DASI_LIBRARY:$LD_LIBRARY_PATH"
```

**Option 2.** pass the environment variable `DASI_DIR` via command line:

```bash
DASI_DIR="PATH_TO_DASI_LIBRARY" python3 example.py
```

### 2. Archive

Archive images (in data folder) to the DASI store.

```
python archive_data.py
```

![original image](data/mic_Feb03_12.26.11.tif)

### 3. Retrieve:

Retrieve images from the DASI store, and make histogram plot of the image.

```
python retrieve_data.py
```

![histogram plot](mic_Feb03_12.26.11.tif.png)

## DASI schema

The DASI schema uses the following keys:

* **User** The OrcID of the user
* **Project** The project name
* **DateTime** The archive timestamp (`YYYYMMDDHHMM`)
* **Processing** The processing type (`g1`, `g2`)
* **Type** The measurement type (`spa`, `tomo`, `edt`, `FIB-SEM`, `screening`)
* **Object** The data type (`metadata`, `image`)
