#!/bin/zsh -e

HOST_ARCH=$(uname -m)

if ! declare -p CCACHE > /dev/null 2>&1; then
    if type "sccache" > /dev/null; then
        export CCACHE=sccache
    elif type "ccache" > /dev/null; then
        export CCACHE=ccache
    fi
    echo "set ${CCACHE} as CCACHE"
    export _USE_DEFAULT_CCACHE=1
fi

declare -A args=(
  ["version"]="3.x"
  ["arch"]=${HOST_ARCH}
)

while (( $# > 0 )); do
  item="$1"
  shift

  if [[ $item = *":"* ]]; then
    kv=(${(@s/:/)item})
    key=$kv[1]
    value=$kv[2]
    args[$key]=$value
  fi
done

# Godot
if [[ ${args[version]} = *"3."* ]]; then
    # 3.x
    args[platform]="osx"
    args[app_template]="osx_tools.app"
    args[app_bin]="godot.osx.tools"
else
    # 4.x
    args[platform]="macos"
    args[app_template]="macos_tools.app"
    args[app_bin]="godot.macos.editor"
fi

pushd godot
scons platform=${args[platform]} arch=${args[arch]} compiledb=yes custom_modules="../gd_spritestudio"

/bin/rm -rf ./Godot.app
/bin/cp -r misc/dist/${args[app_template]} ./Godot.app
/bin/mkdir -p Godot.app/Contents/MacOS
/bin/cp bin/${args[app_bin]}.${args[arch]} Godot.app/Contents/MacOS/Godot
/bin/chmod +x Godot.app/Contents/MacOS/Godot 
codesign --force --timestamp --options=runtime --entitlements misc/dist/${args[platform]}/editor.entitlements -s - Godot.app
popd

if [[ _USE_DEFAULT_CCACHE -eq 1 ]]; then
    unset CCACHE
fi
