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

from dasi import Dasi


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
        print("Retrived %d files." % len(retrieved))

    # Map the (file) names to data
    files: dict[str, bytearray] = {}

    for item in retrieved:
        name = item.key["Name"]
        files[name] = item.data

    return files


if __name__ == "__main__":
    args = cmdline_args()

    query = {
        "UserID": [args.UserID],
        "Institute": [args.Institute],
        "Project": [args.Project],
        "Type": [args.Type],
        "Directory": [args.Directory],
        "Date": ["01-01-2023"],
    }

    session = Dasi("./dasi.yml")

    query["Name"] = query_names(session, query)

    files = retrieve_files(session, query)

    # Work with the files
    for name, data in files.items():
        print("--- [%s] ---" % name)
        if name.endswith("mdoc"):  # mdoc file
            print("Content: %s" % data.decode())
        elif name.endswith("tif"):  # image file
            plot_histogram(data, name)

    print("Finished!")
