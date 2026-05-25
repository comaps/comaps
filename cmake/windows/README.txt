CoMaps for Windows - Portable Edition
======================================

To run CoMaps, launch CoMaps.exe from this folder.

DATA STORAGE
------------
Even in the portable edition, CoMaps stores downloaded maps, bookmarks, and
settings in your Windows user profile rather than next to the executable:

  %LOCALAPPDATA%\CoMaps\

This is typically:
  C:\Users\<your name>\AppData\Local\CoMaps\

This location is used regardless of where you place this folder, to avoid
requiring administrator permissions and to prevent data loss if the app
folder is moved or deleted.

To override the writable directory (e.g. to keep everything on a USB drive),
set the MWM_WRITABLE_DIR environment variable before launching:

  set MWM_WRITABLE_DIR=E:\CoMaps-data
  CoMaps.exe

RESOURCES DIRECTORY
--------------------
CoMaps looks for its data\ folder next to CoMaps.exe. Keep the contents of
this archive together — do not move CoMaps.exe out of the folder.

MORE INFORMATION
----------------
https://comaps.app
