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

import os
import sys

from pycparser import c_generator, parse_file


def usage():
    sys.stderr.write("Usage:\n")
    sys.stderr.write(
        f"\tpython {os.path.basename(__file__)} <c_header.h> <cffi.h>\n"
    )
    sys.exit(1)


if __name__ == "__main__":
    if len(sys.argv) != 3:
        sys.stderr.write("Error: incorrect arguements!\n")
        usage()

    input_file = sys.argv[1]

    if not os.path.isfile(input_file) or not input_file.endswith(".h"):
        sys.stderr.write("Error: The input file is not a header.\n")
        usage()

    # output_filename = input_file.replace(".h", "_cffi.h")
    output_filename = sys.argv[2]

    ast = parse_file(
        input_file,
        use_cpp=True,
        cpp_path="gcc",
        cpp_args="-E",
    )

    with open(output_filename, "w") as f:
        f.write(c_generator.CGenerator().visit(ast))
