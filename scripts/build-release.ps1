#!/usr/bin/env pwsh
#
# Assemble the release: take the per-platform binaries the build matrix
# produced, lay them out as the `addons/spritestudio/` folder a Godot project
# drops in, and zip it.
#
#   .\scripts\build-release.ps1                       # assemble from artifacts\
#   .\scripts\build-release.ps1 api_version=4.7       # a different Godot API
#   .\scripts\build-release.ps1 out=C:\tmp\rel        # somewhere else
#
# release.yml's package job runs the .sh twin for the asset it uploads, so CI
# and a local run cannot drift. The workflow keeps only what is GitHub's -- the
# checkout, the artifact download and the Release itself.
#
# This is the phase AFTER it has been decided what is being released. Which
# commit, which tag and which build's binaries are all settled before this
# script runs -- by the tag that was pushed, the ref the workflow was dispatched
# from, and the download that populated in=. So there is no option here that
# re-decides any of them.
#
# What this does NOT do is build anything. Six platforms need Linux, Windows and
# macOS between them, so there is no local equivalent of the matrix. What it
# replaces is the part that never needed one: the copies that decide what the
# addon folder contains.
#
# The check at the end is the one that matters here, and it is not a formality.
# spritestudio.gdextension names a file per platform and build target -- nineteen
# paths -- and Godot resolves them at load time. A name that does not match what
# actually shipped does not error at build time or at zip time; the extension
# simply fails to load, on that one platform, for whoever downloaded it. So every
# path the descriptor names is looked up inside the finished archive, along with
# every icon it points at.
param(
    [Parameter(ValueFromRemainingArguments = $true)]
    [string[]] $Arguments
)

$ErrorActionPreference = "Stop"

$app = Split-Path -Leaf $PSCommandPath
$rootDirectory = Split-Path -Parent (Split-Path -Parent $PSCommandPath)

# The platforms the matrix builds, and the order they are reported in.
$platforms = @("linux", "windows", "macos", "android", "ios", "web")

# --- options ---------------------------------------------------------------

$in = "artifacts"
$out = "release-dist"
$apiVersion = "4.7"
$verify = "yes"
$clean = "yes"

function Show-Usage {
    Write-Host "Usage: $app [options]"
    Write-Host "$app options:"
    Write-Host "  in=<dir>                Where the per-platform artifact trees are (default:"
    Write-Host "                          artifacts). One subdirectory per upload-artifact name,"
    Write-Host "                          which is exactly what 'gh run download -D <dir>' produces."
    Write-Host "  out=<dir>               Where the zip and SHA256SUMS land (default: release-dist)."
    Write-Host "                          Relative paths resolve against the current directory."
    Write-Host "  api_version=<ver>       Godot API version the binaries were built for (default:"
    Write-Host "                          4.7). Names the artifacts this reads and the zip it"
    Write-Host "                          writes; same spelling as build-extension.ps1."
    Write-Host "  verify=<yes|no>         Re-open the finished zip and check it (default: yes)."
    Write-Host "  clean=<yes|no>          Delete <out> before assembling (default: yes)."
    Write-Host "  -h | --help             Show this help"
    Write-Host ""
    Write-Host "Output: <out>/ssplayer-godot-extension-<api_version>.zip, holding addons/ at the"
    Write-Host "archive root so a user unzips it straight into a project, plus SHA256SUMS."
    Write-Host ""
    Write-Host "Every platform the matrix builds must be present: a partial release is not a"
    Write-Host "thing worth producing, so a missing one is an error that lists everything else"
    Write-Host "missing alongside it rather than failing on the first."
}

foreach ($argument in $Arguments) {
    switch -Regex ($argument) {
        '^(-h|--help|help)$' { Show-Usage; exit 0 }
        # in=/out= are paths; api_version carries a version string.
        '^in='          { $in = $argument.Substring(3) }
        '^out='         { $out = $argument.Substring(4) }
        '^api_version=' { $apiVersion = $argument.Substring(12) }
        '^verify='      { $verify = $argument.Substring(7).ToLower() }
        '^clean='       { $clean = $argument.Substring(6).ToLower() }
        default {
            Write-Error "${app}: unknown argument '$argument' (options are key=value; see $app --help)"
        }
    }
}

if ($verify -notin @("yes", "no")) { Write-Error "${app}: verify must be yes or no (got '$verify')" }
if ($clean  -notin @("yes", "no")) { Write-Error "${app}: clean must be yes or no (got '$clean')" }
if ($apiVersion -notmatch '^[0-9]') {
    Write-Error "${app}: api_version must look like a Godot version, e.g. 4.7 (got '$apiVersion')"
}

