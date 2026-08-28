#!/bin/bash
#
# Download the stock Godot editor pinned in scripts/GODOT_VERSION.txt into
# godot-bin/ (gitignored), for scripts/run-tests.sh to run the headless suite
# against.
#
# Nothing calls this on your behalf. run-tests.sh looks for a binary you
# already have -- godot=<path>, $GODOT, godot-bin/, godot/bin/, PATH -- and
# prints this command rather than downloading tens of MB without being asked.
#
# The editor build is the whole download, and it is all that is needed: the
# export templates are another ~1.2 GB and the suite exports nothing.
#
# This is deliberately NOT the same binary as godot/bin/*. That one is a custom
# module build of the engine, which has SpriteStudio compiled in -- it would
# register the classes a second time and abort on the extension's own project.
# Reach for it only with a project that carries no addons/spritestudio.
#
# Usage: scripts/fetch-godot.sh [force=yes] [out=<dir>]
#   force : yes to re-download even when the pinned version is installed
#   out   : install directory (default: godot-bin/)
#
# Requires curl and unzip.
set -euo pipefail

APP=$(basename "$0")
SCRIPTDIR=$(cd "$(dirname "$0")" && pwd -P)
ROOTDIR=$(cd "$SCRIPTDIR/.." && pwd -P)

usage() { sed -n '/^# Usage:/,/^# Requires/p' "$0" | sed 's/^# \{0,1\}//'; }

FORCE="no"
OUT_DIR="${ROOTDIR}/godot-bin"
for item in "$@"; do
  case "$item" in
    force=*) FORCE="${item#*=}" ;;
    out=*)   OUT_DIR="${item#*=}" ;;
    -h|--help|help) usage; exit 0 ;;
    *) echo "$APP: unknown arg '$item'" >&2; usage; exit 2 ;;
  esac
done

VERSION=$(tr -d ' \r\n' < "${SCRIPTDIR}/GODOT_VERSION.txt")
VERSION_FILE="${OUT_DIR}/VERSION"
CACHE_DIR="${SCRIPTDIR}/.godot-cache"

if [ "$FORCE" != "yes" ] && [ -f "$VERSION_FILE" ] \
   && [ "$(tr -d ' \r\n' < "$VERSION_FILE")" = "$VERSION" ]; then
  echo "$APP: Godot $VERSION is already installed in $(basename "$OUT_DIR")/. Nothing to do."
  exit 0
fi

# The asset name encodes the platform; ARM Linux and 32-bit are not published as
# editor builds, so those hosts have to bring their own binary.
case "$(uname -s)" in
  Darwin) ASSET="Godot_v${VERSION}_macos.universal.zip" ;;
  Linux)
    case "$(uname -m)" in
      x86_64) ASSET="Godot_v${VERSION}_linux.x86_64.zip" ;;
      *) echo "$APP: no published editor build for linux $(uname -m) — install Godot $VERSION yourself and pass godot=<path> to run-tests.sh" >&2; exit 1 ;;
    esac ;;
  *) ASSET="Godot_v${VERSION}_win64.exe.zip" ;;
esac

URL="https://github.com/godotengine/godot-builds/releases/download/${VERSION}/${ASSET}"
ZIP_FILE="${CACHE_DIR}/${ASSET}"

command -v curl >/dev/null || { echo "$APP: curl not on PATH" >&2; exit 1; }
command -v unzip >/dev/null || { echo "$APP: unzip not on PATH" >&2; exit 1; }

mkdir -p "$CACHE_DIR"
if [ ! -f "$ZIP_FILE" ] || [ "$FORCE" = "yes" ]; then
  echo "$APP: downloading $ASSET"
  curl -fL --progress-bar "$URL" -o "${ZIP_FILE}.part"
  mv "${ZIP_FILE}.part" "$ZIP_FILE"
else
  echo "$APP: reusing cached $(basename "$ZIP_FILE")"
fi

rm -rf "$OUT_DIR"
mkdir -p "$OUT_DIR"
unzip -q "$ZIP_FILE" -d "$OUT_DIR"

# Normalise what the archives unpack to, so run-tests.sh has one path per host
# rather than a version-stamped name that changes with every bump.
case "$(uname -s)" in
  Darwin) : ;;   # Godot.app, already a stable name
  Linux)
    BIN=$(find "$OUT_DIR" -maxdepth 1 -type f -name 'Godot_v*' | head -n 1)
    [ -n "$BIN" ] && mv "$BIN" "${OUT_DIR}/godot" && chmod +x "${OUT_DIR}/godot" ;;
  *)
    BIN=$(find "$OUT_DIR" -maxdepth 1 -type f -name 'Godot_v*.exe' | head -n 1)
    [ -n "$BIN" ] && mv "$BIN" "${OUT_DIR}/godot.exe" ;;
esac

printf '%s\n' "$VERSION" > "$VERSION_FILE"
echo "$APP: Godot $VERSION installed in $(basename "$OUT_DIR")/"
