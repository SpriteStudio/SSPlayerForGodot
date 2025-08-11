#!/usr/bin/env pwsh

$baseDirectory = Split-Path -Parent $PSCommandPath
$rootDirectory = Split-Path -Parent $baseDirectory
$arch = (Get-Item Env:PROCESSOR_ARCHITECTURE).Value

pushd $rootDirectory

$targets= "editor", "template_release", "template_debug"
foreach($target in $targets) {
    ./scripts/build.ps1 platform=windows compiledb=no strip=yes target=${target}
}

popd