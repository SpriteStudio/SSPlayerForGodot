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

# --- Code signing & notarization (opt-in via env) ----------------------------
# Signs the GDExtension frameworks with a Developer ID Application identity when
# APPLE_SIGNING_IDENTITY is set (maps to the SS_APPLE_SIGNING_IDENTITY secret; the
# CI sets it in .github/workflows/release.yml). Hardened runtime (--options
# runtime) + a secure timestamp. Notarization of the frameworks additionally
# needs an App Store Connect API key:
#   APPLE_API_KEY_PATH — path to the .p8 private key file
#   APPLE_API_KEY      — the key ID
#   APPLE_API_ISSUER   — the issuer ID
# These map to the SS_APPLE_* GitHub secrets — see .github/workflows/release.yml
# for the CI wiring. No-op (unsigned) when the identity is absent, so local
# builds without a certificate still succeed.
if [ -n "${APPLE_SIGNING_IDENTITY:-}" ]; then
    echo "==> Codesigning macOS frameworks as: ${APPLE_SIGNING_IDENTITY}"
    for fw in ${BINDIR}/*.framework(N); do
        codesign --force --sign "${APPLE_SIGNING_IDENTITY}" \
            --options runtime --timestamp "${fw}"
        codesign --verify --strict --verbose=2 "${fw}"
    done

    # Notarize the signed frameworks. A framework cannot be stapled (stapling
    # targets .app/.dmg/.pkg only), so we notarize a zip of the signed
    # frameworks and rely on Gatekeeper's online ticket check when the Godot
    # editor first loads the library.
    if [ -n "${APPLE_API_KEY_PATH:-}" ] && [ -n "${APPLE_API_KEY:-}" ] && [ -n "${APPLE_API_ISSUER:-}" ]; then
        echo "==> Notarizing macOS frameworks"
        NOTARIZE_ZIP="${BINDIR}/frameworks.notarize.zip"
        /bin/rm -f "${NOTARIZE_ZIP}"
        ( cd "${BINDIR}" && /usr/bin/zip -qry frameworks.notarize.zip *.framework )
        xcrun notarytool submit "${NOTARIZE_ZIP}" \
            --key "${APPLE_API_KEY_PATH}" \
            --key-id "${APPLE_API_KEY}" \
            --issuer "${APPLE_API_ISSUER}" \
            --wait
        /bin/rm -f "${NOTARIZE_ZIP}"
    else
        echo "==> Skipping notarization (APPLE_API_KEY_PATH / APPLE_API_KEY / APPLE_API_ISSUER not all set)"
    fi
else
    echo "==> Skipping macOS codesign (APPLE_SIGNING_IDENTITY not set)"
fi

popd > /dev/null # ${ROOTDIR}
