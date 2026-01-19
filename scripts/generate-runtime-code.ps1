#!/usr/bin/env pwsh
$ErrorActionPreference = "Stop"

$baseDirectory = Split-Path -Parent $PSCommandPath
$rootDirectory = Split-Path -Parent $baseDirectory

pushd $rootDirectory/gd_spritestudio

$FLATC_CMD = "flatc"

mkdir -Force "./runtime" > $null
foreach ($f in Get-ChildItem ./SpriteStudio7-SDK/libs/ssruntime/fbs/*.fbs) {
    $name = $f.BaseName
    & $FLATC_CMD -c $f
    mv "${name}_generated.h" "./runtime/$name.h" -Force
}

foreach ($f in Get-ChildItem ./SpriteStudio7-SDK/libs/ssab/fbs/*.fbs) {
    $name = $f.BaseName
    & $FLATC_CMD -c $f
    mv "${name}_generated.h" "./runtime/$name.h" -Force
}

popd