# Resolve both paths against the invoking directory, before anything cds away.
# GetFullPath(path, basePath) rather than Join-Path: Join-Path concatenates
# unconditionally, so a path that is already absolute would be appended to the
# current directory instead of used as given.
$in  = [System.IO.Path]::GetFullPath($in,  (Get-Location).Path)
$out = [System.IO.Path]::GetFullPath($out, (Get-Location).Path)

$rootFull = [System.IO.Path]::GetFullPath($rootDirectory)
if (($out -eq $rootFull) -or ($out -eq [System.IO.Path]::GetPathRoot($out))) {
    Write-Error "${app}: refusing to use '$out' as out= (it is deleted when clean=yes)"
}

Set-Location $rootDirectory

# --- what this release is called -------------------------------------------
#
# A release is a tag: one is pushed, and the workflow is then dispatched from
# it. Reading the ref here is reading that decision, not making it. Absent, this
# is a build off a branch -- which is the default use of this workflow (QA on a
# release/X.Y branch), and no Release is created from one.
if (($env:GITHUB_REF_TYPE -eq "tag") -and $env:GITHUB_REF_NAME) {
    $tag = $env:GITHUB_REF_NAME
    $tagSource = "GITHUB_REF_NAME -- the ref this release was dispatched from"
} else {
    $described = & git -C $rootDirectory describe --exact-match --match 'v[0-9]*' --tags HEAD 2>$null
    if ($LASTEXITCODE -eq 0 -and $described) {
        $tag = $described.Trim()
        $tagSource = "the release tag HEAD is checked out at"
    } else {
        $tag = ""
        $tagSource = "HEAD is not on a release tag, so this is a build, not a release"
    }
}

$zipName = "ssplayer-godot-extension-$apiVersion.zip"

Write-Host "options"
Write-Host "  in          => $in"
Write-Host "  out         => $out"
Write-Host "  api_version => $apiVersion"
Write-Host "  verify      => $verify"
Write-Host "  clean       => $clean"
if ($tag) { Write-Host "  tag         => $tag" } else { Write-Host "  tag         => (none)" }
Write-Host "                 ($tagSource)"

# --- preflight -------------------------------------------------------------

$gdextension = "$rootDirectory/misc/spritestudio.gdextension"
if (-not (Test-Path $gdextension)) {
    Write-Error "${app}: misc/spritestudio.gdextension not found -- is this the SSPlayerForGodot repo?"
}

# Get-FileHash returns an upper-case hash; sha256sum writes lower-case. Emit the
# lower-case form so SHA256SUMS is byte-identical to the one the shell twin (and
# CI) writes, and so `sha256sum -c` accepts it.
function Get-Sha256($file) { (Get-FileHash -Algorithm SHA256 -Path $file).Hash.ToLower() }

function Write-Step($message) {
    Write-Host ""
    Write-Host ">> $message"
}

$failed = New-Object System.Collections.Generic.List[string]

# --- check the input -------------------------------------------------------
# Everything at once. Finding out about the fifth missing platform on the fifth
# run of a script that takes a matrix build to feed is not a debugging loop
# worth having.
Write-Step "checking $in"

if (-not (Test-Path -PathType Container $in)) {
    Write-Error "${app}: $in does not exist. Populate it first -- 'gh run download <run-id> -D artifacts' -- or point in= at the artifacts"
}

# Each platform job uploads bin/ as extension-<api>-<platform>, so the tree
# holds that platform's binaries under its own name plus the staged licenses/.
# The linux one is where the shared runtime notices are taken from: they are
# identical across platforms, and picking one keeps the copy unambiguous.
$canonical = "extension-$apiVersion-linux"

foreach ($platform in $platforms) {
    $dir = "$in/extension-$apiVersion-$platform"
    if (-not (Test-Path -PathType Container $dir)) {
        $failed.Add("extension-$apiVersion-$platform/ is missing")
    } elseif (-not (Test-Path -PathType Container "$dir/$platform")) {
        $failed.Add("extension-$apiVersion-$platform/ has no $platform/ (the binaries the matrix builds)")
    }
}

foreach ($notice in @("THIRD-PARTY-LICENSES.ssruntime.md", "THIRD-PARTY-LICENSES.ssconverter.md", "runtime-LICENSE.md")) {
    if (-not (Test-Path "$in/$canonical/licenses/$notice")) {
        $failed.Add("$canonical/licenses/$notice is missing")
    }
}

