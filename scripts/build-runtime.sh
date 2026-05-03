#!/usr/bin/env zsh
set -e

BASEDIR=$(dirname $0)
BASEDIR=$(cd $BASEDIR && pwd -P)
ROOTDIR=${BASEDIR}/..
ROOTDIR=$(cd $ROOTDIR && pwd -P)

if [ "$OSTYPE" = "msys" ]; then
    PLATFORM=windows
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

pushd ${ROOTDIR}/ss_player > /dev/null
pushd SpriteStudio7-SDK > /dev/null
if [[ "${opts[build]}" == "release" ]]; then
    ./scripts/release-${opts[platform]}.sh
else
    cargo build -p ssconverter
    cargo build -p ssruntime --features libc_alloc,panic-handler
fi
popd > /dev/null

INPUT=SpriteStudio7-SDK
OUTPUT=runtime/include
/bin/mkdir -p ${OUTPUT}
/bin/cp ${INPUT}/libs/ssconverter/target/ssconverter.h ${OUTPUT}/
/bin/cp ${INPUT}/libs/ssruntime/target/ssruntime.h ${OUTPUT}/

INPUT=SpriteStudio7-SDK/target
PLATFORM=${opts[platform]}
ARCH=${opts[arch]}

if [[ "$PLATFORM" == "macos" || "$PLATFORM" == "ios" || "$PLATFORM" == "web" ]]; then
    OUTPUT=runtime/libs/${PLATFORM}
else
    OUTPUT=runtime/libs/${PLATFORM}/${ARCH}
fi
/bin/mkdir -p ${OUTPUT}

if [[ "${opts[build]}" == "release" ]]; then
    if [[ "$PLATFORM" == "macos" ]]; then
        /bin/cp ${INPUT}/universal-apple-darwin/libssruntime.a ${OUTPUT}/
        /bin/cp ${INPUT}/universal-apple-darwin/libssconverter.a ${OUTPUT}/
    elif [[ "$PLATFORM" == "ios" ]]; then
        /bin/cp ${INPUT}/universal-apple-ios/libssruntime.a ${OUTPUT}/
        /bin/cp ${INPUT}/universal-apple-ios/libssconverter.a ${OUTPUT}/
    else
        /bin/cp ${INPUT}/release/libssruntime.a ${OUTPUT}/
        /bin/cp ${INPUT}/release/libssconverter.a ${OUTPUT}/
    fi
else
    /bin/cp ${INPUT}/${opts[build]}/libssruntime.a ${OUTPUT}/
    /bin/cp ${INPUT}/${opts[build]}/libssconverter.a ${OUTPUT}/
fi

popd > /dev/null
