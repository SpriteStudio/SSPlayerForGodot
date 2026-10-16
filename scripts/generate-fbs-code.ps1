#!/usr/bin/env pwsh
$ErrorActionPreference = "Stop"

$HelpApp = Split-Path -Leaf $PSCommandPath
function Show-Usage {
    Write-Host "Usage: ${HelpApp} [--help]"
    Write-Host "  flatc -c over the SDK's .fbs schemas, into ss_player/format/. Needs flatc."
}

foreach ($arg in $args) {
    switch -Regex ($arg) {
        '^(-h|--help|help)$' { Show-Usage; exit 0 }
        default {
            [Console]::Error.WriteLine("${HelpApp}: unknown argument '$arg'")
            Show-Usage
            exit 2
        }
    }
}

$baseDirectory = Split-Path -Parent $PSCommandPath
$rootDirectory = Split-Path -Parent $baseDirectory

pushd $rootDirectory/ss_player

$FLATC_CMD = "flatc"

mkdir -Force "./format" > $null

foreach ($f in Get-ChildItem ./SpriteStudio-SDK/libs/ssruntime/fbs/*.fbs) {
    $name = $f.BaseName
    & $FLATC_CMD -c $f
    mv "${name}_generated.h" "./format/$name.h" -Force
}

foreach ($f in Get-ChildItem ./SpriteStudio-SDK/libs/ssab/fbs/*.fbs) {
    $name = $f.BaseName
    & $FLATC_CMD -c $f
    mv "${name}_generated.h" "./format/$name.h" -Force
}

popd
