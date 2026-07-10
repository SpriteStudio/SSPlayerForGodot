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

# Godot scons default options
declare -A scons_default_opts=(
    [arch]=${HOST_ARCH}
    [platform]=${PLATFORM}
    [target]="editor"
    [compiledb]="yes"
    [api_version]="4.6"
)

# macbuild default options
declare -A build_default_opts=(
    [cpus]=${CPUS}
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
    echo "  platform=<platform> Target platform (default: ${scons_default_opts[platform]})"
    echo "  cpus=<nums>         number of scons -j option"
    echo "  target=<target>     build target (default: ${scons_default_opts[target]})"
    echo "  api_version=<ver>   Target Godot API version (default: ${scons_default_opts[api_version]})"
    echo "  strip=<yes|no>      Execute strip command to the app binary (default: ${build_default_opts[strip]})"
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

# validate scons command options from macbuild.sh options
for key value in ${(kv)opts}; do
    if [[ -v build_default_opts[$key] ]]; then
        # skip macbuild default options
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

pushd ${ROOTDIR} > /dev/null

BINDIR=./bin/${opts[platform]}
/bin/mkdir -p ${BINDIR}
alias scons_macro="scons ${scons_command_opts}"
if [[ ${opts[arch]} == "universal" ]]; then
    if [[ ${opts[platform]} == "android" ]]; then
        ARCHES=('arm64' 'x86_64')
    else
        ARCHES=${opts[arch]}
    fi
else
    ARCHES=${opts[arch]}
fi
for arch in $ARCHES; do
    echo "scons command build target arch: $arch"
    scons_macro arch=$arch
    if [[ ${opts[platform]} == "ios" ]] && [[ ${opts[ios_simulator]} == "yes" ]]; then
        pushd ${BINDIR} > /dev/null
        mv libSSGodot.${opts[platform]}.${opts[target]}.framework libSSGodot.${opts[platform]}.${opts[target]}.simulator.framework
        popd > /dev/null
    fi
done

# Copy .gdextension and binaries to example projects in the idiomatic addons structure
MAIN_PROJECT="dev_gdextension"
OTHER_PROJECTS=("overall_gdextension" "Ringo" "Override_Ringo")

# Ensure MAIN_PROJECT has the .gdextension and icons
/bin/cp misc/spritestudio.gdextension "./examples/${MAIN_PROJECT}/addons/spritestudio/"
/bin/cp LICENSE.md "./examples/${MAIN_PROJECT}/addons/spritestudio/"
/bin/mkdir -p "./examples/${MAIN_PROJECT}/addons/spritestudio/icons"
/bin/cp ss_player/icons/icon_*.svg "./examples/${MAIN_PROJECT}/addons/spritestudio/icons/"

# Copy from MAIN_PROJECT to OTHER_PROJECTS
for project in "${OTHER_PROJECTS[@]}"; do
    DEST_DIR="./examples/${project}/addons/spritestudio"
    /bin/mkdir -p "${DEST_DIR}"
    /bin/cp misc/spritestudio.gdextension "${DEST_DIR}/"
    echo "Syncing binaries and icons to ${project}..."
    /bin/cp -R "./examples/${MAIN_PROJECT}/addons/spritestudio/bin" "${DEST_DIR}/"
    /bin/cp -R "./examples/${MAIN_PROJECT}/addons/spritestudio/icons" "${DEST_DIR}/"
done

popd > /dev/null # ${ROOTDIR}
