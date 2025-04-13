#/bin/zsh -e

BASEDIR=$(dirname $0)
BASEDIR=$(cd $BASEDIR && pwd -P)
ROOTDIR=${BASEDIR}/..
ROOTDIR=$(cd $ROOTDIR && pwd -P)

pushd ${ROOTDIR} > /dev/null

targets=("template_release" "template_debug")
for target in ${targets[@]}; do
    scripts/build-extension.sh platform=ios arch=universal strip=yes target=${target} ios_simulator=no
    scripts/build-extension.sh platform=ios arch=universal strip=yes target=${target} ios_simulator=yes
done

popd > /dev/null # ${ROOTDIR}
