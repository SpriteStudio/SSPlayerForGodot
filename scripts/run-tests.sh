#!/bin/bash
#
# Run the headless test suite in test_gdextension/ against the GDExtension build.
#
# The work is test_gdextension/run_tests.gd; this finds a Godot binary, runs the import
# pass the project needs before its first run, and forwards the options.
#
# Two prerequisites, both gitignored and so both missing from a fresh clone.
# run_tests.gd checks them before it starts anything and names the script that
# produces each:
#   test_gdextension/addons/spritestudio/  <- build-extension.{sh,ps1}
#   test_gdextension/ssab_generated/       <- deploy-examples.{sh,ps1}
#
# It never downloads anything on your behalf. When no binary is found it prints
# scripts/fetch-godot.sh and stops; a download of tens of MB is your decision.
#
# What this does NOT cover: drawing, and the custom-module build. --headless
# installs a dummy rasteriser, so there are no pixels to compare; and a module
# is compiled into the engine, so testing that build would mean building Godot
# rather than downloading it. The two builds share one copy of the playback
# logic and differ only in a layer of #ifdef SPRITESTUDIO_GODOT_EXTENSION
# adapters -- includes and type conversions, which is what a compiler checks,
# so the module build's guard is that it still builds.
#
# Usage: scripts/run-tests.sh [godot=<path>] [only=<names>] [import=no]
#   godot  : the Godot binary to use (else $GODOT, godot-bin/, then PATH)
#   only   : comma-separated substrings; run only the suites/cases that match
#   import : no to skip the import pass, once test_gdextension/.godot exists
#
# Exit status: 0 all passed, 1 a case failed, 2 preflight or setup failed.
set -euo pipefail

APP=$(basename "$0")
SCRIPTDIR=$(cd "$(dirname "$0")" && pwd -P)
ROOTDIR=$(cd "$SCRIPTDIR/.." && pwd -P)
PROJECT="${ROOTDIR}/test_gdextension"

usage() { sed -n '/^# Usage:/,/^# Exit status/p' "$0" | sed 's/^# \{0,1\}//'; }

GODOT_BIN="${GODOT:-}"
ONLY=""
DO_IMPORT="yes"
for item in "$@"; do
  case "$item" in
    godot=*)  GODOT_BIN="${item#*=}" ;;
    only=*)   ONLY="${item#*=}" ;;
    import=*) DO_IMPORT="${item#*=}" ;;
    -h|--help|help) usage; exit 0 ;;
    *) echo "$APP: unknown arg '$item'" >&2; usage; exit 2 ;;
  esac
done

# --- find a binary --------------------------------------------------------
# godot/bin/* is deliberately not in this list. That is a custom module build
# with SpriteStudio compiled into it, so it registers the classes a second time
# and aborts on a project carrying addons/spritestudio -- see the message below.
if [ -z "$GODOT_BIN" ]; then
  for cand in \
      "${ROOTDIR}/godot-bin/Godot.app/Contents/MacOS/Godot" \
      "${ROOTDIR}/godot-bin/godot" \
      "${ROOTDIR}/godot-bin/godot.exe"; do
    if [ -x "$cand" ]; then GODOT_BIN="$cand"; break; fi
  done
fi
if [ -z "$GODOT_BIN" ] && command -v godot >/dev/null; then
  GODOT_BIN="$(command -v godot)"
fi
if [ -z "$GODOT_BIN" ] || [ ! -x "$GODOT_BIN" ]; then
  cat >&2 <<EOF
$APP: no Godot binary found.

  Pass one:            $APP godot=/path/to/godot
  or install the pin:  scripts/fetch-godot.sh    ($(tr -d ' \r\n' < "${SCRIPTDIR}/GODOT_VERSION.txt"), the editor build only)

Not godot/bin/* — that is a custom module build with SpriteStudio compiled in,
and it cannot open a project that also loads the extension.
EOF
  exit 2
fi

echo "$APP: $("$GODOT_BIN" --headless --version 2>/dev/null | tail -n 1) at ${GODOT_BIN}"

# --- import pass ----------------------------------------------------------
# A project Godot has never opened has no .godot/, and the textures beside each
# .ssab are not importable until it does. Cheap after the first run.
# It is run twice on purpose, and the SECOND run is the one that has to pass.
#
# The first headless scan of a project that loads a godot-cpp GDExtension aborts
# on the way out (null dereference, caught by Godot's own crash handler). It is
# NOT this repository's code: godot-cpp's own test extension, with none of our
# sources in it, reproduces it exactly -- and a project with no extension at all
# does not. godot-cpp has no 4.6/4.7 release branch (it went from
# godot-4.5-stable straight to the 10.0 line), so an extension for Godot 4.7 is
# built from master against api_version=4.7, and that is the combination that
# does this. The scan's work completes: every later run exits 0 with nothing to
# do.
#
# So this is a retry, not a tolerance. A crash that repeats is still a failure
# here, and the clean second run is the evidence that the import finished --
# nothing is being waved through on the strength of the first one.
if [ "$DO_IMPORT" = "yes" ]; then
  IMPORT_LOG=$(mktemp)
  set +e
  "$GODOT_BIN" --headless --path "$PROJECT" --import >"$IMPORT_LOG" 2>&1
  IMPORT_STATUS=$?
  if [ "$IMPORT_STATUS" -ne 0 ]; then
    echo "$APP: the first import exited $IMPORT_STATUS (godot-cpp's known first-scan crash); retrying."
    "$GODOT_BIN" --headless --path "$PROJECT" --import >"$IMPORT_LOG" 2>&1
    IMPORT_STATUS=$?
  fi
  set -e
  if [ "$IMPORT_STATUS" -ne 0 ] || grep -q '^ERROR:' "$IMPORT_LOG"; then
    echo "$APP: the import pass failed." >&2
    tail -n 25 "$IMPORT_LOG" >&2
    rm -f "$IMPORT_LOG"
    exit 2
  fi
  rm -f "$IMPORT_LOG"
fi

# --- run ------------------------------------------------------------------
# --quit-after is a hang guard, not a schedule: a script error inside a case
# aborts run_tests.gd's _init and leaves the tree idling forever. It costs the
# exit code its meaning on that path -- Godot leaves 0 on the way out -- which
# is why the marker below, and not the status, is what says a run completed.
ARGS=(--headless --path "$PROJECT" --quit-after 100000 --script res://run_tests.gd)
[ -n "$ONLY" ] && ARGS+=(-- "--only=${ONLY}")

set +e
OUTPUT=$("$GODOT_BIN" "${ARGS[@]}" 2>&1)
STATUS=$?
set -e
echo "$OUTPUT"

# Godot aborts during extension validation, before run_tests.gd gets to say
# anything, so this is the one failure the wrapper has to explain itself.
if echo "$OUTPUT" | grep -q "appears to be already registered"; then
  cat >&2 <<EOF

$APP: that Godot binary already has SpriteStudio compiled into it (a custom
module build), so loading the extension registers every class twice.

  Use a stock binary (scripts/fetch-godot.sh), or remove
  test_gdextension/addons/spritestudio to run the same suite against the module build.
EOF
  exit 2
fi

# run_tests.gd prints this last. Without it the run stopped in the middle -- a
# parse error in a suite, or a script error inside a case, either of which
# leaves Godot to be shut down by --quit-after with a status of 0.
if ! echo "$OUTPUT" | grep -q "==== SUITE FINISHED ===="; then
  echo "" >&2
  echo "$APP: the run stopped before the suite finished — see above." >&2
  exit 1
fi

exit $STATUS
