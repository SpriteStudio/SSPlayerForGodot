#!/usr/bin/env zsh
set -e

BASEDIR=$(dirname $0)
BASEDIR=$(cd $BASEDIR && pwd -P)
ROOTDIR=${BASEDIR}/..
ROOTDIR=$(cd $ROOTDIR && pwd -P)

pushd ${ROOTDIR} > /dev/null
BINDIR=$(pwd)/bin/web
/bin/rm -rf ${BINDIR}
targets=("template_release" "template_debug")
for target in ${targets[@]}; do
    scripts/build-extension.sh platform=web arch=wasm32 compiledb=no strip=yes target=${target}
    scripts/build-extension.sh platform=web arch=wasm32 compiledb=no strip=yes target=${target} threads=no
done

popd > /dev/null # ${ROOTDIR}
