#!/usr/bin/env bash
#
# Print how a release is marked on GitHub, worked out from the repository's tags.
#
#   scripts/release-channel.sh <tag>    # latest, prerelease or old
#
#   latest      a full release newer than any already tagged, or a prerelease
#               before its package's first full release: the Release is Latest,
#               and pages.yml deploys the site from it.
#   prerelease  a prerelease once its package has a full release: the Release
#               is a pre-release, and the site stays as it is.
#   old         a full release older than one already tagged, a patch to an
#               earlier line: not Latest, and the site stays as it is.
#
# A tag is its package's prefix -- `v`, or `<package>-v` where packages are
# versioned apart -- then the version. A prerelease has a `-` (7.0.0-beta.1) or,
# as Ren'Py writes them, `a`, `b` or `rc` (7.0.0b1). Tags before 7.0 are the
# previous generation (SSPlayerForGodot's 1.x) and are not counted. The caller
# fetches the tags; this reads only what is local.
set -euo pipefail

APP="$(basename "$0")"

if [ "$#" -ne 1 ] || [ -z "$1" ]; then
  echo "$APP: takes <tag>" >&2
  exit 2
fi
tag="$1"
prefix="${tag%%v[0-9]*}v"
version="${tag#"$prefix"}"
case "$version" in
  [0-9]*.[0-9]*.[0-9]*) ;;
  *) echo "$APP: $tag does not end in v<version>" >&2; exit 2 ;;
esac

is_prerelease () {
  case "$1" in *-*) return 0 ;; esac
  [[ "$1" =~ [0-9](a|b|rc)[0-9]+$ ]]
}

full_releases () {
  git tag --list "${prefix}[0-9]*" | while IFS= read -r t; do
    v="${t#"$prefix"}"
    is_prerelease "$v" && continue
    [ "${v%%.*}" -ge 7 ] 2>/dev/null || continue
    printf '%s\n' "$v"
  done
}

fulls="$(full_releases)"
if is_prerelease "$version"; then
  if [ -n "$fulls" ]; then echo prerelease; else echo latest; fi
  exit 0
fi
newest="$(printf '%s\n%s\n' "$fulls" "$version" | grep . | sort -t. -k1,1n -k2,2n -k3,3n | tail -n 1)"
if [ "$newest" = "$version" ]; then echo latest; else echo old; fi
