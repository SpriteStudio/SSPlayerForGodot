$baseDirectory = Split-Path -Parent $PSCommandPath
$rootDirectory = Split-Path -Parent $baseDirectory
$arch = (Get-Item Env:PROCESSOR_ARCHITECTURE).Value

pushd $rootDirectory

$targets= "editor", "template_release", "template_debug"
foreach($target in $targets) {
    ./scripts/winbuild-gdextension.ps1 platform=windows compiledb=no strip=yes target=${target}
}

popd