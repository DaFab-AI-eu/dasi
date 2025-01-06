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

from logging import DEBUG, INFO, NOTSET, WARNING, CRITICAL, getLogger

__logging_config__ = dict(
    version=1,
    formatters={
        "default": {
            "format": "%(name)-12s | %(message)s",
        },
        "compact": {
            "format": "%(message)s",
        },
        "debug": {
            "datefmt": "%d-%m-%Y [%H:%M:%S]",
            "format": "%(asctime)s %(name)-12s | %(levelname)-7s | %(message)s",
        },
    },
    handlers={
        "console": {
            "class": "logging.StreamHandler",
            "formatter": "compact",
            "stream": "ext://sys.stdout",
            "level": INFO,
        },
    },
    root={
        "handlers": ["console"],
        "level": NOTSET,
    },
)


def init_logging(name) -> None:
    from logging.config import dictConfig

    if _check_debug_arg():
        __logging_config__["handlers"]["console"]["level"] = DEBUG  # type: ignore
        __logging_config__["handlers"]["log_file"] = {  # type: ignore
            "class": "logging.handlers.RotatingFileHandler",
            "filename": "pydasi.log",
            "maxBytes": 102400,
            "backupCount": 0,
            "level": DEBUG,
            "formatter": "debug",
        }
        __logging_config__["root"]["handlers"].append("log_file")  # type: ignore
        __logging_config__["root"]["propagate"] = True  # type: ignore

    dictConfig(__logging_config__)

    getLogger().info("===============================================")
    getLogger().info("=   " + name + "   =")
    getLogger().info("===============================================")


def _check_debug_arg() -> bool:
    from argparse import ArgumentParser

    parser = ArgumentParser()
    parser.add_argument(
        "--debug",
        action="store_true",
        help="outputs level=DEBUG to log file.",
    )
    args, _ = parser.parse_known_args()

    return args.debug
