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


def cmdline_args():
    from argparse import ArgumentParser

    parser = ArgumentParser()
    parser.add_argument("-c", "--config", default="dasi.yaml")
    parser.add_argument("-u", "--UserID", default="0001-2345-6789")
    parser.add_argument("-i", "--Institute", default="ECMWF")
    parser.add_argument("-p", "--Project", default="IOSEA")
    parser.add_argument("-t", "--Type", default="demo")
    parser.add_argument("-d", "--Directory", default="data")
    args, _ = parser.parse_known_args()
    return args


def plot_histogram(image: bytearray, name: str):
    from matplotlib import pyplot as plt

    plot_name = "./%s.png" % name

    print("plot: %s" % plot_name)

    plt.hist(x=image, density=True, bins=40)
    plt.xlabel("Data")
    plt.ylabel("Value")
    plt.title("Histogram of {}".format(name))
    plt.savefig(plot_name)
    plt.close()


class DirectoryStore:
    """
    Stores the files in a directory.
    """

    def __init__(self, path: str):
        import os

        self.__metadata = {}
        self.__filepaths: list[str] = []

        with os.scandir(path) as it:
            for entry in it:
                if entry.is_file():
                    self.__filepaths.append(entry.path)

        self.__metadata["NumberOfFiles"] = len(self.__filepaths)

    def files(self):
        from os import path as ospath

        for path in self.__filepaths:
            if ospath.exists(path):
                with open(path, "rb") as f:
                    name, ext = ospath.splitext(ospath.basename(path))
                    yield name, ext[1:], f.read()

    @property
    def metadata(self):
        return self.__metadata
