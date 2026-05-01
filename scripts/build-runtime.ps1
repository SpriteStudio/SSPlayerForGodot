#!/usr/bin/env pwsh

$baseDirectory = Split-Path -Parent $PSCommandPath
$rootDirectory = Split-Path -Parent $baseDirectory
$arch = (Get-Item Env:PROCESSOR_ARCHITECTURE).Value
if ($arch -match "AMD64") {
    $HOST_ARCH = "x86_64"
} else {
    $HOST_ARCH = "arm64"
}
$cpus = (Get-Item Env:NUMBER_OF_PROCESSORS).Value

# Godot scons default options
$default_opts = @{
    arch = $HOST_ARCH
    platform = "windows"
    build = "debug"
}

$opts = @{}
$opts += $default_opts

$APP = Split-Path -Leaf $PSCommandPath
function usage() {
    echo "Usage: $APP [options]"
}

foreach ($item in $Args) {
    if ($item -match "=") {
        $kv = $item -split "="
        $opts[$kv[0]] = $kv[1].ToLower()
    } elseif ($item -cmatch "help" -or $item -eq "-h" -or $item -eq "--help") {
        usage
        exit 0
    }
}
$opts
echo ""

pushd $rootDirectory/gd_spritestudio
pushd SpriteStudio7-SDK
if ($opts.build -eq "release") {
    & ./scripts/release-windows.ps1
} else {
    & cargo build -p ssconverter -p ssruntime
}
popd

$inputDir="SpriteStudio7-SDK"
$outputDir="runtime/include"
New-Item "./${outputDir}" -ItemType Directory -ErrorAction SilentlyContinue
Copy-Item ./${inputDir}/libs/ssconverter/target/ssconverter.h ./${outputDir}/ -Force
Copy-Item ./${inputDir}/libs/ssruntime/target/ssruntime.h ./${outputDir}/ -Force

$inputDir="SpriteStudio7-SDK/target"
$platform = $opts.platform
$arch = $opts.arch

if ($platform -eq "macos" -or $platform -eq "ios" -or $platform -eq "web") {
    $outputDir="runtime/libs/$platform"
} else {
    $outputDir="runtime/libs/$platform/$arch"
}
New-Item "./${outputDir}" -ItemType Directory -ErrorAction SilentlyContinue

if ($opts.build -eq "release") {
    if ($platform -eq "windows") {
        $src_runtime = "./${inputDir}/x86_64-pc-windows-msvc/release/ssruntime.lib"
        $src_converter = "./${inputDir}/x86_64-pc-windows-msvc/release/ssconverter.lib"
    } else {
        $src_runtime = "./${inputDir}/release/ssruntime.lib"
        $src_converter = "./${inputDir}/release/ssconverter.lib"
    }
} else {
    $src_runtime = "./${inputDir}/$($opts.build)/ssruntime.lib"
    $src_converter = "./${inputDir}/$($opts.build)/ssconverter.lib"
}

# Copy to the standard location
Copy-Item $src_runtime ./${outputDir}/ssruntime.lib -Force
Copy-Item $src_converter ./${outputDir}/ssconverter.lib -Force

# Copy for Godot Custom Module (with platform.target.arch naming)
$targets = "editor", "template_release", "template_debug"
foreach($target in $targets) {
    Copy-Item $src_runtime ./${outputDir}/ssruntime.$platform.$target.$arch.lib -Force
    Copy-Item $src_converter ./${outputDir}/ssconverter.$platform.$target.$arch.lib -Force
}


popd
