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
OutFile "${SRCDIR}\CoMaps-windows-x64-setup.exe"
; InstallDir and RequestExecutionLevel are set dynamically in .onInit based on
; whether the installer is running with admin rights.
RequestExecutionLevel highest

VIProductVersion "${VERSION}.0"
VIAddVersionKey "ProductName"     "CoMaps"
VIAddVersionKey "ProductVersion"  "${VERSION}"
VIAddVersionKey "FileVersion"     "${VERSION}.0"
VIAddVersionKey "FileDescription" "CoMaps Installer"
VIAddVersionKey "LegalCopyright"  "Copyright 2026 The CoMaps Community"

; Modern UI + process close support
!include "MUI2.nsh"
!include "LogicLib.nsh"
!include "x64.nsh"

!define MUI_ICON   "${SRCDIR}\qt\res\windows\windows.ico"
!define MUI_UNICON "${SRCDIR}\qt\res\windows\windows.ico"
!define MUI_WELCOMEFINISHPAGE_BITMAP_NOSTRETCH
!define MUI_ABORTWARNING

; Whether this is a machine-wide (admin) or per-user install.
; Set in .onInit; used throughout install and uninstall sections.
Var IsAdminInstall

!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES
; No finish-page launch: the installer runs elevated and any process it spawns
; inherits the admin token, causing platform_win.cpp to treat Program Files as
; writable and store maps there instead of %LOCALAPPDATA%\CoMaps\. Launch from
; the Start Menu shortcut instead, which always runs unelevated.
!insertmacro MUI_PAGE_FINISH

!insertmacro MUI_UNPAGE_COMPONENTS
!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES

!insertmacro MUI_LANGUAGE "English"

; ---------------------------------------------------------------------------
; Uninstaller component descriptions
; ---------------------------------------------------------------------------
LangString DESC_UnMain    ${LANG_ENGLISH} "Remove CoMaps and all installed files."
LangString DESC_UnData    ${LANG_ENGLISH} "Delete downloaded maps and bookmarks stored in %LOCALAPPDATA%\CoMaps\. This cannot be undone."

; ---------------------------------------------------------------------------
; Installer init: detect admin rights and set default install directory
; ---------------------------------------------------------------------------
Function .onInit
  ; Detect elevation by trying to write a test key to HKLM.
  ; UserInfo::GetAccountType is unreliable on corporate machines where UAC
  ; elevates via different-account credentials — it may return "Admin" for the
  ; credential account while the shell context still resolves to that account's
  ; personal AppData instead of the All Users profile.
  ; Writing to HKLM directly is the most reliable elevation check.
  ClearErrors
  WriteRegStr HKLM "Software\CoMaps" "_ElevTest" "1"
  ${If} ${Errors}
    StrCpy $IsAdminInstall 0
  ${Else}
    DeleteRegValue HKLM "Software\CoMaps" "_ElevTest"
    StrCpy $IsAdminInstall 1
  ${EndIf}

  ${If} $IsAdminInstall == 1
    SetShellVarContext all
    ; Restore previous machine-wide install dir from registry, or use default.
    ReadRegStr $INSTDIR HKLM "Software\CoMaps" "InstallDir"
    ${If} $INSTDIR == ""
      StrCpy $INSTDIR "$PROGRAMFILES64\CoMaps"
    ${EndIf}
  ${Else}
    SetShellVarContext current
    ; Restore previous per-user install dir from registry, or use default.
    ReadRegStr $INSTDIR HKCU "Software\CoMaps" "InstallDir"
    ${If} $INSTDIR == ""
      StrCpy $INSTDIR "$LOCALAPPDATA\Programs\CoMaps"
    ${EndIf}
  ${EndIf}
FunctionEnd

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
  File /r "${SRCDIR}\staging\*.*"

  ${If} $IsAdminInstall == 1
    ; Machine-wide: registry in HKLM, shortcuts for all users.
    WriteRegStr HKLM "Software\CoMaps" "InstallDir"   "$INSTDIR"
    WriteRegStr HKLM "Software\CoMaps" "Version"      "${VERSION}"
    WriteRegStr HKLM "Software\CoMaps" "InstallType"  "machine"

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
    ; Per-user: registry in HKCU, shortcuts for current user only.
    WriteRegStr HKCU "Software\CoMaps" "InstallDir"   "$INSTDIR"
    WriteRegStr HKCU "Software\CoMaps" "Version"      "${VERSION}"
    WriteRegStr HKCU "Software\CoMaps" "InstallType"  "user"

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

  ; Start Menu shortcut ($SMPROGRAMS context set in .onInit)
  CreateDirectory "$SMPROGRAMS\CoMaps"
  CreateShortcut "$SMPROGRAMS\CoMaps\CoMaps.lnk" \
    "$INSTDIR\CoMaps.exe" "" "$INSTDIR\CoMaps.exe" 0 \
    SW_SHOWNORMAL "" "Free offline maps and navigation"
  CreateShortcut "$SMPROGRAMS\CoMaps\Uninstall CoMaps.lnk" \
    "$INSTDIR\Uninstall.exe"

  WriteUninstaller "$INSTDIR\Uninstall.exe"
SectionEnd

; ---------------------------------------------------------------------------
; Uninstall init: detect whether this was a machine-wide or per-user install
; ---------------------------------------------------------------------------
Function un.onInit
  ; Use the same HKLM write-test as .onInit to determine elevation.
  ClearErrors
  WriteRegStr HKLM "Software\CoMaps" "_ElevTest" "1"
  ${If} ${Errors}
    StrCpy $IsAdminInstall 0
    SetShellVarContext current
  ${Else}
    DeleteRegValue HKLM "Software\CoMaps" "_ElevTest"
    StrCpy $IsAdminInstall 1
    SetShellVarContext all
  ${EndIf}
FunctionEnd

; ---------------------------------------------------------------------------
; Uninstall sections
; ---------------------------------------------------------------------------
Section "un.CoMaps" SecUnMain
  SectionIn RO  ; required

  ; Remove Start Menu shortcuts (context set in un.onInit)
  Delete "$SMPROGRAMS\CoMaps\CoMaps.lnk"
  Delete "$SMPROGRAMS\CoMaps\Uninstall CoMaps.lnk"
  RMDir  "$SMPROGRAMS\CoMaps"

  ; Remove installed files — delete the entire install directory tree.
  ; User map downloads live in %LOCALAPPDATA%\CoMaps, not here.
  RMDir /r "$INSTDIR"

  ; Remove registry entries
  ${If} $IsAdminInstall == 1
    DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\CoMaps"
    DeleteRegKey HKLM "Software\CoMaps"
  ${Else}
    DeleteRegKey HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\CoMaps"
    DeleteRegKey HKCU "Software\CoMaps"
  ${EndIf}
SectionEnd

Section /o "un.Delete maps and bookmarks" SecUnData
  ; Optional, unchecked by default (enforced in un.onInit).
  ; Removes all downloaded maps and bookmarks from the user data folder.
  RMDir /r "$LOCALAPPDATA\CoMaps"
SectionEnd

!insertmacro MUI_UNFUNCTION_DESCRIPTION_BEGIN
  !insertmacro MUI_DESCRIPTION_TEXT ${SecUnMain} $(DESC_UnMain)
  !insertmacro MUI_DESCRIPTION_TEXT ${SecUnData} $(DESC_UnData)
!insertmacro MUI_UNFUNCTION_DESCRIPTION_END
