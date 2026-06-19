#!/usr/bin/env pwsh

$baseDirectory = Split-Path -Parent $PSCommandPath
$rootDirectory = Split-Path -Parent $baseDirectory
$rawArch = (Get-Item Env:PROCESSOR_ARCHITECTURE).Value
if ($rawArch -match "ARM64") {
    $HOST_ARCH = "arm64"
} else {
    $HOST_ARCH = "x86_64"
}
$HOST_PLATFORM = "windows"

# Godot options
$default_opts = @{
    arch = $HOST_ARCH
    platform = $HOST_PLATFORM
    build = "debug"
    ios_simulator = "no"
}

$opts = @{}
foreach ($key in $default_opts.Keys) { $opts[$key] = $default_opts[$key] }

$APP = Split-Path -Leaf $PSCommandPath
function usage() {
    echo "Usage: $APP [options]"
    echo "Options:"
    echo "  platform=<platform>    Target platform (windows, macos, linux, android, ios, web)"
    echo "  arch=<arch>            Target architecture (x86_64, arm64, universal, etc.)"
    echo "  build=<build>          Build mode (debug, release)"
    echo "  ios_simulator=<yes|no> Build for iOS simulator (default: no)"
}

foreach ($item in $Args) {
    if ($item -match "=") {
        $kv = $item -split "="
        $opts[$kv[0]] = $kv[1].ToLower()
    } elseif ($item -match "help" -or $item -eq "-h" -or $item -eq "--help") {
        usage
        exit 0
    }
}

$PLATFORM = $opts.platform
$ARCH = $opts.arch
$BUILD_MODE = $opts.build
$IOS_SIMULATOR = $opts.ios_simulator

$IS_HOST_BUILD = $false
if ($PLATFORM -eq $HOST_PLATFORM -and $ARCH -eq $HOST_ARCH) {
    $IS_HOST_BUILD = $true
}

echo "Building for $PLATFORM ($ARCH) in $BUILD_MODE mode (iOS Sim: $IOS_SIMULATOR, Host Build: $IS_HOST_BUILD)..."
echo ""

$SDK_DIR = "$rootDirectory/ss_player/SpriteStudio-SDK"

pushd $SDK_DIR

$CARGO_FLAGS = @()
if ($BUILD_MODE -eq "release") { $CARGO_FLAGS = @("--release") }

if ($IS_HOST_BUILD) {
    # Windows host: invoke cargo directly so artifacts land in target/$BUILD_MODE/
    & cargo build @CARGO_FLAGS
    & cargo build @CARGO_FLAGS -p ssruntime --features libc_alloc,panic-handler
} else {
    # Cross-compilation: dispatch to SDK release scripts
    if ($PLATFORM -eq "windows") {
        $script = "./scripts/release-windows.ps1"
        if ($ARCH -eq "arm64") { $script = "./scripts/release-windows-arm64.ps1" }
        echo "Executing $script $BUILD_MODE..."
        & $script $BUILD_MODE
    } elseif ($PLATFORM -eq "ios") {
        & sh ./scripts/release-ios-xcframework.sh $BUILD_MODE
    } elseif ($PLATFORM -eq "web") {
        & sh ./scripts/release-emscripten.sh $BUILD_MODE
    } else {
        & sh "./scripts/release-$PLATFORM.sh" $BUILD_MODE
    }
}
popd

# 1. Collect Headers
$INCLUDE_OUTPUT = "$rootDirectory/ss_player/runtime/include"
New-Item -ItemType Directory -Path $INCLUDE_OUTPUT -Force | Out-Null
Copy-Item "$SDK_DIR/libs/ssconverter/target/ssconverter.h" "$INCLUDE_OUTPUT/" -Force
Copy-Item "$SDK_DIR/libs/ssruntime/target/ssruntime.h" "$INCLUDE_OUTPUT/" -Force

# 1.5. Collect Licenses
$RUNTIME_OUTPUT = "$rootDirectory/ss_player/runtime"
if (Test-Path "$SDK_DIR/LICENSE.md") {
    Copy-Item "$SDK_DIR/LICENSE.md" "$RUNTIME_OUTPUT/" -Force
}
if (Test-Path "$SDK_DIR/target/licenses/THIRD-PARTY-LICENSES.ssruntime.md") {
    Copy-Item "$SDK_DIR/target/licenses/THIRD-PARTY-LICENSES.ssruntime.md" "$RUNTIME_OUTPUT/" -Force
}

if ($PLATFORM -eq "macos" -or $PLATFORM -eq "windows" -or $PLATFORM -eq "linux") {
    if (Test-Path "$SDK_DIR/target/licenses/THIRD-PARTY-LICENSES.ssconverter.md") {
        Copy-Item "$SDK_DIR/target/licenses/THIRD-PARTY-LICENSES.ssconverter.md" "$RUNTIME_OUTPUT/" -Force
    }
}

# 2. Collect Libraries
if ($PLATFORM -eq "macos" -or $PLATFORM -eq "ios" -or $PLATFORM -eq "web") {
    $LIB_OUTPUT = "$rootDirectory/ss_player/runtime/libs/$PLATFORM"
} else {
    $LIB_OUTPUT = "$rootDirectory/ss_player/runtime/libs/$PLATFORM/$ARCH"
}
New-Item -ItemType Directory -Path $LIB_OUTPUT -Force | Out-Null

