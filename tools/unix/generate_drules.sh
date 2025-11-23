#!/usr/bin/env bash
set -e -u

OMIM_PATH="${OMIM_PATH:-$(dirname "$0")/../..}"
DATA_PATH="${DATA_PATH:-$OMIM_PATH/data}"

function BuildDrawingRules() {
  styleType=$1
  styleName=$2
  suffix=${3-}
  echo "Building drawing rules for style $styleType/$styleName"
  # Cleanup old compiled drules and diff
  rm "$DATA_PATH"/drules_proto$suffix.{bin,txt.diff} || true
  # Store old txt version for diff
  mv -f "$DATA_PATH"/drules_proto$suffix.txt{,.prev} || true
  # Run script to build style
  python3 "$OMIM_PATH/tools/kothic/src/libkomwm.py" --txt \
    -s "$DATA_PATH/styles/$styleType/$styleName/style.mapcss" \
    -o "$DATA_PATH/drules_proto$suffix" \
    -p "$DATA_PATH/styles/$styleType/include/"
  # Output diff and store to a file
  if [ -f "$DATA_PATH/drules_proto$suffix.txt.prev" ]; then
    diff -u "$DATA_PATH/drules_proto$suffix.txt.prev" "$DATA_PATH/drules_proto$suffix.txt" > "$DATA_PATH/drules_proto$suffix.txt.diff" || true
  fi
}

outputs=(classificator.txt types.txt visibility.txt colors.txt patterns.txt drules_proto.txt)
# Store old versions for diffs
for item in ${outputs[*]}
do
  if [ -f "$DATA_PATH/$item" ]; then
    mv -f "$DATA_PATH/$item" "$DATA_PATH/$item.prev"
  fi
done

# Building drawing rules
BuildDrawingRules default  light _default_light
BuildDrawingRules default  dark _default_dark
BuildDrawingRules outdoors  light _outdoors_light
BuildDrawingRules outdoors  dark _outdoors_dark
# Keep vehicle style last to produce same visibility.txt & classificator.txt
BuildDrawingRules vehicle  light _vehicle_light
BuildDrawingRules vehicle  dark _vehicle_dark

# TODO: the designer is not used at the moment.
# In designer mode we use drules_proto_design file instead of standard ones
# cp $OMIM_PATH/data/drules_proto_default_light.bin $OMIM_PATH/data/drules_proto_default_design.bin

echo "Exporting transit colors..."
python3 "$OMIM_PATH/tools/python/transit/transit_colors_export.py" \
  "$DATA_PATH/colors.txt" > /dev/null

# Merged drules_proto.bin is used by the map generator.
# It contains max visibilities (min visible zoom) for features across all styles.
echo "Merging styles..."
python3 "$OMIM_PATH/tools/python/stylesheet/drules_merge.py" \
  "$DATA_PATH/drules_proto_default_light.bin" \
  "$DATA_PATH/drules_proto_vehicle_light.bin" \
  "$DATA_PATH/drules_proto_outdoors_light.bin" \
  "$DATA_PATH/drules_proto.bin" \
  "$DATA_PATH/drules_proto.txt" \
   > /dev/null

# Output diffs and store to files
for item in ${outputs[*]}
do
  if [ -f "$DATA_PATH/$item.prev" ] && [ -f "$DATA_PATH/$item" ]; then
    diff -u "$DATA_PATH/$item.prev" "$DATA_PATH/$item" > "$DATA_PATH/$item.diff" || true
  else
    echo "Skipping diff for $item (first run or file missing)"
  fi
done

echo "Diffs for all changes are stored in $DATA_PATH/*.txt.diff"
