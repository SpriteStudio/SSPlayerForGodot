#!/usr/bin/env bash
#
# Match the files tracked at HEAD against public open-source code with SCANOSS,
# and print every match. Only their fingerprints are sent, to SCANOSS's free
# service. It limits calls per address, and GitHub's runners share addresses it
# has often limited already, so this runs on a maintainer's machine rather than
# in provenance.yml. RELEASING.md runs it before a release.
#
#   scripts/scan-snippets.sh            # -> qa/scanoss-<commit>.json
#
# Needs Python 3: the scanner is installed into a throwaway virtual environment.
set -euo pipefail

APP="$(basename "$0")"
if [ "$#" -gt 0 ]; then
  echo "Usage: $APP" >&2
  exit 2
fi

ROOT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
OUT="$ROOT_DIR/qa/scanoss-$(git -C "$ROOT_DIR" rev-parse --short HEAD).json"
WORK="$(mktemp -d)"
trap 'rm -rf "$WORK"' EXIT

PYTHON="$(command -v python3 || command -v python || true)"
if [ -z "$PYTHON" ]; then
  echo "$APP: Python 3 is required" >&2
  exit 1
fi
"$PYTHON" -m venv "$WORK/venv"
BIN="$WORK/venv/bin"
[ -d "$BIN" ] || BIN="$WORK/venv/Scripts"
"$BIN/python" -m pip install --quiet --disable-pip-version-check scanoss==1.54.2

git -C "$ROOT_DIR" archive -o "$WORK/tracked.tar" HEAD
mkdir "$WORK/src"
mkdir -p "$ROOT_DIR/qa"
tar -xf "$WORK/tracked.tar" -C "$WORK/src"

"$BIN/scanoss-py" scan --threads 1 -o "$OUT" "$WORK/src"

"$BIN/python" - "$OUT" <<'EOF'
import json
import sys

with open(sys.argv[1], encoding="utf-8") as f:
    results = json.load(f)
print("| File | Match | Lines | Component | Source file | Source lines |")
print("| --- | --- | --- | --- | --- | --- |")
for path, matches in sorted(results.items()):
    for m in matches:
        if m.get("id") != "none":
            print(f"| `{path}` | {m['id']} {m.get('matched')} | {m.get('lines')} | "
                  f"[{m.get('component')} {m.get('version')}]({m.get('url')}) | `{m.get('file')}` | {m.get('oss_lines')} |")
EOF
echo "$APP: the full report is ${OUT#"$ROOT_DIR"/}" >&2
