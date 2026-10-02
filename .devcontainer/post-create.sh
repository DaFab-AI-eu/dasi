#!/usr/bin/env bash
set -euo pipefail

owner="$(id -u):$(id -g)"
# Volumes keep the image's UID, which differs once the dev user is remapped to the host UID.
sudo -n find /workspace/bundle -path /workspace/bundle/dasi -prune -o -exec chown -h "$owner" {} +
sudo -n chown -R "$owner" /tmp/build /workspace/.ccache /workspace/install
# An earlier root-based devcontainer left root-owned files in the checkout.
sudo -n find /workspace/bundle/dasi -path /workspace/bundle/dasi/.git -prune -o -uid 0 -exec chown -h "$owner" {} +
