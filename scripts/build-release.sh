#!/usr/bin/env bash
#
# Assemble the release: take the per-platform binaries the build matrix
# produced, lay them out as the `addons/spritestudio/` folder a Godot project
# drops in, and zip it.
#
#   scripts/build-release.sh                       # assemble from artifacts/
#   scripts/build-release.sh api_version=4.7       # a different Godot API
#   scripts/build-release.sh out=/tmp/rel          # somewhere else
#
# release.yml's package job runs this script for the asset it uploads, so CI
# and a local run cannot drift. The workflow keeps only what is GitHub's -- the
# checkout, the artifact download and the Release itself.
#
# This is the phase AFTER it has been decided what is being released. Which
# commit, which tag and which build's binaries are all settled before this
# script runs -- by the tag that was pushed, the ref the workflow was dispatched
# from, and the download that populated in=. So there is no option here that
# re-decides any of them:
#
#   git checkout v1.2.3                       # which release
#   gh run download <id> -D artifacts         # which build's binaries
#   scripts/build-release.sh                  # this
#
# What this does NOT do is build anything. Six platforms need Linux, Windows and
# macOS between them, so there is no local equivalent of the matrix. What it
# replaces is the part that never needed one: the `cp` and `mv` that decide what
# the addon folder contains.
#
# The check at the end is the one that matters here, and it is not a formality.
# spritestudio.gdextension names a file per platform and build target -- twenty
# odd paths -- and Godot resolves them at load time. A name that does not match
# what actually shipped does not error at build time or at zip time; the
# extension simply fails to load, on that one platform, for whoever downloaded
# it. So every path the descriptor names is looked up inside the finished
# archive, along with every icon it points at.
set -euo pipefail

APP="$(basename "$0")"
ROOT_DIR="$(cd "$(dirname "$0")/.." && pwd)"

# The platforms the matrix builds, and the order they are reported in.
PLATFORMS="linux windows macos android ios web"

# --- options --------------------------------------------------------------

IN="artifacts"
OUT="release-dist"
API_VERSION="4.7"
VERIFY="yes"
CLEAN="yes"

usage () {
  echo "Usage: $APP [options]"
  echo "$APP options:"
  echo "  in=<dir>                Where the per-platform artifact trees are (default:"
  echo "                          $IN). One subdirectory per upload-artifact name, which"
  echo "                          is exactly what 'gh run download -D <dir>' produces."
  echo "  out=<dir>               Where the zip and SHA256SUMS land (default: $OUT)."
  echo "                          Relative paths resolve against the current directory."
  echo "  api_version=<ver>       Godot API version the binaries were built for"
  echo "                          (default: $API_VERSION). Names the artifacts this reads"
  echo "                          and the zip it writes; same spelling as"
  echo "                          build-extension.sh."
  echo "  verify=<yes|no>         Re-open the finished zip and check it (default:"
  echo "                          $VERIFY)."
  echo "  clean=<yes|no>          Delete <out> before assembling (default: $CLEAN)."
  echo "  -h | --help             Show this help"
  echo ""
  echo "Output: <out>/ssplayer-godot-extension-<api_version>.zip, holding addons/ at the"
  echo "archive root so a user unzips it straight into a project, plus SHA256SUMS."
  echo ""
  echo "Every platform the matrix builds must be present: a partial release is not a"
  echo "thing worth producing, so a missing one is an error that lists everything else"
  echo "missing alongside it rather than failing on the first."
}

while [ $# -gt 0 ]; do
  item="$1"
  shift

  case "$item" in
    -h|--help|help)
      usage
      exit 0
      ;;
    *=*)
      key="${item%%=*}"
      value="${item#*=}"
      case "$key" in
        in)          IN="$value" ;;
        out)         OUT="$value" ;;
        api_version) API_VERSION="$value" ;;
        verify)      VERIFY="$(printf '%s' "$value" | tr '[:upper:]' '[:lower:]')" ;;
        clean)       CLEAN="$(printf '%s' "$value" | tr '[:upper:]' '[:lower:]')" ;;
        *)
          echo "$APP: unknown option '$key' (see $APP --help)" >&2
          exit 2
          ;;
      esac
      ;;
    *)
      echo "$APP: unknown argument '$item' (options are key=value; see $APP --help)" >&2
      exit 2
      ;;
  esac
done

