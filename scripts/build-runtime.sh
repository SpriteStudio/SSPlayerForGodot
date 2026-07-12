#!/usr/bin/env zsh
set -e

BASEDIR=$(dirname $0)
BASEDIR=$(cd $BASEDIR && pwd -P)
ROOTDIR=${BASEDIR}/..
ROOTDIR=$(cd $ROOTDIR && pwd -P)

if [ "$OSTYPE" = "msys" ]; then
    HOST_PLATFORM=windows
elif [[ "$OSTYPE" = "darwin"* ]]; then
    HOST_PLATFORM=macos
else
    HOST_PLATFORM=linux
fi
HOST_ARCH=$(uname -m)
[[ "$HOST_ARCH" == "aarch64" ]] && HOST_ARCH="arm64"

typeset -A default_opts
default_opts=(
    arch "${HOST_ARCH}"
    platform "${HOST_PLATFORM}"
    build "debug"
    ios_simulator "no"
)

typeset -A opts
for k v in "${(@kv)default_opts}"; do
    opts[$k]=$v
done

APP=$(basename $0)
func usage() {
    echo "Usage: $APP [options]"
    echo "Options:"
    echo "  platform=<platform>    Target platform (windows, macos, linux, android, ios, web)"
    echo "  arch=<arch>            Target architecture (x86_64, arm64, universal, etc.)"
    echo "  build=<build>          Build mode (debug, release)"
    echo "  ios_simulator=<yes|no> Build for iOS simulator (default: no)"
}

while (( $# > 0 )); do
    item="$1"
    shift
    if [[ $item = *"="* ]]; then
        kv=(${(@s/=/)item})
        opts[$kv[1]]=$kv[2]:l
    elif [[ $item = *"help"* || $item == "-h" || $item == "--h" ]]; then
        usage
        exit 0
    fi
done

PLATFORM=${opts[platform]}
ARCH=${opts[arch]}
BUILD_MODE=${opts[build]}
IOS_SIMULATOR=${opts[ios_simulator]}

IS_HOST_BUILD=false
if [[ "$PLATFORM" == "$HOST_PLATFORM" && "$ARCH" == "$HOST_ARCH" ]]; then
    IS_HOST_BUILD=true
fi

echo "Building for $PLATFORM ($ARCH) in $BUILD_MODE mode (iOS Sim: $IOS_SIMULATOR, Host Build: $IS_HOST_BUILD)..."

SDK_DIR=${ROOTDIR}/ss_player/SpriteStudio-SDK
pushd ${SDK_DIR} > /dev/null

CARGO_FLAGS=""
[[ "$BUILD_MODE" == "release" ]] && CARGO_FLAGS="--release"

# 1. Build Phase
if [[ "$IS_HOST_BUILD" == "true" ]]; then
    if [[ "$PLATFORM" == "macos" ]]; then
        cargo build $CARGO_FLAGS
        cargo build $CARGO_FLAGS -p ssruntime --features libc_alloc,panic-handler
    elif [[ "$PLATFORM" == "linux" ]]; then
        ./scripts/release-linux.sh $BUILD_MODE
    elif [[ "$PLATFORM" == "windows" ]]; then
        cargo build $CARGO_FLAGS
        cargo build $CARGO_FLAGS -p ssruntime --features libc_alloc,panic-handler
    fi
    SRC_DIR="target/$BUILD_MODE"
else
    # クロスコンパイルまたは特殊なビルド
    case "$PLATFORM" in
        macos)
            ./scripts/release-macos.sh $BUILD_MODE
            SRC_DIR="target/universal-apple-darwin/$BUILD_MODE"
            ;;
        ios)
            # The SDK now emits a single XCFramework (device + simulator slices).
            # Godot links one variant at a time into its per-variant libSSGodot
            # framework, so pick the matching static slice: device (arm64) or the
            # universal simulator slice (arm64 + x86_64, already lipo'd by the SDK).
            ./scripts/release-ios-xcframework.sh $BUILD_MODE
            if [[ "$IOS_SIMULATOR" == "yes" ]]; then
                SRC_DIR="target/xcframework/static/libssruntime.xcframework/ios-arm64_x86_64-simulator"
            else
                SRC_DIR="target/xcframework/static/libssruntime.xcframework/ios-arm64"
            fi
            ;;
        android)
            ./scripts/release-android.sh $BUILD_MODE
            SRC_DIR="target/*-linux-android*/$BUILD_MODE"
            ;;
        linux)
            ./scripts/release-linux.sh $BUILD_MODE
            SRC_DIR="target/$BUILD_MODE"
            ;;
        web)
            ./scripts/release-emscripten.sh $BUILD_MODE
            SRC_DIR="target/wasm32-unknown-unknown/$BUILD_MODE"
            ;;
        windows)
            if command -v pwsh &> /dev/null; then
                pwsh ./scripts/release-windows.ps1 $BUILD_MODE
            fi
            SRC_DIR="target/x86_64-pc-windows-msvc/$BUILD_MODE"
            ;;
    esac
