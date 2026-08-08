# ISOLINES generation

## Understanding SRTM data
This refers to "elevation data" : files containing the altitude of many points on earth

People  may use different names for this:
- DEM  data ( digital elevation model )
- DTM  data ( digital terrain model )

SRTM  (Shuttle Radar Topography Mission) is just a provider for such data

The standard format is .hgt files, which contain elevation data for points in a square having a one degree longitude width, and one degree latitude width.   
A detailed description of this format can be found at the end of this page : [https://viewfinderpanoramas.org/dem3.html#hgt](https://viewfinderpanoramas.org/dem3.html#hgt)

The name of those files is important ! A file named  N41E003.hgt means that the bottom-left corner of the square is latitude N41 and longitude E3

Two levels of precision exist:
- SRTM1 / DEM1 :  1 arc-second precision : each file contains 3601 x 3601 values  
- SRTM3 / DEM3 :  3 arc-second precision : each file contains 1201 x 1201 values  

There are different providers for .hgt files.  ( often coming from other sources than the original SRTM ) .  Examples for Europe :
- [viewfinderpanorama](https://viewfinderpanoramas.org/Coverage%20map%20viewfinderpanoramas_org1.htm)  Note: this url is not obvious to find from the root of the web site
- [Sonny](https://sonny.4lima.de/)

Note:  some areas in Eastern Europe have only 3 arc-seconds precision files.   
This can be a trouble if your tools work only with 1 arc-second precision, and suddently encounter lower precision files.

## Preparation: compile the topography_generator_tool

```
./tools/unix/build_omim.sh -r topography_generator_tool
```

## Generation for a specific country


### Step1: Download .hgt files

Create a directory like `../SRTM` to store the downloaded files

Find out the areas needed to cover the country you want to generate. Download and uncompress files, so that  the directory contains .hgt files 

Caution! If you do not  get 1 arc-second precision .hgt files, the topography_generator tool will  complain with "bad file size" error...

### Step2: Generate isolines

From the root of the repository:

Create a directory like `../isolines` to contain the isolines which will be generated 

Create a subdirectory like `../isolines/tmp`   to contain intermediate results

Inside directory `data/conf/isolines/` , create 2 files `mycountries.json`  and `myprofiles.json`  from `countries-to-generate.json` and `isolines-profiles.json` 
- `mycountries.json` : keep only the countries/regions you want
- `myprofiles.json` :  if you want to generate 10m isolines, instead of 20m isolines, modify the value `alitudesStep` inside profile named `high`

Go to comaps directory, and launch generation

```
../omim-build-release/topography_generator_tool  --profiles_path=data/conf/isolines/myprofiles.json  --countries_to_generate_path=data/conf/isolines/mycountries.json --tiles_isolines_out_dir=../isolines/tmp/ --countries_isolines_out_dir=../isolines/    --data_dir=data/ --srtm_path=../SRTM/  --threads=96 
```

### Step3: Make Generator import isolines to the map

The Generator configuration file `map_generator.ini` : must be modified so that it imports isolines into the map:   
Declare directories used for isolines generation:
- SRTM_PATH : directory containing .hgt files
- ISOLINES_PATH : directory containing the generated isolines

In our example:
```
SRTM_PATH: ${Developer:OMIM_PATH}/../SRTM
ISOLINES_PATH: ${Developer:OMIM_PATH}/../isolines
```
