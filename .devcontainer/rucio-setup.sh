#!/usr/bin/env bash
# One-time Rucio catalogue bootstrap: scope, RSE, protocol, quota.
# Run automatically as postStartCommand from devcontainer.json.
# Idempotent: skips if RSE already exists in the catalogue.
set -euo pipefail

RSE_NAME="${RUCIO_RSE_NAME:-MINIORSE}"
SCOPE="${RUCIO_SCOPE:-user.root}"
ACCOUNT="${RUCIO_ACCOUNT:-root}"
BUCKET="${MINIO_BUCKET:-rucio}"
S3_ACCESS_KEY="${AWS_ACCESS_KEY_ID:-${MINIO_ROOT_USER:-minioadmin}}"
S3_SECRET_KEY="${AWS_SECRET_ACCESS_KEY:-${MINIO_ROOT_PASSWORD:-minioadmin}}"
MINIO_ENDPOINT="${MINIO_HOST:-https://minio:9000}"
MINIO_ENDPOINT="${MINIO_ENDPOINT#http://}"
MINIO_ENDPOINT="${MINIO_ENDPOINT#https://}"
MINIO_ENDPOINT="${MINIO_ENDPOINT%/}"
MINIO_HOSTNAME="${MINIO_HOSTNAME:-${MINIO_ENDPOINT%%:*}}"
if [[ "$MINIO_ENDPOINT" == *:* ]]; then
    DEFAULT_MINIO_PORT="${MINIO_ENDPOINT##*:}"
else
    DEFAULT_MINIO_PORT="9000"
fi
MINIO_PORT="${MINIO_PORT:-$DEFAULT_MINIO_PORT}"

if ! command -v rucio >/dev/null 2>&1; then
    echo "rucio-setup: rucio CLI not found; installing rucio-clients..."
    python -m pip install -q --no-cache-dir boto3 "rucio-clients==40.2.0"
    hash -r
fi

if ! command -v rucio >/dev/null 2>&1; then
    echo "rucio-setup: error: rucio CLI not found in PATH after installation attempt." >&2
    exit 1
fi

RUCIO_PING_URL="${RUCIO_HOST:-http://rucio-server:80}/ping"
MAX_WAIT_SECONDS="${RUCIO_SETUP_MAX_WAIT_SECONDS:-120}"
WAIT_STEP_SECONDS=2
ATTEMPTS=$((MAX_WAIT_SECONDS / WAIT_STEP_SECONDS))
if [[ "$ATTEMPTS" -lt 1 ]]; then
    ATTEMPTS=1
fi

# Wait until the Rucio server is reachable.
echo "rucio-setup: waiting for rucio server at ${RUCIO_PING_URL}..."
for ((i=1; i<=ATTEMPTS; i++)); do
    if curl -sf "${RUCIO_PING_URL}" >/dev/null; then
        break
    fi
    sleep "$WAIT_STEP_SECONDS"
done

if ! curl -sf "${RUCIO_PING_URL}" >/dev/null; then
    echo "rucio-setup: error: server did not become reachable within ${MAX_WAIT_SECONDS}s." >&2
    exit 1
fi

if ! rucio ping &>/dev/null; then
    echo "rucio-setup: error: rucio CLI cannot reach/authenticate against ${RUCIO_HOST:-http://rucio-server:80}." >&2
    exit 1
fi

echo "rucio-setup: server is up."

# Suppress "already exists" errors so re-runs are safe.
_rucio() { rucio "$@" 2>&1 | grep -v "already exists\|Duplicate\|exists" || true; }

if rucio rse list 2>/dev/null | grep -q "^${RSE_NAME}$"; then
    echo "rucio-setup: RSE ${RSE_NAME} already exists; enforcing protocol configuration."
else
    echo "rucio-setup: creating scope ${SCOPE}..."
    _rucio scope add --account "$ACCOUNT" "$SCOPE"

    echo "rucio-setup: creating RSE ${RSE_NAME}..."
    _rucio rse add "$RSE_NAME"
fi

echo "rucio-setup: setting RSE attribute type=DISK..."
_rucio rse attribute add "$RSE_NAME" --key type --value DISK

echo "rucio-setup: removing stale S3 protocol definitions for ${RSE_NAME}..."
rucio rse protocol remove "$RSE_NAME" --scheme s3 --hostname "$MINIO_HOSTNAME" --port "$MINIO_PORT" >/dev/null 2>&1 || true

echo "rucio-setup: registering S3 protocol (MinIO ${MINIO_HOSTNAME}:${MINIO_PORT}, bucket=${BUCKET})..."
_rucio rse protocol add "$RSE_NAME" \
    --hostname "$MINIO_HOSTNAME" \
    --port "$MINIO_PORT" \
    --scheme s3 \
    --prefix "/${BUCKET}/" \
    --impl rucio_s3boto3.Default \
    --domain-json '{"lan":{"read":1,"write":1,"delete":1},"wan":{"read":1,"write":1,"delete":1,"third_party_copy_read":1,"third_party_copy_write":1}}' \
    --extended-attributes-json "{\"s3_access_key\":\"${S3_ACCESS_KEY}\",\"s3_secret_key\":\"${S3_SECRET_KEY}\",\"s3_endpoint_url\":\"${MINIO_HOST:-https://minio:9000}\"}"

echo "rucio-setup: granting quota (10GB) for ${ACCOUNT} on ${RSE_NAME}..."
_rucio account limit add "$ACCOUNT" --rse "$RSE_NAME" --bytes 10GB --locality local

echo "rucio-setup: fail-fast PFN check via server lfns2pfns..."
python - <<PY
from rucio.client import Client

rse = "${RSE_NAME}"
scope = "${SCOPE}"
probe_name = "rucio_pfn_probe.dat"

client = Client()
try:
    resolved = client.lfns2pfns(rse, [f"{scope}:{probe_name}"])
except Exception as exc:
    raise SystemExit(
        "rucio-setup: PFN resolution check failed via server API. "
        "Ensure protocol impl is importable in rucio-server (rucio_s3boto3.Default). "
        f"Original error: {exc}"
    )

if not resolved:
    raise SystemExit(
        "rucio-setup: PFN resolution returned empty result. "
        "Server-side protocol configuration is incomplete."
    )

print(f"rucio-setup: PFN resolution OK for {scope}:{probe_name}")
PY

echo "rucio-setup: done."
