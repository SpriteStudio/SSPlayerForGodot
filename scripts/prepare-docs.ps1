#!/usr/bin/env pwsh
#
# Create the local virtualenv and install the pinned documentation toolchain.
# PowerShell twin of prepare-docs.sh.
#
#   scripts/prepare-docs.ps1           # .venv + docs/requirements.txt
#   scripts/prepare-docs.ps1 force=yes # discard the venv and build it again
#
# Documentation only, and independent of everything else in this directory: the
# other scripts here build and release the code in this repository, and none of
# them has to have run before the docs will build. One Python environment is the
# whole documentation toolchain. The -docs suffix is what keeps the two apart --
# every repository in the SpriteStudio family spells this pair the same way.
#
# The pins in docs/requirements.txt are exact. Rerun this script after a pin
# changes -- pip installs the pinned version over whatever the venv already has.
#
# Then: scripts/build-docs.ps1, which runs zensical out of the venv this created.
$ErrorActionPreference = "Stop"

$APP = Split-Path -Leaf $PSCommandPath
$ROOT_DIR = Split-Path -Parent (Split-Path -Parent $PSCommandPath)
$REQUIREMENTS = "docs/requirements.txt"

# Plain stderr + an exit code, rather than Write-Error's stack trace:
# prepare-docs.sh answers the same mistakes the same way (2 = bad usage, 1 = the
# environment is not ready).
function Fail {
    param([string] $Message, [int] $Code = 1)
    [Console]::Error.WriteLine("${APP}: $Message")
    exit $Code
}

# --- options --------------------------------------------------------------
# `py` is the Windows launcher and the reason this default differs from
# prepare-docs.sh's python3: it is what a stock python.org install puts on PATH.
$opts = [ordered]@{
    python = "py"
    venv   = ".venv"
    force  = "no"
}

function Show-Usage {
    Write-Host "Usage: $APP [options]"
    Write-Host "$APP options:"
    Write-Host "  python=<exe>            Interpreter to create the venv with (default: py)"
    Write-Host "  venv=<dir>              Virtualenv location, relative to the repo root"
    Write-Host "                          (default: .venv)"
    Write-Host "  force=<yes|no>          Delete an existing venv and recreate it, rather than"
    Write-Host "                          installing into it (default: no)"
    Write-Host "  -h | --help             Show this help"
    Write-Host ""
    Write-Host "Installs $REQUIREMENTS, whose pins are exact. Run it again whenever a pin"
    Write-Host "changes; force=yes is only for a venv that has gone wrong."
    Write-Host ""
    Write-Host "Building the site is scripts/build-docs.ps1, not this script."
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
        # Only the yes/no flag is case-folded; python= and venv= are paths.
        $opts[$key] = if ($key -eq "force") { $value.ToLower() } else { $value }
    } else {
        Fail "unknown argument '$item' (options are key=value; see $APP --help)" 2
    }
}

if ($opts.force -notin "yes", "no") { Fail "force must be yes or no (got '$($opts.force)')" 2 }

Write-Host "options"
Write-Host "  python => $($opts.python)"
Write-Host "  venv   => $($opts.venv)"
Write-Host "  force  => $($opts.force)"
Write-Host ""

# --- preflight ------------------------------------------------------------
if (!(Test-Path (Join-Path $ROOT_DIR $REQUIREMENTS))) {
    Fail "$REQUIREMENTS not found -- is this the SSPlayerForGodot repo?"
}

if (!(Get-Command $opts.python -ErrorAction SilentlyContinue)) {
    # `py` ships with the Windows installer only; elsewhere pwsh finds python3.
    if ($opts.python -eq "py" -and (Get-Command "python3" -ErrorAction SilentlyContinue)) {
        $opts.python = "python3"
        Write-Host "${APP}: 'py' not found; using python3"
    } else {
        [Console]::Error.WriteLine("${APP}: '$($opts.python)' not found (install Python 3, or pass python=<exe>)")
        Fail "on macOS and Linux the interpreter is usually 'python3': $APP python=python3"
    }
}

function Invoke-Checked {
    param([string] $What, [scriptblock] $Body)
    & $Body
    if ($LASTEXITCODE -ne 0) { Fail "$What failed ($LASTEXITCODE)" $LASTEXITCODE }
}

$VENV_DIR = Join-Path $ROOT_DIR $opts.venv

# --- venv -----------------------------------------------------------------
if ($opts.force -eq "yes" -and (Test-Path $VENV_DIR)) {
    Write-Host ">> removing the existing venv ($($opts.venv))"
    Remove-Item -Recurse -Force $VENV_DIR
}

if (Test-Path $VENV_DIR) {
    Write-Host ">> reusing the existing venv ($($opts.venv))"
} else {
    Write-Host ">> $($opts.python) -m venv $($opts.venv)"
    Invoke-Checked "venv creation" { & $opts.python -m venv $VENV_DIR }
}

# Scripts/ on Windows, bin/ everywhere else -- pwsh runs on all three, so probe
# for the layout on disk rather than branching on the host OS.
$VENV_BIN = if (Test-Path (Join-Path $VENV_DIR "Scripts")) {
    Join-Path $VENV_DIR "Scripts"
} else {
    Join-Path $VENV_DIR "bin"
}

$VENV_PY = Join-Path $VENV_BIN "python.exe"
if (!(Test-Path $VENV_PY)) { $VENV_PY = Join-Path $VENV_BIN "python" }
if (!(Test-Path $VENV_PY)) {
    Fail "no interpreter in $($opts.venv) -- the venv looks broken; retry with force=yes"
}

# --- toolchain ------------------------------------------------------------
Write-Host ">> pip install -r $REQUIREMENTS"
Invoke-Checked "pip install" { & $VENV_PY -m pip install -r (Join-Path $ROOT_DIR $REQUIREMENTS) }

Write-Host ""
Write-Host ">> installed:"
# Where-Object rather than Select-String: the latter inverse-videos the matched
# text, which is noise in a two-line version report.
& $VENV_PY -m pip show zensical 2>$null | Where-Object { $_ -match '^(Name|Version):' }

# Built from the layout that was actually probed above, and with Join-Path's
# separator, so the hint is copy-pasteable on Windows and on macOS/Linux alike --
# pwsh runs on all three.
$serveHint = Join-Path (Join-Path $opts.venv (Split-Path -Leaf $VENV_BIN)) "zensical"

Write-Host ""
Write-Host "${APP}: done -> $($opts.venv)"
Write-Host "${APP}: build both locales: scripts/build-docs.ps1"
Write-Host "${APP}: preview one:        $serveHint serve  (add -f mkdocs.ja.yml for Japanese)"
