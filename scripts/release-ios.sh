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

# Assemble the simulator-universal archive. build.sh emits a per-arch simulator
# archive for each arch, but each is fat: libtool pulls in the whole fat
# libssruntime simulator slice (arm64+x86_64), so only the matching arch is
# "complete" (has the Godot objects too). Take the complete slice from each and
# lipo them into one clean arm64+x86_64 simulator library.
for target in ${targets[@]}; do
    armsim="godot/bin/libgodot.ios.${target}.arm64.simulator.a"
    x86sim="godot/bin/libgodot.ios.${target}.x86_64.simulator.a"
    if [ -f "$armsim" ] && [ -f "$x86sim" ]; then
        tmp_arm=$(mktemp -t ios_arm64sim).a
        tmp_x86=$(mktemp -t ios_x86sim).a
        lipo -thin arm64  "$armsim" -output "$tmp_arm"
        lipo -thin x86_64 "$x86sim" -output "$tmp_x86"
        lipo -create "$tmp_arm" "$tmp_x86" -output "godot/bin/libgodot.ios.${target}.simulator.universal.a"
        /bin/rm -f "$tmp_arm" "$tmp_x86"
    fi
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
