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
    scripts/build-extension.sh platform=macos arch=universal compiledb=no strip=yes target=${target} "$@"
done
/bin/rm -rf bin/macos/macos.framework

# --- Code signing (opt-in via env) ------------------------------------------
# Signs the GDExtension frameworks with a Developer ID Application identity when
# APPLE_SIGNING_IDENTITY is set (maps to the SS_APPLE_SIGNING_IDENTITY secret; the
# CI sets it in .github/workflows/extension.yml). Hardened runtime (--options
# runtime) + a secure timestamp. Not notarized. No-op (unsigned) when the identity
# is absent, so local builds without a certificate still succeed.
if [ -n "${APPLE_SIGNING_IDENTITY:-}" ]; then
    echo "==> Codesigning macOS frameworks as: ${APPLE_SIGNING_IDENTITY}"
    for fw in ${BINDIR}/*.framework(N); do
        codesign --force --sign "${APPLE_SIGNING_IDENTITY}" \
            --options runtime --timestamp "${fw}"
        codesign --verify --strict --verbose=2 "${fw}"
    done
else
    echo "==> Skipping macOS codesign (APPLE_SIGNING_IDENTITY not set)"
fi

popd > /dev/null # ${ROOTDIR}
