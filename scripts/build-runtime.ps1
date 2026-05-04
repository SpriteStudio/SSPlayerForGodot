#!/usr/bin/env pwsh

$baseDirectory = Split-Path -Parent $PSCommandPath
$rootDirectory = Split-Path -Parent $baseDirectory
$rawArch = (Get-Item Env:PROCESSOR_ARCHITECTURE).Value
if ($rawArch -match "AMD64") {
    $HOST_ARCH = "x86_64"
    $HOST_PLATFORM = "windows"
} elseif ($rawArch -match "ARM64") {
    $HOST_ARCH = "arm64"
    $HOST_PLATFORM = "windows"
} else {
    # Fallback for non-Windows host (if pwsh is running elsewhere)
    $HOST_ARCH = "x86_64"
    $HOST_PLATFORM = "linux"
}

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

$SDK_DIR = "$rootDirectory/ss_player/SpriteStudio7-SDK"

pushd $SDK_DIR
if ($BUILD_MODE -eq "release") {
    $script = "./scripts/release-$PLATFORM.ps1"
    if ($PLATFORM -eq "ios" -and $IOS_SIMULATOR -eq "yes") {
        $script = "./scripts/release-ios-sim.sh"
    } elseif (-not (Test-Path $script)) {
        # Fallback to shell script via sh if ps1 doesn't exist (cross-platform support)
        $script = "./scripts/release-$PLATFORM.sh"
    }
    
    if ($script -match "\.sh$") {
        echo "Executing $script $BUILD_MODE..."
        & sh $script $BUILD_MODE
    } else {
        echo "Executing $script $BUILD_MODE..."
        & $script $BUILD_MODE
    }
} else {
    # In debug mode, we can build directly or use sdk scripts if they support it
    if ($PLATFORM -eq "android" -or $PLATFORM -eq "web" -or $PLATFORM -eq "ios") {
        $script = "./scripts/release-$PLATFORM.sh"
        if ($PLATFORM -eq "ios" -and $IOS_SIMULATOR -eq "yes") {
            $script = "./scripts/release-ios-sim.sh"
        }
        & sh $script debug
    } else {
        & cargo build -p ssconverter
        & cargo build -p ssruntime --features libc_alloc,panic-handler
    }
}
popd

# 1. Collect Headers
$INCLUDE_OUTPUT = "$rootDirectory/ss_player/runtime/include"
New-Item -ItemType Directory -Path $INCLUDE_OUTPUT -Force | Out-Null
Copy-Item "$SDK_DIR/libs/ssconverter/target/ssconverter.h" "$INCLUDE_OUTPUT/" -Force
Copy-Item "$SDK_DIR/libs/ssruntime/target/ssruntime.h" "$INCLUDE_OUTPUT/" -Force

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
} else {
    # Architecture mapping for cross-compilation
    $TARGET_TRIPLE = ""
    if ($PLATFORM -eq "android") {
        if ($ARCH -eq "arm64" -or $ARCH -eq "arm64-v8a") { $TARGET_TRIPLE = "aarch64-linux-android" }
        elseif ($ARCH -eq "arm32" -or $ARCH -eq "armeabi-v7a") { $TARGET_TRIPLE = "armv7-linux-androideabi" }
        elseif ($ARCH -eq "x86_64") { $TARGET_TRIPLE = "x86_64-linux-android" }
    } elseif ($PLATFORM -eq "web") {
        $TARGET_TRIPLE = "wasm32-unknown-unknown"
    } elseif ($PLATFORM -eq "ios") {
        if ($IOS_SIMULATOR -eq "yes") {
            $TARGET_TRIPLE = "universal-apple-ios-sim"
        } else {
            $TARGET_TRIPLE = "universal-apple-ios"
        }
    } elseif ($PLATFORM -eq "macos" -and $ARCH -eq "universal") {
        $TARGET_TRIPLE = "universal-apple-darwin"
    } elseif ($PLATFORM -eq "windows" -and $ARCH -eq "x86_64") {
        $TARGET_TRIPLE = "x86_64-pc-windows-msvc"
    }

    if ($TARGET_TRIPLE -ne "") {
        $SRC_DIR = "$SDK_DIR/target/$TARGET_TRIPLE/$BUILD_MODE"
    } else {
        $SRC_DIR = "$SDK_DIR/target/$BUILD_MODE"
    }
}

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
