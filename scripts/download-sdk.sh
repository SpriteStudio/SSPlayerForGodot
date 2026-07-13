#!/bin/bash
set -e
BASEDIR=$(dirname "$0")
SCRIPTDIR=$(cd "$BASEDIR" && pwd -P)
ROOTDIR=$(cd "$BASEDIR/.." && pwd -P)
TARGET_DIR="${ROOTDIR}/ss_player"
VERSION_FILE="${SCRIPTDIR}/SDK_VERSION.txt"
CURRENT_VERSION_FILE="${TARGET_DIR}/runtime/VERSION"

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
ZIP_FILE="${TARGET_DIR}/sdk.zip"
SUMS_FILE="${TARGET_DIR}/SHA256SUMS"

echo "Target SDK Version: ${TARGET_VERSION}"

# Fetch the release's SHA256SUMS manifest and verify the downloaded zip against
# it before extracting, so a corrupted or tampered download fails loudly.
echo "Downloading checksum manifest: ${BASE_URL}/SHA256SUMS"
curl -fL -o "$SUMS_FILE" "${BASE_URL}/SHA256SUMS"

echo "Downloading SDK: ${BASE_URL}/${ASSET}"
curl -fL -o "$ZIP_FILE" "${BASE_URL}/${ASSET}"

# Verify <file> against the manifest entry for <asset-name>. Uses sha256sum
# (Linux) or shasum -a 256 (macOS), matching the manifest line by asset name.
verify_sha256() {
    local file="$1" name="$2" expected actual
    expected=$(awk -v n="$name" '$2 == n {print $1}' "$SUMS_FILE")
    if [ -z "$expected" ]; then
        echo "ERROR: no checksum entry for ${name} in SHA256SUMS" >&2
        exit 1
    fi
    if command -v sha256sum >/dev/null 2>&1; then
        actual=$(sha256sum "$file" | awk '{print $1}')
    else
        actual=$(shasum -a 256 "$file" | awk '{print $1}')
    fi
    if [ "$expected" != "$actual" ]; then
        echo "ERROR: checksum mismatch for ${name}" >&2
        echo "  expected: ${expected}" >&2
        echo "  actual:   ${actual}" >&2
        exit 1
    fi
    echo "Checksum OK: ${name}"
}
verify_sha256 "$ZIP_FILE" "$ASSET"

echo "Extracting SDK..."
rm -rf "${TARGET_DIR}/runtime"
unzip -q -o "$ZIP_FILE" -d "${TARGET_DIR}/"
rm "$ZIP_FILE" "$SUMS_FILE"

echo "$TARGET_VERSION" > "$CURRENT_VERSION_FILE"

echo "Done."
