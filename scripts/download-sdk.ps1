$ErrorActionPreference = "Stop"

$baseDirectory = Split-Path -Parent $PSCommandPath
$rootDirectory = Split-Path -Parent $baseDirectory
$targetDir = "$rootDirectory/ss_player"
$versionFile = "$targetDir/SDK_VERSION.txt"
$currentVersionFile = "$targetDir/runtime/VERSION"

$targetVersion = (Get-Content $versionFile).Trim()

if (Test-Path $currentVersionFile) {
    $currentVersion = (Get-Content $currentVersionFile).Trim()
    if ($currentVersion -eq $targetVersion) {
        Write-Host "SDK $targetVersion is already up to date. Skipping download."
        exit 0
    }
}

$url = "https://github.com/SpriteStudio/SpriteStudio7-SDK/releases/download/$targetVersion/spritestudio7-sdk-static-libs.zip"
$zipFile = "$targetDir/sdk.zip"

Write-Host "Target SDK Version: $targetVersion"
Write-Host "Download URL: $url"
Write-Host "Downloading SDK..."
Invoke-WebRequest -Uri $url -OutFile $zipFile

Write-Host "Extracting SDK..."
if (Test-Path "$targetDir/runtime") {
    Remove-Item -Recurse -Force "$targetDir/runtime"
}
Expand-Archive -Path $zipFile -DestinationPath $targetDir -Force
Remove-Item $zipFile

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
