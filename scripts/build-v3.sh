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
    PLATFORM=osx
else
    CPUS=$(grep -c ^processor /proc/cpuinfo)
    PLATFORM=linux
fi

HOST_ARCH=$(uname -m)

# Godot scons default options
declare -A scons_default_opts=(
    [arch]=${HOST_ARCH}
    [platform]=${PLATFORM}
    [tools]="yes"
    [compiledb]="no"
    [custom_modules]="../gd_spritestudio"
)

# build default options
declare -A build_default_opts=(
    [cpus]=${CPUS}
    [ccache]="no"
    [version]="3.x"
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
    echo "  cpus=<nums>         number of scons -j option"
    echo "  platform=<platform> Target platform (default: ${PLATFORM})"
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

# set internal parameters for each Godot version
declare -A internal_opts=(
    [app_template]="osx_tools.app"
    [app_target]="tools"
    [app_bin]=""
)
if [[ -n ${opts[target]} ]]; then
    internal_opts[app_target]=${opts[target]}
fi
internal_opts[app_bin]="godot.${opts[platform]}.${internal_opts[app_target]}"

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
if [[ "$PLATFORM" == "osx" ]]; then
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

if [[ "$PLATFORM" == "osx" ]]; then
    # strip binary
    if [[ ${opts[strip]} == "yes" ]]; then
        strip ./bin/godot.*
    fi

    if [[ ${opts[tools]} == "yes" ]]; then
        # create Godot.app
        /bin/rm -rf ./Godot.app
        /bin/cp -r misc/dist/${internal_opts[app_template]} ./Godot.app
        /bin/mkdir -p Godot.app/Contents/MacOS
        if [[ ${opts[arch]} == "universal" ]]; then
            lipo -create bin/${internal_opts[app_bin]}.arm64 bin/${internal_opts[app_bin]}.x86_64 -output bin/${internal_opts[app_bin]}.${opts[arch]}
        fi
        /bin/cp bin/${internal_opts[app_bin]}.${opts[arch]} Godot.app/Contents/MacOS/Godot
        /bin/chmod +x Godot.app/Contents/MacOS/Godot 
        codesign --force --timestamp --options=runtime --entitlements misc/dist/${opts[platform]}/editor.entitlements -s - Godot.app
    fi

    # revert enabling ccache on Godot
    if [[ ${opts[ccache]} == "yes" ]]; then
        git checkout platform/${opts[platform]}/detect.py
    fi
fi

popd # $ROOTDIR/godot

if [[ "$PLATFORM" == "osx" ]]; then
    if [[ _USE_DEFAULT_CCACHE -eq 1 ]]; then
        unset CCACHE
    fi
fi
