$ErrorActionPreference = "Stop"

$baseDirectory = Split-Path -Parent $PSCommandPath
$rootDirectory = Split-Path -Parent $baseDirectory
$targetDir = "$rootDirectory/ss_player"
$versionFile = "$baseDirectory/SDK_VERSION.txt"
$currentVersionFile = "$targetDir/runtime/VERSION"

$targetVersion = (Get-Content $versionFile).Trim()

if (Test-Path $currentVersionFile) {
    $currentVersion = (Get-Content $currentVersionFile).Trim()
    if ($currentVersion -eq $targetVersion) {
        Write-Host "SDK $targetVersion is already up to date. Skipping download."
        exit 0
    }
}

$baseUrl = "https://github.com/cri-middleware/SpriteStudio-SDK/releases/download/$targetVersion"
$asset = "spritestudio-sdk-static-libs.zip"
$zipFile = "$targetDir/sdk.zip"
$sumsFile = "$targetDir/SHA256SUMS"

Write-Host "Target SDK Version: $targetVersion"

# Fetch the release's SHA256SUMS manifest and verify the downloaded zip against
# it before extracting, so a corrupted or tampered download fails loudly.
Write-Host "Downloading checksum manifest: $baseUrl/SHA256SUMS"
Invoke-WebRequest -Uri "$baseUrl/SHA256SUMS" -OutFile $sumsFile

Write-Host "Downloading SDK: $baseUrl/$asset"
Invoke-WebRequest -Uri "$baseUrl/$asset" -OutFile $zipFile

# Verify <file> against the manifest entry for <asset-name>. Get-FileHash returns
# an upper-case hash; sha256sum writes lower-case, so compare case-insensitively.
function Assert-Sha256($file, $name) {
    $expected = $null
    foreach ($line in Get-Content $sumsFile) {
        $parts = $line -split '\s+', 2
        if ($parts.Count -eq 2 -and $parts[1].Trim() -eq $name) { $expected = $parts[0].ToLower(); break }
    }
    if (-not $expected) {
        Write-Error "No checksum entry for $name in SHA256SUMS"
        exit 1
    }
    $actual = (Get-FileHash -Algorithm SHA256 -Path $file).Hash.ToLower()
    if ($expected -ne $actual) {
        Write-Error "Checksum mismatch for $name`n  expected: $expected`n  actual:   $actual"
        exit 1
    }
    Write-Host "Checksum OK: $name"
}
Assert-Sha256 $zipFile $asset

Write-Host "Extracting SDK..."
if (Test-Path "$targetDir/runtime") {
    Remove-Item -Recurse -Force "$targetDir/runtime"
}
Expand-Archive -Path $zipFile -DestinationPath $targetDir -Force
Remove-Item $zipFile
Remove-Item $sumsFile

# Godot Custom Module compatibility (Windows x86_64)
$winLibDir = "$targetDir/runtime/libs/windows/x86_64"
if (Test-Path $winLibDir) {
    Write-Host "Creating library copies for Godot Custom Module..."
    $targets = "editor", "template_release", "template_debug"
    foreach($target in $targets) {
        Copy-Item "$winLibDir/ssruntime.lib" "$winLibDir/ssruntime.windows.$target.x86_64.lib" -Force
        Copy-Item "$winLibDir/ssconverter.lib" "$winLibDir/ssconverter.windows.$target.x86_64.lib" -Force
    }
}

Set-Content -Path $currentVersionFile -Value $targetVersion

Write-Host "Done."
