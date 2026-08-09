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
# APPLE_SIGNING_IDENTITY is set (maps to the APPLE_DEV_ID_APP_NAME secret; the
# CI sets it in .github/workflows/release.yml). Hardened runtime (--options
# runtime) + a secure timestamp. Notarization of the frameworks additionally
# needs an App Store Connect API key:
#   APPLE_API_KEY_PATH — path to the .p8 private key file
#   APPLE_API_KEY      — the key ID
#   APPLE_API_ISSUER   — the issuer ID
# These map to the ASC_API_* GitHub secrets — see .github/workflows/release.yml
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
        NOTARIZE_OUT="${BINDIR}/frameworks.notarize.json"
        # rm before the redirect below too: a zsh configured with `noclobber`
        # would otherwise refuse to overwrite a leftover response file.
        /bin/rm -f "${NOTARIZE_ZIP}" "${NOTARIZE_OUT}"
        ( cd "${BINDIR}" && /usr/bin/zip -qry frameworks.notarize.zip *.framework )

        # `notarytool submit --wait` is not reliably non-zero on a rejected
        # submission, so the exit code alone can let an unnotarized addon ship on
        # a green run. Assert on the reported status instead, and keep going past
        # a non-zero exit (|| NOTARIZE_RC=$?, which set -e tolerates) so the
        # status check below is what decides. plutil parses the JSON — it ships
        # with macOS, unlike jq, so this holds for a local release build too.
        #
        # All three frameworks (editor, template_release, template_debug) go up
        # as one archive, so a rejection is all-or-nothing and the status alone
        # does not say which one failed — fetch the notary log, which reports
        # per-binary issues, before failing the build.
        NOTARIZE_RC=0
        xcrun notarytool submit "${NOTARIZE_ZIP}" \
            --key "${APPLE_API_KEY_PATH}" \
            --key-id "${APPLE_API_KEY}" \
            --issuer "${APPLE_API_ISSUER}" \
            --wait --output-format json > "${NOTARIZE_OUT}" || NOTARIZE_RC=$?
        cat "${NOTARIZE_OUT}"

        # An unparseable response leaves the status empty, which fails below —
        # the safe direction, since the raw response is echoed above.
        NOTARIZE_STATUS=$(/usr/bin/plutil -extract status raw -o - -- "${NOTARIZE_OUT}" 2>/dev/null || true)
        NOTARIZE_ID=$(/usr/bin/plutil -extract id raw -o - -- "${NOTARIZE_OUT}" 2>/dev/null || true)
        if [ "${NOTARIZE_STATUS}" != "Accepted" ]; then
            echo "ERROR: notarization failed (status=${NOTARIZE_STATUS:-unknown}, notarytool exit=${NOTARIZE_RC})" >&2
            if [ -n "${NOTARIZE_ID}" ]; then
                echo "==> Notary log for submission ${NOTARIZE_ID}:" >&2
                xcrun notarytool log "${NOTARIZE_ID}" \
                    --key "${APPLE_API_KEY_PATH}" \
                    --key-id "${APPLE_API_KEY}" \
                    --issuer "${APPLE_API_ISSUER}" >&2 || true
            fi
            exit 1
        fi
        echo "==> Notarization accepted (submission ${NOTARIZE_ID})"

        /bin/rm -f "${NOTARIZE_ZIP}" "${NOTARIZE_OUT}"
    else
        echo "==> Skipping notarization (APPLE_API_KEY_PATH / APPLE_API_KEY / APPLE_API_ISSUER not all set)"
    fi
else
    echo "==> Skipping macOS codesign (APPLE_SIGNING_IDENTITY not set)"
fi

popd > /dev/null # ${ROOTDIR}
