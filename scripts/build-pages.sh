#!/usr/bin/env bash
#
# Build the GitHub Pages site locally -- everything .github/workflows/pages.yml
# publishes, in the same order: the documentation toolchain, then the
# documentation in both locales into site/.
#
#   scripts/build-pages.sh              # the tree pages.yml uploads
#   scripts/build-pages.sh serve=yes    # ... and serve it on :8000
#   scripts/build-pages.sh prepare=yes  # ... reinstalling the pinned toolchain first
#
# The published site here is the documentation and nothing else -- no demo, no
# examples -- so this is scripts/build-docs.sh plus the two things a published
# tree needs that a docs build on its own does not give you:
#
#   1. The toolchain. prepare-docs.sh creates .venv and installs the pins from
#      docs/requirements.txt, which is the step a fresh clone forgets. Here it
#      runs only when no zensical is found (prepare=auto).
#   2. A server over the whole tree. `zensical serve` builds one locale at a
#      time, so it cannot show you the header language selector resolving, and
#      the site it serves is not the site that gets uploaded. One static server
#      over site/ is what a reader meets.
#
# `build-pages.sh` means "build what pages.yml publishes" in every repository in
# the SpriteStudio family. In the ones whose site carries a playable demo or the
# API reference it carries them too; here there is nothing else to carry, and the
# name still points at the same thing.
#
# What this does NOT reproduce is the deploy job (actions/deploy-pages) -- there
# is no local equivalent of publishing. Rerunning is safe; nothing here is
# incremental.
set -euo pipefail

APP="$(basename "$0")"
ROOT_DIR="$(cd "$(dirname "$0")/.." && pwd)"

# --- options --------------------------------------------------------------
PREPARE="auto"
VENV=".venv"
SERVE="no"
PORT="8000"

usage () {
  echo "Usage: $APP [options]"
  echo "$APP options:"
  echo "  prepare=<auto|yes|no>   Run prepare-docs.sh first (default: $PREPARE)."
  echo "                          auto = only when no zensical is found. yes forces it,"
  echo "                          which is what a changed pin in docs/requirements.txt"
  echo "                          needs; no is for CI, where it is a step of its own."
  echo "  venv=<dir>              Virtualenv for the docs toolchain, relative to the"
  echo "                          repo root (default: $VENV). Passed to prepare-docs.sh"
  echo "                          and build-docs.sh."
  echo "  serve=<yes|no>          Serve site/ over HTTP when the build is done"
  echo "                          (default: $SERVE). Foreground; Ctrl-C to stop."
  echo "  port=<n>                Port for serve=yes (default: $PORT)"
  echo "  -h | --help             Show this help"
  echo ""
  echo "Output: site/ (en) + site/ja/ (ja) -- the exact tree pages.yml uploads. Both"
  echo "builds are --strict, so a broken internal link or a nav entry pointing at a"
  echo "missing page fails the build here rather than on a release publish."
  echo ""
  echo "Serve both locales from one server, or the language selector points at pages"
  echo "that are not being served."
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
        # Lower-cased where the option is a keyword; venv=/port= are not.
        prepare) PREPARE="$(printf '%s' "$value" | tr '[:upper:]' '[:lower:]')" ;;
        venv)    VENV="$value" ;;
        serve)   SERVE="$(printf '%s' "$value" | tr '[:upper:]' '[:lower:]')" ;;
        port)    PORT="$value" ;;
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

case "$PREPARE" in auto|yes|no) ;; *) echo "$APP: prepare must be auto, yes or no (got '$PREPARE')" >&2; exit 2 ;; esac
case "$SERVE" in yes|no) ;; *) echo "$APP: serve must be yes or no (got '$SERVE')" >&2; exit 2 ;; esac
case "$PORT" in ''|*[!0-9]*) echo "$APP: port must be a number (got '$PORT')" >&2; exit 2 ;; esac

SERVE_NOTE=""
[ "$SERVE" = "no" ] || SERVE_NOTE=" (port $PORT)"

echo "options"
echo "  prepare => $PREPARE"
echo "  venv    => $VENV"
echo "  serve   => $SERVE$SERVE_NOTE"
echo ""

# --- preflight ------------------------------------------------------------
if [ ! -f "$ROOT_DIR/mkdocs.base.yml" ]; then
  echo "$APP: mkdocs.base.yml not found -- is this the SSPlayerForGodot repo?" >&2
  exit 1
fi

# Every script here resolves its own paths from the repo root, and site/ is
# written relative to it, so run from there however this was invoked.
cd "$ROOT_DIR"

step () {
  echo ""
  echo ">> $*"
  "$@"
}

# --- documentation --------------------------------------------------------
# Same probe as build-docs.sh: a venv laid out either way, else PATH (which is
# how a run with no venv at all still works).
have_zensical () {
  local venv_bin="$ROOT_DIR/$VENV/bin"
  [ -d "$ROOT_DIR/$VENV/Scripts" ] && venv_bin="$ROOT_DIR/$VENV/Scripts"
  [ -x "$venv_bin/zensical" ] || [ -x "$venv_bin/zensical.exe" ] \
    || command -v zensical > /dev/null 2>&1
}

case "$PREPARE" in
  yes)
    step ./scripts/prepare-docs.sh venv="$VENV"
    ;;
  auto)
    if have_zensical; then
      echo "$APP: zensical already available; skipping prepare-docs.sh (prepare=yes forces it)"
    else
      step ./scripts/prepare-docs.sh venv="$VENV"
    fi
    ;;
  no) ;;
esac

# Both locales, English first -- build-docs.sh owns that order, because the
# English build clears site/ and the Japanese site lives inside it at site/ja.
step ./scripts/build-docs.sh venv="$VENV"

# --- what landed ----------------------------------------------------------
echo ""
echo "$APP: done -> site/"
report () { # <path> <label>
  if [ -e "$ROOT_DIR/$1" ]; then
    echo "  $2: $1"
  else
    echo "  $2: MISSING ($1)"
  fi
}
report "site/index.html" "en"
report "site/ja/index.html" "ja"

# --- serve ----------------------------------------------------------------
PYTHON_CMD="python3"
command -v "$PYTHON_CMD" > /dev/null 2>&1 || PYTHON_CMD="python"

echo ""
if [ "$SERVE" = "yes" ]; then
  command -v "$PYTHON_CMD" > /dev/null 2>&1 || {
    echo "$APP: no python on PATH to serve with -- use any static server on site/" >&2
    exit 1
  }
  echo "$APP: http://localhost:$PORT/  (English)"
  echo "$APP: http://localhost:$PORT/ja/  (Japanese)"
  echo "$APP: Ctrl-C to stop"
  echo ""
  exec "$PYTHON_CMD" -m http.server "$PORT" -d site
else
  echo "$APP: serve it (one server over both locales, so the language selector resolves):"
  echo "$APP:   $APP serve=yes"
  echo "$APP:   $PYTHON_CMD -m http.server $PORT -d site"
fi
