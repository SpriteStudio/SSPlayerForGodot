#!/usr/bin/env pwsh
#
# Download the stock Godot editor pinned in scripts\GODOT_VERSION.txt into
# godot-bin\ (gitignored), for scripts\run-tests.ps1 to run the headless suite
# against. Windows counterpart of scripts/fetch-godot.sh.
#
# Nothing calls this on your behalf. run-tests.ps1 looks for a binary you
# already have -- godot=<path>, $env:GODOT, godot-bin\, PATH -- and prints this
# command rather than downloading tens of MB without being asked.
#
# The editor build is the whole download, and it is all that is needed: the
# export templates are another ~1.2 GB and the suite exports nothing.
#
# This is deliberately NOT the same binary as godot\bin\*. That one is a custom
# module build of the engine, which has SpriteStudio compiled in -- it would
# register the classes a second time and abort on the extension's own project.
#
# Usage: scripts\fetch-godot.ps1 [force=yes] [out=<dir>]
#   force : yes to re-download even when the pinned version is installed
#   out   : install directory (default: godot-bin\)
#
# Requires PowerShell 5+ (Invoke-WebRequest, Expand-Archive).

$ErrorActionPreference = "Stop"
$RootDir = Split-Path -Parent (Split-Path -Parent $PSCommandPath)
$ScriptDir = Join-Path $RootDir "scripts"

$Force = "no"
$OutDir = Join-Path $RootDir "godot-bin"
foreach ($item in $Args) {
    if ($item -match "^-?-?help$" -or $item -eq "-h") {
        $emit = $false
        foreach ($line in (Get-Content $PSCommandPath)) {
            if ($line -match '^# Usage:') { $emit = $true }
            if ($emit) { $line -replace '^# ?', '' }
            if ($emit -and $line -match '^# Requires') { break }
        }
        exit 0
    }
    $kv = $item -split "=", 2
    switch ($kv[0]) {
        "force" { $Force = $kv[1] }
        "out"   { $OutDir = $kv[1] }
        default { Write-Error "unknown arg '$item'" }
    }
}

$Version = (Get-Content (Join-Path $ScriptDir "GODOT_VERSION.txt")).Trim()
$VersionFile = Join-Path $OutDir "VERSION"
$CacheDir = Join-Path $ScriptDir ".godot-cache"

if ($Force -ne "yes" -and (Test-Path $VersionFile) -and
        ((Get-Content $VersionFile).Trim() -eq $Version)) {
    Write-Host "fetch-godot.ps1: Godot $Version is already installed in godot-bin\. Nothing to do."
    exit 0
}

$Asset = "Godot_v${Version}_win64.exe.zip"
$Url = "https://github.com/godotengine/godot-builds/releases/download/$Version/$Asset"
$ZipFile = Join-Path $CacheDir $Asset

mkdir $CacheDir -Force | Out-Null
if ((-not (Test-Path $ZipFile)) -or $Force -eq "yes") {
    Write-Host "fetch-godot.ps1: downloading $Asset"
    Invoke-WebRequest -Uri $Url -OutFile "$ZipFile.part"
    Move-Item "$ZipFile.part" $ZipFile -Force
} else {
    Write-Host "fetch-godot.ps1: reusing cached $Asset"
}

if (Test-Path $OutDir) { Remove-Item -Recurse -Force $OutDir }
mkdir $OutDir -Force | Out-Null
Expand-Archive -Path $ZipFile -DestinationPath $OutDir -Force

# Normalise what the archive unpacks to, so run-tests.ps1 has one path rather
# than a version-stamped name that changes with every bump.
$bin = Get-ChildItem -Path $OutDir -Filter "Godot_v*.exe" -File | Select-Object -First 1
if ($bin) { Move-Item $bin.FullName (Join-Path $OutDir "godot.exe") -Force }

Set-Content -Path $VersionFile -Value $Version
Write-Host "fetch-godot.ps1: Godot $Version installed in godot-bin\"
