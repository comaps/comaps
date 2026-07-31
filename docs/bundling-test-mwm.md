# Bundling a test MWM in the debug APK

> TODO: follow the Splitting instructions below to move this out of this branch before merge

Test maps shipped with the debug APK land in
`android/app/src/debug/assets/` and are extracted into the app's versioned
map directory at first launch by `extractBundledMaps()` in
`android/sdk/src/main/java/app/organicmaps/sdk/OrganicMaps.java`.

---

## 1 — Find the right CoMaps region

Target coordinates must fall inside an official region whose ID is in
`data/countries.txt`.  The poly files for all regions live in
`data/borders/`.

```bash
python3 - <<'EOF'
def pip(lon, lat, coords):
    n, inside, j = len(coords), False, len(coords)-1
    for i in range(n):
        xi,yi = coords[i]; xj,yj = coords[j]
        if ((yi>lat)!=(yj>lat)) and lon<(xj-xi)*(lat-yi)/(yj-yi)+xi:
            inside = not inside
        j = i
    return inside

import glob, os
target_lon, target_lat = 2.59664, 48.83862
for f in sorted(glob.glob('data/borders/France_Ile-de-France_*.poly')):
    coords = []
    with open(f) as fp:
        for line in list(fp)[2:]:
            line = line.strip()
            if line in ('END',''): break
            parts = line.split()
            if len(parts)==2:
                coords.append((float(parts[0]),float(parts[1])))
    print(os.path.basename(f).replace('.poly',''), pip(target_lon,target_lat,coords))
EOF
```

---

## 2 — Download the Geofabrik parent region PBF

```bash
GENDIR=/tmp/seine_marne
mkdir -p "$GENDIR"
curl -L -A 'CoMaps-MWM-Generator/1.0' \
  'https://download.geofabrik.de/europe/france/ile-de-france-latest.osm.pbf' \
  -o "$GENDIR/ile-de-france.osm.pbf"
```

---

## 3 — Extract the target sub-region with osmium

```bash
MWMNAME=France_Ile-de-France_Seine-et-Marne
osmium extract \
  --polygon "data/borders/$MWMNAME.poly" \
  "$GENDIR/ile-de-france.osm.pbf" \
  -o "$GENDIR/region.osm.pbf" --overwrite
osmium cat "$GENDIR/region.osm.pbf" -o "$GENDIR/region.osm" --overwrite
```

---

## 4 — Generate the MWM

`generator_tool` needs:
- The OSM XML on **both** the `--preprocess` pass and the `--generate_features` pass
  (`--osm_file_name` must be given to the second invocation or it falls back to
  stdin and reads nothing).
- The border poly files in a **Linux-native path** (CRLF endings from the
  Windows-side `/mnt/c/` mount break parsing — strip them with `sed 's/\r//'`).

```bash
GENBUILD=/home/will/genbuild-rel
DATAPATH=/mnt/c/Users/Will/apps/comaps/data

# Prepare a clean borders dir on the Linux filesystem
mkdir -p /tmp/seine_data/borders
sed 's/\r//' "$DATAPATH/borders/$MWMNAME.poly" > /tmp/seine_data/borders/$MWMNAME.poly

mkdir -p /tmp/seine_marne/intermediate /tmp/seine_marne/intermediate/tmp

# Pass 1: preprocess
$GENBUILD/generator_tool \
  --user_resource_path="$DATAPATH/" \
  --intermediate_data_path=/tmp/seine_marne/intermediate/ \
  --osm_file_type=xml --osm_file_name="$GENDIR/region.osm" \
  --preprocess

# Pass 2: features + geometry + index
$GENBUILD/generator_tool \
  --user_resource_path="$DATAPATH/" \
  --intermediate_data_path=/tmp/seine_marne/intermediate/ \
  --data_path=/tmp/seine_data/ \
  --osm_file_type=xml --osm_file_name="$GENDIR/region.osm" \
  --generate_features --generate_geometry --generate_index \
  --output=$MWMNAME
```

Output: `/tmp/seine_data/$MWMNAME.mwm`

---

## 5 — Add to the debug APK

```bash
cp /tmp/seine_data/$MWMNAME.mwm \
   android/app/src/debug/assets/$MWMNAME.mwm
```

Then register it in `extractBundledMaps()` in `OrganicMaps.java`:

```java
bundledMaps.put("France_Ile-de-France_Seine-et-Marne.mwm",
                "France_Ile-de-France_Seine-et-Marne.mwm");
```

When the asset filename already matches the `countries.txt` ID, both map to the
same string.  For custom/renamed test maps (e.g. `Berlin.mwm` →
`Germany_Berlin.mwm`) the right-hand value must match the entry in
`countries.txt` exactly so the Storage registers it as a real local file rather
than a fake country.

---

## Notes

- `--generate_search_index` was intentionally omitted; it tries to read all
  other MWM files in the data path for cross-MWM search and can hang when
  large maps (IleDeFrance 239 MB) are present.  The missing `addr` and
  `centers` sections would abort the app in debug builds — those `LOG(LERROR)`
  calls in `libs/search/house_to_street_table.cpp` and
  `libs/search/lazy_centers_table.cpp` have been patched to `LWARNING`.
- `--generate_routing` is also omitted.  Without a routing graph the app cannot
  plan routes through this MWM and every routing call throws an exception.
  The routing library (`async_router`, `index_graph_loader`, `index_router`,
  `city_roads`, `maxspeeds`, `restriction_loader`, `transit_graph_loader`,
  `index_road_graph`) logged these at `LERROR`, which in debug builds
  (`g_LogAbortLevel = LERROR`) caused an immediate abort.  All have been
  patched to `LWARNING` so routing returns a "route not found" error instead.
