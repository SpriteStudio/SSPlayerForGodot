#!/usr/bin/env zsh -e

BASEDIR=$(dirname $0)
BASEDIR=$(cd $BASEDIR && pwd -P)
ROOTDIR=${BASEDIR}/..
ROOTDIR=$(cd $ROOTDIR && pwd -P)

pushd ${ROOTDIR} > /dev/null

targets=("editor" "template_release" "template_debug")
for target in ${targets[@]}; do
    scripts/build-extension.sh platform=linux strip=yes target=${target}
done

popd > /dev/null # ${ROOTDIR}
