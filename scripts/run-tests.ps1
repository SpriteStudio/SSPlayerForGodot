#!/usr/bin/env pwsh
#
# Run the headless test suite in test_gdextension\ against the GDExtension build. Windows
# counterpart of scripts/run-tests.sh (same key=value interface).
#
# The work is test_gdextension\run_tests.gd; this finds a Godot binary, runs the import
# pass the project needs before its first run, and forwards the options.
#
# Two prerequisites, both gitignored and so both missing from a fresh clone.
# run_tests.gd checks them before it starts anything and names the script that
# produces each:
#   test_gdextension\addons\spritestudio\  <- build-extension.ps1
#   test_gdextension\ssab_generated\       <- deploy-examples.ps1
#
# It never downloads anything on your behalf. When no binary is found it prints
# scripts\fetch-godot.ps1 and stops; a download of tens of MB is your decision.
#
# What this does NOT cover: drawing, and the custom-module build. --headless
# installs a dummy rasteriser, so there are no pixels to compare; and a module
# is compiled into the engine, so testing that build would mean building Godot
# rather than downloading it. The two builds share one copy of the playback
# logic and differ only in a layer of #ifdef SPRITESTUDIO_GODOT_EXTENSION
# adapters -- includes and type conversions, which is what a compiler checks.
#
# Usage: scripts\run-tests.ps1 [godot=<path>] [only=<names>] [import=no]
#   godot  : the Godot binary to use (else $env:GODOT, godot-bin\, then PATH)
#   only   : comma-separated substrings; run only the suites/cases that match
#   import : no to skip the import pass, once test_gdextension\.godot exists
#
# Exit status: 0 all passed, 1 a case failed, 2 preflight or setup failed.

$ErrorActionPreference = "Stop"
$RootDir = Split-Path -Parent (Split-Path -Parent $PSCommandPath)
$ScriptDir = Join-Path $RootDir "scripts"
$Project = Join-Path $RootDir "test_gdextension"

$GodotBin = $env:GODOT
$Only = ""
$DoImport = "yes"
foreach ($item in $Args) {
    if ($item -match "^-?-?help$" -or $item -eq "-h") {
        $emit = $false
        foreach ($line in (Get-Content $PSCommandPath)) {
            if ($line -match '^# Usage:') { $emit = $true }
            if ($emit) { $line -replace '^# ?', '' }
            if ($emit -and $line -match '^# Exit status') { break }
        }
        exit 0
    }
    $kv = $item -split "=", 2
    switch ($kv[0]) {
        "godot"  { $GodotBin = $kv[1] }
        "only"   { $Only = $kv[1] }
        "import" { $DoImport = $kv[1] }
        default  { Write-Error "unknown arg '$item'" }
    }
}

# --- find a binary --------------------------------------------------------
# godot\bin\* is deliberately not in this list. That is a custom module build
# with SpriteStudio compiled into it, so it registers the classes a second time
# and aborts on a project carrying addons\spritestudio -- see the message below.
if (-not $GodotBin) {
    foreach ($cand in @(
            (Join-Path $RootDir "godot-bin\godot.exe"),
            (Join-Path $RootDir "godot-bin\godot"),
            (Join-Path $RootDir "godot-bin\Godot.app\Contents\MacOS\Godot"))) {
        if (Test-Path $cand) { $GodotBin = $cand; break }
    }
}
if (-not $GodotBin) {
    $onPath = Get-Command "godot" -ErrorAction SilentlyContinue
    if ($onPath) { $GodotBin = $onPath.Source }
}
if (-not $GodotBin -or -not (Test-Path $GodotBin)) {
    $pin = (Get-Content (Join-Path $ScriptDir "GODOT_VERSION.txt")).Trim()
    Write-Host @"
run-tests.ps1: no Godot binary found.

  Pass one:            scripts\run-tests.ps1 godot=C:\path\to\godot.exe
  or install the pin:  scripts\fetch-godot.ps1    ($pin, the editor build only)

Not godot\bin\* — that is a custom module build with SpriteStudio compiled in,
and it cannot open a project that also loads the extension.
"@
    exit 2
}

Write-Host "run-tests.ps1: $(& $GodotBin --headless --version | Select-Object -Last 1) at $GodotBin"

# --- import pass ----------------------------------------------------------
# A project Godot has never opened has no .godot\, and the textures beside each
# .ssab are not importable until it does. Cheap after the first run.
# It is run twice on purpose, and the SECOND run is the one that has to pass.
#
# The run in which Godot first DISCOVERS the extension aborts on the way out
# (null dereference, caught by Godot's own crash handler). Not the import: a
# project with zero importable files does it too. What triggers it is the
# extension being loaded mid-scan rather than at startup from
# .godot/extension_list.cfg -- delete just that file and it happens again.
#
# It is NOT this repository's code. godot-cpp's own test extension, with none of
# our sources and a different descriptor, reproduces it exactly; a project with
# no extension does not; and registering nothing at all still does. godot-cpp
# has no 4.6/4.7 release branch (it went from godot-4.5-stable straight to the
# 10.0 line), so an extension for Godot 4.7 is built from master against
# api_version=4.7, and that is the combination that does this.
#
# The scan's work completes -- the second run exits 0 with nothing left to do.
# So this is a retry, not a tolerance. A crash that repeats is still a failure
# here, and the clean second run is the evidence that the import finished --
# nothing is being waved through on the strength of the first one.
if ($DoImport -eq "yes") {
    $importLog = & $GodotBin --headless --path $Project --import 2>&1
    $importStatus = $LASTEXITCODE
    if ($importStatus -ne 0) {
        Write-Host "run-tests.ps1: the first import exited $importStatus (godot-cpp's known first-scan crash); retrying."
        $importLog = & $GodotBin --headless --path $Project --import 2>&1
        $importStatus = $LASTEXITCODE
    }
    $importErrors = $importLog | Select-String -Pattern '^ERROR:'
    if ($importStatus -ne 0 -or $importErrors) {
        Write-Host "run-tests.ps1: the import pass failed."
        $importLog | Select-Object -Last 25 | ForEach-Object { Write-Host $_ }
        exit 2
    }
}

# --- run ------------------------------------------------------------------
# --quit-after is a hang guard, not a schedule: a script error inside a case
# aborts run_tests.gd's _init and leaves the tree idling forever. It costs the
# exit code its meaning on that path -- Godot leaves 0 on the way out -- which
# is why the marker below, and not the status, is what says a run completed.
$runArgs = @("--headless", "--path", $Project, "--quit-after", "100000",
             "--script", "res://run_tests.gd")
if ($Only) { $runArgs += @("--", "--only=$Only") }

$output = & $GodotBin @runArgs 2>&1
$status = $LASTEXITCODE
$output | ForEach-Object { Write-Host $_ }

# Godot aborts during extension validation, before run_tests.gd gets to say
# anything, so this is the one failure the wrapper has to explain itself.
if ($output -match "appears to be already registered") {
    Write-Host @"

run-tests.ps1: that Godot binary already has SpriteStudio compiled into it (a
custom module build), so loading the extension registers every class twice.

  Use a stock binary (scripts\fetch-godot.ps1), or remove
  test_gdextension\addons\spritestudio to run the same suite against the module build.
"@
    exit 2
}

if ($output -notmatch "==== SUITE FINISHED ====") {
    Write-Host "`nrun-tests.ps1: the run stopped before the suite finished — see above."
    exit 1
}

exit $status
