; NSIS installer script for CoMaps (Windows x64)
;
; Expects to be compiled from the repo root as:
;
;   makensis /DVERSION=<ver> /DSRCDIR=<repo-root> cmake\windows\CoMaps.nsi
;
; where <ver> is the display version string, e.g. "2026.05.22".
; The staging\ directory must already be populated by the CI build steps.

Unicode True

!ifndef VERSION
  !define VERSION "0.0.0"
!endif

; ---------------------------------------------------------------------------
; MultiUser.nsh — provides "Install for all users / just for me" page,
; handles UAC re-elevation automatically when "all users" is selected,
; and sets $INSTDIR, SetShellVarContext, and registry hive accordingly.
; ---------------------------------------------------------------------------
!define MULTIUSER_EXECUTIONLEVEL   Highest
!define MULTIUSER_MUI
!define MULTIUSER_INSTALLMODE_COMMANDLINE
!define MULTIUSER_INSTALLMODE_DEFAULT_REGISTRY_KEY      "Software\CoMaps"
!define MULTIUSER_INSTALLMODE_DEFAULT_REGISTRY_VALUENAME "InstallMode"
!define MULTIUSER_INSTALLMODE_INSTDIR                   "CoMaps"
!define MULTIUSER_INSTALLMODE_INSTDIR_REGISTRY_KEY      "Software\CoMaps"
!define MULTIUSER_INSTALLMODE_INSTDIR_REGISTRY_VALUENAME "InstallDir"
!include "MultiUser.nsh"
!include "MUI2.nsh"
!include "LogicLib.nsh"
!include "x64.nsh"

Name "CoMaps"
OutFile "${SRCDIR}\CoMaps-windows-x64-setup.exe"

VIProductVersion "${VIVERSION}"
VIAddVersionKey "ProductName"     "CoMaps"
VIAddVersionKey "ProductVersion"  "${VERSION}"
VIAddVersionKey "FileVersion"     "${VERSION}.0"
VIAddVersionKey "FileDescription" "CoMaps Installer"
VIAddVersionKey "LegalCopyright"  "Copyright 2026 The CoMaps Community"

!define MUI_ICON   "${SRCDIR}\qt\res\windows\windows.ico"
!define MUI_UNICON "${SRCDIR}\qt\res\windows\windows.ico"
!define MUI_WELCOMEFINISHPAGE_BITMAP_NOSTRETCH
!define MUI_ABORTWARNING

; Installer pages
!insertmacro MUI_PAGE_WELCOME
!define MULTIUSER_PAGE_CUSTOMFUNCTION_PRE PageInstallModeSkipIfUpgrade
!insertmacro MULTIUSER_PAGE_INSTALLMODE   ; "All users" / "Just me" radio buttons — skipped on upgrade
!define MUI_PAGE_CUSTOMFUNCTION_PRE PageDirectorySkipIfUpgrade
!insertmacro MUI_PAGE_DIRECTORY           ; skipped on upgrade (location locked to existing install)
!insertmacro MUI_PAGE_INSTFILES
; No finish-page launch: the installer may be running elevated and any process
; it spawns inherits the admin token, causing platform_win.cpp to treat
; Program Files as writable and store maps there instead of %LOCALAPPDATA%\CoMaps\.
; Launch from the Start Menu shortcut instead, which always runs unelevated.
!insertmacro MUI_PAGE_FINISH

; Uninstaller pages
!insertmacro MUI_UNPAGE_COMPONENTS
!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES

!insertmacro MUI_LANGUAGE "English"

; ---------------------------------------------------------------------------
; Uninstaller component descriptions
; ---------------------------------------------------------------------------
LangString DESC_UnMain ${LANG_ENGLISH} "Remove CoMaps and all installed files."
LangString DESC_UnData ${LANG_ENGLISH} "Delete downloaded maps and bookmarks stored in %LOCALAPPDATA%\CoMaps\. This cannot be undone."

; Whether we detected an existing install and are upgrading (1) or doing a fresh install (0).
Var IsUpgrade

; ---------------------------------------------------------------------------
; Installer / uninstaller init (MultiUser.nsh handles mode & elevation)
; ---------------------------------------------------------------------------
Function .onInit
  !insertmacro MULTIUSER_INIT

  ; Detect an existing install. Check HKLM first (all-users), then HKCU (per-user).
  ; If found, lock the install mode and directory to match, and flag as an upgrade.
  StrCpy $IsUpgrade 0

  ReadRegStr $0 HKLM "Software\CoMaps" "InstallDir"
  ${If} $0 != ""
    StrCpy $IsUpgrade 1
    ; Force all-users mode and restore directory.
    Call MultiUser.InstallMode.AllUsers
    StrCpy $INSTDIR $0
  ${Else}
    ReadRegStr $0 HKCU "Software\CoMaps" "InstallDir"
    ${If} $0 != ""
      StrCpy $IsUpgrade 1
      ; Force current-user mode and restore directory.
      Call MultiUser.InstallMode.CurrentUser
      StrCpy $INSTDIR $0
    ${EndIf}
  ${EndIf}

  ; When upgrading, skip the install-mode page (mode is locked).
  ; Note: Name is a compile-time directive and cannot be changed here at runtime.
