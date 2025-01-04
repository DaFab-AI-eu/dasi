# Copyright 2023 European Centre for Medium-Range Weather Forecasts (ECMWF)
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


from helper import cmdline_args, plot_histogram

from pydasi import Dasi


def query_names(session: Dasi, query):
    """Query the filenames"""

    names: list[str] = []
    for item in session.list(query):
        file = item.key["Name"]
        names.append(file)

    return names


def retrieve_files(session: Dasi, query):
    """Retrieve the data"""

    retrieved = session.retrieve(query)

    if len(retrieved) < 1:
        exit("No results from retrieve!\nQuery={}\n".format(query))
    else:
        print("Retrieved %d files." % len(retrieved))

    # Map the (file) names to data
    files: dict[str, bytearray] = {}

    for item in retrieved:
        name = item.key["Name"]
        files[name] = item.data

    return files


def main():
    args = cmdline_args()

    query = {
        "UserID": [args.UserID],
        "Institute": [args.Institute],
        "Project": [args.Project],
        "Type": [args.Type],
        "Directory": [args.Directory],
        "Date": ["01-01-2023"],
    }

    session = Dasi(args.config)

    # Setup query
    query["Type"] = ["tif", "mdoc"]

    query["Name"] = []
    for item in session.list(query):
        query["Name"].append(item.key["Name"])

    # Retrieve data
    for item in session.retrieve(query):
        name = item.key["Name"]
        ext = item.key["Type"]
        print("--- [%s] ---" % name)
        if ext == "mdoc":  # mdoc file
            print("Content: %s" % item.data.decode())
        elif ext == "tif":  # image file
            plot_histogram(item.data, name)

    print("Finished!")


if __name__ == "__main__":
    main()
