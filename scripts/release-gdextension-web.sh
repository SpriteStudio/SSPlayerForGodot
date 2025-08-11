#!/usr/bin/env zsh -e

BASEDIR=$(dirname $0)
BASEDIR=$(cd $BASEDIR && pwd -P)
ROOTDIR=${BASEDIR}/..
ROOTDIR=$(cd $ROOTDIR && pwd -P)

pushd ${ROOTDIR} > /dev/null

targets=("template_release" "template_debug")
for target in ${targets[@]}; do
    scripts/build-extension.sh platform=web arch=wasm32 strip=yes target=${target}
    scripts/build-extension.sh platform=web arch=wasm32 strip=yes target=${target} threads=no
done

popd > /dev/null # ${ROOTDIR}
