#!/usr/bin/env zsh
set -e

BASEDIR=$(dirname $0)
BASEDIR=$(cd $BASEDIR && pwd -P)
ROOTDIR=${BASEDIR}/..
ROOTDIR=$(cd $ROOTDIR && pwd -P)

pushd ${ROOTDIR} > /dev/null

targets=("template_release" "template_debug")
for target in ${targets[@]}; do
    # build.sh handles building Godot custom module.
    # With xcframework, we no longer need to swap out libssruntime.a per variant.
    scripts/build.sh platform=ios arch=arm64 compiledb=no strip=yes target=${target} ios_simulator=no
    scripts/build.sh platform=ios arch=universal compiledb=no strip=yes target=${target} ios_simulator=yes
done

# iOS requires .xcframework containing both device (arm64) and simulator (universal) binaries
for target in ${targets[@]}; do
    if [ -f "godot/bin/libgodot.ios.${target}.arm64.a" ] && [ -f "godot/bin/libgodot.ios.${target}.simulator.universal.a" ]; then
        /bin/rm -rf "godot/bin/libgodot.ios.${target}.xcframework"
        xcodebuild -create-xcframework \
            -library "godot/bin/libgodot.ios.${target}.arm64.a" \
            -library "godot/bin/libgodot.ios.${target}.simulator.universal.a" \
            -output "godot/bin/libgodot.ios.${target}.xcframework"
    fi
done

popd > /dev/null # ${ROOTDIR}
