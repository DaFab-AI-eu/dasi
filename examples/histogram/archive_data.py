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

import logging

from helper import DirectoryStore, cmdline_args, setup_logging

from pydasi import Dasi

logger = logging.getLogger(__name__)


def main():
    setup_logging()

    args = cmdline_args()
    key = {
        "UserID": args.UserID,
        "Institute": args.Institute,
        "Project": args.Project,
        "Type": args.Type,
        "Directory": args.Directory,
    }
    key["Date"] = "01-01-2023"

    dir = DirectoryStore(args.Directory)

    logger.info("Archiving data from: %s", args.Directory)

    session = Dasi(args.config)

    for name, ext, data in dir.files():
        logger.info("Archiving: %s", name)
        key["Name"] = name
        key["Type"] = ext
        session.archive(key, data)
        logger.info("Archived: %s.%s", name, ext)

    logger.info("Finished!")


if __name__ == "__main__":
    main()
