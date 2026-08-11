#!/usr/bin/env bash
#
# Build the documentation site -- one run per locale, in the order that works.
#
#   scripts/build-docs.sh              # English then Japanese, both strict
#   scripts/build-docs.sh locale=ja    # Japanese only, into site/ja
#
# Two things this script exists to get right:
#
#   1. Order. The Japanese site is written *inside* the English one (site/ja),
#      and a build clears its own output directory first -- so `zensical build`
#      deletes site/ja on its way in. English first, Japanese second; the other
#      order publishes a site with no Japanese pages at all.
#   2. --strict, always. Nothing builds the docs on a pull request
#      (.github/workflows/pages.yml runs on a published release and on dispatch),
#      so this is the only thing standing between a broken internal link and the
#      published site. mkdocs.base.yml sets strict: true as well, but the flag is
#      spelled out here because it is what the docs tell people to run.
#
# The -docs suffix keeps this apart from the scripts that build the code itself;
# the whole SpriteStudio family spells the documentation pair the same way.
#
# `zensical serve` is deliberately not wrapped: it validates nothing (--strict is
# accepted and does nothing), it serves one locale at a time, and it is a
# foreground process with no arguments worth defaulting.
set -euo pipefail

APP="$(basename "$0")"
ROOT_DIR="$(cd "$(dirname "$0")/.." && pwd)"

# --- options --------------------------------------------------------------
LOCALE="all"
VENV=".venv"

usage () {
  echo "Usage: $APP [options]"
  echo "$APP options:"
  echo "  locale=<all|en|ja>      Which locale to build (default: $LOCALE)"
  echo "                          all = English then Japanese, the only safe order"
  echo "  venv=<dir>              Virtualenv to run zensical from, relative to the repo"
  echo "                          root (default: $VENV). Falls back to zensical on PATH,"
  echo "                          which is how CI runs with no venv at all."
  echo "  -h | --help             Show this help"
  echo ""
  echo "Output: site/ (English) and site/ja/ (Japanese). Both builds are --strict, so a"
  echo "broken internal link or a nav entry pointing at a missing page fails the build."
  echo ""
  echo "locale=en on its own DELETES site/ja: the English build clears site/, and the"
  echo "Japanese site lives inside it. Use locale=all before serving or publishing."
  echo ""
  echo "Set the venv up first with scripts/prepare-docs.sh."
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
        locale) LOCALE="$(printf '%s' "$value" | tr '[:upper:]' '[:lower:]')" ;;
        venv)   VENV="$value" ;;
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

case "$LOCALE" in
  all|both) LOCALE="all" ;;
  en|english) LOCALE="en" ;;
  ja|japanese) LOCALE="ja" ;;
  *) echo "$APP: locale must be all, en or ja (got '$LOCALE')" >&2; exit 2 ;;
esac

echo "options"
echo "  locale => $LOCALE"
echo "  venv   => $VENV"
echo ""

# --- preflight ------------------------------------------------------------
# Same probe as prepare-docs.sh: bin/ everywhere, Scripts/ for a venv built by
# the Windows launcher.
VENV_DIR="$ROOT_DIR/$VENV"
if [ -d "$VENV_DIR/Scripts" ]; then
  VENV_BIN="$VENV_DIR/Scripts"
else
  VENV_BIN="$VENV_DIR/bin"
fi

ZENSICAL=""
for candidate in "$VENV_BIN/zensical" "$VENV_BIN/zensical.exe"; do
  if [ -x "$candidate" ]; then
    ZENSICAL="$candidate"
    break
  fi
done

# CI installs the pins with plain `pip install -r`, so there is no venv there and
# zensical is simply on PATH. Fall back to it rather than demanding a venv.
if [ -z "$ZENSICAL" ] && command -v zensical > /dev/null 2>&1; then
  ZENSICAL="$(command -v zensical)"
  echo "$APP: no venv at $VENV; using zensical from PATH"
fi

if [ -z "$ZENSICAL" ]; then
  echo "$APP: zensical not found in $VENV or on PATH" >&2
  echo "$APP: run 'scripts/prepare-docs.sh' first" >&2
  exit 1
fi

# Every path in the configs is relative to the repo root, so build from there
# however the script was invoked.
cd "$ROOT_DIR"

# --- build ----------------------------------------------------------------
if [ "$LOCALE" = "all" ] || [ "$LOCALE" = "en" ]; then
  echo ">> zensical build --strict (en) -> site/"
  "$ZENSICAL" build --strict
fi

if [ "$LOCALE" = "all" ] || [ "$LOCALE" = "ja" ]; then
  echo ">> zensical build -f mkdocs.ja.yml --strict (ja) -> site/ja/"
  "$ZENSICAL" build -f mkdocs.ja.yml --strict
fi

echo ""
case "$LOCALE" in
  all)
    echo "$APP: done -> site/ (en) + site/ja/ (ja)"
    echo "$APP: serve both, so the language selector resolves:"
    echo "$APP:   python3 -m http.server 8000 -d site"
    ;;
  en)
    echo "$APP: done -> site/ (en)"
    echo "$APP: WARNING: this build cleared site/, so site/ja no longer exists." >&2
    echo "$APP:          run '$APP locale=ja' before serving or publishing." >&2
    ;;
  ja)
    echo "$APP: done -> site/ja/ (ja)"
    echo "$APP: site/ still holds whatever the last English build left there."
    ;;
esac
