# Base directories
$SDK_DIR = Resolve-Path (Join-Path $PSScriptRoot "../ss_player/SpriteStudio-SDK")
$SDK_TESTS_DIR = Join-Path $SDK_DIR "tests"
$SDK_CLI_DIR = Join-Path $SDK_DIR "cli"
$EXAMPLES_DIR = $PSScriptRoot

# Build ssconverter-cli
Write-Host "Building ssconverter-cli (debug)..."
Push-Location $SDK_CLI_DIR
cargo build
$CONVERTER = Join-Path $SDK_DIR "target/debug/ssconverter-cli.exe"
Pop-Location

# List of tests to update SSABs
$TESTS = @("overall", "Ringo")

foreach ($TEST in $TESTS) {
    $SSPJ_PATH = Join-Path $SDK_TESTS_DIR "$TEST/$TEST.sspj"

    # Standard project
    $OUTPUT_DIR = Join-Path $EXAMPLES_DIR "$TEST/ssab_generated/$TEST"
    Write-Host "Updating SSAB for $TEST in $OUTPUT_DIR..."
    if (!(Test-Path $OUTPUT_DIR)) { New-Item -ItemType Directory -Path $OUTPUT_DIR }
    & $CONVERTER "$SSPJ_PATH" -o "$OUTPUT_DIR"

    # overall project special handling for overall_gdextension
    if ($TEST -eq "overall") {
        $GD_OUTPUT_DIR = Join-Path $EXAMPLES_DIR "overall_gdextension/ssab_generated/overall"
        Write-Host "Updating SSAB for $TEST in $GD_OUTPUT_DIR..."
        if (!(Test-Path $GD_OUTPUT_DIR)) { New-Item -ItemType Directory -Path $GD_OUTPUT_DIR }
        & $CONVERTER "$SSPJ_PATH" -o "$GD_OUTPUT_DIR"
    }

    # Ringo is also converted into the Override_Ringo and Scripting demo projects
    if ($TEST -eq "Ringo") {
        $OR_OUTPUT_DIR = Join-Path $EXAMPLES_DIR "Override_Ringo/ssab_generated/Ringo"
        Write-Host "Updating SSAB for $TEST in $OR_OUTPUT_DIR..."
        if (!(Test-Path $OR_OUTPUT_DIR)) { New-Item -ItemType Directory -Path $OR_OUTPUT_DIR }
        & $CONVERTER "$SSPJ_PATH" -o "$OR_OUTPUT_DIR"

        $SCR_OUTPUT_DIR = Join-Path $EXAMPLES_DIR "Scripting/ssab_generated/Ringo"
        Write-Host "Updating SSAB for $TEST in $SCR_OUTPUT_DIR..."
        if (!(Test-Path $SCR_OUTPUT_DIR)) { New-Item -ItemType Directory -Path $SCR_OUTPUT_DIR }
        & $CONVERTER "$SSPJ_PATH" -o "$SCR_OUTPUT_DIR"
    }
}
Write-Host "Done!"