- `data/borders/*.poly` files use CRLF on Windows checkouts.  The
  `generator_tool` border parser does not strip `\r`, so always copy through
  `sed 's/\r//'` before use on Linux/WSL.

---

## Style changes — regenerating drule binaries

After any edit to `data/styles/` (MapCSS or priority files) you must regenerate
the drule binaries and copy them into the Android assets:

```bash
# From the repo root (Git Bash or WSL with Python 3 + protobuf installed)
PYTHONUTF8=1 bash tools/unix/generate_drules.sh
```

The script writes directly into `data/` which is symlinked to
`android/sdk/src/main/assets/`, so the updated `.bin` files are picked up by
the next Gradle build automatically.  The Gradle build itself skips drule
generation ("Skipping generate drules…"), so **you must run this manually**
after every style change.

New OSM tag values used as drule type names (e.g. `indoor=stairs`) must also be
added to `data/mapcss-mapping.csv` with a sequential type ID; otherwise kothic
reports "no style defined" and the type is silently dropped.  After editing
`mapcss-mapping.csv`, add the new type to the `priorities_3_FG.prio.txt` in
**every** style family that references it (`default/`, `outdoors/`) or kothic
will fail with a validation error.

---

## Splitting "bundle test MWMs with the APK" into its own branch (future work)

The APK-bundling of test MWMs is **generic dev tooling** that rode along on the
indoor-mapping feature branch (`zy-indoor-mapping`). It is not part of the indoor
feature and should not ship in that PR. This section is a self-contained plan to
move it to a standalone branch. **Leave it in place for now** — do this only when
splitting the indoor PR.

### What "bundle test MWMs with the APK" actually is

Exactly one code file plus this doc (the map binaries themselves are already
local-only / untracked):

- **Code — `android/sdk/src/main/java/app/organicmaps/sdk/OrganicMaps.java`:**
  - the call `extractBundledMaps(writablePath, mContext.getAssets());` (right after
    `createPlatformDirectories(...)` in the platform-init method);
  - the private method `extractBundledMaps(...)` (its Javadoc + body);
  - the private helper `readCountriesVersion(...)` (used only by that method —
    confirm with a grep before moving);
  - any `import`s used only by the above (candidates: `LinkedHashMap`, `Map`,
    `AssetManager`, `BufferedReader`, `InputStreamReader`, `InputStream`,
    `FileOutputStream`, `java.util.regex.Matcher`, `java.util.regex.Pattern`) —
    verify each is otherwise unused before adding/removing.
- **Doc — `docs/bundling-test-mwm.md`** (this file).
- **`.gitignore`** — the test-MWM ignore line(s) added for this
  (e.g. `android/app/src/debug/assets/France_Ile-de-France_Seine-et-Marne.mwm`);
  `grep -nE 'mwm|debug/assets' .gitignore` to find them.

### What does NOT move (stays with the indoor feature)

- `generator/generator_tests/osm_type_test.cpp` — the `OsmType_Indoor` test
  (verifies `indoor=*` + `level=*` → type + `FMD_LEVEL`). It is a feature test, not
  bundling. **Keep it.**
- The actual `.mwm` files and symlinks under `android/app/src/debug/assets/` and
  `data/`, and the `*.local.sh` generation scripts — all already untracked; leave
  them exactly where they are on both branches.

### Why not `git cherry-pick`

The bundling work is spread across ~9 interleaved commits mixed with unrelated
indoor work (`6599665244`, `bed900c01c`, `212c82c393`, `df4e596a28`, `e6af9a1611`,
`7b4cd96520`, `8535dce7fc`, `d79dd6b9c2`, and `6ae9d8ec7d` which removed a related
scratch test). Cherry-picking would drag in unrelated hunks. Do a **file/hunk-based**
move instead.

### Step 1 — create the standalone branch from `main`

```bash
git switch main && git pull
git switch -c zy-bundle-test-mwms
```

### Step 2 — bring the doc over

```bash
git checkout zy-indoor-mapping -- docs/bundling-test-mwm.md
# optional: trim this "Splitting…" section on the new branch — it's no longer future work there
```

### Step 3 — bring the code over (by hand, not a whole-file checkout)

`OrganicMaps.java` on `zy-indoor-mapping` may contain other indoor changes, so do
**not** `git checkout` the whole file. Instead, view the branch version and copy
only the three blocks into `main`'s `OrganicMaps.java`:

```bash
git show zy-indoor-mapping:android/sdk/src/main/java/app/organicmaps/sdk/OrganicMaps.java
```

Add: (a) the `extractBundledMaps(...)` call after `createPlatformDirectories(...)`,
(b) the `extractBundledMaps` method + `readCountriesVersion` helper, (c) only the
imports those need. Compile (`bash build-android.local.sh`) to catch missing/extra
imports.

### Step 4 — bring the `.gitignore` line(s) and commit

Add the test-MWM ignore entries, then commit with a clear message
(`[Android] Bundle test MWMs from debug assets on first launch (dev tooling)`).
Keep the `.mwm` binaries local-only per the sections above.

### Step 5 — remove it from `zy-indoor-mapping` (only when actually splitting)

1. In `OrganicMaps.java`: delete the `extractBundledMaps(...)` call, the
   `extractBundledMaps` method, the `readCountriesVersion` helper, and any imports
   left unused.
2. Remove the test-MWM `.gitignore` entries added for this.
3. Either `git rm docs/bundling-test-mwm.md` or keep it (duplicated across branches —
   your call).
4. Leave the untracked local `.mwm` files/symlinks alone.

### Verification

- New branch: a debug APK bundles a local test MWM and it appears in-app on first
  launch with **no** download prompt.
- Indoor branch after removal: `grep -rn extractBundledMaps android/` is empty, the
  debug APK still builds, and Android lint is clean (no unused imports).
