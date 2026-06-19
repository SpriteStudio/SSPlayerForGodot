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
# Per-variant handling:
#   1. libssruntime: device and simulator need different slices and CI has no
#      separate provisioning step, so (re)build/place the runtime here per
#      variant via build-runtime.sh (ios_simulator selects the slice).
#   2. ss_player/*.os are not namespaced by device/simulator (unlike godot-cpp's
#      objects), so clear them before each variant build to avoid linking the
#      wrong slice into the binary.
# Order matters: build the simulator first. build-extension.sh renames the
# simulator output to *.simulator.framework, freeing the plain *.framework name
# for the device build; doing device first would let the simulator build clobber
# it before the rename.
build_variant() {
    local target=$1 sim=$2 rtmode=$3
    find ${ROOTDIR}/ss_player -name '*.os' -delete 2>/dev/null || true
    scripts/build-runtime.sh platform=ios build=${rtmode} ios_simulator=${sim}
    scripts/build-extension.sh platform=ios arch=universal compiledb=no strip=yes target=${target} ios_simulator=${sim}
}

for target in ${targets[@]}; do
    if [[ "${target}" == "template_debug" ]]; then
        rtmode="debug"
    else
        rtmode="release"
    fi
    build_variant ${target} yes ${rtmode}
    build_variant ${target} no  ${rtmode}
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

popd > /dev/null # ${ROOTDIR}
