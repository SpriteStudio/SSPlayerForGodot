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
    api_version = "4.7"
}

# winbuild default options
$winbuild_default_opts = @{
    cpus = $cpus
    ccache = "no"
}

$opts = @{}
$opts += $scons_default_opts
$opts += $winbuild_default_opts

$APP = Split-Path -Leaf $PSCommandPath
function usage() {
    echo "Usage: $APP [options]"
    echo "$APP options:"
    echo "  arch=<arch>         Target architecture (default: ${HOST_ARCH})"
    echo "  platform=<platform> Target platform (default: $($scons_default_opts.platform))"
    echo "  cpus=<nums>         number of scons -j option (default: $cpus)"
    echo "  target=<target>     build target (default: $($scons_default_opts.target))"
    echo "  api_version=<ver>   Target Godot API version (default: $($scons_default_opts.api_version))"
    # echo "  ccache=<yes|no>     Enable ccache (default: $($winbuild_default_opts.ccache))"
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

$j = $opts["cpus"]
$scons_command_opts += " -j $j"


echo "scons command options: $scons_command_opts"

pushd $rootDirectory

$BINDIR = "bin/$($opts.platform)"
mkdir "$BINDIR" -Force
Invoke-Expression "scons $scons_command_opts"

# Copy .gdextension and binaries to example projects in the idiomatic addons structure
$MAIN_PROJECT = "dev_gdextension"
$OTHER_PROJECTS = @("overall_gdextension")

# Ensure MAIN_PROJECT has the .gdextension and icons
Copy-Item "misc\spritestudio.gdextension" "examples\$MAIN_PROJECT\addons\spritestudio\spritestudio.gdextension" -Force
Copy-Item "LICENSE.md" "examples\$MAIN_PROJECT\addons\spritestudio\LICENSE.md" -Force
Copy-Item "LICENSE.ja.md" "examples\$MAIN_PROJECT\addons\spritestudio\LICENSE.ja.md" -Force
mkdir "examples\$MAIN_PROJECT\addons\spritestudio\icons" -Force | Out-Null
Copy-Item "ss_player\icons\icon_*.svg" "examples\$MAIN_PROJECT\addons\spritestudio\icons\" -Force

# Copy from MAIN_PROJECT to OTHER_PROJECTS
foreach ($project in $OTHER_PROJECTS) {
    $dest_dir = "examples\$project\addons\spritestudio"
    mkdir $dest_dir -Force | Out-Null
    Copy-Item "misc\spritestudio.gdextension" "$dest_dir\spritestudio.gdextension" -Force
    Write-Host "Syncing binaries and icons to $project..."
    Copy-Item "examples\$MAIN_PROJECT\addons\spritestudio\bin" "$dest_dir\" -Recurse -Force
    Copy-Item "examples\$MAIN_PROJECT\addons\spritestudio\icons" "$dest_dir\" -Recurse -Force
}

# The headless test project is not a sample, so it is not under examples\ --
# but it loads the extension exactly as one does, hence the _gdextension suffix
# every project carrying the addon wears. See test_gdextension\project.godot.
$test_dest = "test_gdextension\addons\spritestudio"
mkdir $test_dest -Force | Out-Null
Copy-Item "misc\spritestudio.gdextension" "$test_dest\spritestudio.gdextension" -Force
Write-Host "Syncing binaries and icons to test_gdextension..."
Copy-Item "examples\$MAIN_PROJECT\addons\spritestudio\bin" "$test_dest\" -Recurse -Force
Copy-Item "examples\$MAIN_PROJECT\addons\spritestudio\icons" "$test_dest\" -Recurse -Force

popd