if ($failed.Count -gt 0) {
    Write-Error "${app}: $in is missing pieces the release needs:`n  $($failed -join "`n  ")`n`nPoint in= at a complete matrix build for Godot $apiVersion."
}
Write-Host "   every platform the matrix builds is present"

# --- assemble --------------------------------------------------------------
# The addon a user drops into their project: `addons/spritestudio/` with the
# descriptor at its root, the binaries under bin/<platform>/, the editor icons
# the [icons] section points at, and every licence the shipped binaries carry.
$work = "$rootDirectory/build/release-staging"
$addon = "$work/addons/spritestudio"

Write-Step "assembling the addon in $work"

if (Test-Path $work) { Remove-Item -Recurse -Force $work }
New-Item -ItemType Directory -Force -Path "$addon/bin", "$addon/icons", "$addon/licenses" | Out-Null

Copy-Item -Force $gdextension "$addon/"
Copy-Item -Force "$rootDirectory/LICENSE.md" "$addon/"
Copy-Item -Force "$rootDirectory/LICENSE.ja.md" "$addon/"
Write-Host "   spritestudio.gdextension + LICENSE.md + LICENSE.ja.md"

# Editor icons referenced by the [icons] section. They are source files, not
# build outputs, so they never reach the platform artifacts -- copy them
# straight from the tree (same set as build-extension.ps1).
Copy-Item -Force "$rootDirectory/ss_player/icons/icon_*.svg" "$addon/icons/"
Write-Host "   icons/ ($((Get-ChildItem "$addon/icons").Count) files)"

# Third-party notices next to the statically-linked native binaries:
# FlatBuffers (Apache-2.0), godot-cpp (MIT), and the Rust runtime crates. The
# runtime crate licences were staged into each platform artifact and are
# identical across them, so they come from the canonical one.
Copy-Item -Force "$rootDirectory/LICENSE.md" "$addon/licenses/"
Copy-Item -Force "$rootDirectory/LICENSE.ja.md" "$addon/licenses/"
Copy-Item -Force "$rootDirectory/THIRD_PARTY_NOTICES.md" "$addon/licenses/"
Copy-Item -Force "$rootDirectory/licenses/Apache-2.0.txt" "$addon/licenses/"
Copy-Item -Force "$in/$canonical/licenses/*" "$addon/licenses/"
Write-Host "   licenses/ ($((Get-ChildItem "$addon/licenses").Count) files)"

# Copied, not moved: unlike the workflow this replaces, the input tree survives,
# so one download serves any number of attempts.
foreach ($platform in $platforms) {
    Copy-Item -Recurse -Force "$in/extension-$apiVersion-$platform/$platform" "$addon/bin/"
}
Write-Host "   bin/ ($($platforms -join ' '))"

# --- archive ---------------------------------------------------------------
Write-Step "archiving into $out"

if (($clean -eq "yes") -and (Test-Path $out)) { Remove-Item -Recurse -Force $out }
New-Item -ItemType Directory -Force -Path $out | Out-Null

# addons/ at the archive root, so a user unzips it straight into a project.
# `zip` rather than Compress-Archive: -y stores symlinks rather than following
# them, which Compress-Archive cannot do at all. The macOS .framework bundles
# are built by SCons and a future one carrying version symlinks would otherwise
# be flattened, taking its code signature with it. On Windows there are no
# symlinks to store and the flag is a no-op, but using the same tool as the .sh
# twin is what keeps the two archives identical.
if (-not (Get-Command zip -ErrorAction SilentlyContinue)) {
    Write-Error "${app}: zip not found -- it is what packages the archive, and Compress-Archive cannot store symlinks (see the comment here). Install it, or run scripts/build-release.sh."
}
Push-Location $work
try { & zip -qry "$out/$zipName" addons/ } finally { Pop-Location }
Write-Host "   $zipName"

# Canonical checksum manifest so a consumer can verify the downloaded zip's
# integrity (`sha256sum -c SHA256SUMS --ignore-missing`). Two spaces, LF, no
# BOM: that is the format, on every platform.
$sumLines = foreach ($file in (Get-ChildItem "$out/*.zip" | Sort-Object Name)) {
    "$(Get-Sha256 $file.FullName)  $($file.Name)"
}
[System.IO.File]::WriteAllText("$out/SHA256SUMS", ($sumLines -join "`n") + "`n", (New-Object System.Text.UTF8Encoding $false))
Write-Host "   SHA256SUMS"

# --- verify ----------------------------------------------------------------
if ($verify -eq "yes") {
    Write-Step "verifying $out"

    if (-not (Test-Path "$out/$zipName")) { Write-Error "${app}: $zipName is missing" }

    $entries = & unzip -Z1 "$out/$zipName"
    function Test-Entry($path) { $entries -contains $path }
    function Test-EntryPrefix($prefix) { ($entries | Where-Object { $_.StartsWith($prefix) }).Count -gt 0 }

    if (-not (Test-Entry "addons/spritestudio/spritestudio.gdextension")) {
        $failed.Add("$zipName has no addons/spritestudio/spritestudio.gdextension (Godot loads nothing without it)")
    }

    # THE check. Every path the descriptor names, looked up in the archive. Godot
    # resolves these at load time, so a name that does not match what shipped
    # fails silently on that platform for whoever downloaded it -- and nothing
    # earlier in this pipeline compares the two.
    #
    # [libraries] paths are relative to the addon folder; [icons] paths are
    # res:// URLs into it. A .framework or .xcframework is a directory, so it is
    # matched by something existing underneath it rather than as a file.
    $descriptor = Get-Content $gdextension
    $section = ""
    $libraryPaths = New-Object System.Collections.Generic.List[string]
    $iconPaths = New-Object System.Collections.Generic.List[string]
    foreach ($line in $descriptor) {
        if ($line -match '^\s*\[(.+)\]\s*$') { $section = $Matches[1]; continue }
        if ($line -match '^[^=]*=\s*"(.*)"\s*$') {
            if ($section -eq "libraries") { $libraryPaths.Add($Matches[1]) }
            elseif ($section -eq "icons") { $iconPaths.Add($Matches[1]) }
        }
    }

    $missingLibs = 0
    foreach ($path in $libraryPaths) {
        $entry = "addons/spritestudio/$path"
        if (Test-Entry $entry) { continue }
        if (Test-EntryPrefix "$entry/") { continue }
        $failed.Add("spritestudio.gdextension names $path, which is not in $zipName")
        $missingLibs++
    }
    foreach ($path in $iconPaths) {
        $entry = $path -replace '^res://', ''
        if (-not (Test-Entry $entry)) {
            $failed.Add("spritestudio.gdextension points at $path, which is not in $zipName")
        }
    }

    if ($libraryPaths.Count -eq 0) {
        $failed.Add("no [libraries] entries parsed out of spritestudio.gdextension -- the check above proved nothing")
    } elseif ($missingLibs -eq 0) {
        Write-Host "   spritestudio.gdextension: all $($libraryPaths.Count) libraries and $($iconPaths.Count) icons are in the archive"
    }

    # The licences the shipped binaries carry.
    foreach ($notice in @("LICENSE.md", "LICENSE.ja.md", "THIRD_PARTY_NOTICES.md", "Apache-2.0.txt",
                          "THIRD-PARTY-LICENSES.ssruntime.md", "THIRD-PARTY-LICENSES.ssconverter.md",
                          "runtime-LICENSE.md")) {
        if (-not (Test-Entry "addons/spritestudio/licenses/$notice")) {
            $failed.Add("$zipName is missing licenses/$notice")
        }
    }

    $symlinks = (& unzip -Z "$out/$zipName" | Where-Object { $_.StartsWith("l") }).Count
    Write-Host "   $($entries.Count) entries, $symlinks symlink(s) stored"

    foreach ($line in Get-Content "$out/SHA256SUMS") {
        $parts = $line -split '\s+', 2
        if ($parts.Count -ne 2) { continue }
        if ((Get-Sha256 "$out/$($parts[1].Trim())") -ne $parts[0].ToLower()) {
            $failed.Add("SHA256SUMS does not verify for $($parts[1].Trim())")
        }
    }

    if ($failed.Count -gt 0) {
        Write-Host ""
        Write-Error "${app}: the release in $out is not shippable:`n  $($failed -join "`n  ")"
    }
    Write-Host "   SHA256SUMS verified"
    Write-Host "   all checks passed"
} else {
    Write-Step "verify=no -- the archive is not checked"
}

# --- done ------------------------------------------------------------------
Write-Host ""
if ($tag) {
    Write-Host "${app}: done -> $out  (Godot $apiVersion, $tag)"
} else {
    Write-Host "${app}: done -> $out  (Godot $apiVersion)"
}
Write-Host ""
Get-ChildItem -Path $out | ForEach-Object { Write-Host ("  {0,12}  {1}" -f $_.Length, $_.Name) }
Write-Host ""
Write-Host "${app}: this is what release.yml attaches to the draft Release."
Write-Host "${app}: try it before shipping it -- unzip into a project and open the editor:"
Write-Host "  unzip -o $out/$zipName -d <your-godot-project>/"
