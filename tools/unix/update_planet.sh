#!/usr/bin/env bash
set -euxo pipefail

OSMUPDATE=~/osmctools/osmupdate
# osmconvert should be accessible in PATH.
PATH="$(dirname "$OSMUPDATE"):$PATH"

# Pass pbf or o5m file as a parameter
OLD="$1"
NEW="${1/.pbf/.new.pbf}"
NEW="${NEW/.o5m/.new.o5m}"

"$OSMUPDATE" -v --drop-authors --drop-version --hash-memory=512000 "$OLD" "$NEW"
# Uncomment to replace old planet.
mv "$NEW" "$OLD"
#md5sum -b "$OLD" > "$OLD.md5"
echo "Successfully updated $OLD"
