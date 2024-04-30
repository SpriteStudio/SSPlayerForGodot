#!/bin/bash -e

HOST_ARCH=$(uname -m)
_BUILD_ARCH=${HOST_ARCH}
if [ $# -ge 1 ]; then
    _BUILD_ARCH=$1
fi

pushd godot
scons platform=osx arch=${_BUILD_ARCH} compiledb=yes custom_modules="../gd_spritestudio"
rm -rf ./Godot.app
cp -r misc/dist/osx_tools.app ./Godot.app
mkdir -p Godot.app/Contents/MacOS
cp bin/godot.osx.tools.$_BUILD_ARCH Godot.app/Contents/MacOS/Godot
chmod +x Godot.app/Contents/MacOS/Godot 
codesign --force --timestamp --options=runtime --entitlements misc/dist/osx/editor.entitlements -s - Godot.app
popd
