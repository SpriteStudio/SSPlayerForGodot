#!/usr/bin/env bash
set -e

if [ -z "$1" ]; then
    echo "Usage: $0 <platform>"
    echo "Available platforms: macos, windows, linux, ios, android, web"
    exit 1
fi

PLATFORM=$1

BASEDIR=$(dirname "$0")
BASEDIR=$(cd "$BASEDIR" && pwd -P)
ROOTDIR="${BASEDIR}/.."
ROOTDIR=$(cd "$ROOTDIR" && pwd -P)

pushd "$ROOTDIR" > /dev/null

# 1. Detect the host OS and choose the base export-templates directory.
if [ "$(uname)" = "Darwin" ]; then
    TEMPLATES_BASE="$HOME/Library/Application Support/Godot/export_templates"
else
    TEMPLATES_BASE="$HOME/.local/share/godot/export_templates"
fi

# 2. Read the Godot version from version.py to build the templates directory name.
GODOT_MAJOR=$(grep '^major' godot/version.py | cut -d '=' -f 2 | tr -d ' ')
GODOT_MINOR=$(grep '^minor' godot/version.py | cut -d '=' -f 2 | tr -d ' ')
GODOT_PATCH=$(grep '^patch' godot/version.py | cut -d '=' -f 2 | tr -d ' ')
GODOT_STATUS=$(grep '^status' godot/version.py | cut -d '=' -f 2 | tr -d ' "' )

if [ "$GODOT_PATCH" != "0" ]; then
    VERSION_STRING="${GODOT_MAJOR}.${GODOT_MINOR}.${GODOT_PATCH}.${GODOT_STATUS}"
else
    VERSION_STRING="${GODOT_MAJOR}.${GODOT_MINOR}.${GODOT_STATUS}"
fi

TEMPLATES_DIR="${TEMPLATES_BASE}/${VERSION_STRING}"
mkdir -p "$TEMPLATES_DIR"
echo "Target template directory: $TEMPLATES_DIR"

pushd godot/bin > /dev/null

case "$PLATFORM" in
    macos)
        echo "Processing macOS templates..."
        [ -d macos_template.app ] && rm -r macos_template.app
        [ -f macos.zip ] && rm macos.zip
        cp -R ../misc/dist/macos_template.app ./
        mkdir -p macos_template.app/Contents/MacOS

        [ -f godot.macos.template_release.universal ] && cp godot.macos.template_release.universal macos_template.app/Contents/MacOS/godot_macos_release.universal
        [ -f godot.macos.template_debug.universal ] && cp godot.macos.template_debug.universal macos_template.app/Contents/MacOS/godot_macos_debug.universal
        chmod +x macos_template.app/Contents/MacOS/* 2>/dev/null || true
        
        zip -q -9 -r macos.zip macos_template.app
        cp macos.zip "$TEMPLATES_DIR/macos.zip"
        ;;
        
    windows)
        echo "Processing Windows templates..."
        for f in godot.windows.template_release.*.exe; do
            [ -e "$f" ] || continue
            arch=$(echo "$f" | sed -E 's/godot\.windows\.template_release\.(.*)\.exe/\1/')
            cp "$f" "$TEMPLATES_DIR/windows_release_${arch}.exe"
        done
        for f in godot.windows.template_debug.*.exe; do
            [ -e "$f" ] || continue
            arch=$(echo "$f" | sed -E 's/godot\.windows\.template_debug\.(.*)\.exe/\1/')
            cp "$f" "$TEMPLATES_DIR/windows_debug_${arch}.exe"
        done
        ;;
        
    linux)
        echo "Processing Linux templates..."
        for f in godot.linuxbsd.template_release.*; do
            [ -e "$f" ] || continue
            arch=$(echo "$f" | sed -E 's/godot\.linuxbsd\.template_release\.(.*)/\1/')
            cp "$f" "$TEMPLATES_DIR/linux_release.${arch}"
        done
        for f in godot.linuxbsd.template_debug.*; do
            [ -e "$f" ] || continue
            arch=$(echo "$f" | sed -E 's/godot\.linuxbsd\.template_debug\.(.*)/\1/')
            cp "$f" "$TEMPLATES_DIR/linux_debug.${arch}"
        done
        ;;
        
    ios)
        echo "Processing iOS templates..."
        [ -d apple_embedded_xcode ] && rm -r apple_embedded_xcode
        [ -f ios.zip ] && rm ios.zip
        cp -R ../misc/dist/apple_embedded_xcode ./
        
        if [ -d libgodot.ios.template_release.xcframework ]; then
            rm -rf apple_embedded_xcode/libgodot.ios.release.xcframework
            cp -R libgodot.ios.template_release.xcframework apple_embedded_xcode/libgodot.ios.release.xcframework
        fi
        if [ -d libgodot.ios.template_debug.xcframework ]; then
            rm -rf apple_embedded_xcode/libgodot.ios.debug.xcframework
            cp -R libgodot.ios.template_debug.xcframework apple_embedded_xcode/libgodot.ios.debug.xcframework
        fi
        
        pushd apple_embedded_xcode > /dev/null
        zip -q -9 -r ../ios.zip *
        popd > /dev/null
        cp ios.zip "$TEMPLATES_DIR/ios.zip"
        ;;
        
    android)
        echo "Processing Android templates..."
        [ -f android_source.zip ] && cp android_source.zip "$TEMPLATES_DIR/android_source.zip"
        [ -f android_release.apk ] && cp android_release.apk "$TEMPLATES_DIR/android_release.apk"
        [ -f android_debug.apk ] && cp android_debug.apk "$TEMPLATES_DIR/android_debug.apk"
        ;;
        
    web)
        echo "Processing Web (nothreads) templates..."
        [ -f godot.web.template_release.wasm32.nothreads.zip ] && cp godot.web.template_release.wasm32.nothreads.zip "$TEMPLATES_DIR/web_nothreads_release.zip"
        [ -f godot.web.template_debug.wasm32.nothreads.zip ] && cp godot.web.template_debug.wasm32.nothreads.zip "$TEMPLATES_DIR/web_nothreads_debug.zip"
        [ -f godot.web.template_release.wasm32.nothreads.dlink.zip ] && cp godot.web.template_release.wasm32.nothreads.dlink.zip "$TEMPLATES_DIR/web_nothreads_dlink_release.zip"
        [ -f godot.web.template_debug.wasm32.nothreads.dlink.zip ] && cp godot.web.template_debug.wasm32.nothreads.dlink.zip "$TEMPLATES_DIR/web_nothreads_dlink_debug.zip"
        ;;
        
    *)
        echo "Error: Unknown platform '$PLATFORM'."
        echo "Available platforms: macos, windows, linux, ios, android, web"
        exit 1
        ;;
esac

echo "======================================"
echo "Successfully installed $PLATFORM templates to:"
echo "$TEMPLATES_DIR"

popd > /dev/null
popd > /dev/null