case "$VERIFY" in yes|no) ;; *) echo "$APP: verify must be yes or no (got '$VERIFY')" >&2; exit 2 ;; esac
case "$CLEAN" in yes|no) ;; *) echo "$APP: clean must be yes or no (got '$CLEAN')" >&2; exit 2 ;; esac
case "$API_VERSION" in
  [0-9]*) ;;
  *) echo "$APP: api_version must look like a Godot version, e.g. 4.7 (got '$API_VERSION')" >&2; exit 2 ;;
esac

# Resolve both paths against the invoking directory, before anything cds away.
case "$IN"  in /*) ;; *) IN="$PWD/$IN" ;; esac
case "$OUT" in /*) ;; *) OUT="$PWD/$OUT" ;; esac

# clean=yes deletes out=, so refuse the paths that would take the repository (or
# the filesystem) with them.
if [ "$OUT" = "/" ] || [ "$OUT" = "$ROOT_DIR" ]; then
  echo "$APP: refusing to use '$OUT' as out= (it is deleted when clean=yes)" >&2
  exit 2
fi

cd "$ROOT_DIR"

# --- what this release is called ------------------------------------------
#
# A release is a tag: one is pushed, and the workflow is then dispatched from
# it. Reading the ref here is reading that decision, not making it. Absent, this
# is a build off a branch -- which is the default use of this workflow (QA on a
# release/X.Y branch), and no Release is created from one.
if [ "${GITHUB_REF_TYPE:-}" = "tag" ] && [ -n "${GITHUB_REF_NAME:-}" ]; then
  TAG="$GITHUB_REF_NAME"
  TAG_SOURCE="GITHUB_REF_NAME -- the ref this release was dispatched from"
elif TAG="$(git -C "$ROOT_DIR" describe --exact-match --match 'v[0-9]*' --tags HEAD 2>/dev/null)"; then
  TAG_SOURCE="the release tag HEAD is checked out at"
else
  TAG=""
  TAG_SOURCE="HEAD is not on a release tag, so this is a build, not a release"
fi

ZIP="ssplayer-godot-extension-${API_VERSION}.zip"

echo "options"
echo "  in          => $IN"
echo "  out         => $OUT"
echo "  api_version => $API_VERSION"
echo "  verify      => $VERIFY"
echo "  clean       => $CLEAN"
echo "  tag         => ${TAG:-(none)}"
echo "                 ($TAG_SOURCE)"

# --- preflight ------------------------------------------------------------

GDEXTENSION="$ROOT_DIR/misc/spritestudio.gdextension"
if [ ! -f "$GDEXTENSION" ]; then
  echo "$APP: misc/spritestudio.gdextension not found -- is this the SSPlayerForGodot repo?" >&2
  exit 1
fi

for tool in zip unzip; do
  command -v "$tool" > /dev/null 2>&1 || {
    echo "$APP: $tool not found -- it is what packages the archive" >&2
    exit 1
  }
done

# The runners have sha256sum (coreutils); macOS ships shasum instead. Both write
# the same `<hash>  <name>` lines and both take -c, so SHA256SUMS is identical
# either way -- which is the only reason a release can be assembled from either.
if command -v sha256sum > /dev/null 2>&1; then
  sha256 () { sha256sum "$@"; }
elif command -v shasum > /dev/null 2>&1; then
  sha256 () { shasum -a 256 "$@"; }
else
  echo "$APP: neither sha256sum nor shasum found -- one of them writes SHA256SUMS" >&2
  exit 1
fi

step () {
  echo ""
  echo ">> $*"
}

FAILED=""
fail () {
  FAILED="${FAILED}
  $1"
}

# --- check the input ------------------------------------------------------
# Everything at once. Finding out about the fifth missing platform on the fifth
# run of a script that takes a matrix build to feed is not a debugging loop
# worth having.
step "checking $IN"

if [ ! -d "$IN" ]; then
  echo "$APP: $IN does not exist" >&2
  echo "$APP: populate it first -- 'gh run download <run-id> -D $(basename "$IN")' -- or point in= at the artifacts" >&2
  exit 1
fi

# Each platform job uploads bin/ as extension-<api>-<platform>, so the tree
# holds that platform's binaries under its own name plus the staged licenses/.
# The linux one is where the shared runtime notices are taken from: they are
# identical across platforms, and picking one keeps the copy unambiguous.
CANONICAL="extension-${API_VERSION}-linux"

for p in $PLATFORMS; do
  dir="$IN/extension-${API_VERSION}-${p}"
  if [ ! -d "$dir" ]; then
    fail "extension-${API_VERSION}-${p}/ is missing"
  elif [ ! -d "$dir/$p" ]; then
    fail "extension-${API_VERSION}-${p}/ has no ${p}/ (the binaries the matrix builds)"
  fi
done

for f in THIRD-PARTY-LICENSES.ssruntime.md THIRD-PARTY-LICENSES.ssconverter.md runtime-LICENSE.md; do
  [ -f "$IN/$CANONICAL/licenses/$f" ] || fail "$CANONICAL/licenses/$f is missing"
done

if [ -n "$FAILED" ]; then
  echo "$APP: $IN is missing pieces the release needs:$FAILED" >&2
  echo "" >&2
  echo "$APP: point in= at a complete matrix build for Godot $API_VERSION." >&2
  exit 1
fi
echo "   every platform the matrix builds is present"

# --- assemble -------------------------------------------------------------
# The addon a user drops into their project: `addons/spritestudio/` with the
# descriptor at its root, the binaries under bin/<platform>/, the editor icons
# the [icons] section points at, and every licence the shipped binaries carry.
WORK="$ROOT_DIR/build/release-staging"
ADDON="$WORK/addons/spritestudio"

step "assembling the addon in $WORK"

rm -rf "$WORK"
mkdir -p "$ADDON/bin" "$ADDON/icons" "$ADDON/licenses"

cp "$GDEXTENSION" "$ADDON/"
cp "$ROOT_DIR/LICENSE.md" "$ROOT_DIR/LICENSE.ja.md" "$ADDON/"
echo "   spritestudio.gdextension + LICENSE.md + LICENSE.ja.md"

# Editor icons referenced by the [icons] section. They are source files, not
# build outputs, so they never reach the platform artifacts -- copy them
# straight from the tree (same set as scripts/build-extension.sh).
cp "$ROOT_DIR/ss_player/icons"/icon_*.svg "$ADDON/icons/"
echo "   icons/ ($(ls -1 "$ADDON/icons" | wc -l | tr -d ' ') files)"

# Third-party notices next to the statically-linked native binaries:
# FlatBuffers (Apache-2.0), godot-cpp (MIT), and the Rust runtime crates. The
# runtime crate licences were staged into each platform artifact and are
# identical across them, so they come from the canonical one.
cp "$ROOT_DIR/LICENSE.md" "$ROOT_DIR/LICENSE.ja.md" "$ADDON/licenses/"
cp "$ROOT_DIR/THIRD_PARTY_NOTICES.md" "$ADDON/licenses/"
cp "$ROOT_DIR/licenses/Apache-2.0.txt" "$ADDON/licenses/"
cp "$IN/$CANONICAL/licenses"/* "$ADDON/licenses/"
echo "   licenses/ ($(ls -1 "$ADDON/licenses" | wc -l | tr -d ' ') files)"

# Copied, not moved: unlike the workflow this replaces, the input tree survives,
# so one download serves any number of attempts.
for p in $PLATFORMS; do
  cp -R "$IN/extension-${API_VERSION}-${p}/${p}" "$ADDON/bin/"
done
echo "   bin/ ($PLATFORMS)"

# --- archive --------------------------------------------------------------
step "archiving into $OUT"

if [ "$CLEAN" = "yes" ]; then
  rm -rf "$OUT"
fi
mkdir -p "$OUT"

# addons/ at the archive root, so a user unzips it straight into a project.
# -y stores symlinks rather than following them: the macOS .framework bundles
# are built by SCons and a future one carrying version symlinks would otherwise
# be flattened, taking its code signature with it. It is a no-op when there are
# none, and the check below says how many were stored.
( cd "$WORK" && zip -qry "$OUT/$ZIP" addons/ )
echo "   $ZIP"

# Canonical checksum manifest so a consumer can verify the downloaded zip's
# integrity (`sha256sum -c SHA256SUMS --ignore-missing`). Covers the zips in
# out/; a follow-up dispatch for another Godot version writes its own.
( cd "$OUT" && sha256 ./*.zip | sed 's|\./||' > SHA256SUMS )
echo "   SHA256SUMS"

# --- verify ---------------------------------------------------------------
if [ "$VERIFY" = "yes" ]; then
  step "verifying $OUT"

  if [ ! -s "$OUT/$ZIP" ]; then
    echo "$APP: $ZIP is missing or empty" >&2
    exit 1
  fi

  ENTRIES="$(unzip -Z1 "$OUT/$ZIP")"
  has () { printf '%s\n' "$ENTRIES" | grep -qx "$1"; }
  has_any () { printf '%s\n' "$ENTRIES" | grep -q "^$1"; }

  has "addons/spritestudio/spritestudio.gdextension" \
    || fail "$ZIP has no addons/spritestudio/spritestudio.gdextension (Godot loads nothing without it)"

  # THE check. Every path the descriptor names, looked up in the archive. Godot
  # resolves these at load time, so a name that does not match what shipped
  # fails silently on that platform for whoever downloaded it -- and nothing
  # earlier in this pipeline compares the two.
  #
  # [libraries] paths are relative to the addon folder; [icons] paths are
  # res:// URLs into it. A .framework or .xcframework is a directory, so it is
  # matched by something existing underneath it rather than as a file.
  libs=0
  missing_libs=0
  while IFS= read -r path; do
    [ -n "$path" ] || continue
    libs=$((libs + 1))
    entry="addons/spritestudio/${path}"
    esc="$(printf '%s' "$entry" | sed 's/[.[\*^$]/\\&/g')"
    has "$esc" && continue
    has_any "${esc}/" && continue
    fail "spritestudio.gdextension names $path, which is not in $ZIP"
    missing_libs=$((missing_libs + 1))
  done <<EOF
$(sed -n '/^\[libraries\]/,/^\[/p' "$GDEXTENSION" | sed -n 's/^[^=]*=[[:space:]]*"\(.*\)"[[:space:]]*$/\1/p')
EOF

  icons=0
  while IFS= read -r path; do
    [ -n "$path" ] || continue
    icons=$((icons + 1))
    entry="${path#res://}"
    esc="$(printf '%s' "$entry" | sed 's/[.[\*^$]/\\&/g')"
    has "$esc" || fail "spritestudio.gdextension points at $path, which is not in $ZIP"
  done <<EOF
$(sed -n '/^\[icons\]/,$p' "$GDEXTENSION" | sed -n 's/^[^=]*=[[:space:]]*"\(.*\)"[[:space:]]*$/\1/p')
EOF

  if [ "$libs" -eq 0 ]; then
    fail "no [libraries] entries parsed out of spritestudio.gdextension -- the check above proved nothing"
  elif [ "$missing_libs" -eq 0 ]; then
    echo "   spritestudio.gdextension: all $libs libraries and $icons icons are in the archive"
  fi

  # The licences the shipped binaries carry.
  for f in LICENSE.md LICENSE.ja.md THIRD_PARTY_NOTICES.md Apache-2.0.txt \
           THIRD-PARTY-LICENSES.ssruntime.md THIRD-PARTY-LICENSES.ssconverter.md runtime-LICENSE.md; do
    has "addons/spritestudio/licenses/$f" || fail "$ZIP is missing licenses/$f"
  done

  SYMLINKS="$(unzip -Z "$OUT/$ZIP" | grep -c '^l' || true)"
  echo "   $(printf '%s\n' "$ENTRIES" | wc -l | tr -d ' ') entries, $SYMLINKS symlink(s) stored"

  ( cd "$OUT" && sha256 -c SHA256SUMS > /dev/null ) || fail "SHA256SUMS does not verify"

  if [ -n "$FAILED" ]; then
    echo "" >&2
    echo "$APP: the release in $OUT is not shippable:$FAILED" >&2
    exit 1
  fi
  echo "   SHA256SUMS verified"
  echo "   all checks passed"
else
  step "verify=no -- the archive is not checked"
fi

# --- done -----------------------------------------------------------------
echo ""
echo "$APP: done -> $OUT  (Godot $API_VERSION${TAG:+, $TAG})"
echo ""
( cd "$OUT" && ls -l ./*.zip SHA256SUMS | sed 's|^|  |' )
echo ""
echo "$APP: this is what release.yml attaches to the draft Release."
echo "$APP: try it before shipping it -- unzip into a project and open the editor:"
echo "  unzip -o $OUT/$ZIP -d <your-godot-project>/"
