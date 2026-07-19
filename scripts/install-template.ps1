param (
    [Parameter(Mandatory=$true, HelpMessage="Available platforms: macos, windows, linux, ios, android, web")]
    [ValidateSet("macos", "windows", "linux", "ios", "android", "web")]
    [string]$Platform
)

$ErrorActionPreference = "Stop"

$BaseDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$RootDir = Resolve-Path "$BaseDir\.." | Select-Object -ExpandProperty Path

Push-Location $RootDir

# 1. Set the base export-templates directory (Windows / %APPDATA%).
$TemplatesBase = "$env:APPDATA\Godot\export_templates"

# 2. Read the Godot version from version.py to build the templates directory name.
$VersionPy = Get-Content "godot\version.py"
$GodotMajor = ($VersionPy | Select-String '^major').Line.Split('=')[1].Trim(" `'""")
$GodotMinor = ($VersionPy | Select-String '^minor').Line.Split('=')[1].Trim(" `'""")
$GodotPatch = ($VersionPy | Select-String '^patch').Line.Split('=')[1].Trim(" `'""")
$GodotStatus = ($VersionPy | Select-String '^status').Line.Split('=')[1].Trim(" `'""")

if ($GodotPatch -ne "0") {
    $VersionString = "${GodotMajor}.${GodotMinor}.${GodotPatch}.${GodotStatus}"
} else {
    $VersionString = "${GodotMajor}.${GodotMinor}.${GodotStatus}"
}

$TemplatesDir = "$TemplatesBase\$VersionString"
if (-not (Test-Path $TemplatesDir)) {
    New-Item -ItemType Directory -Force -Path $TemplatesDir | Out-Null
}
Write-Host "Target template directory: $TemplatesDir"

Push-Location "godot\bin"

switch ($Platform) {
    "windows" {
        Write-Host "Processing Windows templates..."
        Get-ChildItem -Filter "godot.windows.template_release.*.exe" | ForEach-Object {
            if ($_.Name -match "godot\.windows\.template_release\.(.*)\.exe") {
                $arch = $matches[1]
                Copy-Item $_.FullName "$TemplatesDir\windows_release_${arch}.exe" -Force
            }
        }
        Get-ChildItem -Filter "godot.windows.template_debug.*.exe" | ForEach-Object {
            if ($_.Name -match "godot\.windows\.template_debug\.(.*)\.exe") {
                $arch = $matches[1]
                Copy-Item $_.FullName "$TemplatesDir\windows_debug_${arch}.exe" -Force
            }
        }
    }
    
    "linux" {
        Write-Host "Processing Linux templates..."
        Get-ChildItem -Filter "godot.linuxbsd.template_release.*" | ForEach-Object {
            if ($_.Name -match "godot\.linuxbsd\.template_release\.(.*)") {
                $arch = $matches[1]
                Copy-Item $_.FullName "$TemplatesDir\linux_release.${arch}" -Force
            }
        }
        Get-ChildItem -Filter "godot.linuxbsd.template_debug.*" | ForEach-Object {
            if ($_.Name -match "godot\.linuxbsd\.template_debug\.(.*)") {
                $arch = $matches[1]
                Copy-Item $_.FullName "$TemplatesDir\linux_debug.${arch}" -Force
            }
        }
    }
    
    "android" {
        Write-Host "Processing Android templates..."
        if (Test-Path "android_source.zip") { Copy-Item "android_source.zip" "$TemplatesDir\android_source.zip" -Force }
        if (Test-Path "android_release.apk") { Copy-Item "android_release.apk" "$TemplatesDir\android_release.apk" -Force }
        if (Test-Path "android_debug.apk") { Copy-Item "android_debug.apk" "$TemplatesDir\android_debug.apk" -Force }
    }
    
    "web" {
        Write-Host "Processing Web (nothreads) templates..."
        if (Test-Path "godot.web.template_release.wasm32.nothreads.zip") { Copy-Item "godot.web.template_release.wasm32.nothreads.zip" "$TemplatesDir\web_nothreads_release.zip" -Force }
        if (Test-Path "godot.web.template_debug.wasm32.nothreads.zip") { Copy-Item "godot.web.template_debug.wasm32.nothreads.zip" "$TemplatesDir\web_nothreads_debug.zip" -Force }
        if (Test-Path "godot.web.template_release.wasm32.nothreads.dlink.zip") { Copy-Item "godot.web.template_release.wasm32.nothreads.dlink.zip" "$TemplatesDir\web_nothreads_dlink_release.zip" -Force }
        if (Test-Path "godot.web.template_debug.wasm32.nothreads.dlink.zip") { Copy-Item "godot.web.template_debug.wasm32.nothreads.dlink.zip" "$TemplatesDir\web_nothreads_dlink_debug.zip" -Force }
    }
    
    "macos" {
        Write-Host "Processing macOS templates..."
        if (Test-Path "macos_template.app") { Remove-Item -Recurse -Force "macos_template.app" }
        if (Test-Path "macos.zip") { Remove-Item -Force "macos.zip" }
        Copy-Item -Recurse -Force "..\misc\dist\macos_template.app" ".\"
        New-Item -ItemType Directory -Force -Path "macos_template.app\Contents\MacOS" | Out-Null
        
        if (Test-Path "godot.macos.template_release.universal") { Copy-Item "godot.macos.template_release.universal" "macos_template.app\Contents\MacOS\godot_macos_release.universal" -Force }
        if (Test-Path "godot.macos.template_debug.universal") { Copy-Item "godot.macos.template_debug.universal" "macos_template.app\Contents\MacOS\godot_macos_debug.universal" -Force }
        
        Compress-Archive -Path "macos_template.app" -DestinationPath "macos.zip" -Force
        Copy-Item "macos.zip" "$TemplatesDir\macos.zip" -Force
    }
    
    "ios" {
        Write-Host "Processing iOS templates..."
        if (Test-Path "apple_embedded_xcode") { Remove-Item -Recurse -Force "apple_embedded_xcode" }
        if (Test-Path "ios.zip") { Remove-Item -Force "ios.zip" }
        Copy-Item -Recurse -Force "..\misc\dist\apple_embedded_xcode" ".\"
        
        if (Test-Path "libgodot.ios.template_release.xcframework") {
            Remove-Item -Recurse -Force "apple_embedded_xcode\libgodot.ios.release.xcframework" -ErrorAction SilentlyContinue
            Copy-Item -Recurse -Force "libgodot.ios.template_release.xcframework" "apple_embedded_xcode\libgodot.ios.release.xcframework"
        }
        if (Test-Path "libgodot.ios.template_debug.xcframework") {
            Remove-Item -Recurse -Force "apple_embedded_xcode\libgodot.ios.debug.xcframework" -ErrorAction SilentlyContinue
            Copy-Item -Recurse -Force "libgodot.ios.template_debug.xcframework" "apple_embedded_xcode\libgodot.ios.debug.xcframework"
        }
        
        Push-Location "apple_embedded_xcode"
        # Compress-Archive puts files in zip but we want contents. 
        # * requires PS 5.1+ to work nicely or just zip the folder
        Compress-Archive -Path "*" -DestinationPath "..\ios.zip" -Force
        Pop-Location
        
        Copy-Item "ios.zip" "$TemplatesDir\ios.zip" -Force
    }
}

Write-Host "======================================"
Write-Host "Successfully installed $Platform templates to:"
Write-Host $TemplatesDir

Pop-Location
Pop-Location
