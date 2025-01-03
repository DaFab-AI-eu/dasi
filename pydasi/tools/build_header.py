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

import sys

from pycparser import c_generator, parse_file


def usage():
    sys.stderr.write("Usage:\n")
    sys.stderr.write("\tpython build_header.py <header.h> <header_cffi.h>\n")


if __name__ == "__main__":
    if len(sys.argv) != 3:
        usage()
        sys.exit(-1)

    input_filename = sys.argv[1]
    output_filename = sys.argv[2]

    ast = parse_file(
        input_filename,
        use_cpp=True,
        cpp_path="gcc",
        cpp_args="-E",
    )
    # ast.show()
    with open(output_filename, "w") as f:
        f.write(c_generator.CGenerator().visit(ast))
