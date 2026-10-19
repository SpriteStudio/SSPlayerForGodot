#!/usr/bin/env pwsh
#
# Build the documentation site -- one run per locale, in the order that works.
# PowerShell twin of build-docs.sh.
#
#   scripts/build-docs.ps1             # English then Japanese, both strict
#   scripts/build-docs.ps1 locale=ja   # Japanese only, into site/ja
#
# Two things this script exists to get right:
#
#   1. Order. The Japanese site is written *inside* the English one (site/ja),
#      and a build clears its own output directory first -- so `zensical build`
#      deletes site/ja on its way in. English first, Japanese second; the other
#      order publishes a site with no Japanese pages at all.
#   2. --strict, always. mkdocs.base.yml sets strict: true as well; the flag is
#      spelled out here so the build stays strict if the config ever loses it.
#
# The -docs suffix keeps this apart from the scripts that build the code itself;
# the whole SpriteStudio family spells the documentation pair the same way.
#
# `zensical serve` is deliberately not wrapped: it validates nothing (--strict is
# accepted and does nothing), it serves one locale at a time, and it is a
# foreground process with no arguments worth defaulting.
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
    locale = "all"
    venv   = ".venv"
}

function Show-Usage {
    Write-Host "Usage: $APP [options]"
    Write-Host "$APP options:"
    Write-Host "  locale=<all|en|ja>      Which locale to build (default: all)"
    Write-Host "                          all = English then Japanese, the only safe order"
    Write-Host "  venv=<dir>              Virtualenv to run zensical from, relative to the repo"
    Write-Host "                          root (default: .venv). Falls back to zensical on PATH,"
    Write-Host "                          where a plain pip install -r leaves it."
    Write-Host "  -h | --help             Show this help"
    Write-Host ""
    Write-Host "Output: site/ (English) and site/ja/ (Japanese). Both builds are --strict, so a"
    Write-Host "broken internal link fails the build."
    Write-Host ""
    Write-Host "locale=en on its own DELETES site/ja: the English build clears site/, and the"
    Write-Host "Japanese site lives inside it. Use locale=all before serving or publishing."
    Write-Host ""
    Write-Host "Set the venv up first with scripts/prepare-docs.ps1."
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
        # Only the locale is an enum; venv= is a path.
        $opts[$key] = if ($key -eq "locale") { $value.ToLower() } else { $value }
    } else {
        Fail "unknown argument '$item' (options are key=value; see $APP --help)" 2
    }
}

switch ($opts.locale) {
    { $_ -in "all", "both" }    { $opts.locale = "all" }
    { $_ -in "en", "english" }  { $opts.locale = "en" }
    { $_ -in "ja", "japanese" } { $opts.locale = "ja" }
    default { Fail "locale must be all, en or ja (got '$($opts.locale)')" 2 }
}

Write-Host "options"
Write-Host "  locale => $($opts.locale)"
Write-Host "  venv   => $($opts.venv)"
Write-Host ""

# --- preflight ------------------------------------------------------------
# Same probe as prepare-docs.ps1: Scripts/ on Windows, bin/ everywhere else.
$VENV_DIR = Join-Path $ROOT_DIR $opts.venv
$VENV_BIN = if (Test-Path (Join-Path $VENV_DIR "Scripts")) {
    Join-Path $VENV_DIR "Scripts"
} else {
    Join-Path $VENV_DIR "bin"
}

$ZENSICAL = $null
foreach ($candidate in @((Join-Path $VENV_BIN "zensical.exe"), (Join-Path $VENV_BIN "zensical"))) {
    if (Test-Path $candidate) {
        $ZENSICAL = $candidate
        break
    }
}

# A plain `pip install -r docs/requirements.txt` leaves zensical on PATH with no
# venv. Fall back to it rather than demanding a venv.
if (-not $ZENSICAL) {
    $onPath = Get-Command zensical -ErrorAction SilentlyContinue
    if ($onPath) {
        $ZENSICAL = $onPath.Source
        Write-Host "${APP}: no venv at $($opts.venv); using zensical from PATH"
    }
}

if (-not $ZENSICAL) {
    [Console]::Error.WriteLine("${APP}: zensical not found in $($opts.venv) or on PATH")
    Fail "run 'scripts/prepare-docs.ps1' first"
}

function Invoke-Checked {
    param([string] $What, [scriptblock] $Body)
    & $Body
    if ($LASTEXITCODE -ne 0) { Fail "$What failed ($LASTEXITCODE)" $LASTEXITCODE }
}

# Every path in the configs is relative to the repo root, so build from there
# however the script was invoked.
Push-Location $ROOT_DIR
try {
    # --- build ------------------------------------------------------------
    if ($opts.locale -in "all", "en") {
        Write-Host ">> zensical build --strict (en) -> site/"
        Invoke-Checked "English build" { & $ZENSICAL build --strict }
    }

    if ($opts.locale -in "all", "ja") {
        Write-Host ">> zensical build -f mkdocs.ja.yml --strict (ja) -> site/ja/"
        Invoke-Checked "Japanese build" { & $ZENSICAL build -f mkdocs.ja.yml --strict }
    }
} finally {
    Pop-Location
}

Write-Host ""
switch ($opts.locale) {
    "all" {
        Write-Host "${APP}: done -> site/ (en) + site/ja/ (ja)"
        # Loopback: on every interface http.server announces http://[::]:8000/,
        # which is not a secure context, so Copy as Markdown cannot copy there.
        Write-Host "${APP}: serve both, so the language selector resolves:"
        Write-Host "${APP}:   py -m http.server 8000 --bind 127.0.0.1 -d site"
    }
    "en" {
        Write-Host "${APP}: done -> site/ (en)"
        [Console]::Error.WriteLine("${APP}: WARNING: this build cleared site/, so site/ja no longer exists.")
        [Console]::Error.WriteLine("${APP}:          run '$APP locale=ja' before serving or publishing.")
    }
    "ja" {
        Write-Host "${APP}: done -> site/ja/ (ja)"
        Write-Host "${APP}: site/ still holds whatever the last English build left there."
    }
}
