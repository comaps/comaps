# French National Library Archiving

The library has taken an interest in archiving CoMaps and its data as a snapshot
of our world and the way people interact with maps, in a way that doesn't rely on
maintaining servers etc. (With an APK and MWM files and some copy-paste, you can
reproduce our app on an emulator etc.)

## Instructions

Every 6 months or so, @jeanbaptisteC may ask to upload the most recent map version
and a custom APK with bundled World map (googleRelease) with production keys (like web release).

Credentials for `frlibrary` are in the mapgen rclone, or in zyphlar/pastk's password managers.

To upload (modify dates accordingly):

```
rclone copy CoMaps-25110702-google-release.apk frlibrary:/apk/
rclone copy 251104 frlibrary:/maps/251104
```