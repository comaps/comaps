#!/bin/bash
set -euo pipefail
cd /tmp
date -u

CODEBERG=https://codeberg.org/api/v1/repos/comaps/comaps
PLAY=https://androidpublisher.googleapis.com/androidpublisher/v3/applications/app.comaps.google
PLAY_UPLOAD=https://androidpublisher.googleapis.com/upload/androidpublisher/v3/applications/app.comaps.google

artifact=$(curl -sSf --retry 3 "$CODEBERG/actions/artifacts?name=google-release-bundle" | jq -c 'map(select(.expired | not)) | max_by(.id)')
if [ "$artifact" = null ]; then
  echo "No bundle"
  exit 0
fi
run_id=$(echo "$artifact" | jq -r .run_id)
url=$(echo "$artifact" | jq -r .archive_download_url)

run=$(curl -sSf --retry 3 "$CODEBERG/actions/runs/$run_id")
echo "$run" | jq -e '.workflow_id == "android-google-internal.yaml" and .prettyref == "main" and .status == "success"
  and (.event == "schedule" or .event == "workflow_dispatch")' > /dev/null || { echo "Not trusted: run $run_id"; exit 1; }

curl -sSfL --retry 3 -o artifact.zip "$url"
unzip -q -j artifact.zip '*.aab' '*/mapping.txt'
aab=$(ls *.aab)
code=$(echo "$aab" | cut -d- -f2)

key=$(jq -r .private_key /run/secrets/google_play_json)
email=$(jq -r .client_email /run/secrets/google_play_json)
now=$(date +%s)
b64() { openssl base64 -A | tr '+/' '-_' | tr -d '='; }
jwt=$(echo -n '{"alg":"RS256","typ":"JWT"}' | b64).$(echo -n "{\"iss\":\"$email\",\"scope\":\"https://www.googleapis.com/auth/androidpublisher\",\"aud\":\"https://oauth2.googleapis.com/token\",\"iat\":$now,\"exp\":$((now + 600))}" | b64)
jwt=$jwt.$(echo -n "$jwt" | openssl dgst -sha256 -sign <(echo "$key") | b64)
token=$(curl -sSf --retry 3 -d grant_type=urn:ietf:params:oauth:grant-type:jwt-bearer -d "assertion=$jwt" https://oauth2.googleapis.com/token | jq -r .access_token)
echo "Authorization: Bearer $token" > auth

edit=$(curl -sS --fail-with-body -H @auth -H 'Content-Type: application/json' -d '{}' "$PLAY/edits" | jq -r .id)
latest=$(curl -sS --fail-with-body -H @auth "$PLAY/edits/$edit/bundles" | jq '[.bundles[]?.versionCode] | max // 0')
if [ "$code" -le "$latest" ]; then
  echo "Play already has $latest, not uploading $code"
  exit 0
fi

zip -q -d "$aab" 'META-INF/*'
export STORE_PASS=$(sed -n 's/^STORE_PASS=//p' /run/secrets/upload_key.env)
export KEY_PASS=$(sed -n 's/^KEY_PASS=//p' /run/secrets/upload_key.env)
alias=$(sed -n 's/^KEY_ALIAS=//p' /run/secrets/upload_key.env)
jarsigner -keystore /run/secrets/upload_keystore -storepass:env STORE_PASS -keypass:env KEY_PASS "$aab" "$alias"

uploaded=$(curl -sS --fail-with-body -H @auth -H 'Content-Type: application/octet-stream' -X POST -T "$aab" \
  "$PLAY_UPLOAD/edits/$edit/bundles?uploadType=media" | jq -r .versionCode)
curl -sS --fail-with-body -H @auth -H 'Content-Type: application/octet-stream' -X POST -T mapping.txt \
  "$PLAY_UPLOAD/edits/$edit/apks/$uploaded/deobfuscationFiles/proguard?uploadType=media" > /dev/null
name="20${uploaded:0:2}.${uploaded:2:2}.${uploaded:4:2}-$((10#${uploaded:6:2}))"
curl -sS --fail-with-body -H @auth -H 'Content-Type: application/json' -X PUT \
  -d "{\"releases\": [{\"name\": \"$name\", \"versionCodes\": [\"$uploaded\"], \"status\": \"completed\"}]}" "$PLAY/edits/$edit/tracks/internal" > /dev/null

if [ -n "${DRY_RUN:-}" ]; then
  curl -sS --fail-with-body -H @auth -X POST -d '' "$PLAY/edits/$edit:validate" > /dev/null
  echo "Validated $name ($uploaded)"
else
  curl -sS --fail-with-body -H @auth -X POST -d '' "$PLAY/edits/$edit:commit" > /dev/null
  echo "Published $name ($uploaded)"
fi
