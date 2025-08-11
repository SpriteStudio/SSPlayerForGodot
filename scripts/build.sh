#!/usr/bin/env zsh
set -e

BASEDIR=$(dirname $0)
BASEDIR=$(cd $BASEDIR && pwd -P)
ROOTDIR=${BASEDIR}/..
ROOTDIR=$(cd $ROOTDIR && pwd -P)

CPUS=2
if [ "$OSTYPE" = "msys" ]; then
    CPUS=$NUMBER_OF_PROCESSORS
    PLATFORM=win
elif [[ "$OSTYPE" = "darwin"* ]]; then
    CPUS=$(sysctl -n hw.logicalcpu)
    PLATFORM=macos
else
    CPUS=$(grep -c ^processor /proc/cpuinfo)
    PLATFORM=linux
fi

HOST_ARCH=$(uname -m)
pushd $ROOTDIR/godot > /dev/null
GODOT_BRANCH=$(git branch --show-current | sed -e "s/[\r\n]\+//g")
GODOT_TAG=$(git describe --tags --abbrev=0 | sed -e "s/[\r\n]\+//g")
popd > /dev/null

# Godot scons default options
declare -A scons_default_opts=(
    [arch]=${HOST_ARCH}
    [platform]=${PLATFORM}
    [target]="editor"
    [compiledb]="yes"
    [custom_modules]="../gd_spritestudio"
)

# build default options
declare -A build_default_opts=(
    [cpus]=${CPUS}
    [ccache]="no"
    [version]="4.4"
    [strip]="no"
)

declare -A opts=(
    ${(kv)build_default_opts}
    ${(kv)scons_default_opts}
)

APP=$(basename $0)
func usage() {
    echo "Usage: $APP [options]"
    echo "$APP options:"
    echo "  arch=<arch>         Target architecture (default: ${HOST_ARCH})"
    echo "  platform=<platform> Target platform (default: ${PLATFORM})"
    echo "  target=<target>     Target (default: ${scons_default_opts[target]})"
    echo "  cpus=<nums>         number of scons -j option"
    echo "  ccache=<yes|no>     Enable ccache (default: ${build_default_opts[ccache]})"
    echo "  version=<version>   Godot version. $APP uses this version at can not getting Godot version from git branch or tag. (default: ${build_default_opts[version]})"
    echo "  strip=<yes|no>      Execute strip command to the app binary (default: ${build_default_opts[strip]})"
    echo "Godot scons options: "
    pushd $ROOTDIR/godot > /dev/null
    scons --help
    popd > /dev/null
}

while (( $# > 0 )); do
    item="$1"
    shift

    if [[ $item = *"="* ]]; then
        kv=(${(@s/=/)item})
        key=$kv[1]
        value=${kv[2]:l}
        opts[$key]=$value
    elif [[ $item = *"help"* || $item == "-h" || $item == "--h" ]]; then
        usage
        exit 0
    fi
done

echo "options"
for key value in ${(kv)opts}; do
    echo "  $key => $value"
done
echo ""

# enable ccache
if [[ ${opts[ccache]} == "yes" ]]; then
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

# get Godot Version
if [[ -n $GODOT_BRANCH ]]; then
    VERSION=${GODOT_BRANCH}
elif [[ -n $GODOT_TAG ]]; then
    VERSION=${GODOT_TAG}
else
    VERSION=${opts[version]}
fi
echo "Godot Version: ${VERSION}"

# set internal parameters for each Godot version
declare -A internal_opts=(
    [app_template]=""
    [app_bin]=""
)
if [[ ${VERSION} =~ ^3\..* ]]; then
    # 3.x
    if [[ "${PLATFORM}" == "macos" ]]; then
        internal_opts[platform]="osx"
    else
        internal_opts[platform]=${PLATFORM}
    fi
    internal_opts[app_target]="tools"
    internal_opts[app_template]="osx_tools.app"
else
    # 4.x
    internal_opts[platform]=${PLATFORM}
    internal_opts[app_target]="editor"
    internal_opts[app_template]="macos_tools.app"
fi
if [[ -n ${opts[target]} ]]; then
    internal_opts[app_target]=${opts[target]}
fi
internal_opts[app_bin]="godot.${internal_opts[platform]}.${internal_opts[app_target]}"

# validate scons command options from build.sh options
for key value in ${(kv)opts}; do
    if [[ -v build_default_opts[$key] ]]; then
        # skip build default options
        continue
    fi
    if [[ $key == "arch" ]]; then
        # skip arch option
        continue
    fi
    scons_command_opts="$scons_command_opts $key=$value"
done
scons_command_opts="$scons_command_opts -j $build_default_opts[cpus]"

echo "scons command options: $scons_command_opts"

pushd $ROOTDIR/godot

# enable ccache on Godot
if [[ "$PLATFORM" == "macos" ]]; then
    if [[ ${opts[ccache]} == "yes" ]]; then
        git checkout platform/${internal_opts[platform]}/detect.py
        git apply ../misc/ccache/${internal_opts[platform]}_ccache.patch
    fi
fi

# build Godot
alias scons_macro="scons ${scons_command_opts}"
if [[ ${opts[arch]} == "universal" ]]; then
    ARCHES=('arm64' 'x86_64')
else
    ARCHES=${opts[arch]}
fi
for arch in $ARCHES; do
    echo "scons command build target arch: $arch"
    scons_macro arch=$arch
done

# strip binary
if [[ ${opts[strip]} == "yes" ]]; then
    strip ./bin/godot.*
fi

if [[ "$PLATFORM" == "macos" ]]; then
    if [[ ${opts[target]} == "editor" ]]; then
        # create Godot.app
        /bin/rm -rf ./Godot.app
        /bin/cp -r misc/dist/${internal_opts[app_template]} ./Godot.app
        /bin/mkdir -p Godot.app/Contents/MacOS
        if [[ ${opts[arch]} == "universal" ]]; then
            lipo -create bin/${internal_opts[app_bin]}.arm64 bin/${internal_opts[app_bin]}.x86_64 -output bin/${internal_opts[app_bin]}.${opts[arch]}
        fi
        /bin/cp bin/${internal_opts[app_bin]}.${opts[arch]} Godot.app/Contents/MacOS/Godot
        /bin/chmod +x Godot.app/Contents/MacOS/Godot 
        codesign --force --timestamp --options=runtime --entitlements misc/dist/${internal_opts[platform]}/editor.entitlements -s - Godot.app
    fi

fi

popd # $ROOTDIR/godot

if [[ "$PLATFORM" == "macos" ]]; then
    # revert enabling ccache on Godot
    if [[ ${opts[ccache]} == "yes" ]]; then
        git checkout platform/${internal_opts[platform]}/detect.py
    fi

    if [[ _USE_DEFAULT_CCACHE -eq 1 ]]; then
        unset CCACHE
    fi
fi
