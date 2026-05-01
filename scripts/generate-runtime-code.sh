#!/bin/bash -e

BASEDIR=$(dirname $0)
BASEDIR=$(cd $BASEDIR && pwd -P)
ROOTDIR=${BASEDIR}/..
ROOTDIR=$(cd $ROOTDIR && pwd -P)

pushd ${ROOTDIR}/gd_spritestudio > /dev/null
FLATC=flatc

/bin/mkdir -p runtime
for f in SpriteStudio7-SDK/libs/ssruntime/fbs/*.fbs; do
    name=$(basename "$f" .fbs)
    ${FLATC} -c $f    
    /bin/mv "${name}_generated.h" ./runtime/${name}.h
done

for f in SpriteStudio7-SDK/libs/ssab/fbs/*.fbs; do
    name=$(basename "$f" .fbs)
    ${FLATC} -c $f    
    /bin/mv "${name}_generated.h" ./runtime/${name}.h
done

popd > /dev/null
