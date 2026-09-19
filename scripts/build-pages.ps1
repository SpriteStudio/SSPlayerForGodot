#!/usr/bin/env pwsh
#
# Build the GitHub Pages site locally -- everything .github/workflows/pages.yml
# publishes, in the same order: the documentation toolchain, then the
# documentation in both locales into site/.
# PowerShell twin of build-pages.sh.
#
#   scripts/build-pages.ps1              # the tree pages.yml uploads
#   scripts/build-pages.ps1 serve=yes    # ... and serve it on :8000
#   scripts/build-pages.ps1 prepare=yes  # ... reinstalling the pinned toolchain first
#
# The published site here is the documentation and nothing else -- no demo, no
# examples -- so this is scripts/build-docs.ps1 plus the two things a published
# tree needs that a docs build on its own does not give you:
#
#   1. The toolchain. prepare-docs.ps1 creates .venv and installs the pins from
#      docs/requirements.txt, which is the step a fresh clone forgets. Here it
#      runs only when no zensical is found (prepare=auto).
#   2. A server over the whole tree. `zensical serve` builds one locale at a
#      time, so it cannot show you the header language selector resolving, and
#      the site it serves is not the site that gets uploaded. One static server
#      over site/ is what a reader meets.
#
# `build-pages` means "build what pages.yml publishes" in every repository in
# the SpriteStudio family. In the ones whose site carries a playable demo or the
# API reference it carries them too; here there is nothing else to carry, and the
# name still points at the same thing.
#
# What this does NOT reproduce is the deploy job (actions/deploy-pages) -- there
# is no local equivalent of publishing. Rerunning is safe; nothing here is
# incremental.
$ErrorActionPreference = "Stop"

$APP = Split-Path -Leaf $PSCommandPath
$ROOT_DIR = Split-Path -Parent (Split-Path -Parent $PSCommandPath)

function Fail {
    param([string] $Message, [int] $Code = 1)
    [Console]::Error.WriteLine("${APP}: $Message")
    exit $Code
}

# --- options --------------------------------------------------------------
$opts = [ordered]@{
    prepare = "auto"
    venv    = ".venv"
    serve   = "no"
    port    = "8000"
}

function Show-Usage {
    Write-Host "Usage: $APP [options]"
    Write-Host "$APP options:"
    Write-Host "  prepare=<auto|yes|no>   Run prepare-docs.ps1 first (default: auto)."
    Write-Host "                          auto = only when no zensical is found. yes forces it,"
    Write-Host "                          which is what a changed pin in docs/requirements.txt"
    Write-Host "                          needs; no is for CI, where it is a step of its own."
    Write-Host "  venv=<dir>              Virtualenv for the docs toolchain, relative to the"
    Write-Host "                          repo root (default: .venv). Passed to prepare-docs.ps1"
    Write-Host "                          and build-docs.ps1."
    Write-Host "  serve=<yes|no>          Serve site/ over HTTP when the build is done"
    Write-Host "                          (default: no). Foreground; Ctrl-C to stop."
    Write-Host "  port=<n>                Port for serve=yes (default: 8000)"
    Write-Host "  -h | --help             Show this help"
    Write-Host ""
    Write-Host "Output: site/ (en) + site/ja/ (ja) -- the exact tree pages.yml uploads. Both"
    Write-Host "builds are --strict, so a broken internal link or a nav entry pointing at a"
    Write-Host "missing page fails the build here rather than on a release publish."
    Write-Host ""
    Write-Host "Serve both locales from one server, or the language selector points at pages"
    Write-Host "that are not being served."
}

foreach ($item in $args) {
    if ($item -in "-h", "--help", "help") {
        Show-Usage
        exit 0
    } elseif ($item -match "=") {
        $key, $value = $item -split "=", 2
        if (-not $opts.Contains($key)) {
            Fail "unknown option '$key' (see $APP --help)" 2
        }
        # Lower-cased where the option is a keyword; venv=/port= are not.
        $opts[$key] = if ($key -in "prepare", "serve") { $value.ToLower() } else { $value }
    } else {
        Fail "unknown argument '$item' (options are key=value; see $APP --help)" 2
    }
}

if ($opts.prepare -notin @("auto", "yes", "no")) { Fail "prepare must be auto, yes or no (got '$($opts.prepare)')" 2 }
if ($opts.serve   -notin @("yes", "no"))         { Fail "serve must be yes or no (got '$($opts.serve)')" 2 }
if ($opts.port -notmatch '^\d+$')                { Fail "port must be a number (got '$($opts.port)')" 2 }

