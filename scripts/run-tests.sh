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
#
# The extension is named in .godot/extension_list.cfg BEFORE Godot is started,
# because the run in which Godot discovers an extension mid-scan crashes on the
# way out. That crash is Godot's own, and it is not the import -- a project with
# zero importable files does it too:
#
#   EditorHelp::_gen_extensions_docs() dereferences the static DocTools without
#   a null check. Loading an extension mid-scan emits extensions_reloaded, which
#   sends EditorNode off to regenerate the class reference on a worker thread;
#   that thread's last act is to queue _gen_extensions_docs as a deferred call.
#   --import then quits before the message queue is flushed, so the call lands
#   on the flush at the end of Main::cleanup() -- by which time
#   EditorHelp::cleanup_doc() has freed the DocTools and set it to null.
#
# Naming the extension up front means it is loaded at startup instead, so
# extensions_reloaded never fires and nothing is ever queued. Godot rewrites
# this file itself, so seeding it decides only the run in which it did not exist
# yet. There is no retry here: with the file seeded the import either works or
# has failed for some other reason, and a crash is a crash.
if [ "$DO_IMPORT" = "yes" ]; then
  EXT_LIST="${PROJECT}/.godot/extension_list.cfg"
  if [ ! -f "$EXT_LIST" ]; then
    for desc in "${PROJECT}"/addons/spritestudio/*.gdextension; do
      [ -f "$desc" ] || continue
      mkdir -p "${PROJECT}/.godot"
      echo "res://${desc#"${PROJECT}/"}" >>"$EXT_LIST"
    done
  fi

  IMPORT_LOG=$(mktemp)
  set +e
  "$GODOT_BIN" --headless --path "$PROJECT" --import >"$IMPORT_LOG" 2>&1
  IMPORT_STATUS=$?
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
# --quit-after is a hang guard, not a schedule: an error run_tests.gd's _init
# cannot walk away from -- a suite that will not parse -- leaves the tree idling
# forever. It costs the exit code its meaning on that path -- Godot leaves 0 on
# the way out -- which is why the marker below, and not the status, is what says
# a run completed. A script error inside one case is not that: GDScript
# abandons the case and carries on, and run_tests.gd fails it for recording
# nothing.
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
# suite that will not parse, say -- and Godot was shut down by --quit-after with
# a status of 0. A script error inside one case does not stop the run; the
# runner catches that itself, by failing a case that recorded nothing.
if ! echo "$OUTPUT" | grep -q "==== SUITE FINISHED ===="; then
  echo "" >&2
  echo "$APP: the run stopped before the suite finished — see above." >&2
  exit 1
fi

exit $STATUS
