#!/usr/bin/env bash
# This script fills the placeholders in the version.h.in template.
# Usage: ./tools/generate_version_header.sh TEMPLATE NAME VERSION DESCRIPTION

set -euo pipefail

TEMPLATE=${1:?Missing version.h.in template path}
NAME=${2:-unknown}
VERSION=${3:-unknown.unknown.unknown}
DESCRIPTION=${4:-description}
STATUS_FILE="bazel-out/stable-status.txt"

if [[ ! -f "$TEMPLATE" ]]; then
  printf 'Version header template not found: %s\n' "$TEMPLATE" >&2
  exit 1
fi
if [[ ! -f "$STATUS_FILE" ]]; then
  printf 'Bazel stable status file not found: %s\n' "$STATUS_FILE" >&2
  exit 1
fi

IFS='.' read -r MAJOR MINOR REVISION _ <<< "$VERSION"
REPOSITORY_STATUS="$(grep '^STABLE_REPOSITORY_STATUS ' bazel-out/stable-status.txt  | cut -d' ' -f2-)" # get repository status from bazel-out/stable-status.txt

escape_sed_replacement() {
  printf '%s' "$1" | sed 's/[\\&|]/\\&/g'
}

sed \
  -e "s|@PROJECT_NAME@|$(escape_sed_replacement "$NAME")|g" \
  -e "s|@PROJECT_VERSION@|$(escape_sed_replacement "$VERSION")|g" \
  -e "s|@PROJECT_VERSION_FULL@|$(escape_sed_replacement "$VERSION")|g" \
  -e "s|@PROJECT_VERSION_MAJOR@|$(escape_sed_replacement "$MAJOR")|g" \
  -e "s|@PROJECT_VERSION_MINOR@|$(escape_sed_replacement "$MINOR")|g" \
  -e "s|@PROJECT_VERSION_PATCH@|$(escape_sed_replacement "$REVISION")|g" \
  -e "s|@PROJECT_VERSION_REPOSTAT@|$(escape_sed_replacement "$REPOSITORY_STATUS")|g" \
  -e "s|@PROJECT_DESCRIPTION@|$(escape_sed_replacement "$DESCRIPTION")|g" \
  "$TEMPLATE"
