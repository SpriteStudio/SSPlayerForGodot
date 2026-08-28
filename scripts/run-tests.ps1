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
#
# The extension is named in .godot\extension_list.cfg BEFORE Godot is started,
# because the run in which Godot discovers an extension mid-scan crashes on the
# way out. That crash is Godot's own, and it is not the import -- a project with
# zero importable files does it too:
#
#   EditorHelp::_gen_extensions_docs() dereferences the static DocTools without
#   a null check. Loading an extension mid-scan emits extensions_reloaded, which
#   sends EditorNode off to regenerate the class reference on a worker thread;
#   that thread's last act is to queue _gen_extensions_docs as a deferred call.
#   --import then quits before the message queue is flushed, so the call lands
#   on the flush at the end of Main::cleanup() -- by which time
#   EditorHelp::cleanup_doc() has freed the DocTools and set it to null.
#
# Naming the extension up front means it is loaded at startup instead, so
# extensions_reloaded never fires and nothing is ever queued. Godot rewrites
# this file itself, so seeding it decides only the run in which it did not exist
# yet. There is no retry here: with the file seeded the import either works or
# has failed for some other reason, and a crash is a crash.
if ($DoImport -eq "yes") {
    $extList = Join-Path $Project ".godot\extension_list.cfg"
    if (-not (Test-Path $extList)) {
        $descriptors = Get-ChildItem -Path (Join-Path $Project "addons\spritestudio") -Filter "*.gdextension" -ErrorAction SilentlyContinue
        foreach ($desc in $descriptors) {
            New-Item -ItemType Directory -Force -Path (Join-Path $Project ".godot") | Out-Null
            $rel = $desc.FullName.Substring($Project.Length).TrimStart('\', '/').Replace('\', '/')
            Add-Content -Path $extList -Value "res://$rel"
        }
    }

    $importLog = & $GodotBin --headless --path $Project --import 2>&1
    $importStatus = $LASTEXITCODE
    $importErrors = $importLog | Select-String -Pattern '^ERROR:'
    if ($importStatus -ne 0 -or $importErrors) {
        Write-Host "run-tests.ps1: the import pass failed."
        $importLog | Select-Object -Last 25 | ForEach-Object { Write-Host $_ }
        exit 2
    }
}

# --- run ------------------------------------------------------------------
# --quit-after is a hang guard, not a schedule: an error run_tests.gd's _init
# cannot walk away from -- a suite that will not parse -- leaves the tree idling
# forever. It costs the exit code its meaning on that path -- Godot leaves 0 on
# the way out -- which is why the marker below, and not the status, is what says
# a run completed. A script error inside one case is not that: GDScript
# abandons the case and carries on, and run_tests.gd fails it for recording
# nothing.
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
