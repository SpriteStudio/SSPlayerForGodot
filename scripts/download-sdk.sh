#!/bin/bash
set -e
BASEDIR=$(dirname $0)
ROOTDIR=$(cd "$BASEDIR/.." && pwd -P)
TARGET_DIR="${ROOTDIR}/gd_spritestudio"
VERSION_FILE="${TARGET_DIR}/SDK_VERSION.txt"
CURRENT_VERSION_FILE="${TARGET_DIR}/runtime/VERSION"

TARGET_VERSION=$(cat "$VERSION_FILE" | tr -d '
')

if [ -f "$CURRENT_VERSION_FILE" ]; then
    CURRENT_VERSION=$(cat "$CURRENT_VERSION_FILE" | tr -d '
')
    if [ "$CURRENT_VERSION" = "$TARGET_VERSION" ]; then
        echo "SDK $TARGET_VERSION is already up to date. Skipping download."
        exit 0
    fi
fi

URL="https://github.com/SpriteStudio/SpriteStudio7-SDK/releases/download/${TARGET_VERSION}/spritestudio7-sdk-static-libs.zip"
ZIP_FILE="${TARGET_DIR}/sdk.zip"

echo "Target SDK Version: ${TARGET_VERSION}"
echo "Download URL: ${URL}"
echo "Downloading SDK..."
curl -L -o "$ZIP_FILE" "$URL"

echo "Extracting SDK..."
rm -rf "${TARGET_DIR}/runtime"
unzip -q -o "$ZIP_FILE" -d "${TARGET_DIR}/"
rm "$ZIP_FILE"

echo "$TARGET_VERSION" > "$CURRENT_VERSION_FILE"

echo "Done."

