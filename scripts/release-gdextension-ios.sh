#!/usr/bin/env zsh
set -e
set -x

BASEDIR=$(dirname $0)
BASEDIR=$(cd $BASEDIR && pwd -P)
ROOTDIR=${BASEDIR}/..
ROOTDIR=$(cd $ROOTDIR && pwd -P)

pushd ${ROOTDIR} > /dev/null
BINDIR=$(pwd)/bin/ios
/bin/rm -rf ${BINDIR}
targets=("template_release" "template_debug")

# Build a simulator framework and a device framework per target, then combine
# them into one XCFramework (device + simulator) referenced by the .gdextension.
#
for target in ${targets[@]}; do
    # Build simulator first so it can be renamed to *.simulator.framework before the device build clobbers it
    find ${ROOTDIR}/ss_player -name '*.os' -delete 2>/dev/null || true
    scripts/build-extension.sh platform=ios arch=universal compiledb=no strip=yes target=${target} ios_simulator=yes "$@"

    # Clear .os files again and build device
    find ${ROOTDIR}/ss_player -name '*.os' -delete 2>/dev/null || true
    scripts/build-extension.sh platform=ios arch=arm64 compiledb=no strip=yes target=${target} ios_simulator=no "$@"
done

# Combine device + simulator frameworks into XCFrameworks. xcodebuild requires
# each framework's bundle name to match its binary, so stage the device and
# simulator frameworks under the same bundle name in separate directories first.
pushd ${BINDIR} > /dev/null
for target in ${targets[@]}; do
    /bin/rm -rf tmp
    /bin/mkdir -p tmp/ios tmp/ios-simulator
    /bin/mv libSSGodot.ios.${target}.framework tmp/ios/libSSGodot.ios.${target}.framework
    /bin/mv libSSGodot.ios.${target}.simulator.framework tmp/ios-simulator/libSSGodot.ios.${target}.framework
    xcodebuild -create-xcframework \
        -framework tmp/ios/libSSGodot.ios.${target}.framework \
        -framework tmp/ios-simulator/libSSGodot.ios.${target}.framework \
        -output libSSGodot.ios.${target}.xcframework
done
/bin/rm -rf tmp
popd > /dev/null # ${BINDIR}

# --- Code signing (opt-in via env) ------------------------------------------
# Signs the XCFrameworks inside-out when APPLE_SIGNING_IDENTITY is set (maps to the
# SS_APPLE_SIGNING_IDENTITY secret set by CI).
# iOS is NOT notarized and does NOT use the macOS hardened runtime (--options runtime)
# nor --deep: sign each embedded .framework first, then the .xcframework wrapper. The
# consuming app re-signs the embedded framework with its own identity at build time;
# this signature only lets consumers verify the package's provenance. Runs before the
# examples sync below so the copied frameworks carry the signature. No-op when unset.
if [ -n "${APPLE_SIGNING_IDENTITY:-}" ]; then
    echo "==> Codesigning iOS XCFrameworks as: ${APPLE_SIGNING_IDENTITY}"
    # Inner frameworks (one per slice) first...
    for fw in ${BINDIR}/*.xcframework/*/*.framework(N); do
        codesign --force --sign "${APPLE_SIGNING_IDENTITY}" --timestamp "${fw}"
        codesign --verify --strict --verbose=2 "${fw}"
    done
    # ...then the xcframework wrappers.
    for xcfw in ${BINDIR}/*.xcframework(N); do
        codesign --force --sign "${APPLE_SIGNING_IDENTITY}" --timestamp "${xcfw}"
        codesign --verify --strict --verbose=2 "${xcfw}"
    done
else
    echo "==> Skipping iOS codesign (APPLE_SIGNING_IDENTITY not set)"
fi

# Sync the xcframework to the examples and remove the leftover .frameworks
MAIN_PROJECT="dev_gdextension"
OTHER_PROJECTS=("overall_gdextension")

# Clean up leftover .frameworks and copy .xcframeworks to MAIN_PROJECT
rm -rf examples/${MAIN_PROJECT}/addons/spritestudio/bin/ios/*.framework
cp -R bin/ios/*.xcframework examples/${MAIN_PROJECT}/addons/spritestudio/bin/ios/

# Sync from MAIN_PROJECT to OTHER_PROJECTS
for project in "${OTHER_PROJECTS[@]}"; do
    DEST_DIR="./examples/${project}/addons/spritestudio"
    /bin/mkdir -p "${DEST_DIR}/bin/ios"
    rm -rf "${DEST_DIR}/bin/ios/*.framework"
    cp -R examples/${MAIN_PROJECT}/addons/spritestudio/bin/ios/*.xcframework "${DEST_DIR}/bin/ios/"
done

popd > /dev/null # ${ROOTDIR}
