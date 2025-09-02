#!/usr/bin/env zsh
set -e
set -x

BASEDIR=$(dirname $0)
BASEDIR=$(cd $BASEDIR && pwd -P)
ROOTDIR=${BASEDIR}/..
ROOTDIR=$(cd $ROOTDIR && pwd -P)

pushd ${ROOTDIR} > /dev/null
BINDIR=$(pwd)/bin/ios
/bin/rm -rf ${BINDIR}
targets=("template_release" "template_debug")
for target in ${targets[@]}; do
#   scripts/build-extension.sh platform=ios arch=universal compiledb=no strip=yes target=${target} ios_simulator=yes
    scripts/build-extension.sh platform=ios arch=universal compiledb=no strip=yes target=${target} ios_simulator=no
done

pushd ${BINDIR}

## create xcframeworks
#for target in ${targets[@]}; do
#    rm -rf tmp
#    mkdir -p tmp/ios tmp/ios-simulator
#    mv $BINDIR/libSSGodot.ios.${target}.framework tmp/ios/libSSGodot.ios.${target}.framework
#    mv $BINDIR/libSSGodot.ios.${target}.simulator.framework tmp/ios-simulator/libSSGodot.ios.${target}.framework
#    xcodebuild -create-xcframework -framework tmp/ios/libSSGodot.ios.${target}.framework -framework tmp/ios-simulator/libSSGodot.ios.${target}.framework -output libSSGodot.ios.${target}.xcframework
#done
#/bin/rm -rf tmp

/bin/rm -rf ios.framework
popd > /dev/null # ${BINDIR}

popd > /dev/null # ${ROOTDIR}
