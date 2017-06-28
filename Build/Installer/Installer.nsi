/* Copyright (C) 2012 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

!verbose 3

Unicode true

!addplugindir ".\"
!include "MUI2.nsh"
!include "x64.nsh"
!include "FileFunc.nsh"
!include "WordFunc.nsh"
!include "WinVer.nsh"
!include "UAC.nsh"

!ifndef OUTFILE
 !define OUTFILE "Rainmeter-test.exe"
 !define VERSION_FULL "0.0.0.0"
 !define VERSION_SHORT "0.0"
 !define VERSION_REVISION "000"
!else
 !define INCLUDEFILES
!endif

Name "Rainmeter"
VIAddVersionKey "CompanyName" "Rainmeter"
VIAddVersionKey "ProductName" "Rainmeter"
VIAddVersionKey "FileDescription" "Rainmeter Installer"
VIAddVersionKey "FileVersion" "${VERSION_FULL}"
VIAddVersionKey "ProductVersion" "${VERSION_FULL}"
VIAddVersionKey "OriginalFilename" "${OUTFILE}"
VIAddVersionKey "LegalCopyright" "Copyright (C) 2016 Rainmeter Team"
VIProductVersion "${VERSION_FULL}"
BrandingText " "
SetCompressor /SOLID lzma
RequestExecutionLevel user
InstallDirRegKey HKLM "SOFTWARE\Rainmeter" ""
ShowInstDetails nevershow
AllowSkipFiles off
XPStyle on
OutFile "..\${OUTFILE}"
ReserveFile "${NSISDIR}\Plugins\x86-unicode\LangDLL.dll"
ReserveFile "${NSISDIR}\Plugins\x86-unicode\nsDialogs.dll"
ReserveFile "${NSISDIR}\Plugins\x86-unicode\System.dll"
ReserveFile ".\UAC.dll"

!define REQUIREDSPACE 5 ; Minimum required space for install (in MB)

; Additional Windows definitions
!define PF_XMMI64_INSTRUCTIONS_AVAILABLE 10

!define MUI_ICON ".\Icon.ico"
!define MUI_UNICON ".\Icon.ico"
!define MUI_WELCOMEFINISHPAGE_BITMAP ".\Wizard.bmp"
!define MUI_FINISHPAGE_RUN
!define MUI_FINISHPAGE_RUN_FUNCTION FinishRun
!define MUI_WELCOMEPAGE ; For language strings

Page custom PageWelcome PageWelcomeOnLeave
Page custom PageOptions PageOptionsOnLeave
!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_PAGE_FINISH

UninstPage custom un.PageOptions un.GetOptions
!insertmacro MUI_UNPAGE_INSTFILES

; Include languages
!macro IncludeLanguage LANGUAGE CUSTOMLANGUAGE
	!insertmacro MUI_LANGUAGE ${LANGUAGE}
	!insertmacro LANGFILE_INCLUDE "..\..\Language\${CUSTOMLANGUAGE}.nsh"
!macroend
!define IncludeLanguage "!insertmacro IncludeLanguage"
!include "Languages.nsh"

Var NonDefaultLanguage
Var AutoStartup
Var Install64Bit
Var InstallPortable
Var un.DeleteAll

; Install
; --------------------------------------
Function .onInit
	${If} ${RunningX64}
		${EnableX64FSRedirection}
	${EndIf}

	${IfNot} ${UAC_IsInnerInstance}
		SetSilent normal

		${If} ${IsWin7}
		${OrIf} ${IsWin2008R2}
			${IfNot} ${AtLeastServicePack} 1
				MessageBox MB_OK|MB_ICONSTOP "Rainmeter ${VERSION_SHORT} requires at least Windows 7 with Service Pack 1.$\n$\nPlease install Service Pack 1 or download Rainmeter 3.3 from www.rainmeter.net"
				Quit
			${EndIf}

			; Try instantiating a ID2D1Factory1 to check for presense of D2D 1.1.
			!define D2D1_FACTORY_TYPE_SINGLE_THREADED 0
			!define IID_ID2D1Factory1 {bb12d362-daee-4b9a-aa1d-14ba401cfa1f}
			System::Call "d2d1::D2D1CreateFactory( \
				i ${D2D1_FACTORY_TYPE_SINGLE_THREADED}, \
				g '${IID_ID2D1Factory1}', \
				p 0, \
				*p .r0) i.r1"
			${If} $1 <> 0
				MessageBox MB_OK|MB_ICONSTOP "Rainmeter ${VERSION_SHORT} requires at least Windows 7 with the Platform Update installed.$\n$\nPlease install the Windows 7 Platform Update or download Rainmeter 3.3 from www.rainmeter.net"
				Quit
			${Endif}
			; Call Release
			System::Call "$0->2()"
		${ElseIfNot} ${AtLeastWin8}
			MessageBox MB_OK|MB_ICONSTOP "Rainmeter ${VERSION_SHORT} requires at least Windows 7 with Service Pack 1.$\n$\nFor XP and Vista, you can download Rainmeter 3.3 from www.rainmeter.net"
			Quit
		${EndIf}

		System::Call 'kernel32::IsProcessorFeaturePresent(i${PF_XMMI64_INSTRUCTIONS_AVAILABLE})i.r0'
		${If} $0 = 0
			MessageBox MB_OK|MB_ICONSTOP "Rainmeter requires a Pentium 4 or later processor."
			Quit
		${EndIf}

		ReadRegStr $0 HKLM "SOFTWARE\Rainmeter" "Language"
		ReadRegDWORD $NonDefaultLanguage HKLM "SOFTWARE\Rainmeter" "NonDefault"

		${If} $0 == ""
		${OrIf} $0 <> $LANGUAGE
		${AndIf} $NonDefaultLanguage != 1
			; New install or better match
			LangDLL::LangDialog "$(^SetupCaption)" "Please select the installer language.$\n$(SELECTLANGUAGE)" AC ${LANGDLL_PARAMS} ""
			Pop $0
			${If} $0 == "cancel"
				Abort
			${EndIf}

			${If} $0 <> $LANGUAGE
				; User selected non-default language
				StrCpy $NonDefaultLanguage 1
			${EndIf}
		${EndIf}

		StrCpy $LANGUAGE $0

		; If the language was set to a non-existent language, reset it back to English.
		${WordFind} ",${LANGUAGE_IDS}" ",$LANGUAGE," "E+1{" $0
		${If} ${Errors}
			StrCpy $LANGUAGE "1033"
		${EndIf}
	${Else}
		; Exchange settings with user instance
		!insertmacro UAC_AsUser_Call Function ExchangeSettings ${UAC_SYNCREGISTERS}
		StrCpy $AutoStartup $1
		StrCpy $Install64Bit $2
		StrCpy $NonDefaultLanguage $3
		StrCpy $LANGUAGE $4
		StrCpy $INSTDIR $5
	${EndIf}
FunctionEnd

Function ExchangeSettings
	StrCpy $1 $AutoStartup
	StrCpy $2 $Install64Bit
	StrCpy $3 $NonDefaultLanguage
	StrCpy $4 $LANGUAGE
	StrCpy $5 $INSTDIR
	HideWindow
FunctionEnd

Function PageWelcome
	${If} ${UAC_IsInnerInstance}
		${If} ${UAC_IsAdmin}
			; Skip page
			Abort
		${Else}
			MessageBox MB_OK|MB_ICONSTOP "$(ADMINERROR) (Inner)"
			Quit
		${EndIf}
	${EndIf}

	!insertmacro MUI_HEADER_TEXT "$(INSTALLOPTIONS)" "$(^ComponentsSubText1)"
	nsDialogs::Create 1044
	Pop $0
	nsDialogs::SetRTL $(^RTL)
	SetCtlColors $0 "" "${MUI_BGCOLOR}"

	${NSD_CreateBitmap} 0u 0u 109u 193u ""
	Pop $0
	${NSD_SetImage} $0 "$PLUGINSDIR\modern-wizard.bmp" $R0

	${NSD_CreateLabel} 120u 10u 195u 38u "$(MUI_TEXT_WELCOME_INFO_TITLE)"
	Pop $0
	SetCtlColors $0 "" "${MUI_BGCOLOR}"
	CreateFont $1 "$(^Font)" "12" "700"
	SendMessage $0 ${WM_SETFONT} $1 0

	${NSD_CreateLabel} 120u 55u 195u 12u "$(^ComponentsSubText1)"
	Pop $0
	SetCtlColors $0 "" "${MUI_BGCOLOR}"

	${NSD_CreateRadioButton} 120u 70u 205u 12u "$(STANDARDINST)"
	Pop $R1
	SetCtlColors $R1 "" "${MUI_BGCOLOR}"
	${NSD_AddStyle} $R1 ${WS_GROUP}
	SendMessage $R1 ${WM_SETFONT} $mui.Header.Text.Font 0

	${NSD_CreateLabel} 132u 82u 185u 24u "$(STANDARDINSTDESC)"
	Pop $0
	SetCtlColors $0 "" "${MUI_BGCOLOR}"

	${NSD_CreateRadioButton} 120u 106u 310u 12u "$(PORTABLEINST)"
	Pop $R2
	SetCtlColors $R2 "" "${MUI_BGCOLOR}"
	${NSD_AddStyle} $R2 ${WS_TABSTOP}
	SendMessage $R2 ${WM_SETFONT} $mui.Header.Text.Font 0

	${NSD_CreateLabel} 132u 118u 185u 52u "$(PORTABLEINSTDESC)"
	Pop $0
	SetCtlColors $0 "" "${MUI_BGCOLOR}"

	${If} $InstallPortable = 1
		${NSD_Check} $R2
	${Else}
		${NSD_Check} $R1
	${EndIf}

	Call muiPageLoadFullWindow

	nsDialogs::Show
	${NSD_FreeImage} $R0
FunctionEnd

Function PageWelcomeOnLeave
	${NSD_GetState} $R2 $InstallPortable
	Call muiPageUnloadFullWindow
FunctionEnd

Function PageOptions
	${If} ${UAC_IsInnerInstance}
	${AndIf} ${UAC_IsAdmin}
		; Skip page
		Abort
	${EndIf}

	!insertmacro MUI_HEADER_TEXT "$(INSTALLOPTIONS)" "$(INSTALLOPTIONSDESC)"
	nsDialogs::Create 1018
	nsDialogs::SetRTL $(^RTL)

	${NSD_CreateGroupBox} 0 0u -1u 36u "$(^DirSubText)"

	${NSD_CreateDirRequest} 6u 14u 232u 14u ""
	Pop $R0
	${NSD_OnChange} $R0 PageOptionsDirectoryOnChange

	${NSD_CreateBrowseButton} 242u 14u 50u 14u "$(^BrowseBtn)"
	Pop $R1
	${NSD_OnClick} $R1 PageOptionsBrowseOnClick

	StrCpy $1 0

	StrCpy $R2 0
	${If} ${RunningX64}
		${If} $InstallPortable = 1
		${OrIf} $INSTDIR == ""
		${OrIfNot} ${FileExists} "$INSTDIR\Rainmeter.exe"
			${NSD_CreateCheckBox} 6u 54u 285u 12u "$(INSTALL64BIT)"
			Pop $R2
			StrCpy $1 30u
		${EndIf}
	${EndIf}

	${If} $InstallPortable <> 1
		${If} $1 = 0
			StrCpy $0 54u
			StrCpy $1 30u
		${Else}
			StrCpy $0 66u
			StrCpy $1 42u
		${EndIf}

		${NSD_CreateCheckbox} 6u $0 285u 12u "$(AUTOSTARTUP)"
		Pop $R3

		${If} $INSTDIR == ""
			${NSD_Check} $R3
		${Else}
			SetShellVarContext all
			${If} ${FileExists} "$SMSTARTUP\Rainmeter.lnk"
				${NSD_Check} $R3
			${EndIf}

			SetShellVarContext current
			${If} ${FileExists} "$SMSTARTUP\Rainmeter.lnk"
				${NSD_Check} $R3
			${EndIf}
		${EndIf}
	${Else}
		StrCpy $R3 0
	${EndIf}

	${If} $1 <> 0
		${NSD_CreateGroupBox} 0 42u -1u $1 "$(ADDITIONALOPTIONS)"
	${EndIf}

	; Set default directory
	${If} $InstallPortable = 1
		${GetRoot} "$WINDIR" $0
		${NSD_SetText} $R0 "$0\Rainmeter"
	${Else}
		; Disable Directory editbox and Browse button if already installed
		SendMessage $R0 ${EM_SETREADONLY} 1 0

		${If} $INSTDIR != ""
			EnableWindow $R1 0
			${NSD_SetText} $R0 "$INSTDIR"

			${If} ${RunningX64}
			${AndIfNot} ${FileExists} "$INSTDIR\Rainmeter.exe"
				${NSD_Check} $R2
			${EndIf}
		${Else}
			; Fresh install
			${If} ${RunningX64}
				${NSD_SetText} $R0 "$PROGRAMFILES64\Rainmeter"
				${NSD_Check} $R2
			${Else}
				${NSD_SetText} $R0 "$PROGRAMFILES\Rainmeter"
			${EndIf}
		${EndIf}
	${EndIf}

	; Show UAC shield on Install button if requiredd
	GetDlgItem $0 $HWNDPARENT 1
	${If} $InstallPortable = 1
		SendMessage $0 ${BCM_SETSHIELD} 0 0
	${Else}
		SendMessage $0 ${BCM_SETSHIELD} 0 1

		; Hide Back button
		GetDlgItem $0 $HWNDPARENT 3
		ShowWindow $0 ${SW_HIDE}
	${EndIf}

	nsDialogs::Show
FunctionEnd

Function PageOptionsDirectoryOnChange
	${NSD_GetText} $R0 $0

	; Disable Install button if not enough space
	GetDlgItem $1 $HWNDPARENT 1
	${GetRoot} $0 $2
	${DriveSpace} "$2\" "/D=F /S=M" $3
	${If} $3 < ${REQUIREDSPACE}
		EnableWindow $1 0
	${Else}
		EnableWindow $1 1
	${EndIf}

	StrCpy $Install64Bit 0
	${If} ${RunningX64}
		${If} ${FileExists} "$0\Rainmeter.exe"
			MoreInfo::GetProductVersion "$0\Rainmeter.exe"
			Pop $0
			StrCpy $0 $0 2 -7
			${If} $0 == 64
				StrCpy $Install64Bit 1
			${EndIf}

			${If} $R2 != 0
				${NSD_SetState} $R2 $Install64Bit
				EnableWindow $R2 0
			${EndIf}
		${Else}
			${If} $R2 != 0
				EnableWindow $R2 1
			${EndIf}
		${EndIf}
	${EndIf}
FunctionEnd

Function PageOptionsBrowseOnClick
	${NSD_GetText} $R0 $0
	nsDialogs::SelectFolderDialog "$(^DirBrowseText)" $0
	Pop $1
	${If} $1 != error
		${NSD_SetText} $R0 $1
	${EndIf}
FunctionEnd

Function PageOptionsOnLeave
	; Verify that selected folder is writable
	${NSD_GetText} $R0 $0
	${If} $InstallPortable = 1
		ClearErrors
		CreateDirectory "$0"
		WriteINIStr "$0\writetest~.rm" "1" "1" "1"
		Delete "$0\writetest~.rm"

		${If} ${Errors}
			RMDir "$0"
			MessageBox MB_OK|MB_ICONEXCLAMATION "$(WRITEERROR)"
			Abort
		${EndIf}

		RMDir "$0"
	${EndIf}

	StrCpy $INSTDIR $0

	GetDlgItem $0 $HWNDPARENT 1
	EnableWindow $0 0

	${If} $R2 != 0
		${NSD_GetState} $R2 $Install64Bit
	${EndIf}

	${If} $R3 != 0
		${NSD_GetState} $R3 $AutoStartup
	${EndIf}

	${If} $InstallPortable <> 1
		${IfNot} ${UAC_IsAdmin}
			; UAC_IsAdmin seems to return incorrect result sometimes. Recheck with UserInfo::GetAccountType to be sure.
			UserInfo::GetAccountType
			Pop $0
			${If} $0 != "Admin"
UAC_TryAgain:
				!insertmacro UAC_RunElevated
				${Switch} $0
				${Case} 0
					${IfThen} $1 = 1 ${|} Quit ${|}
					${IfThen} $3 <> 0 ${|} ${Break} ${|}
					${If} $1 = 3
						MessageBox MB_OK|MB_ICONSTOP|MB_TOPMOST|MB_SETFOREGROUND "$(ADMINERROR)" /SD IDNO IDOK UAC_TryAgain IDNO 0
					${EndIf}
				${Case} 1223
					Quit
				${Case} 1062
					MessageBox MB_OK|MB_ICONSTOP|MB_TOPMOST|MB_SETFOREGROUND "$(LOGONERROR)"
					Quit
				${Default}
					MessageBox MB_OK|MB_ICONSTOP|MB_TOPMOST|MB_SETFOREGROUND "$(UACERROR) ($0)"
					Quit
				${EndSwitch}
			${EndIf}
		${EndIf}
	${EndIf}
FunctionEnd

!macro InstallFiles DIR ARCH
	SetOutPath "$INSTDIR"
	File "..\..\${DIR}-Release\Rainmeter.exe"
	File "..\..\${DIR}-Release\Rainmeter.dll"
	File "..\..\${DIR}-Release\SkinInstaller.exe"

	SetOutPath "$INSTDIR\Plugins"
	File /x *Example*.dll "..\..\${DIR}-Release\Plugins\*.dll"
!macroend

!macro RemoveStartMenuShortcuts STARTMENUPATH
	Delete "${STARTMENUPATH}\Rainmeter.lnk"
	Delete "${STARTMENUPATH}\Rainmeter Help.lnk"
	Delete "${STARTMENUPATH}\Rainmeter Help.URL"
	Delete "${STARTMENUPATH}\Remove Rainmeter.lnk"
	Delete "${STARTMENUPATH}\RainThemes.lnk"
	Delete "${STARTMENUPATH}\RainThemes Help.lnk"
	Delete "${STARTMENUPATH}\RainBrowser.lnk"
	Delete "${STARTMENUPATH}\RainBackup.lnk"
	Delete "${STARTMENUPATH}\Rainstaller.lnk"
	Delete "${STARTMENUPATH}\Skin Installer.lnk"
	Delete "${STARTMENUPATH}\Rainstaller Help.lnk"
	RMDir "${STARTMENUPATH}"
!macroend

Section
	SetOutPath "$PLUGINSDIR"
	SetShellVarContext current

	Var /GLOBAL InstArc
	${If} $Install64Bit = 1
		StrCpy $InstArc "x64"
	${Else}
		StrCpy $InstArc "x86"
	${EndIf}

	SetOutPath "$INSTDIR"

	; Close Rainmeter (and wait up to five seconds)
	${ForEach} $0 10 0 - 1
		FindWindow $1 "DummyRainWClass" "Rainmeter control window"
		ClearErrors
		Delete "$INSTDIR\Rainmeter.exe"
		${If} $1 = 0
		${AndIfNot} ${Errors}
			${Break}
		${EndIf}

		SendMessage $1 ${WM_CLOSE} 0 0

		${If} $0 = 0
			MessageBox MB_RETRYCANCEL|MB_ICONSTOP "$(RAINMETERCLOSEERROR)" IDRETRY +2
			Quit
		${EndIf}

		Sleep 500
	${Next}

	; Move Rainmeter.ini to %APPDATA% if needed
	${If} ${FileExists} "$INSTDIR\Rainmeter.ini"
		${If} $InstallPortable <> 1
			${If} $Install64Bit = 1
			${AndIf} "$INSTDIR" == "$PROGRAMFILES64\Rainmeter"
			${OrIf} "$INSTDIR" == "$PROGRAMFILES\Rainmeter"
				MessageBox MB_YESNO|MB_ICONEXCLAMATION "$(SETTINGSFILEERROR)" IDNO SkipIniMove
				StrCpy $0 1
				!insertmacro UAC_AsUser_Call Function CopyIniToAppData ${UAC_SYNCREGISTERS}
				${If} $0 = 1
					; Copy succeeded
					Delete "$INSTDIR\Rainmeter.ini"
				${Else}
					MessageBox MB_OK|MB_ICONSTOP "$(SETTINGSMOVEERROR)"
				${EndIf}
SkipIniMove:
			${EndIf}
		${Else}
			ReadINIStr $0 "$INSTDIR\Rainmeter.ini" "Rainmeter" "SkinPath"
			${If} $0 == "$INSTDIR\Skins\"
				DeleteINIStr "$INSTDIR\Rainmeter.ini" "Rainmeter" "SkinPath"
			${EndIf}
		${EndIf}
	${EndIf}

	SetOutPath "$INSTDIR"

	; Cleanup old stuff
	Delete "$INSTDIR\Rainmeter.chm"
	Delete "$INSTDIR\Default.ini"
	Delete "$INSTDIR\Launcher.exe"
	Delete "$INSTDIR\SkinInstaller.dll"
	Delete "$INSTDIR\Defaults\Plugins\FileView.dll"
	Delete "$INSTDIR\Defaults\Plugins\AudioLevel.dll"
	RMDir /r "$INSTDIR\Addons\Rainstaller"
	RMDir /r "$INSTDIR\Addons\RainBackup"
	RMDir /r "$INSTDIR\Runtime"

	${If} $InstallPortable <> 1
		CreateDirectory "$INSTDIR\Defaults"
		Rename "$INSTDIR\Skins" "$INSTDIR\Defaults\Skins"

		Rename "$INSTDIR\Themes" "$INSTDIR\Defaults\Layouts"
		Rename "$INSTDIR\Defaults\Themes" "$INSTDIR\Defaults\Layouts"
		${Locate} "$INSTDIR\Defaults\Layouts" "/L=F /M=Rainmeter.thm /G=1" "RenameToRainmeterIni"

		${If} ${FileExists} "$INSTDIR\Addons\Backup"
		${OrIf} ${FileExists} "$INSTDIR\Plugins\Backup"
			CreateDirectory "$INSTDIR\Defaults\Backup"
			Rename "$INSTDIR\Addons\Backup" "$INSTDIR\Defaults\Backup\Addons"
			Rename "$INSTDIR\Plugins\Backup" "$INSTDIR\Defaults\Backup\Plugins"
		${EndIf}

		Rename "$INSTDIR\Addons" "$INSTDIR\Defaults\Addons"
		${Locate} "$INSTDIR\Plugins" "/L=F /M=*.dll /G=0" "HandlePlugins"
	${EndIf}

!ifdef INCLUDEFILES
	File "..\..\Application\Rainmeter.exe.config"

	${If} $instArc == "x86"
		!insertmacro InstallFiles "x32" "x86"
	${Else}
		!insertmacro InstallFiles "x64" "x64"
	${EndIf}

	RMDir /r "$INSTDIR\Languages"
	SetOutPath "$INSTDIR\Languages"
	File "..\..\x32-Release\Languages\*.*"

	SetOutPath "$INSTDIR\Defaults\Skins"
	File /r "..\Skins\*.*"

	SetOutPath "$INSTDIR\Defaults\Layouts"
	File /r "..\Layouts\*.*"
!endif

	SetOutPath "$INSTDIR"

	${If} $InstallPortable <> 1
		ReadRegStr $0 HKLM "SOFTWARE\Rainmeter" ""
		WriteRegStr HKLM "SOFTWARE\Rainmeter" "" "$INSTDIR"
		WriteRegStr HKLM "SOFTWARE\Rainmeter" "Language" "$LANGUAGE"
		WriteRegDWORD HKLM "SOFTWARE\Rainmeter" "NonDefault" $NonDefaultLanguage

		WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Rainmeter" "DisplayName" "Rainmeter"
		WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Rainmeter" "DisplayIcon" "$INSTDIR\Rainmeter.exe,0"
		WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Rainmeter" "URLInfoAbout" "http://rainmeter.net"
		WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Rainmeter" "UninstallString" "$INSTDIR\uninst.exe"

!ifdef BETA
		WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Rainmeter" "DisplayVersion" "${VERSION_SHORT} beta r${VERSION_REVISION}"
!else
		WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Rainmeter" "DisplayVersion" "${VERSION_SHORT} r${VERSION_REVISION}"
!endif

		; Create .rmskin association
		WriteRegStr HKCR ".rmskin" "" "Rainmeter.SkinInstaller"
		DeleteRegKey HKCR "Rainmeter skin"	; Old key
		WriteRegStr HKCR "Rainmeter.SkinInstaller" "" "Rainmeter Skin Installer"
		WriteRegStr HKCR "Rainmeter.SkinInstaller\shell" "" "open"
		WriteRegStr HKCR "Rainmeter.SkinInstaller\DefaultIcon" "" "$INSTDIR\SkinInstaller.exe,0"
		WriteRegStr HKCR "Rainmeter.SkinInstaller\shell\open\command" "" '"$INSTDIR\SkinInstaller.exe" %1'
		WriteRegStr HKCR "Rainmeter.SkinInstaller\shell\edit" "" "Install Rainmeter skin"
		WriteRegStr HKCR "Rainmeter.SkinInstaller\shell\edit\command" "" '"$INSTDIR\SkinInstaller.exe" %1'

		; If .inc isn't associated, use the .ini association for it.
		ReadRegStr $1 HKCR ".inc" ""
		${If} $1 == ""
			ReadRegStr $1 HKCR ".ini" ""
			${If} $1 != ""
				WriteRegStr HKCR ".inc" "" "$1"
			${EndIf}
		${EndIf}

		; Refresh shell icons if new install
		${If} $0 == ""
			${RefreshShellIcons}
		${EndIf}

		; Remove all start menu shortcuts
		SetShellVarContext all
		Call RemoveStartMenuShortcuts

		StrCpy $0 "$SMPROGRAMS\Rainmeter.lnk"
		${If} ${FileExists} "$SMPROGRAMS\Rainmeter"
			StrCpy $0 "$SMPROGRAMS\Rainmeter\Rainmeter.lnk"
		${EndIf}
		CreateShortcut "$0" "$INSTDIR\Rainmeter.exe" "" "$INSTDIR\Rainmeter.exe" 0

		${If} $AutoStartup = 1
			${If} ${FileExists} "$SMSTARTUP\Rainmeter.lnk"
				; Remove user shortcut to prevent duplicate with all users shortcut
				!insertmacro UAC_AsUser_Call Function RemoveUserStartupShortcut ${UAC_SYNCREGISTERS}
			${Else}
				!insertmacro UAC_AsUser_Call Function CreateUserStartupShortcut ${UAC_SYNCREGISTERS}
			${EndIf}
		${EndIf}

		SetShellVarContext current
		Call RemoveStartMenuShortcuts

		!insertmacro UAC_AsUser_Call Function RemoveStartMenuShortcuts ${UAC_SYNCREGISTERS}

		WriteUninstaller "$INSTDIR\uninst.exe"
	${Else}
		${IfNot} ${FileExists} "Rainmeter.ini"
			CopyFiles /SILENT "$INSTDIR\Defaults\Layouts\illustro default\Rainmeter.ini" "$INSTDIR\Rainmeter.ini"
		${EndIf}

		WriteINIStr "$INSTDIR\Rainmeter.ini" "Rainmeter" "Language" "$LANGUAGE"
	${EndIf}
SectionEnd

Function CopyIniToAppData
	ClearErrors
	CreateDirectory "$APPDATA\Rainmeter"
	CopyFiles /SILENT "$INSTDIR\Rainmeter.ini" "$APPDATA\Rainmeter\Rainmeter.ini"
	${If} ${Errors}
		StrCpy $0 0
	${EndIf}
FunctionEnd

Function RenameToRainmeterIni
	${If} ${FileExists} "$R8\Rainmeter.ini"
		Delete "$R8\Rainmeter.thm"
	${Else}
		Rename "$R9" "$R8\Rainmeter.ini"
	${EndIf}

	Push $0
FunctionEnd

Function HandlePlugins
	${If} $R7 == "MediaKey.dll"
	${OrIf} $R7 == "NowPlaying.dll"
	${OrIf} $R7 == "RecycleManager.dll"
	${OrIf} $R7 == "WebParser.dll"
		Delete "$R9"
	${ElseIf} $R7 != "ActionTimer.dll"
	${AndIf} $R7 != "AdvancedCPU.dll"
	${AndIf} $R7 != "CoreTemp.dll"
	${AndIf} $R7 != "FileView.dll"
	${AndIf} $R7 != "FolderInfo.dll"
	${AndIf} $R7 != "InputText.dll"
	${AndIf} $R7 != "iTunesPlugin.dll"
	${AndIf} $R7 != "PerfMon.dll"
	${AndIf} $R7 != "PingPlugin.dll"
	${AndIf} $R7 != "PowerPlugin.dll"
	${AndIf} $R7 != "Process.dll"
	${AndIf} $R7 != "QuotePlugin.dll"
	${AndIf} $R7 != "RecycleManager.dll"
	${AndIf} $R7 != "ResMon.dll"
	${AndIf} $R7 != "RunCommand.dll"
	${AndIf} $R7 != "SpeedFanPlugin.dll"
	${AndIf} $R7 != "SysInfo.dll"
	${AndIf} $R7 != "VirtualDesktops.dll"
	${AndIf} $R7 != "WifiStatus.dll"
	${AndIf} $R7 != "Win7AudioPlugin.dll"
	${AndIf} $R7 != "WindowMessagePlugin.dll"
		CreateDirectory "$INSTDIR\Defaults\Plugins"
		Delete "$INSTDIR\Defaults\Plugins\$R7"
		Rename "$R9" "$INSTDIR\Defaults\Plugins\$R7"
	${EndIf}

	Push $0
FunctionEnd

Function RemoveStartMenuShortcuts
	!insertmacro RemoveStartMenuShortcuts "$SMPROGRAMS\Rainmeter"
FunctionEnd

Function CreateUserStartupShortcut
	SetShellVarContext current
	CreateShortcut "$SMSTARTUP\Rainmeter.lnk" "$INSTDIR\Rainmeter.exe" "" "$INSTDIR\Rainmeter.exe" 0
FunctionEnd

Function RemoveUserStartupShortcut
	SetShellVarContext current
	Delete "$SMSTARTUP\Rainmeter.lnk"
FunctionEnd

Function FinishRun
	!insertmacro UAC_AsUser_ExecShell "" "$INSTDIR\Rainmeter.exe" "" "" ""
FunctionEnd


; Uninstall
; --------------------------------------
Function un.onInit
UAC_TryAgain:
	; Request administrative rights
	!insertmacro UAC_RunElevated
	${Switch} $0
	${Case} 0
		${IfThen} $1 = 1 ${|} Quit ${|}
		${IfThen} $3 <> 0 ${|} ${Break} ${|}
		${If} $1 = 3
			MessageBox MB_OK|MB_ICONSTOP|MB_TOPMOST|MB_SETFOREGROUND "$(ADMINERROR)" /SD IDNO IDOK UAC_TryAgain IDNO 0
		${EndIf}
	${Case} 1223
		Quit
	${Case} 1062
		MessageBox MB_OK|MB_ICONSTOP|MB_TOPMOST|MB_SETFOREGROUND "$(LOGONERROR)"
		Quit
	${Default}
		MessageBox MB_OK|MB_ICONSTOP|MB_TOPMOST|MB_SETFOREGROUND "$(UACERROR) ($0)"
		Quit
	${EndSwitch}

	ReadRegStr $0 HKLM "SOFTWARE\Rainmeter" "Language"
	${If} $0 != ""
		StrCpy $LANGUAGE $0
	${EndIf}
FunctionEnd

Function un.PageOptions
	!insertmacro MUI_HEADER_TEXT "$(UNSTALLOPTIONS)" "$(UNSTALLOPTIONSDESC)"
	nsDialogs::Create 1018
	nsDialogs::SetRTL $(^RTL)

	${NSD_CreateCheckbox} 0 0u 95% 12u "$(UNSTALLRAINMETER)"
	Pop $0
	EnableWindow $0 0
	${NSD_Check} $0

	${NSD_CreateCheckbox} 0 15u 70% 12u "$(UNSTALLSETTINGS)"
	Pop $R0

	${NSD_CreateLabel} 16 26u 95% 12u "$(UNSTALLSETTINGSDESC)"

	nsDialogs::Show
FunctionEnd

Function un.GetOptions
	${NSD_GetState} $R0 $un.DeleteAll
FunctionEnd

Section Uninstall
	; Close Rainmeter (and wait up to five seconds)
	${ForEach} $0 10 0 - 1
		FindWindow $1 "DummyRainWClass" "Rainmeter control window"
		ClearErrors
		Delete "$INSTDIR\Rainmeter.exe"
		${If} $1 = 0
		${AndIfNot} ${Errors}
			${Break}
		${EndIf}

		SendMessage $1 ${WM_CLOSE} 0 0

		${If} $0 = 0
			MessageBox MB_RETRYCANCEL|MB_ICONSTOP "$(RAINMETERCLOSEERROR)" IDRETRY +2
			Quit
		${EndIf}

		Sleep 500
	${Next}

	; Old stuff
	RMDir /r "$INSTDIR\Addons"
	RMDir /r "$INSTDIR\Fonts"

	RMDir /r "$INSTDIR\Defaults"
	RMDir /r "$INSTDIR\Languages"
	RMDir /r "$INSTDIR\Plugins"
	RMDir /r "$INSTDIR\Runtime"
	RMDir /r "$INSTDIR\Skins"
	Delete "$INSTDIR\Rainmeter.dll"
	Delete "$INSTDIR\Rainmeter.exe"
	Delete "$INSTDIR\Rainmeter.exe.config"
	Delete "$INSTDIR\SkinInstaller.exe"
	Delete "$INSTDIR\SkinInstaller.dll"
	Delete "$INSTDIR\uninst.exe"

	RMDir "$INSTDIR"

	SetShellVarContext all
	RMDir /r "$APPDATA\Rainstaller"

	SetShellVarContext current
	Call un.RemoveShortcuts
	${If} $un.DeleteAll = 1
		RMDir /r "$APPDATA\Rainmeter"
		RMDir /r "$DOCUMENTS\Rainmeter\Skins"
		RMDir "$DOCUMENTS\Rainmeter"
		RMDir /r "$1\Rainmeter"
	${EndIf}
	
	!insertmacro UAC_AsUser_Call Function un.RemoveShortcuts ${UAC_SYNCREGISTERS}
	${If} $un.DeleteAll = 1
		RMDir /r "$APPDATA\Rainmeter"
		RMDir /r "$DOCUMENTS\Rainmeter\Skins"
		RMDir "$DOCUMENTS\Rainmeter"
	${EndIf}

	SetShellVarContext all
	Call un.RemoveShortcuts
	Delete "$SMPROGRAMS\Rainmeter.lnk"

	DeleteRegKey HKLM "SOFTWARE\Rainmeter"
	DeleteRegKey HKCR ".rmskin"
	DeleteRegKey HKCR "Rainmeter.SkinInstaller"
	DeleteRegKey HKLM "SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\Rainmeter"
	${RefreshShellIcons}
SectionEnd

Function un.RemoveShortcuts
	!insertmacro RemoveStartMenuShortcuts "$SMPROGRAMS\Rainmeter"
	Delete "$SMSTARTUP\Rainmeter.lnk"
	Delete "$DESKTOP\Rainmeter.lnk"
FunctionEnd