$LIB_EXT = ".a"
if ($PLATFORM -eq "windows") { $LIB_EXT = ".lib" }

# Determine source directory
if ($IS_HOST_BUILD) {
    $SRC_DIR = "$SDK_DIR/target/$BUILD_MODE"
} elseif ($PLATFORM -eq "ios") {
    # iOS: pick the matching static slice from the SDK XCFramework
    # (release-ios-xcframework.sh already lipo'd the simulator archs).
    if ($IOS_SIMULATOR -eq "yes") {
        $SRC_DIR = "$SDK_DIR/target/xcframework/static/libssruntime.xcframework/ios-arm64_x86_64-simulator"
    } else {
        $SRC_DIR = "$SDK_DIR/target/xcframework/static/libssruntime.xcframework/ios-arm64"
    }
    # Architecture mapping for cross-compilation
    $TARGET_TRIPLE = ""
    if ($PLATFORM -eq "android") {
        $SRC_DIR = "$SDK_DIR/target/*-linux-android*/$BUILD_MODE"
    } elseif ($PLATFORM -eq "web") {
        $TARGET_TRIPLE = "wasm32-unknown-unknown"
    } elseif ($PLATFORM -eq "macos" -and $ARCH -eq "universal") {
        $TARGET_TRIPLE = "universal-apple-darwin"
    } elseif ($PLATFORM -eq "windows" -and $ARCH -eq "x86_64") {
        $TARGET_TRIPLE = "x86_64-pc-windows-msvc"
    }

    if ($PLATFORM -ne "android") {
        if ($TARGET_TRIPLE -ne "") {
            $SRC_DIR = "$SDK_DIR/target/$TARGET_TRIPLE/$BUILD_MODE"
        } else {
            $SRC_DIR = "$SDK_DIR/target/$BUILD_MODE"
        }
    }
}

echo "Copying libraries from $SRC_DIR..."

if ($PLATFORM -eq "android") {
    $android_archs = @{
        "arm64" = "aarch64-linux-android"
        "arm32" = "armv7-linux-androideabi"
        "x86_64" = "x86_64-linux-android"
        "x86" = "i686-linux-android"
    }
    
    foreach ($a in $android_archs.Keys) {
        $target = $android_archs[$a]
        $arch_src_dir = "$SDK_DIR/target/$target/$BUILD_MODE"
        $arch_lib_out = "$rootDirectory/ss_player/runtime/libs/$PLATFORM/$a"
        
        if (Test-Path "$arch_src_dir/libssruntime.a") {
            New-Item -ItemType Directory -Path $arch_lib_out -Force | Out-Null
            Copy-Item "$arch_src_dir/libssruntime.a" "$arch_lib_out/libssruntime.a" -Force
        }
    }
} else {
    if ($PLATFORM -eq "ios") {
        $xcframework_src = "$SDK_DIR/target/xcframework/static/libssruntime.xcframework"
        if (Test-Path $xcframework_src) {
            Copy-Item -Recurse $xcframework_src "$LIB_OUTPUT/" -Force
        }
    } else {
        echo "Copying libraries from $SRC_DIR to $LIB_OUTPUT..."
        if (Test-Path "$SRC_DIR/libssruntime$LIB_EXT") {
            Copy-Item "$SRC_DIR/libssruntime$LIB_EXT" "$LIB_OUTPUT/ssruntime$LIB_EXT" -Force
        } elseif (Test-Path "$SRC_DIR/ssruntime$LIB_EXT") {
            Copy-Item "$SRC_DIR/ssruntime$LIB_EXT" "$LIB_OUTPUT/ssruntime$LIB_EXT" -Force
        }

        if (Test-Path "$SRC_DIR/libssconverter$LIB_EXT") {
            Copy-Item "$SRC_DIR/libssconverter$LIB_EXT" "$LIB_OUTPUT/ssconverter$LIB_EXT" -Force
        } elseif (Test-Path "$SRC_DIR/ssconverter$LIB_EXT") {
            Copy-Item "$SRC_DIR/ssconverter$LIB_EXT" "$LIB_OUTPUT/ssconverter$LIB_EXT" -Force
        }

        # Copy for Godot Custom Module (with platform.target.arch naming)
        $targets = "editor", "template_release", "template_debug"
        foreach($target in $targets) {
            if (Test-Path "$LIB_OUTPUT/ssruntime$LIB_EXT") {
                Copy-Item "$LIB_OUTPUT/ssruntime$LIB_EXT" "$LIB_OUTPUT/ssruntime.$PLATFORM.$target.$ARCH$LIB_EXT" -Force
            }
            if (Test-Path "$LIB_OUTPUT/ssconverter$LIB_EXT") {
                Copy-Item "$LIB_OUTPUT/ssconverter$LIB_EXT" "$LIB_OUTPUT/ssconverter.$PLATFORM.$target.$ARCH$LIB_EXT" -Force
            }
        }
    }
}
