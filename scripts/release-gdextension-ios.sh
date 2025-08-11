#!/usr/bin/env zsh -e

BASEDIR=$(dirname $0)
BASEDIR=$(cd $BASEDIR && pwd -P)
ROOTDIR=${BASEDIR}/..
ROOTDIR=$(cd $ROOTDIR && pwd -P)

pushd ${ROOTDIR} > /dev/null
BINDIR=$(pwd)/bin/ios
targets=("template_release" "template_debug")
for target in ${targets[@]}; do
    scripts/build-extension.sh platform=ios arch=universal strip=yes target=${target} ios_simulator=yes
    scripts/build-extension.sh platform=ios arch=universal strip=yes target=${target} ios_simulator=no
done

# create xcframeworks
pushd ${BINDIR}
for target in ${targets[@]}; do
    rm -rf tmp
    mkdir -p tmp/ios tmp/ios-simulator
    mv $BINDIR/ios.framework/libSSGodot.ios.${target} tmp/ios/libSSGodot.ios.${target}.dylib
    mv $BINDIR/ios.framework/libSSGodot.ios.${target}.simulator tmp/ios-simulator/libSSGodot.ios.${target}.dylib
    xcodebuild -create-xcframework -library tmp/ios/libSSGodot.ios.${target}.dylib -library tmp/ios-simulator/libSSGodot.ios.${target}.dylib -output libSSGodot.ios.${target}.xcframework
done
rm -rf tmp
rm -rf ios.framework
popd > /dev/null # ${BINDIR}

popd > /dev/null # ${ROOTDIR}
