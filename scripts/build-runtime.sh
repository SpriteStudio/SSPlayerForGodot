#!/usr/bin/env zsh
set -e

BASEDIR=$(dirname $0)
BASEDIR=$(cd $BASEDIR && pwd -P)
ROOTDIR=${BASEDIR}/..
ROOTDIR=$(cd $ROOTDIR && pwd -P)

if [ "$OSTYPE" = "msys" ]; then
    PLATFORM=win
elif [[ "$OSTYPE" = "darwin"* ]]; then
    PLATFORM=macos
else
    PLATFORM=linux
fi
HOST_ARCH=$(uname -m)

declare -A default_opts=(
    [arch]=${HOST_ARCH}
    [platform]=${PLATFORM}
    [build]="debug"
)

declare -A opts=(
    ${(kv)default_opts}
)

APP=$(basename $0)
func usage() {
    echo "Usage: $APP [options]"
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

pushd ${ROOTDIR}/gd_spritestudio > /dev/null
pushd SpriteStudio7-SDK > /dev/null
if [[ "${opts[build]}" == "release" ]]; then
    ./scripts/release-${opts[platform]}.sh
else
    cargo build -p ssconverter -p ssruntime
fi


popd > /dev/null
INPUT=SpriteStudio7-SDK/target
OUTPUT=runtime/libs/${opts[platform]}
/bin/mkdir -p ${OUTPUT}
if [[ "${opts[build]}" == "release" ]]; then
    if [[ "${opts[platform]}" == "macos" ]]; then
        /bin/cp ${INPUT}/universal-apple-darwin/libssruntime.a ${OUTPUT}/
        /bin/cp ${INPUT}/universal-apple-darwin/libssconverter.a ${OUTPUT}/        
    fi
else
    /bin/cp ${INPUT}/${opts[build]}/libssruntime.a ${OUTPUT}/
    /bin/cp ${INPUT}/${opts[build]}/libssconverter.a ${OUTPUT}/
fi

popd > /dev/null
