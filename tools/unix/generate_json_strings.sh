#!/usr/bin/env bash
set -euo pipefail

if ! command -v jq &> /dev/null
then
    echo -e "\033[1;31mjq could not be found"
    echo 'take a look at https://jqlang.org/download/'
    exit 1
fi


OMIM_PATH="${OMIM_PATH:-$(cd "$(dirname "$0")/../.."; pwd)}"
cd "${OMIM_PATH}"

# Most of the overhead is just startup of processes, don't worry too much about starting many processes
JOBS=8

echo "Starting..."
# copy symlinks verbatim (they are all relative)
find data/translations/ -type l -name 'localize.json' -exec bash -c '
mkdir -p "$(dirname "$(echo "$1" | sed "s|^data/translations/|data/|")")"
cp -d "$1" "$(echo "$1" | sed "s|^data/translations/|data/|")"
' inline-shell '{}' \;
# convert data
find data/translations/ -type f -name 'localize.json' -print0 | xargs -P"${JOBS}" -0 -I'{}' bash -c '
mkdir -p "$(dirname "$(echo "$1" | sed "s|^data/translations/|data/|")")"
jq -c '"'"'map_values(.defaultMessage)'"'"' "$1" > "$(echo "$1" | sed "s|^data/translations/|data/|")"
' inline-shell '{}' \;
echo "Finished"
