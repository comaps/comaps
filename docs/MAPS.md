# Map generation process

Please refer to maps_generator tool [instructions](../tools/python/maps_generator/README.md). 



# More knowledge about the internal magics ...

Here some extra information that can help you to understand the internal mechanisms.

## Tags manipulation
The elements in the input osm .pbf file are associated with Tags ( like  highway=path  )

Comaps will rework those tags
- Keep or Modify them.
- Create new tags ( eg:  tag `_path_grade` is a combination of `sac_scale` and `trail_visibility` ).

This is done by:
- configuration files ( see `data/replaced_tags.txt` )
- C++ code ( see `generator/osm2type.cpp`  and also `generator/relation_tags.cpp` )

Notes:
- The processing of Relation elements, can also copy the relation 's attributes to the associated point and line elements.
- The next chapters explain how those "tags" will be  transformed into "Feature-Types" ... 

## Styles

Style definitions are in `data/styles` .

Look at the document [STYLES.md](STYLES.md) for a detailed description, and especially:
- the existence of different styles: `default` , `outdoors` ...
- the fact that each style provides a `dark` and `light` mode.
- the organization into sub-directories.



## File mapcss-mapping.csv and Feature-Types

When a rule like `line|z11-[highway=path][_path_grade=expert]` is found in a .mapcss style definition, the Generator expects to find a corresponding pattern in mapcss-mapping.csv .

`highway|path|expert ; [highway=path][_path_grade=expert] ;  ;  name  ; int_name ; 465`

`highway-path-expert` is the "feature-type" associated with the condition `[highway=path][_path_grade=expert]`    ( | is replaced by - )
It is given ID 465 .   
This ID, named "type id" is its line index in the generated file `data/types.txt`

This notion is important, because in the final map, elements do not contain the original tags ( highway , _path_grade .... ), but only the ID of their feature-types.   
(That's why the mapcss-mapping.csv file is not used by the running application)

In the header lines of mapcss-mapping.csv, you will find a detailed description, and the different possible syntaxes to use.

Note:  Most entries in types.txt begin by * . This is to differentiate them from types issued from a replacement mechanism.

## Strategy for defining Feature-Types

A Feature-Type like `highway-path`  is the first importance level, because it gives a clear indication of what the element is.

Using a secondary level `highway-path-expert`  allows to refine this description, in order to use a special color rendering for such paths.

It we want to show something concerning the surface type, we will not create something like `highway-path-expert-surfacepavedgood`  because the combination of possible cases would be huge...

In this case, we rather define another separated feature-type  `psurface-paved_good`  corresponding to the condition `[psurface=paved_good]`   
(Well, in this case, this feature-type was defined for routing management, not rendering)

Concretely, it is very rare to find more than 2 levels.

**Caution !**   
The .mapcss format allows to define conditions and associate them to styles definitions...   
You might believe that you are free to assemble those conditions as you like... This is not true !

If you have defined 2 independant feature types `highway|path;[highway=path]`  and `routebike|mtb;[routebike=mtb]`  in mapping-mapcss.csv,   
you cannot write a style condition like `[highway=path][routebike=mtb]{ width:10;}`  because the combination `[highway=path][routebike=mtb]` does not directly match a feature-type, and the map drawer will not try to check that the element contains both the individual feature-types `highway|path`  and `routebike|mtb`    
In this case, the Generator may not complain when compiling the rules, but the map renderer will complain and do nothing.

Have a look at the proto*.txt ( see STYLES.md ) to get a clear view of the drawing rules that will be applied !

## Elements in the  .mwm map
The elements in the final .mwm map are named "features" .  

They contain the IDs of their features-types, and also the value of a few standard attributes ( name,ref ...)

There again, a simplification is done: if an element in the .mwm file,  has the type `highway-path-expert` , its list of feature types does not include `highway-path` ...   
But this type is automatically inherited. This means:
- If you use some code to check whether the element matches the feature-type `highway-path`, the answer will be yes.
- If you define no style rule for `highway-path-expert`, the element will nevertheless contain the style rules defined for `highway-path`


You can dump a .mwm file to see the list of features types associated to elements ( with their name ):

```
cd tools/python
python3 -m mwm dump_mwm  --format json --need_features  --path pathname-of-mwm-file
```

Another tool only gives statistics (like how often a feature-type is used)
```
../omim-build-release/generator_tool --stats_general --stats_geometry --stats_types --dump_types --data_path=data/YYMMDD/ --output="Andorra" 
```

## Rendering ##

What happens if an element in the .mwm file has several feature-types, each having an associated style ?   
It will be drawn several times ! in the order defined by the priorities files.







