#!/bin/zsh -e

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
pushd $ROOTDIR/godot-cpp > /dev/null
GODOT_BRANCH=$(git branch --show-current | sed -e "s/[\r\n]\+//g")
GODOT_TAG=$(git describe --tags --abbrev=0 | sed -e "s/[\r\n]\+//g")
popd > /dev/null

# Godot scons default options
declare -A scons_default_opts=(
    [arch]=${HOST_ARCH}
    [target]="editor"
    [compiledb]="yes"
)

# macbuild default options
declare -A macbuild_default_opts=(
    [cpus]=${CPUS}
    [version]="4.3"
    [strip]="no"
)

declare -A opts=(
    ${(kv)macbuild_default_opts}
    ${(kv)scons_default_opts}
)

APP=$(basename $0)
func usage() {
    echo "Usage: $APP [options]"
    echo "$APP options:"
    echo "  arch=<arch>         Target architecture (default: ${HOST_ARCH})"
    echo "  cpus=<nums>         number of scons -j option"
    echo "  target=<target>     build target (default: ${macbuild_default_opts[target]})"
    echo "  version=<version>   Godot version. $APP uses this version at can not getting Godot version from git branch or tag. (default: ${macbuild_default_opts[version]})"
    echo "  strip=<yes|no>      Execute strip command to the app binary (default: ${macbuild_default_opts[strip]})"
    echo "Godot scons options: "
    pushd $ROOTDIR/godot-cpp > /dev/null
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
    [platform]=""
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
    internal_opts[app_template]="macos_tools.app"
fi
if [[ -n ${opts[target]} ]]; then
    internal_opts[app_target]=${opts[target]}
fi
internal_opts[app_bin]="godot.${internal_opts[platform]}.${internal_opts[app_target]}"

# validate scons command options from macbuild.sh options
scons_command_opts="platform=${internal_opts[platform]}"
for key value in ${(kv)opts}; do
    if [[ -v macbuild_default_opts[$key] ]]; then
        # skip macbuild default options
        continue
    fi
    if [[ $key == "arch" ]]; then
        # skip arch option
        continue
    fi
    scons_command_opts="$scons_command_opts $key=$value"
done
scons_command_opts="$scons_command_opts -j $macbuild_default_opts[cpus]"

echo "scons command options: $scons_command_opts"

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

if [[ "$PLATFORM" == "macos" ]]; then
    # strip binary
    if [[ ${opts[strip]} == "yes" ]]; then
        strip ./bin/${internal_opts[platform]}/macos.framework/libSSGodot.macos.${internal_opts[target]}
    fi
fi
