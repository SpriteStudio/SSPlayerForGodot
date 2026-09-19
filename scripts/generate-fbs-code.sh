#!/bin/bash -e

HELP_APP="$(basename "$0")"
usage () {
    cat <<USAGE
Usage: ${HELP_APP} [--help]
  flatc -c over the SDK's .fbs schemas, into ss_player/format/. Needs flatc.
USAGE
}

for arg in "$@"; do
  case "$arg" in
    -h|--help|help) usage; exit 0 ;;
    *) echo "${HELP_APP}: unknown argument '${arg}'" >&2; usage >&2; exit 2 ;;
  esac
done

BASEDIR=$(dirname $0)
BASEDIR=$(cd $BASEDIR && pwd -P)
ROOTDIR=${BASEDIR}/..
ROOTDIR=$(cd $ROOTDIR && pwd -P)

pushd ${ROOTDIR}/ss_player > /dev/null
FLATC=flatc

/bin/mkdir -p format

for f in SpriteStudio-SDK/libs/ssruntime/fbs/*.fbs; do
    name=$(basename "$f" .fbs)
    ${FLATC} -c $f    
    /bin/mv "${name}_generated.h" ./format/${name}.h
done

for f in SpriteStudio-SDK/libs/ssab/fbs/*.fbs; do
    name=$(basename "$f" .fbs)
    ${FLATC} -c $f    
    /bin/mv "${name}_generated.h" ./format/${name}.h
done

popd > /dev/null