fi

popd > /dev/null

# 2. Artifact Collection Phase
echo "Collecting artifacts from $SRC_DIR..."
RUNTIME_DIR=${ROOTDIR}/ss_player/runtime

# Headers
mkdir -p ${RUNTIME_DIR}/include
cp ${SDK_DIR}/libs/ssruntime/target/ssruntime.hpp ${RUNTIME_DIR}/include/
if [[ "$PLATFORM" == "macos" || "$PLATFORM" == "windows" || "$PLATFORM" == "linux" ]]; then
    cp ${SDK_DIR}/libs/ssconverter/target/ssconverter.hpp ${RUNTIME_DIR}/include/
fi

# Licenses
cp ${SDK_DIR}/LICENSE.md ${RUNTIME_DIR}/
if [[ -f "${SDK_DIR}/target/licenses/THIRD-PARTY-LICENSES.ssruntime.md" ]]; then
    cp ${SDK_DIR}/target/licenses/THIRD-PARTY-LICENSES.ssruntime.md ${RUNTIME_DIR}/
fi

if [[ "$PLATFORM" == "macos" || "$PLATFORM" == "windows" || "$PLATFORM" == "linux" ]]; then
    if [[ -f "${SDK_DIR}/target/licenses/THIRD-PARTY-LICENSES.ssconverter.md" ]]; then
        cp ${SDK_DIR}/target/licenses/THIRD-PARTY-LICENSES.ssconverter.md ${RUNTIME_DIR}/
    fi
fi

# Libs Destination
if [[ "$PLATFORM" == "android" ]]; then
    typeset -A android_archs
    android_archs=(
        "arm64-v8a" "aarch64-linux-android"
        "armeabi-v7a" "armv7-linux-androideabi"
        "x86_64" "x86_64-linux-android"
        "x86" "i686-linux-android"
    )
    for a in ${(k)android_archs}; do
        target=${android_archs[$a]}
        arch_src_dir="target/$target/$BUILD_MODE"
        LIB_OUT_DIR="${RUNTIME_DIR}/libs/${PLATFORM}/${a}"
        
        if [[ -f "${SDK_DIR}/${arch_src_dir}/libssruntime.a" ]]; then
            mkdir -p "${LIB_OUT_DIR}"
            cp "${SDK_DIR}/${arch_src_dir}/libssruntime.a" "${LIB_OUT_DIR}/libssruntime.a"
        fi
    done
else
    if [[ "$PLATFORM" == "macos" || "$PLATFORM" == "ios" || "$PLATFORM" == "web" ]]; then
        LIB_OUT_DIR=${RUNTIME_DIR}/libs/${PLATFORM}
    else
        LIB_OUT_DIR=${RUNTIME_DIR}/libs/${PLATFORM}/${ARCH}
    fi
    mkdir -p ${LIB_OUT_DIR}

    # Copy function
    function copy_lib() {
        local src_name=$1
        local dest_name=$2
        local full_src="${SDK_DIR}/${SRC_DIR}/${src_name}"
        if [[ -f "${full_src}" ]]; then
            cp "${full_src}" "${LIB_OUT_DIR}/${dest_name}"
            return 0
        fi
        return 1
    }

    if [[ "$PLATFORM" == "windows" ]]; then
        copy_lib "ssruntime.lib" "ssruntime.lib" || copy_lib "libssruntime.a" "ssruntime.lib"
        copy_lib "ssconverter.lib" "ssconverter.lib" || copy_lib "libssconverter.a" "ssconverter.lib"
    elif [[ "$PLATFORM" == "ios" ]]; then
        if [[ -d "${SDK_DIR}/target/xcframework/static/libssruntime.xcframework" ]]; then
            cp -R "${SDK_DIR}/target/xcframework/static/libssruntime.xcframework" "${LIB_OUT_DIR}/"
        fi
    else
        copy_lib "libssruntime.a" "libssruntime.a"
        if [[ "$PLATFORM" == "macos" || "$PLATFORM" == "linux" ]]; then
            copy_lib "libssconverter.a" "libssconverter.a"
        fi
    fi
fi

echo "Done."