FunctionEnd

; Skip the install-mode page when upgrading (mode is locked).
Function PageInstallModeSkipIfUpgrade
  ${If} $IsUpgrade == 1
    Abort  ; Abort the page — NSIS skips to the next one.
  ${EndIf}
FunctionEnd

; Skip the directory page when upgrading (location is locked).
Function PageDirectorySkipIfUpgrade
  ${If} $IsUpgrade == 1
    Abort
  ${EndIf}
FunctionEnd

Function un.onInit
  !insertmacro MULTIUSER_UNINIT
FunctionEnd

; ---------------------------------------------------------------------------
; Close CoMaps if running before install/upgrade
; ---------------------------------------------------------------------------
Function CloseCoMapsIfRunning
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
  File /r "${SRCDIR}\staging\*.*"

  ; MultiUser.nsh sets $MultiUser.InstallMode ("AllUsers" or "CurrentUser"),
  ; SetShellVarContext, $INSTDIR, and the registry hive automatically.
  ; We only need to write Add/Remove Programs entries and shortcuts.
  ${If} $MultiUser.InstallMode == "AllUsers"
    WriteRegStr   HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\CoMaps" \
                       "DisplayName"     "CoMaps"
    WriteRegStr   HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\CoMaps" \
                       "DisplayVersion"  "${VERSION}"
    WriteRegStr   HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\CoMaps" \
                       "Publisher"       "CoMaps Community"
    WriteRegStr   HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\CoMaps" \
                       "DisplayIcon"     "$INSTDIR\CoMaps.exe"
    WriteRegStr   HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\CoMaps" \
                       "UninstallString" "$INSTDIR\Uninstall.exe"
    WriteRegStr   HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\CoMaps" \
                       "InstallLocation" "$INSTDIR"
    WriteRegStr   HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\CoMaps" \
                       "URLInfoAbout"    "https://comaps.app"
    WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\CoMaps" \
                       "NoModify"        1
    WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\CoMaps" \
                       "NoRepair"        1
  ${Else}
    WriteRegStr   HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\CoMaps" \
                       "DisplayName"     "CoMaps"
    WriteRegStr   HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\CoMaps" \
                       "DisplayVersion"  "${VERSION}"
    WriteRegStr   HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\CoMaps" \
                       "Publisher"       "CoMaps Community"
    WriteRegStr   HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\CoMaps" \
                       "DisplayIcon"     "$INSTDIR\CoMaps.exe"
    WriteRegStr   HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\CoMaps" \
                       "UninstallString" "$INSTDIR\Uninstall.exe"
    WriteRegStr   HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\CoMaps" \
                       "InstallLocation" "$INSTDIR"
    WriteRegStr   HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\CoMaps" \
                       "URLInfoAbout"    "https://comaps.app"
    WriteRegDWORD HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\CoMaps" \
                       "NoModify"        1
    WriteRegDWORD HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\CoMaps" \
                       "NoRepair"        1
  ${EndIf}

  ; $SMPROGRAMS resolves to All Users or current user based on SetShellVarContext,
  ; which MultiUser.nsh sets correctly for the chosen install mode.
  CreateDirectory "$SMPROGRAMS\CoMaps"
  CreateShortcut "$SMPROGRAMS\CoMaps\CoMaps.lnk" \
    "$INSTDIR\CoMaps.exe" "" "$INSTDIR\CoMaps.exe" 0 \
    SW_SHOWNORMAL "" "Free offline maps and navigation"
  CreateShortcut "$SMPROGRAMS\CoMaps\Uninstall CoMaps.lnk" \
    "$INSTDIR\Uninstall.exe"

  WriteUninstaller "$INSTDIR\Uninstall.exe"
SectionEnd

; ---------------------------------------------------------------------------
; Uninstall sections
; ---------------------------------------------------------------------------
Section "un.CoMaps" SecUnMain
  SectionIn RO  ; required

  Delete "$SMPROGRAMS\CoMaps\CoMaps.lnk"
  Delete "$SMPROGRAMS\CoMaps\Uninstall CoMaps.lnk"
  RMDir  "$SMPROGRAMS\CoMaps"

  RMDir /r "$INSTDIR"

  ${If} $MultiUser.InstallMode == "AllUsers"
    DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\CoMaps"
    DeleteRegKey HKLM "Software\CoMaps"
  ${Else}
    DeleteRegKey HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\CoMaps"
    DeleteRegKey HKCU "Software\CoMaps"
  ${EndIf}
SectionEnd

Section /o "un.Delete maps and bookmarks" SecUnData
  ; Optional, unchecked by default (/o flag).
  ; Removes all downloaded maps and bookmarks from the user data folder.
  RMDir /r "$LOCALAPPDATA\CoMaps"
SectionEnd

!insertmacro MUI_UNFUNCTION_DESCRIPTION_BEGIN
  !insertmacro MUI_DESCRIPTION_TEXT ${SecUnMain} $(DESC_UnMain)
  !insertmacro MUI_DESCRIPTION_TEXT ${SecUnData} $(DESC_UnData)
!insertmacro MUI_UNFUNCTION_DESCRIPTION_END
