#!/usr/bin/env bash
#
# Create the local virtualenv and install the pinned documentation toolchain.
#
#   scripts/prepare-docs.sh            # .venv + docs/requirements.txt
#   scripts/prepare-docs.sh force=yes  # discard the venv and build it again
#
# Documentation only, and independent of everything else in this directory: the
# other scripts here build and release the code in this repository, and none of
# them has to have run before the docs will build. One Python environment is the
# whole documentation toolchain. The -docs suffix is what keeps the two apart --
# every repository in the SpriteStudio family spells this pair the same way.
#
# The pins in docs/requirements.txt are exact. Rerun this script after a pin
# changes -- pip installs the pinned version over whatever the venv already has.
#
# Then: scripts/build-docs.sh, which runs zensical out of the venv this created.
set -euo pipefail

APP="$(basename "$0")"
ROOT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
REQUIREMENTS="docs/requirements.txt"

# --- options --------------------------------------------------------------
PYTHON="python3"
VENV=".venv"
FORCE="no"

usage () {
  echo "Usage: $APP [options]"
  echo "$APP options:"
  echo "  python=<exe>            Interpreter to create the venv with (default: $PYTHON)"
  echo "  venv=<dir>              Virtualenv location, relative to the repo root"
  echo "                          (default: $VENV)"
  echo "  force=<yes|no>          Delete an existing venv and recreate it, rather than"
  echo "                          installing into it (default: $FORCE)"
  echo "  -h | --help             Show this help"
  echo ""
  echo "Installs $REQUIREMENTS, whose pins are exact. Run it again whenever a pin"
  echo "changes; force=yes is only for a venv that has gone wrong."
  echo ""
  echo "Building the site is scripts/build-docs.sh, not this script."
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
        python) PYTHON="$value" ;;
        venv)   VENV="$value" ;;
        # Lower-cased, because this one is a yes/no flag rather than a path.
        force)  FORCE="$(printf '%s' "$value" | tr '[:upper:]' '[:lower:]')" ;;
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

case "$FORCE" in
  yes|no) ;;
  *) echo "$APP: force must be yes or no (got '$FORCE')" >&2; exit 2 ;;
esac

echo "options"
echo "  python => $PYTHON"
echo "  venv   => $VENV"
echo "  force  => $FORCE"
echo ""

# --- preflight ------------------------------------------------------------
if [ ! -f "$ROOT_DIR/$REQUIREMENTS" ]; then
  echo "$APP: $REQUIREMENTS not found -- is this the SSPlayerForGodot repo?" >&2
  exit 1
fi

if ! command -v "$PYTHON" > /dev/null 2>&1; then
  echo "$APP: '$PYTHON' not found (install Python 3, or pass python=<exe>)" >&2
  echo "$APP: on Windows the launcher is usually 'py': $APP python=py" >&2
  exit 1
fi

VENV_DIR="$ROOT_DIR/$VENV"

# --- venv -----------------------------------------------------------------
if [ "$FORCE" = "yes" ] && [ -d "$VENV_DIR" ]; then
  echo ">> removing the existing venv ($VENV)"
  rm -rf "$VENV_DIR"
fi

if [ -d "$VENV_DIR" ]; then
  echo ">> reusing the existing venv ($VENV)"
else
  echo ">> $PYTHON -m venv $VENV"
  "$PYTHON" -m venv "$VENV_DIR"
fi

# A venv made by py/python.exe puts its executables in Scripts/, everything else
# in bin/. Probe rather than branch on the OS: this script also runs under Git
# Bash and MSYS, where $OSTYPE says Windows but either layout may be on disk.
if [ -d "$VENV_DIR/Scripts" ]; then
  VENV_BIN="$VENV_DIR/Scripts"
else
  VENV_BIN="$VENV_DIR/bin"
fi

VENV_PY="$VENV_BIN/python"
[ -x "$VENV_PY" ] || VENV_PY="$VENV_BIN/python.exe"
if [ ! -x "$VENV_PY" ]; then
  echo "$APP: no interpreter in $VENV -- the venv looks broken; retry with force=yes" >&2
  exit 1
fi

# --- toolchain ------------------------------------------------------------
echo ">> pip install -r $REQUIREMENTS"
"$VENV_PY" -m pip install -r "$ROOT_DIR/$REQUIREMENTS"

echo ""
echo ">> installed:"
"$VENV_PY" -m pip show zensical 2>/dev/null | grep -E '^(Name|Version):' || true

# Built from the layout probed above rather than assuming bin/, so the hint is
# right under Git Bash too, where the venv may well be a Scripts/ one.
SERVE_HINT="$VENV/$(basename "$VENV_BIN")/zensical"

echo ""
echo "$APP: done -> $VENV"
echo "$APP: build both locales: scripts/build-docs.sh"
echo "$APP: preview one:        $SERVE_HINT serve  (add -f mkdocs.ja.yml for Japanese)"
