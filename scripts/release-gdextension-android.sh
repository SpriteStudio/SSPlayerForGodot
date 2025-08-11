#!/usr/bin/env zsh
set -e

BASEDIR=$(dirname $0)
BASEDIR=$(cd $BASEDIR && pwd -P)
ROOTDIR=${BASEDIR}/..
ROOTDIR=$(cd $ROOTDIR && pwd -P)

pushd ${ROOTDIR} > /dev/null

targets=("template_release" "template_debug")
for target in ${targets[@]}; do
    scripts/build-extension.sh platform=android arch=arm32 strip=yes target=${target}
    scripts/build-extension.sh platform=android arch=arm64 strip=yes target=${target}
    scripts/build-extension.sh platform=android arch=x86_64 strip=yes target=${target}
done

popd > /dev/null # ${ROOTDIR}
