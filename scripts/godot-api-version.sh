#!/usr/bin/env bash
#
# Print the Godot API the extension targets -- its floor.
#
#   scripts/godot-api-version.sh          # 4.7
#
# It is `compatibility_minimum` in misc/spritestudio.gdextension, which Godot
# reads at load time. build-extension and build-release take it as their default
# api_version, and release.yml builds every platform against it. The custom
# module's guard in ss_player/ss_macros.h and the docs name it as well;
# scripts/check-godot-versions.sh says where they disagree.
set -euo pipefail

APP="$(basename "$0")"
ROOT_DIR="$(cd "$(dirname "$0")/.." && pwd)"

if [ "$#" -gt 0 ]; then
  echo "$APP: takes no arguments (got '$1')" >&2
  exit 2
fi

v="$(sed -n 's/^compatibility_minimum *= *"\([0-9][0-9]*\.[0-9][0-9]*\)".*$/\1/p' "$ROOT_DIR/misc/spritestudio.gdextension")"
if [ -z "$v" ]; then
  echo "$APP: misc/spritestudio.gdextension has no compatibility_minimum" >&2
  exit 1
fi
echo "$v"
