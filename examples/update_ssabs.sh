#!/bin/bash
set -e

# Base directories
# Script location is assumed to be in the 'examples' directory
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"
SDK_DIR="${SCRIPT_DIR}/../ss_player/SpriteStudio-SDK"
SDK_TESTS_DIR="${SDK_DIR}/tests"
SDK_CLI_DIR="${SDK_DIR}/cli"
EXAMPLES_DIR="${SCRIPT_DIR}"

# Build ssconverter-cli
echo "Building ssconverter-cli (debug)..."
cd "${SDK_CLI_DIR}"
cargo build
CONVERTER="${SDK_DIR}/target/debug/ssconverter-cli"
cd "${EXAMPLES_DIR}"

# List of tests to update SSABs
# Note: overall is also converted into overall_gdextension
TESTS=("overall" "Ringo")

for TEST in "${TESTS[@]}"; do
    SSPJ_PATH="${SDK_TESTS_DIR}/${TEST}/${TEST}.sspj"
    
    # Standard project
    OUTPUT_DIR="${EXAMPLES_DIR}/${TEST}/ssab_generated/${TEST}"
    echo "Updating SSAB for ${TEST} in ${OUTPUT_DIR}..."
    mkdir -p "${OUTPUT_DIR}"
    "${CONVERTER}" "${SSPJ_PATH}" -o "${OUTPUT_DIR}"
    
    # overall project special handling for overall_gdextension
    if [ "${TEST}" == "overall" ]; then
        GD_OUTPUT_DIR="${EXAMPLES_DIR}/overall_gdextension/ssab_generated/overall"
        echo "Updating SSAB for ${TEST} in ${GD_OUTPUT_DIR}..."
        mkdir -p "${GD_OUTPUT_DIR}"
        "${CONVERTER}" "${SSPJ_PATH}" -o "${GD_OUTPUT_DIR}"
    fi

    # Ringo is also converted into the Override_Ringo and Scripting demo projects
    if [ "${TEST}" == "Ringo" ]; then
        OR_OUTPUT_DIR="${EXAMPLES_DIR}/Override_Ringo/ssab_generated/Ringo"
        echo "Updating SSAB for ${TEST} in ${OR_OUTPUT_DIR}..."
        mkdir -p "${OR_OUTPUT_DIR}"
        "${CONVERTER}" "${SSPJ_PATH}" -o "${OR_OUTPUT_DIR}"

        SCR_OUTPUT_DIR="${EXAMPLES_DIR}/Scripting/ssab_generated/Ringo"
        echo "Updating SSAB for ${TEST} in ${SCR_OUTPUT_DIR}..."
        mkdir -p "${SCR_OUTPUT_DIR}"
        "${CONVERTER}" "${SSPJ_PATH}" -o "${SCR_OUTPUT_DIR}"
    fi
done

echo "Done!"
