$ErrorActionPreference = "Stop"

$baseDirectory = Split-Path -Parent $PSCommandPath
$rootDirectory = Split-Path -Parent $baseDirectory
$targetDir = "$rootDirectory/ss_player"
$versionFile = "$baseDirectory/SDK_VERSION.txt"
$currentVersionFile = "$targetDir/runtime/VERSION"
# Downloaded zip is cached here (gitignored) so a re-run that needs the same
# version reuses it instead of re-downloading tens of MB.
$cacheDir = "$baseDirectory/.sdk-cache"

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
# Keep the release asset's own name so it matches the SHA256SUMS entry directly.
$zipFile = "$cacheDir/$asset"
$sumsFile = "$cacheDir/SHA256SUMS"

Write-Host "Target SDK Version: $targetVersion"
New-Item -ItemType Directory -Force -Path $cacheDir | Out-Null

function Get-Sha256($file) { (Get-FileHash -Algorithm SHA256 -Path $file).Hash.ToLower() }

# Fetch the checksum manifest (small): it both verifies integrity and decides
# whether an already-cached zip can be reused without re-downloading.
Write-Host "Downloading checksum manifest: $baseUrl/SHA256SUMS"
Invoke-WebRequest -Uri "$baseUrl/SHA256SUMS" -OutFile $sumsFile

$expected = $null
foreach ($line in Get-Content $sumsFile) {
    $parts = $line -split '\s+', 2
    if ($parts.Count -eq 2 -and $parts[1].Trim() -eq $asset) { $expected = $parts[0].ToLower(); break }
}
if (-not $expected) {
    Write-Error "No checksum entry for $asset in SHA256SUMS"
    exit 1
}

# Reuse the cached zip when it already matches the manifest; otherwise download.
if ((Test-Path $zipFile) -and ((Get-Sha256 $zipFile) -eq $expected)) {
    Write-Host "Reusing cached $asset"
} else {
    Write-Host "Downloading SDK: $baseUrl/$asset"
    Invoke-WebRequest -Uri "$baseUrl/$asset" -OutFile $zipFile
    if ((Get-Sha256 $zipFile) -ne $expected) {
        Remove-Item -Force $zipFile
        Write-Error "Checksum mismatch for $asset"
        exit 1
    }
}
Write-Host "Checksum OK: $asset"

# The release archive is a flat tree (libs/, include/, VERSION, …) with no
# wrapper directory, so extract it straight into ss_player/runtime/.
Write-Host "Extracting SDK..."
if (Test-Path "$targetDir/runtime") {
    Remove-Item -Recurse -Force "$targetDir/runtime"
}
Expand-Archive -Path $zipFile -DestinationPath "$targetDir/runtime" -Force

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
