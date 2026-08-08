# Map styling and icons

Here is the basic workflow to update styles:
1. Edit the styles file you want, e.g. [`Roads.mapcss`](../data/styles/default/include/Roads.mapcss)
2. Rebuild the app or run the `tools/unix/generate_drules.sh` script
3. Test how your changes look in the app
4. Commit your edits
5. Send a pull request!

Please prepend `[styles]` to your commit message and add [Developers Certificate of Origin](CONTRIBUTING.md#legal-requirements) to it.

Please check [a list of current styling issues](https://codeberg.org/comaps/comaps/issues?q=&type=all&sort=&labels=410439)
and ["icons" issues](https://codeberg.org/comaps/comaps/issues?q=&type=all&sort=&labels=446883).

An overview of currently used icons can be found in the [Wiki](https://codeberg.org/comaps/comaps/wiki/Icons).

## Requirements

To work with styles first [clone the CoMaps repository](INSTALL.md#getting-sources)
and run `./configure.sh`. That sets up a local `.venv` in the repository root with
the required Python `protobuf` version, which the style generation scripts pick up
automatically. To manage `protobuf` via your system Python instead, set
`SKIP_PYTHON_VENV=1` before running `./configure.sh`.

To run the `generate_symbols.sh` script, you need to install `optipng`, e.g. for Ubuntu
```
sudo apt install optipng
```

If you use WSL on Windows 10 you might need to run [X Server](INSTALL.md#windows-10-wsl) before running `generate_symbols.sh`

## Styles directories and files

Map styles are defined in text files located in `data/styles/`:

**There are 3 subdirectories**:
* `default`: the default style
* `outdoors`: the outdoors specific style, which inherits from `default` ( outdoors style files include default style files, then add their own rules )
* `vehicle`: specific to the navigation mode

Each style directory has again 3 parts:
* `light`:  entry point style.mapcss and colors definitions for the "light mode", which is the default
* `dark`: same thing for the "dark mode"
* `include` :  style definitions .mapcss files  and priorities files

**The style definitions files**:
* Forests, rivers, buildings, etc. [`Basemap.mapcss`](../data/styles/default/include/Basemap.mapcss)
* Their text labels [`Basemap_label.mapcss`](../data/styles/default/include/Basemap_label.mapcss)
* Roads, bridges, foot and bicycle paths, etc. [`Roads.mapcss`](../data/styles/default/include/Roads.mapcss)
* Their text labels [`Roads_label.mapcss`](../data/styles/default/include/Roads_label.mapcss)
* Icons for POIs and other features [`Icons.mapcss`](../data/styles/default/include/Icons.mapcss)
* City-specific subway networks [`Subways.mapcss`](../data/styles/default/include/Subways.mapcss)

**Priorities files**:   
They describe the rendering priority of each feature-type.   
A high priority number means that the feature-type is rendered **after** the others.   
Look at the detailed description in the header of each file, to get a good understanding of their role, and format.   
* Priorities of overlays (icons, captions..) [`priorities_4_overlays.prio.txt`](../data/styles/default/include/priorities_4_overlays.prio.txt)
* Priorities of lines and areas [`priorities_3_FG.prio.txt`](../data/styles/default/include/priorities_3_FG.prio.txt), [`priorities_2_BG-top.prio.txt`](../data/styles/default/include/priorities_2_BG-top.prio.txt), [`priorities_1_BG-by-size.prio.txt`](../data/styles/default/include/priorities_1_BG-by-size.prio.txt)


**Icons**:   
They are stored in [`data/styles/default/light/symbols/`](../data/styles/default/light/symbols/) and their dark/night counterparts are in [`data/styles/default/dark/symbols/`](../data/styles/default/dark/symbols/).

## How to add a new icon

1. Add an svg icon to `data/styles/default/light/symbols/` (and to `dark` too)
preferably look for icons in [collections CoMaps uses already](../data/copyright.html#icons)
2. Add icon rendering/visibility rules into `data/styles/default/include/Icons.mapcss` and to "navigation style" `data/styles/vehicle/include/Icons.mapcss`
3. Rebuild the app
    * Or run `tools/unix/generate_symbols.sh` to add new icons into skin files and `tools/unix/generate_drules.sh` to generate drawing rules for the new icons
5. [Test](#testing-your-changes) your changes

## How to add a new map feature / POI type

1. Add it into `data/mapcss-mapping.csv` (or better replace existing `deprecated` line) to make CoMaps import it from OSM
2. If necessary merge similar tags in via `data/replaced_tags.txt`
3. Define a priority for the new feature type in e.g. [`priorities_4_overlays.prio.txt`](../data/styles/default/include/priorities_4_overlays.prio.txt) and/or other priorities files
4. Add a new icon (see [above](#how-to-add-a-new-icon)) and/or other styling (area, line..)
5. If a new POI should be OSM-addable/editable then add it to `data/editor.config`
6. Add the English string (and optionally translations e.g. for your native language) into iOS and Android type strings
e strings
7. Add search keywords into `data/categories.txt`
8. Add new or fix current classifier tests at `generator/generator_tests/osm_type_tests.cpp` if you can
9. [Test](#testing-your-changes) your changes
10. Relax and wait for the next maps update :)

## Testing your changes

The most convenient way is using [the desktop app](INSTALL_DESKTOP.md#desktop-app).
There is a "Designer" version of it also, which facilitates development
by rebuilding styles and symbols quickly. See the guide referenced above to find
details on how to build and execute it.

To test on Android or iOS device either re-build the app or put
the compiled style files (e.g. `drules_proto_default_light.bin`) into
a `styles/` subfolder of maps directory on the device
(e.g. `Android/data/app.comaps/files/styles/`).

Changing display zoom level for features (e.g. from z16- to z14-) might
not take effect until map's visibility/scale index is rebuilt:
1. [Build](INSTALL.md#desktop-app) the `generator_tool` binary
2. Put a map file, e.g. `Georgia.mwm` into the `data/` folder in the repository
3. Run
```
../omim-build-release/generator_tool --generate_index=true --output="Georgia"
```
4. The index of `Georgia.mwm` will be updated in place

A whole map needs to be [regenerated](MAPS.md) for the changes to take effect if:
* the visibility change crosses a geometry index boundary
* e.g. `area` style rules are added for a feature that didn't have them before
* a new feature type is added or the mapping of existing one is changed

## Technical details

Map style files syntax is based on [MapCSS/0.2](https://wiki.openstreetmap.org/wiki/MapCSS/0.2),
though the specification is not supported in full and there are CoMaps-specific extensions to it.

The `tools/unix/generate_drules.sh` script uses a customized version of [Kothic](https://codeberg.org/comaps/kothic)
stylesheet processor to compile MapCSS files into binary drawing rules files `data/drules_proto*.bin`.
The processor also produces text versions of these files (`data/drules_proto*.txt`) to ease debugging.

Those binary files, are generated by a Python Serialize operation applied on python structures.   
For each feature type, and for each acceptable zoom level , they contain the styles to apply.      
( looks like: `array[feature-type][zoom] ==> style` )


The `tools/unix/generate_symbols.sh` script assembles all icons into skin files in various resolutions (`data/resources-*/symbols.png` and `symbols.sdf`).
