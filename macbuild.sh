#!/bin/zsh -e

HOST_ARCH=$(uname -m)
pushd godot > /dev/null
GODOT_BRANCH=$(git branch --show-current | sed -e "s/[\r\n]\+//g")
GODOT_TAG=$(git describe --tags --abbrev=0 | sed -e "s/[\r\n]\+//g")
popd > /dev/null

declare -A args=(
  ["arch"]=${HOST_ARCH}
  ["ccache"]="false"
  ["version"]="3.x"
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

echo "arguments"
for key in ${(k)args}; do
  echo "  $key => $args[$key]"
done
echo ""

# ccache
if [[ ${args[ccache]} == "true" ]]; then
    if ! declare -p CCACHE > /dev/null 2>&1; then
        if type "sccache" > /dev/null; then
            export CCACHE=sccache
            export _USE_DEFAULT_CCACHE=1
        elif type "ccache" > /dev/null; then
            export CCACHE=ccache
            export _USE_DEFAULT_CCACHE=1
        fi
        echo "set ${CCACHE} as CCACHE"
    fi
fi

# Godot Version
if [[ -n $GODOT_BRANCH ]]; then
    VERSION=${GODOT_BRANCH}
elif [[ -n $GODOT_TAG ]]; then
    VERSION=${GODOT_TAG}
else
    VERSION=${args[version]}
fi
echo "Godot Version: ${VERSION}"

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
# enable ccache on Godot
if [[ ${args[ccache]} == "true" ]]; then
  git checkout platform/${args[platform]}/detect.py
  git apply ../misc/ccache/${args[platform]}_ccache.patch
fi

scons platform=${args[platform]} arch=${args[arch]} compiledb=yes custom_modules="../gd_spritestudio"

/bin/rm -rf ./Godot.app
/bin/cp -r misc/dist/${args[app_template]} ./Godot.app
/bin/mkdir -p Godot.app/Contents/MacOS
/bin/cp bin/${args[app_bin]}.${args[arch]} Godot.app/Contents/MacOS/Godot
/bin/chmod +x Godot.app/Contents/MacOS/Godot 
codesign --force --timestamp --options=runtime --entitlements misc/dist/${args[platform]}/editor.entitlements -s - Godot.app

# revert enabling ccache on Godot
if [[ ${args[ccache]} == "true" ]]; then
  git checkout platform/${args[platform]}/detect.py
fi
popd

if [[ _USE_DEFAULT_CCACHE -eq 1 ]]; then
    unset CCACHE
fi
