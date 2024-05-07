#!/bin/zsh -e

HOST_ARCH=$(uname -m)
pushd godot > /dev/null
GODOT_BRANCH=$(git branch --show-current | sed -e "s/[\r\n]\+//g")
GODOT_TAG=$(git describe --tags --abbrev=0 | sed -e "s/[\r\n]\+//g")
popd > /dev/null

declare -A opts=(
  # Godot Build Options
  ["arch"]=${HOST_ARCH}
  ["scons_opts"]=""

  # Build Options
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
    opts[$key]=$value
  fi
done

echo "options"
for key in ${(k)opts}; do
  echo "  $key => $opts[$key]"
done
echo ""

# ccache
if [[ ${opts[ccache]} == "true" ]]; then
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
    VERSION=${opts[version]}
fi
echo "Godot Version: ${VERSION}"

if [[ ${VERSION} = *"3."* ]]; then
    # 3.x
    opts[platform]="osx"
    opts[app_template]="osx_tools.app"
    opts[app_bin]="godot.osx.tools"
else
    # 4.x
    opts[platform]="macos"
    opts[app_template]="macos_tools.app"
    opts[app_bin]="godot.macos.editor"
fi

if [[ ${opts[arch]:l} == "universal" ]]; then
    ARCHES=('arm64' 'x86_64')
else
    ARCHES=${opts[arch]}
fi

pushd godot
# enable ccache on Godot
if [[ ${opts[ccache]} == "true" ]]; then
    git checkout platform/${opts[platform]}/detect.py
    git apply ../misc/ccache/${opts[platform]}_ccache.patch
fi

for arch in $ARCHES; do
    echo "build target arch: $arch"
    echo "${opts[scons_opts]}"
    scons platform=${opts[platform]} arch=${arch} compiledb=yes custom_modules="../gd_spritestudio" ${opts[scons_opts]}
done

/bin/rm -rf ./Godot.app
/bin/cp -r misc/dist/${opts[app_template]} ./Godot.app
/bin/mkdir -p Godot.app/Contents/MacOS
if [[ ${opts[arch]:l} == "universal" ]]; then
    lipo -create bin/${opts[app_bin]}.arm64 bin/${opts[app_bin]}.x86_64 -output bin/${opts[app_bin]}.${opts[arch]}
fi
/bin/cp bin/${opts[app_bin]}.${opts[arch]} Godot.app/Contents/MacOS/Godot
/bin/chmod +x Godot.app/Contents/MacOS/Godot 
codesign --force --timestamp --options=runtime --entitlements misc/dist/${opts[platform]}/editor.entitlements -s - Godot.app

# revert enabling ccache on Godot
if [[ ${opts[ccache]} == "true" ]]; then
    git checkout platform/${opts[platform]}/detect.py
fi
popd

if [[ _USE_DEFAULT_CCACHE -eq 1 ]]; then
    unset CCACHE
fi
