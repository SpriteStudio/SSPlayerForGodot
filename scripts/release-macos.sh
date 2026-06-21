#!/usr/bin/env zsh
set -e

BASEDIR=$(dirname $0)
BASEDIR=$(cd $BASEDIR && pwd -P)
ROOTDIR=${BASEDIR}/..
ROOTDIR=$(cd $ROOTDIR && pwd -P)

pushd ${ROOTDIR} > /dev/null

targets=("editor" "template_release" "template_debug")
for target in ${targets[@]}; do
    scripts/build.sh platform=macos arch=universal compiledb=no strip=yes target=${target}
done

# Godot 4 macOS templates build x86_64 and arm64 separately even with arch=universal, so we lipo them.
if [ -f "godot/bin/godot.macos.template_release.x86_64" ] && [ -f "godot/bin/godot.macos.template_release.arm64" ]; then
    lipo -create godot/bin/godot.macos.template_release.x86_64 godot/bin/godot.macos.template_release.arm64 -output godot/bin/godot.macos.template_release.universal
fi
if [ -f "godot/bin/godot.macos.template_debug.x86_64" ] && [ -f "godot/bin/godot.macos.template_debug.arm64" ]; then
    lipo -create godot/bin/godot.macos.template_debug.x86_64 godot/bin/godot.macos.template_debug.arm64 -output godot/bin/godot.macos.template_debug.universal
fi

popd > /dev/null # ${ROOTDIR}
