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
$scons_default_opts = @{
    arch = $HOST_ARCH
    platform = "windows"
    target = "editor"
    compiledb = "yes"
    use_static_cpp = "no"
}

# winbuild default options
$winbuild_default_opts = @{
    cpus = $cpus
    ccache = "no"
    version = "4.6"
}

$opts = @{}
$opts += $scons_default_opts
$opts += $winbuild_default_opts

$APP = Split-Path -Leaf $PSCommandPath
function usage() {
    echo "Usage: $APP [options]"
    echo "$APP options:"
    echo "  arch=<arch>         Target architecture (default: ${HOST_ARCH})"
    echo "  platform=<platform> Target platform (default: ${scons_default_opts[platform]})"
    echo "  cpus=<nums>         number of scons -j option (default: $cpus)"
    echo "  target=<target>     build target (default: ${winbuild_default_opts[target]})"
    # echo "  ccache=<yes|no>     Enable ccache (default: $($winbuild_default_opts.ccache))"
    echo "  version=<version>   Godot version. $APP uses this version at can not getting Godot version from git branch or tag. (default: $($winbuild_default_opts.version))"
    echo "Godot scons options: "
    pushd $rootDirectory/godot-cpp
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

# validate scons command options from winbuild options
$scons_command_opts = ""
foreach ($key in $opts.Keys) {
    if ($winbuild_default_opts.ContainsKey($key)) {
        # skip winbuild default options
        continue
    }
    $scons_command_opts += " $key=$($opts[$key])"
}

# Map version to api_version for SCons
if ($opts.version) {
    $scons_command_opts += " api_version=$($opts.version)"
}

$j = $opts["cpus"]
$scons_command_opts += " -j $j"


echo "scons command options: $scons_command_opts"

pushd $rootDirectory

$BINDIR = "bin/$($opts.platform)"
mkdir "$BINDIR" -Force
Invoke-Expression "scons $scons_command_opts"

# Copy .gdextension and binaries to example projects in the idiomatic addons structure
$MAIN_PROJECT = "dev_gdextension"
$OTHER_PROJECTS = @("overall_gdextension", "Ringo")

# Ensure MAIN_PROJECT has the .gdextension
Copy-Item "misc\spritestudio.gdextension" "examples\$MAIN_PROJECT\addons\spritestudio\spritestudio.gdextension" -Force

# Copy from MAIN_PROJECT to OTHER_PROJECTS
foreach ($project in $OTHER_PROJECTS) {
    $dest_dir = "examples\$project\addons\spritestudio"
    mkdir $dest_dir -Force | Out-Null
    Copy-Item "misc\spritestudio.gdextension" "$dest_dir\spritestudio.gdextension" -Force
    Write-Host "Syncing binaries to $project..."
    Copy-Item "examples\$MAIN_PROJECT\addons\spritestudio\bin" "$dest_dir\" -Recurse -Force
}

popd
