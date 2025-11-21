#!/usr/bin/env bash
#
# Linking relnotes from F-Droid to Google Play
#

set -e -u

REPO_PATH="$(cd "$(dirname "$0")/../.."; pwd -P)"
ANDROID_PATH="$REPO_PATH/android/app/src"
GPLAY_PATH="$ANDROID_PATH/google/play/release-notes"

pushd $ANDROID_PATH >/dev/null

echo "Deleting all GPlay relnotes symlinks in $GPLAY_PATH"
rm -rf $GPLAY_PATH
mkdir -p $GPLAY_PATH

pushd fdroid/play/listings >/dev/null

echo "Symlinking to F-Droid relnotes in $(pwd)"

for loc in */; do
  if [ -f "$loc/release-notes.txt" ]; then
    echo "Adding $loc relnotes"
    pushd ../../../google/play/release-notes >/dev/null
    mkdir -p $loc
    cd $loc
    ln -sT "../../../../fdroid/play/listings/$loc"release-notes.txt default.txt
    popd >/dev/null
  fi
done

popd >/dev/null
popd >/dev/null

exit 0