$serveNote = if ($opts.serve -eq "no") { "" } else { " (port $($opts.port))" }

Write-Host "options"
Write-Host "  prepare => $($opts.prepare)"
Write-Host "  venv    => $($opts.venv)"
Write-Host "  serve   => $($opts.serve)$serveNote"
Write-Host ""

# --- preflight ------------------------------------------------------------
if (-not (Test-Path (Join-Path $ROOT_DIR "mkdocs.base.yml"))) {
    Fail "mkdocs.base.yml not found -- is this the SSPlayerForGodot repo?"
}

# Every script here resolves its own paths from the repo root, and site/ is
# written relative to it, so run from there however this was invoked.
Push-Location $ROOT_DIR
try {
    function Step {
        param([string] $What, [scriptblock] $Body)
        Write-Host ""
        Write-Host ">> $What"
        & $Body
        if ($LASTEXITCODE -ne 0) { Fail "$What failed ($LASTEXITCODE)" $LASTEXITCODE }
    }

    # --- documentation ----------------------------------------------------
    # Same probe as build-docs.ps1: a venv laid out either way, else PATH (which
    # is how a run with no venv at all still works).
    function Test-Zensical {
        $venvDir = Join-Path $ROOT_DIR $opts.venv
        $venvBin = if (Test-Path (Join-Path $venvDir "Scripts")) {
            Join-Path $venvDir "Scripts"
        } else {
            Join-Path $venvDir "bin"
        }
        foreach ($candidate in @((Join-Path $venvBin "zensical.exe"), (Join-Path $venvBin "zensical"))) {
            if (Test-Path $candidate) { return $true }
        }
        return [bool](Get-Command zensical -ErrorAction SilentlyContinue)
    }

    $prepareScript = Join-Path $ROOT_DIR "scripts/prepare-docs.ps1"
    $buildScript = Join-Path $ROOT_DIR "scripts/build-docs.ps1"

    switch ($opts.prepare) {
        "yes" {
            Step "prepare-docs.ps1 venv=$($opts.venv)" { & $prepareScript "venv=$($opts.venv)" }
        }
        "auto" {
            if (Test-Zensical) {
                Write-Host "${APP}: zensical already available; skipping prepare-docs.ps1 (prepare=yes forces it)"
            } else {
                Step "prepare-docs.ps1 venv=$($opts.venv)" { & $prepareScript "venv=$($opts.venv)" }
            }
        }
        "no" { }
    }

    # Both locales, English first -- build-docs.ps1 owns that order, because the
    # English build clears site/ and the Japanese site lives inside it at site/ja.
    Step "build-docs.ps1 venv=$($opts.venv)" { & $buildScript "venv=$($opts.venv)" }
} finally {
    Pop-Location
}

# --- what landed ----------------------------------------------------------
Write-Host ""
Write-Host "${APP}: done -> site/"
function Report {
    param([string] $Path, [string] $Label)
    if (Test-Path (Join-Path $ROOT_DIR $Path)) {
        Write-Host "  ${Label}: $Path"
    } else {
        Write-Host "  ${Label}: MISSING ($Path)"
    }
}
Report "site/index.html" "en"
Report "site/ja/index.html" "ja"

# --- serve ----------------------------------------------------------------
# py first: it is the Windows launcher, and what build-docs.ps1 already tells
# people to serve with. python/python3 are the fallbacks elsewhere -- on Windows
# a bare `python` can be the Store stub that does nothing.
$PYTHON_CMD = $null
foreach ($candidate in @("py", "python", "python3")) {
    if (Get-Command $candidate -ErrorAction SilentlyContinue) { $PYTHON_CMD = $candidate; break }
}

Write-Host ""
if ($opts.serve -eq "yes") {
    if (-not $PYTHON_CMD) {
        Fail "no python on PATH to serve with -- use any static server on site/"
    }
    Write-Host "${APP}: http://localhost:$($opts.port)/  (English)"
    Write-Host "${APP}: http://localhost:$($opts.port)/ja/  (Japanese)"
    Write-Host "${APP}: Ctrl-C to stop"
    Write-Host ""
    Push-Location $ROOT_DIR
    try {
        & $PYTHON_CMD -m http.server $opts.port -d site
    } finally {
        Pop-Location
    }
    exit $LASTEXITCODE
} else {
    $shown = if ($PYTHON_CMD) { $PYTHON_CMD } else { "python" }
    Write-Host "${APP}: serve it (one server over both locales, so the language selector resolves):"
    Write-Host "${APP}:   $APP serve=yes"
    Write-Host "${APP}:   $shown -m http.server $($opts.port) -d site"
}
