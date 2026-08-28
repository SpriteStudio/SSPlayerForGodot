#!/bin/bash
#
# Convert the SpriteStudio-SDK bundled test projects (tests/overall, tests/Ringo)
# into the .ssab assets the sample projects under examples/ load.
#
# Each sample project also carries a .ssplayer_sources.cfg pointing at the same
# .sspj, so opening it in the Godot Editor regenerates the same output through the
# import dock. This script is the headless equivalent -- ssab_generated/ is not
# tracked in git, so use it to populate the samples without launching the editor.
#
# The dev_* sample projects are intentionally excluded: their sources config is
# gitignored and set up by hand in the editor.
#
# Requires the SpriteStudio-SDK submodule and a Rust toolchain (ssconverter-cli is
# built from source; the prebuilt SDK packages only ship libssconverter).
set -euo pipefail

BASEDIR=$(dirname $0)
BASEDIR=$(cd $BASEDIR && pwd -P)
ROOTDIR=${BASEDIR}/..
ROOTDIR=$(cd $ROOTDIR && pwd -P)

SDK_DIR="${ROOTDIR}/ss_player/SpriteStudio-SDK"
SDK_TESTS_DIR="${SDK_DIR}/tests"
SDK_CLI_DIR="${SDK_DIR}/cli"
EXAMPLES_DIR="${ROOTDIR}/examples"
APP=$(basename $0)

if [ ! -f "${SDK_CLI_DIR}/Cargo.toml" ]; then
    echo "${APP}: SpriteStudio-SDK submodule is not initialized (${SDK_DIR})" >&2
    echo "${APP}: run 'git submodule update --init --recursive' first" >&2
    exit 1
fi

if ! command -v cargo > /dev/null; then
    echo "${APP}: cargo not found (a Rust toolchain is required to build ssconverter-cli)" >&2
    exit 1
fi

# Build ssconverter-cli
echo "Building ssconverter-cli (debug)..."
pushd "${SDK_CLI_DIR}" > /dev/null
cargo build
popd > /dev/null
CONVERTER="${SDK_DIR}/target/debug/ssconverter-cli"

# "<SDK tests subdir>|<examples subdir>" -- one entry per destination sample
# project. Output goes to <examples subdir>/ssab_generated/<SDK tests subdir>/,
# matching the layout the editor's source sync produces.
DEPLOYMENTS=(
    "overall|overall"
    "overall|overall_gdextension"
    "Ringo|Ringo"
    "Ringo|Override_Ringo"
    "Ringo|Scripting"
)

for ENTRY in "${DEPLOYMENTS[@]}"; do
    TEST="${ENTRY%%|*}"
    DEST="${ENTRY##*|}"
    SSPJ_PATH="${SDK_TESTS_DIR}/${TEST}/${TEST}.sspj"
    OUTPUT_DIR="${EXAMPLES_DIR}/${DEST}/ssab_generated/${TEST}"

    if [ ! -f "${SSPJ_PATH}" ]; then
        echo "${APP}: ${SSPJ_PATH} not found" >&2
        exit 1
    fi

    echo "Updating SSAB for ${TEST} in ${OUTPUT_DIR}..."
    mkdir -p "${OUTPUT_DIR}"
    "${CONVERTER}" "${SSPJ_PATH}" -o "${OUTPUT_DIR}"
done

# The headless test project is not a sample and so is not under examples/, but
# it reads the same fixtures -- one entry per pack run_tests.gd's preflight
# names. Keeping the .ssab out of git and regenerating it here is why the suite
# cannot quietly test a stale conversion.
TEST_PACKS=(
    "overall"
    "Ringo"
)

for TEST in "${TEST_PACKS[@]}"; do
    SSPJ_PATH="${SDK_TESTS_DIR}/${TEST}/${TEST}.sspj"
    OUTPUT_DIR="${ROOTDIR}/test_gdextension/ssab_generated/${TEST}"

    if [ ! -f "${SSPJ_PATH}" ]; then
        echo "${APP}: ${SSPJ_PATH} not found" >&2
        exit 1
    fi

    echo "Updating SSAB for ${TEST} in ${OUTPUT_DIR}..."
    mkdir -p "${OUTPUT_DIR}"
    "${CONVERTER}" "${SSPJ_PATH}" -o "${OUTPUT_DIR}"
done

echo "Done!"
