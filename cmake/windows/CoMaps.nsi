; NSIS installer script for CoMaps (Windows x64)
;
; Expects to be compiled with the staging\ directory as the working directory,
; i.e. run from the repo root as:
;
;   makensis /DVERSION=<ver> cmake\windows\CoMaps.nsi
;
; where <ver> is the display version string, e.g. "2026.05.22".
; The staging\ directory must already be populated by the CI build steps.

Unicode True

!ifndef VERSION
  !define VERSION "0.0.0"
!endif

Name "CoMaps"
OutFile "..\CoMaps-windows-x64-setup.exe"
InstallDir "$PROGRAMFILES64\CoMaps"
InstallDirRegKey HKLM "Software\CoMaps" "InstallDir"
RequestExecutionLevel admin

VIProductVersion "${VERSION}.0"
VIAddVersionKey "ProductName"     "CoMaps"
VIAddVersionKey "ProductVersion"  "${VERSION}"
VIAddVersionKey "FileVersion"     "${VERSION}.0"
VIAddVersionKey "FileDescription" "CoMaps Installer"
VIAddVersionKey "LegalCopyright"  "Copyright 2026 The CoMaps Community"

; Modern UI + process close support
!include "MUI2.nsh"
!include "x64.nsh"

!define MUI_ICON   "..\..\qt\res\windows\windows.ico"
!define MUI_UNICON "..\..\qt\res\windows\windows.ico"
!define MUI_WELCOMEFINISHPAGE_BITMAP_NOSTRETCH
!define MUI_ABORTWARNING

!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES
!define MUI_FINISHPAGE_RUN "$INSTDIR\CoMaps.exe"
!define MUI_FINISHPAGE_RUN_TEXT "Launch CoMaps"
!insertmacro MUI_PAGE_FINISH

!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES

!insertmacro MUI_LANGUAGE "English"

; ---------------------------------------------------------------------------
; Close CoMaps if running before install/upgrade
; ---------------------------------------------------------------------------
Function CloseCoMapsIfRunning
  ; Try graceful close first, then force-kill if still running after 3 s.
  FindWindow $0 "" "CoMaps"
  IntCmp $0 0 done_close
    MessageBox MB_OKCANCEL|MB_ICONINFORMATION \
      "CoMaps is running. Click OK to close it and continue, or Cancel to abort." \
      IDOK do_close IDCANCEL abort_install
    abort_install:
      Abort "Installation cancelled."
    do_close:
      SendMessage $0 ${WM_CLOSE} 0 0
      Sleep 2000
      ; If still running, force-kill
      FindWindow $0 "" "CoMaps"
      IntCmp $0 0 done_close
        nsExec::ExecToLog 'taskkill /f /im CoMaps.exe'
  done_close:
FunctionEnd

; ---------------------------------------------------------------------------
; Install section
; ---------------------------------------------------------------------------
Section "CoMaps" SecMain
  SectionIn RO  ; required section, cannot be deselected

  Call CloseCoMapsIfRunning

  SetOutPath "$INSTDIR"
  File /r "staging\*.*"

  ; Store install dir and version in registry
  WriteRegStr HKLM "Software\CoMaps" "InstallDir" "$INSTDIR"
  WriteRegStr HKLM "Software\CoMaps" "Version"    "${VERSION}"

  ; Add/Remove Programs entry
  WriteRegStr   HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\CoMaps" \
                     "DisplayName"          "CoMaps"
  WriteRegStr   HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\CoMaps" \
                     "DisplayVersion"       "${VERSION}"
  WriteRegStr   HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\CoMaps" \
                     "Publisher"            "CoMaps Community"
  WriteRegStr   HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\CoMaps" \
                     "DisplayIcon"          "$INSTDIR\CoMaps.exe"
  WriteRegStr   HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\CoMaps" \
                     "UninstallString"      "$INSTDIR\Uninstall.exe"
  WriteRegStr   HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\CoMaps" \
                     "InstallLocation"      "$INSTDIR"
  WriteRegStr   HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\CoMaps" \
                     "URLInfoAbout"         "https://comaps.app"
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\CoMaps" \
                     "NoModify"             1
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\CoMaps" \
                     "NoRepair"             1

  ; Start Menu shortcut
  CreateDirectory "$SMPROGRAMS\CoMaps"
  CreateShortcut "$SMPROGRAMS\CoMaps\CoMaps.lnk" \
    "$INSTDIR\CoMaps.exe" "" "$INSTDIR\CoMaps.exe" 0 \
    SW_SHOWNORMAL "" "Free offline maps and navigation"
  CreateShortcut "$SMPROGRAMS\CoMaps\Uninstall CoMaps.lnk" \
    "$INSTDIR\Uninstall.exe"

  WriteUninstaller "$INSTDIR\Uninstall.exe"
SectionEnd

; ---------------------------------------------------------------------------
; Uninstall section
; ---------------------------------------------------------------------------
Section "Uninstall"
  ; Remove Start Menu shortcuts
  Delete "$SMPROGRAMS\CoMaps\CoMaps.lnk"
  Delete "$SMPROGRAMS\CoMaps\Uninstall CoMaps.lnk"
  RMDir  "$SMPROGRAMS\CoMaps"

  ; Remove installed files — delete the entire install directory tree.
  ; User map downloads live in %LOCALAPPDATA%\CoMaps, not here, so they
  ; are preserved by default.
  RMDir /r "$INSTDIR"

  ; Remove registry entries
  DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\CoMaps"
  DeleteRegKey HKLM "Software\CoMaps"
SectionEnd
