#!/usr/bin/env bash
#
# Check that every Godot version the docs, the READMEs, the shipped add-on README
# and the custom module's guard name is the floor or the tested line.
#
#   scripts/check-godot-versions.sh
#
# The floor is what scripts/godot-api-version.sh prints, `compatibility_minimum`
# in misc/spritestudio.gdextension. The tested line is the major.minor of
# scripts/GODOT_VERSION.txt, the editor CI runs and the engine the custom module
# builds against. Raising either is one edit there, then whatever this lists.
# The examples' project.godot are left out: Godot writes their feature tag when
# a project is saved.
set -euo pipefail

APP="$(basename "$0")"
ROOT_DIR="$(cd "$(dirname "$0")/.." && pwd)"

if [ "$#" -gt 0 ]; then
  echo "$APP: takes no arguments (got '$1')" >&2
  exit 2
fi

cd "$ROOT_DIR"
FLOOR="$(./scripts/godot-api-version.sh)"
LINE="$(cut -d. -f1,2 < scripts/GODOT_VERSION.txt | tr -d '[:space:]')"
MAJOR="${FLOOR%%.*}"
MINOR="${FLOOR#*.}"
STATUS=0

if [ "$(printf '%s\n%s\n' "$FLOOR" "$LINE" | sort -t. -k1,1n -k2,2n | head -n 1)" != "$FLOOR" ]; then
  echo "$APP: scripts/GODOT_VERSION.txt tests $LINE, below the floor $FLOOR" >&2
  STATUS=1
fi

while IFS=: read -r file lineno token; do
  token="${token#"${token%%[0-9]*}"}"
  case "$(printf '%s' "$token" | cut -d. -f1,2)" in
    "$FLOOR" | "$LINE") ;;
    *)
      echo "$APP: $file:$lineno names Godot $token, neither the floor ($FLOOR) nor the tested line ($LINE)" >&2
      STATUS=1
      ;;
  esac
done < <(git ls-files 'README*.md' CONTRIBUTING.md misc/ADDON_README.md 'docs/*.md' ss_player/ss_macros.h |
  xargs grep -H -n -o -E "(^|[^0-9.])${MAJOR}\.[0-9]+(\.[0-9]+)*" || true)

GUARD="$(printf '0x%02x%02x00' "$MAJOR" "$MINOR")"
if ! grep -q "GODOT_VERSION_HEX < ${GUARD}" ss_player/ss_macros.h; then
  echo "$APP: ss_player/ss_macros.h does not guard on ${GUARD} (Godot $FLOOR)" >&2
  STATUS=1
fi

[ "$STATUS" = 0 ] && echo "$APP: every Godot version named is $FLOOR (floor) or $LINE (tested line)"
exit "$STATUS"
