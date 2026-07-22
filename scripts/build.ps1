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
    target = "editor"
    compiledb = "yes"
    custom_modules = "../ss_player"
}

# winbuild default options
$winbuild_default_opts = @{
    cpus = $cpus
    ccache = "no"
    version = "4.7"
    strip = "no"
    deps = "yes"
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
    echo "  strip=<yes|no>      Accepted for parity with build.sh; MSVC keeps debug info in a separate .pdb, so nothing is stripped (default: $($winbuild_default_opts.strip))"
    echo "  deps=<yes|no>       Install the missing Godot third-party build dependencies (AccessKit / ANGLE) before building (default: $($winbuild_default_opts.deps))"
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

# Godot 4.6+ links two prebuilt third-party SDKs that are not part of the engine
# source tree: AccessKit (screen reader support) and ANGLE (OpenGL ES driver).
# Without them scons silently disables those drivers, so the produced editor and
# templates would lack features the official builds ship with. The installers
# come from the Godot checkout itself, which keeps the dependency versions in
# sync with the engine revision being built.
function install_godot_deps() {
    # Mirrors the destination logic of godot/misc/scripts/install_*.py.
    if ($env:LOCALAPPDATA -and -not $env:MSYSTEM) {
        $deps_dir = Join-Path (Join-Path $env:LOCALAPPDATA "Godot") "build_deps"
    } else {
        $deps_dir = Join-Path (Join-Path (Join-Path $rootDirectory "godot") "bin") "build_deps"
    }

    # install_angle.py unpacks one directory per arch (angle-<arch>-<abi>).
    $angle_dirs = @(Get-ChildItem -Path $deps_dir -Directory -Filter "angle*" -ErrorAction SilentlyContinue)

    # The installers resolve their destination relative to the working directory.
    pushd $rootDirectory/godot
    if (-not (Test-Path (Join-Path $deps_dir "accesskit"))) {
        echo "Installing AccessKit into $deps_dir ..."
        python misc/scripts/install_accesskit.py
    }
    if ($angle_dirs.Count -eq 0) {
        echo "Installing ANGLE into $deps_dir ..."
        python misc/scripts/install_angle.py
    }
    popd
}

# AccessKit and ANGLE only apply to the desktop platforms; the mobile and web
# platforms have no such dependency, so skip the download there.
if ($opts.deps -eq "yes" -and @("windows", "win") -contains $opts.platform) {
    install_godot_deps
}

pushd $rootDirectory/godot
Invoke-Expression "scons $scons_command_opts"
popd
