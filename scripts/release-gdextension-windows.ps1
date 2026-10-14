#!/usr/bin/env pwsh

$baseDirectory = Split-Path -Parent $PSCommandPath
$rootDirectory = Split-Path -Parent $baseDirectory
$arch = (Get-Item Env:PROCESSOR_ARCHITECTURE).Value

pushd $rootDirectory

# Start from an empty bin/windows, as the other platforms' release scripts do:
# a DLL left by an earlier build would otherwise ship beside the new ones.
Remove-Item -Recurse -Force -ErrorAction SilentlyContinue ./bin/windows

$targets= "editor", "template_release", "template_debug"
foreach($target in $targets) {
    ./scripts/build-extension.ps1 platform=windows compiledb=no strip=yes target=${target} $args
}

# The MSVC link step leaves an import library and an export file beside each
# DLL. Only the DLL is loaded at runtime, and spritestudio.gdextension names
# nothing else, so shipping them would put two megabytes of build residue in
# every download. Removed here rather than in release.yml, so what a local run
# produces in bin/windows is what a release contains.
Remove-Item -Force -ErrorAction SilentlyContinue ./bin/windows/*.exp
Remove-Item -Force -ErrorAction SilentlyContinue ./bin/windows/*.lib

popd