#!/usr/bin/env zsh
set -e

BASEDIR=$(dirname $0)
BASEDIR=$(cd $BASEDIR && pwd -P)
ROOTDIR=${BASEDIR}/..
ROOTDIR=$(cd $ROOTDIR && pwd -P)

pushd ${ROOTDIR} > /dev/null

targets=("editor" "template_release" "template_debug")
for target in ${targets[@]}; do
    scripts/build.sh platform=ios arch=arm64 compiledb=no strip=yes target=${target} ios_simulator=no
    scripts/build.sh platform=ios arch=universal compiledb=no strip=yes target=${target} ios_simulator=yes
done

popd > /dev/null # ${ROOTDIR}
