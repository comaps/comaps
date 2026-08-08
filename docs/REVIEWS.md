# Reviews

This file documents how place reviews are handled in CoMaps. It contains both the ops instructions and pointers
to the relevant parts of the codebase. If you're primarily interested in generating MWM files with reviews, see
the [Operation](#operation) section. If you'd like to make changes to where the reviews are sourced, how they are stored
or displayed, head to [Development](#development).

All commands and paths are specified with the assumption that you are running them in the root directory of the `compas`
git repository.

## Operation

### Overview

```
                   [mangrove dump]
                          |
                          V
                 <mangrove-osm-coder>
                          |
                          V
                    [reviews.json]
                          |
                          V
<maps_generator>: ... |Reviews| ... -> [region.mwm]
```

The review pipeline: a mangrove.reviews [JSON dump](https://docs.mangrove.reviews/#tag/Dump) is processed
by [mangrove-osm-coder](https://codeberg.org/mmakowski/mangrove-osm-coder) which produces a [
`reviews.json` file](https://codeberg.org/mmakowski/mangrove-osm-coder/src/branch/main/README.md#reviews-.son). That
file is then read by the `Reviews` stage of the [maps_generator](../tools/python/maps_generator/README.md).

### Running in Codeberg

The map generation workflow expects the `reviews.json` file to be present in a specific path. To generate that file,
run [mapgen-reviews](../.forgejo/workflows/mapgen-reviews.yml)
workflow [in Codeberg](https://codeberg.org/comaps/comaps/actions). The workflow takes one optional flag: _reload
postgis_. When selected, the workflow will re-import the OSM data existing `planet.o5m` file into the postgis database
used for spatial resolution. Otherwise, new Mangrove data will be downloaded and processed, but the OSM data from the
previous run will be used.

The full workflow with postgis reload can take around 2 hours to run. Without a reload, it should take no more than a
couple of minutes.

### Running Locally

See the [manual end-to-end testing section](#manual-end-to-end-testing).

## Development

The code to support reviews is split into three categories:

1. the core library with the model and serialisation/deserialisation logic;
2. the map generator stage;
3. the app code that reads and displays the reviews.

### The Library

The `libs/indexer/reviews_*` files contain:

* the logical model of reviews;
* the serialisation/deserialisation logic.

They reside in the `reviews` namespace.

To test the library code, run in the root of the project:

```shell
tools/unix/build_omim.sh -d indexer_tests && \                    
  pushd ../omim-build-debug && \
  ./indexer_tests --filter="Reviews"; \  
  popd
```

### Map Generator

The generator processes a single region MWM file at the time. It parses the reviews JSON file, identifies the reviews
which correspond to features in the current region and uses the library to write a `reviews` section to the MWM file.

To test the generation run in the root of the project:

```shell
tools/unix/build_omim.sh -d generator_tests && \
  pushd ../omim-build-debug && \
  ./generator_tests --filter="Reviews"; \
  popd
```

### Apps

#### Common

The review information is stored as a part of `place_page::Info` object defined in the `libs/map` library. The library
depends on [`libs/indexer`](#the-library) for loading the review data.

#### Android

TODO

#### iOS

TODO

#### Qt

The review summaries are displayed in `PlacePageDialogUser` and `PlacePageDialogDeveloper`. They use a custom
`qt::StarRatingWidget` to render the star rating.

### Manual End-to-End Testing

1. prepare a reviews file:
    1. Follow the instructions in
       the [mangrove-osm-coder README](https://codeberg.org/mmakowski/mangrove-osm-coder/src/branch/main/README.md) to
       run the review preprocessing tool. Use a full planet file if possible. If not, configure the region of interest
       in `regions.txt`; `europe/poland/mazowieckie` will allow you to follow the steps of this tutorial exactly.
    2. Once the tool runs (the full import of the entire planet file can take a few hours), copy the output
       `<mangrove-osm-coder>/out/reviews.json` to `../maps-build/reviews/`.
2. build the generator
    ```shell
    tools/unix/build_omim.sh -r generator_tool mwm_diff_tool
    ```
3. configure the generator by following the instructions
   in [tools/python/maps_generator/README.md](../tools/python/maps_generator/README.md), with the following changes:
    1. create and activate a `venv` (for example, with `uv venv`) before running `pip install`
    2. in the `map_generator.ini`, set:
        1. `PLANET_URL: https://download.geofabrik.de/europe/poland/mazowieckie-latest.osm.pbf` to use a region with
           reviewed features;
        2. `REVIEWS_PATH: ${Main:MAIN_OUT_PATH}/reviews/reviews.json`
4. run the generator
    ```shell
    pushd tools/python
    source .venv/bin/activate
    python3 -m maps_generator \
      --countries="Poland_Masovian Voivodeship" \
      --skip="Coastline,CitiesIdsWorld,RoutingWorld,Ugc,Popularity,PopularityWorld,Srtm,IsolinesInfo,Descriptions,Routing,RoutingTransit,MwmDiffs"
   popd
    ```
5. build the desktop app
   ```shell
   tools/unix/build_omim.sh -d desktop
   ```
6. place the generated map files in `data`
    1. `readlink -f data/World.mwm` --- make note of the `<yymmdd>` (for example `.../world_mwm/260421/World.mwm`)
       directory name;
    2. symlink the output directory of the generator as `data/<yyyymmdd>`, for example:
       `ln -s /home/me/Projects/OSS/comaps/maps_build/2026_05_01__10_04_46/260501 data/260421`; whatever map was used
       by the generator, the date in `data` must be on or before the one used by `World.mwm`, otherwise the app won't
       pick up the map.
7. start the app: `../omim-build-debug/CoMaps`, then search for `Tekla`; the top result should be a café with at least
   one review.

### Debugging

#### Generator

As of writing this, the complete map generation workflow requires a release build of the generator tool. After running
it once, we can then switch to a debug build and run individual stages. To use a debug build in the python tool, update
`tools/python/maps_generator/var/etc/map_generator.ini`:

```ini
BUILD_PATH : ${Developer:OMIM_PATH}/../omim-build-debug
```

And build the debug versions of the tools:

```shell
tools/unix/build_omim.sh -d generator_tool mwm_diff_tool
```

To rerun just the _reviews_ stage of the generator, we can use using the python tool:

```shell
python3 -m maps_generator \
  --countries="Poland_Masovian Voivodeship" \
  --skip="Coastline,CitiesIdsWorld,RoutingWorld,Ugc,Popularity,PopularityWorld,Srtm,IsolinesInfo,Descriptions,Routing,RoutingTransit,MwmDiffs" \
  --continue \
  --from_stage=Reviews
```

Alternatively, the `generator_tool` can be run directly to just process the review data (set the `BASE` and `TS`
variables as appropriate for your system and maps build):

```shell
cd ../omim-build-debug
BASE='/home/me/Projects/OSS/comaps/'
TS='2026_04_17__14_03_23'
MAPS_BUILD="$BASE/maps_build/$TS/"
./generator_tool \
  --threads_count=1 \
  --data_path=$MAPS_BUILD/intermediate_data \
  --intermediate_data_path=$MAPS_BUILD/intermediate_data \
  --cache_path=$MAPS_BUILD/intermediate_data \
  --osm_file_type=o5m \
  --osm_file_name=$MAPS_BUILD/planet.o5m \
  --node_storage=map \
  --user_resource_path=$BASE/comaps/data \
  --cities_boundaries_data=$MAPS_BUILD/intermediate_data/cities_boundaries.bin \
  --generate_features=true \
  --addresses_path= \
  --generate_packed_borders=true
```

This is more direct, but also more sensitive to changes in the `generator_tool` command line interface, so if you
struggle to get that to work using the python `maps_generator` tool is a safer option.

#### Qt App

Review loading can be debugged by creating a debug build of the desktop app and running it from within an IDE.
