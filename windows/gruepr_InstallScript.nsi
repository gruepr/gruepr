; gruepr_InstallScript.nsi
;
; This installer will install gruepr into a directory that the user selects.
; It remembers the directory in the registry, has uninstall support, and
; (optionally) installs start menu and desktop shortcuts.
;--------------------------------

  !define APPNAME "gruepr"
  !ifndef BASELOCATION
    !define BASELOCATION "\"
  !endif
  !define APPLOCATION "${BASELOCATION}\gruepr"
  !ifndef BUILDDIR
    !define BUILDDIR "${BASELOCATION}\gruepr"
  !endif
  !define BUILDLOCATION "${BUILDDIR}\release"
  !define VCREDIST "vc_redist.x64.exe"

;--------------------------------
  !getdllversion "${BUILDLOCATION}\gruepr.exe" expv_
  !if ${expv_3} == 0
    !define VERSION ${expv_1}.${expv_2}
  !else
    !define VERSION ${expv_1}.${expv_2}.${expv_3}
  !endif
  !define LONGVERSION ${expv_1}.${expv_2}.${expv_3}.${expv_4}

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
  OutFile "install_${APPNAME}V${VERSION}.exe"
  InstallDir "$PROGRAMFILES\${APPNAME}"
  InstallDirRegKey HKCU "Software\${APPNAME}" "Install_Dir"

  SetCompressor /solid lzma
  RequestExecutionLevel admin
  ShowInstDetails show
  ShowUninstDetails show
  Unicode True

;--------------------------------
; Version numbering

  VIProductVersion "${LONGVERSION}"
  VIAddVersionKey "ProductName" "${APPNAME}"
  VIAddVersionKey "LegalCopyright" "(c) 2019-2024"
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
  !define MUI_FINISHPAGE_RUN_TEXT "Add Shortcuts to Start Menu"
  !define MUI_FINISHPAGE_RUN_FUNCTION StartMenuShortcuts
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
; Installation

  Section "Visual Studio Runtime"

    ; Set output path to the installation directory.
    SetOutPath $INSTDIR
    File "${BUILDLOCATION}\vc_redist.x64.exe"
    ExecWait "$INSTDIR\${VCREDIST} /install /passive"
    Delete "$INSTDIR\${VCREDIST}"

  SectionEnd
;
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
    WriteRegStr HKCU "${UN_REG_LOC}" "URLInfoAbout" "http://gruepr.com"
    WriteRegDWORD HKCU "${UN_REG_LOC}" "NoModify" 1
    WriteRegDWORD HKCU "${UN_REG_LOC}" "NoRepair" 1
    WriteUninstaller "$INSTDIR\uninstall.exe"

  SectionEnd

;------------------------------
; Optional Shortcuts

  Function StartMenuShortcuts
    CreateDirectory "$SMPROGRAMS\${APPNAME}"
    CreateShortcut "$SMPROGRAMS\${APPNAME}\Uninstall.lnk" "$INSTDIR\uninstall.exe"
    CreateShortcut "$SMPROGRAMS\${APPNAME}\${APPNAME}.lnk" "$INSTDIR\gruepr.exe"
  FunctionEnd

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
    RMDir /r "$INSTDIR\bearer"
    RMDir /r "$INSTDIR\generic"
    RMDir /r "$INSTDIR\iconengines"
    RMDir /r "$INSTDIR\imageformats"
    RMDir /r "$INSTDIR\networkinformation"
    RMDir /r "$INSTDIR\platforms"
    RMDir /r "$INSTDIR\position"
    RMDir /r "$INSTDIR\printsupport"
    RMDir /r "$INSTDIR\resources"
    RMDir /r "$INSTDIR\sample survey results"
    RMDir /r "$INSTDIR\sample surveys"
    RMDir /r "$INSTDIR\styles"
    RMDir /r "$INSTDIR\tls"
    RMDir /r "$INSTDIR\translations"
    Delete "$INSTDIR\gruepr.exe"
    Delete "$INSTDIR\uninstall.exe"
    Delete "$INSTDIR\D3D*.dll"
    Delete "$INSTDIR\lib*.dll"
    Delete "$INSTDIR\opengl*.dll"
    Delete "$INSTDIR\Qt*.dll"
    Delete "$INSTDIR\Qt*.exe"
    Delete "$INSTDIR\vc_*.exe"

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
