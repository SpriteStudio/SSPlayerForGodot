#!/usr/bin/env pwsh
#
# Convert the SpriteStudio-SDK bundled test projects (tests/overall, tests/Ringo)
# into the .ssab assets the sample projects under examples/ load.
#
# Each sample project also carries a .ssplayer_sources.cfg pointing at the same
# .sspj, so opening it in the Godot Editor regenerates the same output through the
# import dock. This script is the headless equivalent -- ssab_generated/ is not
# tracked in git, so use it to populate the samples without launching the editor.
#
# The dev_* sample projects are intentionally excluded: their sources config is
# gitignored and set up by hand in the editor.
#
# Requires the SpriteStudio-SDK submodule and a Rust toolchain (ssconverter-cli is
# built from source; the prebuilt SDK packages only ship libssconverter).
$ErrorActionPreference = "Stop"

$baseDirectory = Split-Path -Parent $PSCommandPath
$rootDirectory = Split-Path -Parent $baseDirectory

$SDK_DIR = Join-Path $rootDirectory "ss_player/SpriteStudio-SDK"
$SDK_TESTS_DIR = Join-Path $SDK_DIR "tests"
$SDK_CLI_DIR = Join-Path $SDK_DIR "cli"
$EXAMPLES_DIR = Join-Path $rootDirectory "examples"
$APP = Split-Path -Leaf $PSCommandPath

if (!(Test-Path (Join-Path $SDK_CLI_DIR "Cargo.toml"))) {
    Write-Error "${APP}: SpriteStudio-SDK submodule is not initialized ($SDK_DIR)`n${APP}: run 'git submodule update --init --recursive' first"
}

if (!(Get-Command cargo -ErrorAction SilentlyContinue)) {
    Write-Error "${APP}: cargo not found (a Rust toolchain is required to build ssconverter-cli)"
}

# Build ssconverter-cli
Write-Host "Building ssconverter-cli (debug)..."
pushd $SDK_CLI_DIR
& cargo build
if ($LASTEXITCODE -ne 0) {
    popd
    Write-Error "${APP}: cargo build failed ($LASTEXITCODE)"
}
popd
$CONVERTER = Join-Path $SDK_DIR "target/debug/ssconverter-cli.exe"

# "<SDK tests subdir>|<examples subdir>" -- one entry per destination sample
# project. Output goes to <examples subdir>/ssab_generated/<SDK tests subdir>/,
# matching the layout the editor's source sync produces.
$DEPLOYMENTS = @(
    "overall|overall"
    "overall|overall_gdextension"
    "Ringo|Ringo"
    "Ringo|Override_Ringo"
    "Ringo|Scripting"
)

foreach ($ENTRY in $DEPLOYMENTS) {
    $TEST, $DEST = $ENTRY -split "\|"
    $SSPJ_PATH = Join-Path $SDK_TESTS_DIR "$TEST/$TEST.sspj"
    $OUTPUT_DIR = Join-Path $EXAMPLES_DIR "$DEST/ssab_generated/$TEST"

    if (!(Test-Path $SSPJ_PATH)) {
        Write-Error "${APP}: $SSPJ_PATH not found"
    }

    Write-Host "Updating SSAB for $TEST in $OUTPUT_DIR..."
    mkdir -Force $OUTPUT_DIR > $null
    & $CONVERTER "$SSPJ_PATH" -o "$OUTPUT_DIR"
    if ($LASTEXITCODE -ne 0) {
        Write-Error "${APP}: ssconverter-cli failed for $TEST ($LASTEXITCODE)"
    }
}

# The headless test project is not a sample and so is not under examples\, but
# it reads the same fixtures -- one entry per pack run_tests.gd's preflight
# names. Keeping the .ssab out of git and regenerating it here is why the suite
# cannot quietly test a stale conversion.
$TEST_PACKS = @(
    "overall"
    "Ringo"
)

foreach ($TEST in $TEST_PACKS) {
    $SSPJ_PATH = Join-Path $SDK_TESTS_DIR "$TEST/$TEST.sspj"
    $OUTPUT_DIR = Join-Path $rootDirectory "test_gdextension/ssab_generated/$TEST"

    if (!(Test-Path $SSPJ_PATH)) {
        Write-Error "${APP}: $SSPJ_PATH not found"
    }

    Write-Host "Updating SSAB for $TEST in $OUTPUT_DIR..."
    mkdir -Force $OUTPUT_DIR > $null
    & $CONVERTER "$SSPJ_PATH" -o "$OUTPUT_DIR"
    if ($LASTEXITCODE -ne 0) {
        Write-Error "${APP}: ssconverter-cli failed for $TEST ($LASTEXITCODE)"
    }
}

Write-Host "Done!"
