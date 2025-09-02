#!/usr/bin/env zsh
set -e

BASEDIR=$(dirname $0)
BASEDIR=$(cd $BASEDIR && pwd -P)
ROOTDIR=${BASEDIR}/..
ROOTDIR=$(cd $ROOTDIR && pwd -P)

pushd ${ROOTDIR} > /dev/null
BINDIR=$(pwd)/bin/macos
/bin/rm -rf ${BINDIR}
targets=("editor" "template_release" "template_debug")
for target in ${targets[@]}; do
    scripts/build-extension.sh platform=macos arch=universal compiledb=no strip=yes target=${target}
done
/bin/rm -rf bin/macos/macos.framework

popd > /dev/null # ${ROOTDIR}
