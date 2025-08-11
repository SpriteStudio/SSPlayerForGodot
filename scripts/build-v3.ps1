#!/usr/bin/env pwsh

$baseDirectory = Split-Path -Parent $PSCommandPath
$rootDirectory = Split-Path -Parent $baseDirectory
$arch = (Get-Item Env:PROCESSOR_ARCHITECTURE).Value
if ($arch -match "AMD64") {
    $HOST_ARCH = "x64"
} else {
    $HOST_ARCH = "arm64"
}
$cpus = (Get-Item Env:NUMBER_OF_PROCESSORS).Value

pushd $rootDirectory/godot
try {
    $GODOT_BRANCH = (git branch --show-current).Trim()
} catch {
    $GODOT_BRANCH = ""
}
try {
    $GODOT_TAG = (git describe --tags --abbrev=0).Trim()
} catch {
    $GODOT_TAG = ""
}
popd

# Godot scons default options
$scons_default_opts = @{
    arch = $HOST_ARCH
    platform = "windows"
    vsproj = "no"
    tools = "yes"
    compiledb = "yes"
    custom_modules = "../gd_spritestudio"
}

# winbuild default options
$winbuild_default_opts = @{
    cpus = $cpus
    ccache = "no"
    version = "3.x"
}

$opts = @{}
$opts += $scons_default_opts
$opts += $winbuild_default_opts

$APP = Split-Path -Leaf $PSCommandPath
function usage() {
    echo "Usage: $APP [options]"
    echo "$APP options:"
    echo "  arch=<arch>         Target architecture (default: ${HOST_ARCH})"
    echo "  target=<target>     Target (default: ${scons_default_opts.target})"
    echo "  cpus=<nums>         number of scons -j option (default: $cpus)"
    # echo "  ccache=<yes|no>     Enable ccache (default: $($winbuild_default_opts.ccache))"
    echo "  version=<version>   Godot version. $APP uses this version at can not getting Godot version from git branch or tag. (default: $($winbuild_default_opts.version))"
    echo "Godot scons options: "
    pushd $rootDirectory/godot
    scons --help
    popd
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

# # ccache
# if ($opts.ccache -eq "yes") 
# {
#     if ($env:CCACHE -ne $null) {
#         $CCACHE = $env:CCACHE
#     } else {
#         if (Get-Command -Name sccache) {
#             $CCACHE = sccache
#         } elseif (Get-Command -Name sccache) {
#             $CCACHE = ccache
#         }
#         echo "set $CCACHE as CCACHE"
#     }
# }

# get Godot Version
if (![string]::IsNullOrEmpty($GODOT_BRANCH)) {
    $VERSION = $GODOT_BRANCH
} elseif (![string]::IsNullOrEmpty($GODOT_TAG)) {
    $VERSION = $GODOT_TAG
} else {
    $VERSION = $opts.version
}
echo "Godot Version: ${VERSION}" 

# validate scons command options from winbuild options
foreach ($key in $opts.Keys) {
    if ($winbuild_default_opts.ContainsKey($key)) {
        # skip winbuild default options
        continue
    }
    $scons_command_opts += " $key=$($opts[$key])"
}
$j = $opts["cpus"]
$scons_command_opts += "$scons_command_opts -j $j"


echo "scons command options: $scons_command_opts"

pushd $rootDirectory/godot
Invoke-Expression "scons $scons_command_opts"
popd