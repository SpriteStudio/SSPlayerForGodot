#!/usr/bin/env zsh
set -e

BASEDIR=$(dirname $0)
BASEDIR=$(cd $BASEDIR && pwd -P)
ROOTDIR=${BASEDIR}/..
ROOTDIR=$(cd $ROOTDIR && pwd -P)

pushd ${ROOTDIR} > /dev/null

targets=("editor" "template_release" "template_debug")
for target in ${targets[@]}; do
    if [[ "${target}" == "template_debug" ]]; then
        rtmode="debug"
    else
        rtmode="release"
    fi
    # build.sh does not provision libssruntime, and device/simulator need
    # different slices (arm64 vs arm64+x86_64), so place the matching runtime
    # before each variant. Custom-module object files are namespaced by Godot
    # (…arm64[.simulator].o), so no object cleanup is needed between variants.
    scripts/build-runtime.sh platform=ios build=${rtmode} ios_simulator=no
    scripts/build.sh platform=ios arch=arm64 compiledb=no strip=yes target=${target} ios_simulator=no
    scripts/build-runtime.sh platform=ios build=${rtmode} ios_simulator=yes
    scripts/build.sh platform=ios arch=universal compiledb=no strip=yes target=${target} ios_simulator=yes
done

popd > /dev/null # ${ROOTDIR}
