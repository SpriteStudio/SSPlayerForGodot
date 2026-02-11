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
$outputDir="runtime"
Copy-Item ./${inputDir}/libs/ssconverter/target/ssconverter.h ./${outputDir}/ -Force
Copy-Item ./${inputDir}/libs/ssruntime/target/ssruntime.h ./${outputDir}/ -Force

$inputDir="SpriteStudio7-SDK/target"
$outputDir="runtime/libs//$($opts.platform)"
New-Item "./${outputDir}" -ItemType Directory -ErrorAction SilentlyContinue
New-Item "./${outputDir}/$($opts.arch)" -ItemType Directory -ErrorAction SilentlyContinue

$targets= "editor", "template_release", "template_debug"
foreach($target in $targets) {
    Copy-Item ./${inputDir}/$($opts.build)/ssruntime.lib ./${outputDir}/ssruntime.$($opts.platform).${target}.$($opts.arch).lib -Force
    Copy-Item ./${inputDir}/$($opts.build)/ssconverter.lib ./${outputDir}/ssconverter.$($opts.platform).${target}.$($opts.arch).lib -Force
}
Copy-Item ./${inputDir}/$($opts.build)/ssruntime.lib ./${outputDir}/$($opts.arch)/ssruntime.lib -Force
Copy-Item ./${inputDir}/$($opts.build)/ssconverter.lib ./${outputDir}/$($opts.arch)/ssconverter.lib -Force


popd
