#!/usr/bin/env bash
set -euo pipefail

if [[ -f /var/lib/rucio-init/.done ]]; then
    echo "rucio-init already completed; skipping"
    exit 0
fi

set +e
python3 - <<'PY'
import os
import re
import sys

from sqlalchemy import create_engine, text
from rucio.common.config import config_get

# Prefer explicit env override from docker-compose; fallback to config file.
db_url = os.environ.get('RUCIO_CFG_DATABASE_DEFAULT') or config_get('database', 'default')
# rucio-init:40+ ships psycopg v3, not psycopg2 - normalize dialect.
db_url = re.sub(r'^postgresql(?:\+psycopg2)?://', 'postgresql+psycopg://', db_url)

engine = create_engine(db_url)
with engine.connect() as conn:
    has_vos = conn.execute(
        text("SELECT 1 FROM information_schema.tables WHERE table_name='vos' LIMIT 1")
    ).scalar() is not None
    if has_vos:
        has_def_vo = conn.execute(text("SELECT 1 FROM vos WHERE vo='def' LIMIT 1")).scalar() is not None
        if has_def_vo:
            print("rucio-init detected existing bootstrap state; skipping")
            sys.exit(10)
PY
rc=$?
set -e

if [[ "$rc" -eq 10 ]]; then
    touch /var/lib/rucio-init/.done
    exit 0
elif [[ "$rc" -ne 0 ]]; then
    exit "$rc"
fi

/docker-entrypoint.sh
touch /var/lib/rucio-init/.done
