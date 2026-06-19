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
    scripts/build-extension.sh platform=ios arch=universal compiledb=no strip=yes target=${target} ios_simulator=yes

    # Clear .os files again and build device
    find ${ROOTDIR}/ss_player -name '*.os' -delete 2>/dev/null || true
    scripts/build-extension.sh platform=ios arch=arm64 compiledb=no strip=yes target=${target} ios_simulator=no
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

# Sync the xcframework to the examples and remove the leftover .frameworks
MAIN_PROJECT="dev_gdextension"
OTHER_PROJECTS=("overall_gdextension" "Ringo")

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
