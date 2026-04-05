# How to edit hierarchy.txt

The hierarchy.txt file is used to define how each map files is structured on the map downloader screen on different app versions.
This file can be updated without updates any borders of map files.
This doc doesn't explain how to split or merge two map files or more because the app doesn't have any feature to know how to manage updates if map files have changed.

### Structure

The indentation defines the level of the country or the region.
World, WorldCoasts and each country don't have any space and they are all visible on the map downloader screen by default.
If countries or regions are split into more smaller map files, the map file needs to be added under the region/country with an indentation (a space).
This file can contain region names without map files like country or region names with map files like departments in France.

Example:
```
Algeria;Q262;dz;ar
 Algeria_Central;Q262-Centre
```
 
 All lines need to be written in this format
 [Name of the country or region];[wikidata tag];[main language];[other languages used seperate by a semicolon]
 
 Or this for map files that don't follow official regions (official region split in two map files)
  [Name of the country or region];[wikidata tag-North|Center|South|West|East];[main language];[other languages used seperate by a semicolon]

For map files, the name of the region needs to match with the name of [poly files](https://codeberg.org/comaps/comaps/src/branch/main/data/borders)

### How to rename regions or countries

- For regions or countries that don't have map files
  - Rename the region in hierarchy.txt and update the wikidata tag if necessary
  - Change the id and rename the region in each file in [countries-strings](https://codeberg.org/comaps/comaps/src/branch/main/data/countries-strings)
  
- For regions or countries that have map files
  - Update wikidata tag if necessary and continue to keep the same name as the poly file
  - Rename the region in each file in [countries-strings](https://codeberg.org/comaps/comaps/src/branch/main/data/countries-strings)
  
  
### How to subdivide, merge or move regions

#### Subdivide without map border changes
- For the main region, add a new line with an indentation
- Add the new region with the format explained above
- Move region map files under with an indentation
- Add new region translations and reorder translations in each [countries-strings files](https://codeberg.org/comaps/comaps/src/branch/main/data/countries-strings)

#### Merge without map border changes
- Remove the line about the region that you want to remove
- Update indentation of subregions
- Remove unused translations and reorder translations in each [countries-strings files](https://codeberg.org/comaps/comaps/src/branch/main/data/countries-strings)

#### Move without map border changes
- Move and update the indentation of each line that you want to move
- Reorder translations in each [countries-strings files](https://codeberg.org/comaps/comaps/src/branch/main/data/countries-strings)

You can find more examples here:
- https://codeberg.org/comaps/comaps/pulls/3191
- https://codeberg.org/comaps/comaps/pulls/3456
