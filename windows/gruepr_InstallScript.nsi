; gruepr_InstallScript.nsi
;
; This installer will install gruepr into a directory that the user selects.
; It remembers the directory in the registry, has uninstall support, and
; installs start menu shortcuts and (optionally) a desktop shortcut.
;--------------------------------

  !define APPNAME "gruepr"

  !ifndef BASELOCATION
    !define BASELOCATION "C:\Users\jhertz\Documents\GDrive\CPP\Qt"
  !endif
  !define APPLOCATION "${BASELOCATION}\gruepr"
  !ifndef BUILDDIR
    !define BUILDDIR "${BASELOCATION}\gruepr"
  !endif
  !define BUILDLOCATION "${BUILDDIR}\release"

;--------------------------------
  !getdllversion "${BUILDLOCATION}\gruepr.exe" expv_
  !if ${expv_3} == 0
    !define VERSION ${expv_1}.${expv_2}
  !else
    !define VERSION ${expv_1}.${expv_2}.${expv_3}
  !endif
  !define LONGVERSION ${expv_1}.${expv_2}.${expv_3}.${expv_4}

;--------------------------------
; Copyright year — single-sourced from gruepr.pro (the "copyright_year = ..." line).
; In CI, Build.yaml passes /DCOPYRIGHT_YEAR with the trimmed value. For local builds,
; read it straight out of gruepr.pro

  !ifndef COPYRIGHT_YEAR
    !searchparse /file "${BUILDDIR}\gruepr.pro" `copyright_year = ` COPYRIGHT_YEAR
  !endif

;--------------------------------
; Modern User Interface 2 Graphics

  !include "MUI2.nsh"
  !define MUI_ICON "${APPLOCATION}\icons_new\icons.ico"
  !define MUI_UNICON "${APPLOCATION}\icons_new\uninstall.ico"
  !define MUI_WELCOMEFINISHPAGE_BITMAP "${APPLOCATION}\windows\install.bmp"
  !define MUI_UNWELCOMEFINISHPAGE_BITMAP "${APPLOCATION}\windows\uninstall.bmp"

;--------------------------------
; Basics

  Name "${APPNAME}"
  !ifdef CI
    OutFile "install_${APPNAME}.exe"
  !else
    OutFile "install_${APPNAME}V${VERSION}.exe"
  !endif
  InstallDir "$LOCALAPPDATA\Programs\${APPNAME}"

  SetCompressor /solid lzma
  RequestExecutionLevel user
  ShowInstDetails show
  ShowUninstDetails show
  Unicode True

;--------------------------------
; Version numbering

  VIProductVersion "${LONGVERSION}"
  VIAddVersionKey "ProductName" "${APPNAME}"
  VIAddVersionKey "LegalCopyright" "(c) ${COPYRIGHT_YEAR}"
  VIAddVersionKey "FileDescription" "${APPNAME} Installer"
  VIAddVersionKey "FileVersion" "${VERSION}"
  VIAddVersionKey "ProductVersion" "${VERSION}"

;--------------------------------
; Installation Pages

  !define MUI_WELCOMEPAGE_TITLE "${APPNAME} Installation"
  !define MUI_WELCOMEPAGE_TEXT "This wizard will guide you through the installation of ${APPNAME} version ${VERSION}.$\r$\n$\r$\nClick Next to continue."
  !insertmacro MUI_PAGE_WELCOME

  !insertmacro MUI_PAGE_DIRECTORY

  !insertmacro MUI_PAGE_INSTFILES

  !define MUI_FINISHPAGE_TITLE "Completing ${APPNAME} Installation"
  !define MUI_FINISHPAGE_TEXT "gruepr has been installed on your computer.$\r$\n$\r$\nClick Finish to close Setup"
  !define MUI_FINISHPAGE_LINK "Click here to visit the gruepr homepage at gruepr.com."
  !define MUI_FINISHPAGE_LINK_LOCATION https://www.gruepr.com/
  !define MUI_FINISHPAGE_RUN ""
  ;!define MUI_FINISHPAGE_RUN_NOTCHECKED
  !define MUI_FINISHPAGE_SHOWREADME ""
  !define MUI_FINISHPAGE_SHOWREADME_TEXT "Add Shortcuts to Desktop"
  !define MUI_FINISHPAGE_SHOWREADME_FUNCTION DesktopShortcut
  ;!define MUI_FINISHPAGE_SHOWREADME_NOTCHECKED
  !insertmacro MUI_PAGE_FINISH

