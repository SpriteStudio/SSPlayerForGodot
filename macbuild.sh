#!/bin/zsh -e

if ! declare -p CCACHE > /dev/null 2>&1; then
    if type "sccache" > /dev/null; then
        export CCACHE=sccache
    elif type "ccache" > /dev/null; then
        export CCACHE=ccache
    fi
    echo "set ${CCACHE} as CCACHE"
    export _USE_DEFAULT_CCACHE=1
fi

HOST_ARCH=$(uname -m)
pushd godot > /dev/null
GODOT_BRANCH=$(git branch --show-current | sed -e "s/[\r\n]\+//g")
popd > /dev/null

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
if [[ -n $GODOT_BRANCH ]]; then
    VERSION=${GODOT_BRANCH}
else
    VERSION=${args[version]}
fi
echo "VERSION: ${VERSION}"

if [[ ${VERSION} = *"3."* ]]; then
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
git checkout platform/${args[platform]}/detect.py
git apply ../misc/ccache/${args[platform]}_ccache.patch

scons platform=${args[platform]} arch=${args[arch]} compiledb=yes custom_modules="../gd_spritestudio"

/bin/rm -rf ./Godot.app
/bin/cp -r misc/dist/${args[app_template]} ./Godot.app
/bin/mkdir -p Godot.app/Contents/MacOS
/bin/cp bin/${args[app_bin]}.${args[arch]} Godot.app/Contents/MacOS/Godot
/bin/chmod +x Godot.app/Contents/MacOS/Godot 
codesign --force --timestamp --options=runtime --entitlements misc/dist/${args[platform]}/editor.entitlements -s - Godot.app

git checkout platform/${args[platform]}/detect.py
popd

if [[ _USE_DEFAULT_CCACHE -eq 1 ]]; then
    unset CCACHE
fi
