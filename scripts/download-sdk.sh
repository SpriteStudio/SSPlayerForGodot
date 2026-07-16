#!/bin/bash
set -e
BASEDIR=$(dirname "$0")
SCRIPTDIR=$(cd "$BASEDIR" && pwd -P)
ROOTDIR=$(cd "$BASEDIR/.." && pwd -P)
TARGET_DIR="${ROOTDIR}/ss_player"
VERSION_FILE="${SCRIPTDIR}/SDK_VERSION.txt"
CURRENT_VERSION_FILE="${TARGET_DIR}/runtime/VERSION"
# Downloaded zip is cached here (gitignored) so a re-run that needs the same
# version reuses it instead of re-downloading tens of MB.
CACHE_DIR="${SCRIPTDIR}/.sdk-cache"

TARGET_VERSION=$(tr -d ' \r\n' < "$VERSION_FILE")

if [ -f "$CURRENT_VERSION_FILE" ]; then
    CURRENT_VERSION=$(tr -d ' \r\n' < "$CURRENT_VERSION_FILE")
    if [ "$CURRENT_VERSION" = "$TARGET_VERSION" ]; then
        echo "SDK $TARGET_VERSION is already up to date. Skipping download."
        exit 0
    fi
fi

BASE_URL="https://github.com/cri-middleware/SpriteStudio-SDK/releases/download/${TARGET_VERSION}"
ASSET="spritestudio-sdk-static-libs.zip"
# Keep the release asset's own name so it matches the SHA256SUMS entry directly.
ZIP_FILE="${CACHE_DIR}/${ASSET}"
SUMS_FILE="${CACHE_DIR}/SHA256SUMS"

echo "Target SDK Version: ${TARGET_VERSION}"
mkdir -p "$CACHE_DIR"

sha256_of() {
    if command -v sha256sum >/dev/null 2>&1; then
        sha256sum "$1" | awk '{print $1}'
    else
        shasum -a 256 "$1" | awk '{print $1}'
    fi
}

# Fetch the checksum manifest (small): it both verifies integrity and decides
# whether an already-cached zip can be reused without re-downloading.
echo "Downloading checksum manifest: ${BASE_URL}/SHA256SUMS"
curl -fL -o "$SUMS_FILE" "${BASE_URL}/SHA256SUMS"
EXPECTED=$(awk -v n="$ASSET" '$2 == n {print $1}' "$SUMS_FILE")
if [ -z "$EXPECTED" ]; then
    echo "ERROR: no checksum entry for ${ASSET} in SHA256SUMS" >&2
    exit 1
fi

# Reuse the cached zip when it already matches the manifest; otherwise download.
if [ -f "$ZIP_FILE" ] && [ "$(sha256_of "$ZIP_FILE")" = "$EXPECTED" ]; then
    echo "Reusing cached ${ASSET}"
else
    echo "Downloading SDK: ${BASE_URL}/${ASSET}"
    curl -fL -o "$ZIP_FILE" "${BASE_URL}/${ASSET}"
    if [ "$(sha256_of "$ZIP_FILE")" != "$EXPECTED" ]; then
        echo "ERROR: checksum mismatch for ${ASSET}" >&2
        rm -f "$ZIP_FILE"
        exit 1
    fi
fi
echo "Checksum OK: ${ASSET}"

# The release archive is a flat tree (libs/, include/, VERSION, …) with no
# wrapper directory, so extract it straight into ss_player/runtime/.
echo "Extracting SDK..."
rm -rf "${TARGET_DIR}/runtime"
unzip -q -o "$ZIP_FILE" -d "${TARGET_DIR}/runtime"

echo "$TARGET_VERSION" > "$CURRENT_VERSION_FILE"

echo "Done."
