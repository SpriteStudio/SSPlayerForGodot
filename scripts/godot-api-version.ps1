#!/usr/bin/env pwsh
#
# Print the Godot API the extension targets -- its floor.
# PowerShell twin of godot-api-version.sh.
#
#   .\scripts\godot-api-version.ps1          # 4.7
#
# It is `compatibility_minimum` in misc/spritestudio.gdextension, which Godot
# reads at load time. build-extension and build-release take it as their default
# api_version, and release.yml builds every platform against it. The custom
# module's guard in ss_player/ss_macros.h and the docs name it as well;
# scripts/check-godot-versions.sh says where they disagree.

$ErrorActionPreference = "Stop"
$app = Split-Path -Leaf $PSCommandPath
$rootDirectory = Split-Path -Parent (Split-Path -Parent $PSCommandPath)

if ($args.Count -gt 0) {
    [Console]::Error.WriteLine("${app}: takes no arguments (got '$($args[0])')")
    exit 2
}

$v = ""
foreach ($line in Get-Content -LiteralPath (Join-Path $rootDirectory "misc/spritestudio.gdextension")) {
    if ($line -match '^compatibility_minimum\s*=\s*"([0-9]+\.[0-9]+)"') { $v = $Matches[1]; break }
}
if (-not $v) {
    [Console]::Error.WriteLine("${app}: misc/spritestudio.gdextension has no compatibility_minimum")
    exit 1
}
Write-Output $v