;--------------------------------
; Uninstallation Pages

  !insertmacro MUI_UNPAGE_CONFIRM

  !insertmacro MUI_UNPAGE_INSTFILES

;--------------------------------
;Languages

  !insertmacro MUI_LANGUAGE "English"

;--------------------------------
; Check if gruepr is currently running

Function .onInit
    nsExec::ExecToStack 'cmd /c tasklist /NH 2>nul | findstr /B /I "gruepr.exe"'
    Pop $0  ; return value (findstr exit code: 0 = found, 1 = not found)
    Pop $1  ; output string (matched lines, if any)
    StrCmp $1 "" done
        MessageBox MB_OK|MB_ICONEXCLAMATION "gruepr is currently running. Please close it before installing."
        Abort
    done:
FunctionEnd

;--------------------------------
; Installation

  Section "gruepr (required)"

    SectionIn RO

    ; Set output path to the installation directory.
    SetOutPath $INSTDIR

    ; Install all the files
    File /r "${BUILDLOCATION}\*.*"

    ; Write the installation path into the registry
    WriteRegStr HKCU "Software\${APPNAME}" "Install_Dir" "$INSTDIR"

    ; Write the uninstall keys for Windows
    !define UN_REG_LOC "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APPNAME}"
    WriteRegStr HKCU "${UN_REG_LOC}" "DisplayName" "${APPNAME}"
    WriteRegStr HKCU "${UN_REG_LOC}" "DisplayIcon" "${APPLOCATION}\icons_new\icons.ico"
    WriteRegStr HKCU "${UN_REG_LOC}" "UninstallString" '"$INSTDIR\uninstall.exe"'
    WriteRegStr HKCU "${UN_REG_LOC}" "URLInfoAbout" "https://gruepr.com"
    WriteRegStr HKCU "${UN_REG_LOC}" "Publisher" "gruepr"
    WriteRegStr HKCU "${UN_REG_LOC}" "DisplayVersion" "${VERSION}"
    WriteRegDWORD HKCU "${UN_REG_LOC}" "NoModify" 1
    WriteRegDWORD HKCU "${UN_REG_LOC}" "NoRepair" 1
    WriteUninstaller "$INSTDIR\uninstall.exe"

    ; Create Start Menu shortcut
    CreateDirectory "$SMPROGRAMS\${APPNAME}"
    CreateShortcut "$SMPROGRAMS\${APPNAME}\${APPNAME}.lnk" "$INSTDIR\gruepr.exe"
    CreateShortcut "$SMPROGRAMS\${APPNAME}\Uninstall.lnk" "$INSTDIR\uninstall.exe"

  SectionEnd

;------------------------------
; Optional Shortcuts

  Function DesktopShortcut
    CreateShortcut "$DESKTOP\${APPNAME}.lnk" "$INSTDIR\gruepr.exe"
  FunctionEnd

;--------------------------------
; Uninstallation

  Section "Uninstall"

    ; Remove registry keys
    DeleteRegKey HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APPNAME}"
    DeleteRegKey HKCU "Software\${APPNAME}"

    ; Remove files
    RMDir /r "$INSTDIR"

    ; Remove shortcuts, if any
    Delete "$SMPROGRAMS\${APPNAME}\*.lnk"
    Delete "$DESKTOP\${APPNAME}.lnk"

    ; Remove directories
    RMDir "$SMPROGRAMS\${APPNAME}"
    RMDir "$INSTDIR"

  SectionEnd

  Function un.onInit
    SetAutoClose true
  FunctionEnd

  Function un.onUninstSuccess
    MessageBox MB_OK "${APPNAME} succesfully uninstalled."
  FunctionEnd
