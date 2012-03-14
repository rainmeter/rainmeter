!verbose 3
!addplugindir ".\"
!ifndef VER
 !define VER "0.0"
 !define REV "000"
!endif
!ifdef BETA
 !define OUTFILE "Rainmeter-${VER}-r${REV}-beta.exe"
!else
 !define OUTFILE "Rainmeter-${VER}.exe"
!endif

Name "Rainmeter"
VIAddVersionKey "ProductName" "Rainmeter"
VIAddVersionKey "FileDescription" "Rainmeter Installer"
VIAddVersionKey "FileVersion" "${VER}.0"
VIAddVersionKey "ProductVersion" "${VER}.0.${REV}"
VIAddVersionKey "OriginalFilename" "${OUTFILE}"
VIAddVersionKey "LegalCopyright" "Copyright (C) 2009-2012 - All authors"
VIProductVersion "${VER}.0.${REV}"
BrandingText " "
SetCompressor /SOLID lzma
RequestExecutionLevel user
InstallDirRegKey HKLM "SOFTWARE\Rainmeter" ""
XPStyle on
OutFile "..\${OUTFILE}"
ReserveFile "${NSISDIR}\Plugins\LangDLL.dll"
ReserveFile "${NSISDIR}\Plugins\nsDialogs.dll"
ReserveFile "${NSISDIR}\Plugins\System.dll"
ReserveFile ".\UAC.dll"

!include "MUI2.nsh"
!include "x64.nsh"
!include "ProcFunc.nsh"
!include "FileFunc.nsh"
!include "WordFunc.nsh"
!include "WinVer.nsh"
!include "UAC.nsh"

!define BCM_SETSHIELD 0x0000160c

!define MUI_HEADERIMAGE
!define MUI_ICON ".\Icon.ico"
!define MUI_UNICON ".\Icon.ico"
!define MUI_HEADERIMAGE_BITMAP ".\Header.bmp"
!define MUI_HEADERIMAGE_UNBITMAP ".\Header.bmp"
!define MUI_WELCOMEFINISHPAGE_BITMAP ".\Wizard.bmp"
!define MUI_FINISHPAGE_RUN
!define MUI_FINISHPAGE_RUN_FUNCTION FinishRun

!define MUI_PAGE_CUSTOMFUNCTION_SHOW PageWelcomeOnShow
!insertmacro MUI_PAGE_WELCOME
Page custom PageOptions PageOptionsOnLeave
!define MUI_PAGE_CUSTOMFUNCTION_SHOW PageDirectoryOnShow
!define MUI_PAGE_CUSTOMFUNCTION_LEAVE PageDirectoryOnLeave
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_PAGE_FINISH

UninstPage custom un.PageOptions un.GetOptions
!insertmacro MUI_UNPAGE_INSTFILES

!macro IncludeLanguage LANGUAGE CUSTOMLANGUAGE
	; Modified variant of the MUI_LANGUAGE macro in Localization.h
	; Uses "EnglishLangName (LocalizedLangName)" instead of "LocalizedLangName" for the language selection dialog
	!insertmacro MUI_INSERT

	LoadLanguageFile "${NSISDIR}\Contrib\Language files\${LANGUAGE}.nlf"

	; Include language file
	!insertmacro LANGFILE_INCLUDE_WITHDEFAULT "${NSISDIR}\Contrib\Language files\${LANGUAGE}.nsh" "${NSISDIR}\Contrib\Language files\English.nsh"

	;Add language to list of languages for selection dialog
	!ifndef MUI_LANGDLL_LANGUAGES
		!define MUI_LANGDLL_LANGUAGES "'${LANGFILE_${LANGUAGE}_NAME}' '${LANG_${LANGUAGE}}' "
		!define MUI_LANGDLL_LANGUAGES_CP "'${LANGFILE_IDNAME} -  ${LANGFILE_${LANGUAGE}_NAME}' '${LANG_${LANGUAGE}}' '${LANG_${LANGUAGE}_CP}' "
	!else
		!ifdef MUI_LANGDLL_LANGUAGES_TEMP
			!undef MUI_LANGDLL_LANGUAGES_TEMP
		!endif
		!define MUI_LANGDLL_LANGUAGES_TEMP "${MUI_LANGDLL_LANGUAGES}"
		!undef MUI_LANGDLL_LANGUAGES

		!ifdef MUI_LANGDLL_LANGUAGES_CP_TEMP
			!undef MUI_LANGDLL_LANGUAGES_CP_TEMP
		!endif
		!define MUI_LANGDLL_LANGUAGES_CP_TEMP "${MUI_LANGDLL_LANGUAGES_CP}"
		!undef MUI_LANGDLL_LANGUAGES_CP

		!define MUI_LANGDLL_LANGUAGES "'${LANGFILE_${LANGUAGE}_NAME}' '${LANG_${LANGUAGE}}' ${MUI_LANGDLL_LANGUAGES_TEMP}"
		!define MUI_LANGDLL_LANGUAGES_CP "'${LANGFILE_IDNAME} -  ${LANGFILE_${LANGUAGE}_NAME}' '${LANG_${LANGUAGE}}' '${LANG_${LANGUAGE}_CP}' ${MUI_LANGDLL_LANGUAGES_CP_TEMP}"
	!endif

	!insertmacro LANGFILE_INCLUDE "..\..\Language\${CUSTOMLANGUAGE}.nsh"
!macroend

!include "Languages.nsh"

; Error levels (for silent install)
!define ERROR_UNSUPPORTED	3
!define ERROR_NOTADMIN		4
!define ERROR_WRITEFAIL		5
!define ERROR_NOVCREDIST	6
!define ERROR_CLOSEFAIL		7

Var ctlDesktop
Var ctlStartup
Var ctlAllUsers
Var ctlDelete
Var ctlStandard
Var ctlPortable
Var ctlBit32
Var ctlBit64
Var instArc
Var instType
Var defLang


; Install
; --------------------------------------
Function .onInit
	${If} ${RunningX64}
		${EnableX64FSRedirection}
	${EndIf}

	${IfNot} ${UAC_IsInnerInstance}
		${If} ${IsWin2000}
			${If} ${Silent}
				SetErrorLevel ${ERROR_UNSUPPORTED}
			${Else}
				MessageBox MB_OK|MB_ICONINFORMATION "$(WIN2KERROR)"
			${EndIf}
			Quit
		${ElseIf} ${IsWinXP}
		${AndIf} ${AtMostServicePack} 1
			${If} ${Silent}
				SetErrorLevel ${ERROR_UNSUPPORTED}
			${Else}
				MessageBox MB_OK|MB_ICONINFORMATION "$(WINXPS2ERROR)"
			${EndIf}
			Quit
		${ElseIf} ${IsWin2003}
		${AndIf} ${AtMostServicePack} 0
			${If} ${Silent}
				SetErrorLevel ${ERROR_UNSUPPORTED}
			${Else}
				MessageBox MB_OK|MB_ICONINFORMATION "$(WIN2003SP1ERROR)"
			${EndIf}
			Quit
		${EndIf}

		StrCpy $R0 $LANGUAGE
		StrCpy $defLang "0"
		ReadRegDWORD $0 HKLM "SOFTWARE\Rainmeter" "NonDefault"
		${If} $0 == 1
			; Rainmeter already installed and user selected non-default language last time
			ReadRegStr $0 HKLM "SOFTWARE\Rainmeter" "Language"
			${If} $0 != ""
				StrCpy $LANGUAGE $0
			${EndIf}
		${EndIf}

		${IfNot} ${Silent}
			LangDLL::LangDialog "$(^SetupCaption)" "Please select the installer language.$\n$(SELECTLANGUAGE)" AC ${MUI_LANGDLL_LANGUAGES_CP} ""
			Pop $LANGUAGE
			${If} $LANGUAGE == "cancel"
				Abort
			${EndIf}

			${If} $LANGUAGE == $R0
				; User selected default language
				StrCpy $defLang "1"
			${EndIf}
		${Else}
			${GetParameters} $R1

			ClearErrors
			${GetOptions} $R1 "/LANGUAGE=" $0
			${IfNot} ${Errors}
				StrCpy $LANGUAGE $0
			${EndIf}
			${If} $LANGUAGE == $R0
				; User selected default language
				StrCpy $defLang "1"
			${EndIf}

			${GetOptions} $R1 "/DESKTOPSHORTCUT=" $0
			${If} $0 = 1
				StrCpy $ctlDesktop "1"
			${EndIf}

			${GetOptions} $R1 "/STARTUP=" $0
			${If} $0 = 1
				StrCpy $ctlStartup "1"
			${EndIf}

			${GetOptions} $R1 "/ALLUSERS=" $0
			${If} $0 = 1
				StrCpy $ctlAllUsers "1"
			${EndIf}

			${GetOptions} $R1 "/PORTABLE=" $0
			${If} $0 = 1
				StrCpy $instType "P"
			${Else}
				${IfNot} ${UAC_IsAdmin}
					SetErrorLevel ${ERROR_NOTADMIN}
					Quit
				${EndIf}

				StrCpy $instType "S"
			${EndIf}

			${GetOptions} $R1 "/VERSION=" $0
			${If} $0 = 64
				StrCpy $instArc "x64"

				${If} $INSTDIR == ""
					StrCpy $INSTDIR "$PROGRAMFILES64\Rainmeter"
				${EndIf}
			${Else}
				StrCpy $instArc "x86"

				${If} $INSTDIR == ""
					StrCpy $INSTDIR "$PROGRAMFILES\Rainmeter"
				${EndIf}
			${EndIf}

			ClearErrors
			CreateDirectory "$INSTDIR"
			WriteINIStr "$INSTDIR\_rainmeter_writetest.tmp" "1" "1" "1"
			Delete "$INSTDIR\_rainmeter_writetest.tmp"

			${If} ${Errors}
				SetErrorLevel ${ERROR_WRITEFAIL}
				Quit
			${EndIf}
		${EndIf}
	${Else}
		; Sync variables with user instance
		!insertmacro UAC_AsUser_Call Function ExchangeVars ${UAC_SYNCREGISTERS}
		StrCpy $instType "S"
		StrCpy $ctlDesktop $0
		StrCpy $ctlStartup $1
		StrCpy $ctlAllUsers $2
		StrCpy $instArc $3
		StrCpy $defLang $4
		StrCpy $LANGUAGE $5
	${EndIf}
FunctionEnd

Function ExchangeVars
	StrCpy $0 $ctlDesktop
	StrCpy $1 $ctlStartup
	StrCpy $2 $ctlAllUsers
	StrCpy $3 $instArc
	StrCpy $4 $defLang
	StrCpy $5 $LANGUAGE
	HideWindow
FunctionEnd

Function PageWelcomeOnShow
	; Skip to the directory page if we're the elevated process
	${If} ${UAC_IsInnerInstance}
		${If} ${UAC_IsAdmin}
			SendMessage $HWNDPARENT "0x408" "2" ""
		${Else}
			MessageBox MB_OK|MB_ICONSTOP "$(ADMINERROR) (Inner)"
			Quit
		${EndIf}
	${EndIf}
FunctionEnd

Function PageOptions
	!insertmacro MUI_HEADER_TEXT "$(INSTALLOPTIONS)" "$(INSTALLOPTIONSDESC)"
	nsDialogs::Create 1018
	nsDialogs::SetRTL $(^RTL)

	${NSD_CreateRadioButton} 0 0u 310u 12u "$(STANDARDINST)"
	Pop $ctlStandard
	${NSD_AddStyle} $ctlStandard ${WS_GROUP}
	SendMessage $ctlStandard ${WM_SETFONT} $mui.Header.Text.Font 0
	${NSD_OnClick} $ctlStandard setStandard

	${NSD_CreateLabel} 12u 12u 285u 12u "$(STANDARDINSTDESC)"

	${NSD_CreateRadioButton} 0 24u 310u 12u "$(PORTABLEINST)"
	Pop $ctlPortable
	${NSD_AddStyle} $ctlPortable ${WS_TABSTOP}
	SendMessage $ctlPortable ${WM_SETFONT} $mui.Header.Text.Font 0
	${NSD_OnClick} $ctlPortable setPortable

	${NSD_CreateLabel} 12u 36u 285u 32u "$(PORTABLEINSTDESC)"

	${NSD_CreateGroupBox} 0 72u 200u 54u "$(ADDITIONALOPTIONS)"

	Push $ctlDesktop
	${NSD_CreateCheckbox} 6u 84u 190u 12u "$(DESKTOPSHORTCUT)"
	Pop $ctlDesktop
	Pop $0
	StrCmp $0 "1" 0 +2
		${NSD_Check} $ctlDesktop

	Push $ctlAllUsers
	${NSD_CreateCheckbox} 6u 96u 190u 12u "$(ALLUSERSSHORTCUT)"
	Pop $ctlAllUsers
	Pop $0
	StrCmp $0 "1" 0 +2
		${NSD_Check} $ctlAllUsers

	Push $ctlStartup
	${NSD_CreateCheckbox} 6u 108u 190u 12u "$(AUTOSTARTUP)"
	Pop $ctlStartup
	Pop $0
	StrCmp $0 "1" 0 +2
		${NSD_Check} $ctlStartup

	${NSD_CreateGroupBox} 205u 72u 94u 40u "$(RAINMETERVERSION)"

	${NSD_CreateRadioButton} 211u 82u 80u 12u "$(32BIT)"
	Pop $ctlBit32
	${NSD_AddStyle} $ctlBit32 ${WS_GROUP}

	${NSD_CreateRadioButton} 211u 94u 80u 12u "$(64BIT)"
	Pop $ctlBit64

	ReadRegStr $0 HKLM "Software\Rainmeter" ""
	${If} $0 == ""
		${NSD_Check} $ctlStartup
		${NSD_Check} $ctlAllUsers
	${Else}
		SetShellVarContext all
		Call GetEnvPaths
		StrCpy $R1 $1
		StrCpy $R2 $2
		StrCpy $R3 $3
		SetShellVarContext current
		!insertmacro UAC_AsUser_Call Function GetEnvPaths ${UAC_SYNCREGISTERS}

		${If} ${FileExists} "$R1\Rainmeter\Rainmeter.lnk"
			${NSD_Check} $ctlAllUsers
		${EndIf}
		${If} ${FileExists} "$R2\Rainmeter.lnk"
		${OrIf} ${FileExists} "$2\Rainmeter.lnk"
			${NSD_Check} $ctlStartup
		${EndIf}
		${If} ${FileExists} "$R3\Rainmeter.lnk"
		${OrIf} ${FileExists} "$3\Rainmeter.lnk"
			${NSD_Check} $ctlDesktop
		${EndIf}
	${EndIf}

	${If} $instType == "P"
		${NSD_Check} $ctlPortable
		Call SetPortable
	${Else}
		Call SetStandard
		${NSD_Check} $ctlStandard
	${EndIf}

	nsDialogs::Show
FunctionEnd

Function SetStandard
	EnableWindow $ctlDesktop 1
	EnableWindow $ctlAllUsers 1
	EnableWindow $ctlStartup 1

	${If} ${RunningX64}
		${If} ${FileExists} "$INSTDIR\Rainmeter.exe"
			MoreInfo::GetProductVersion "$INSTDIR\Rainmeter.exe"
			Pop $0
			StrCpy $0 $0 2 -7

			${If} $0 == "32"
				${NSD_Check} $ctlBit32
				${NSD_UnCheck} $ctlBit64
				EnableWindow $ctlBit64 0
			${Else}
				${NSD_Check} $ctlBit64
				${NSD_UnCheck} $ctlBit32
				EnableWindow $ctlBit32 0
			${EndIf}
		${Else}
			${NSD_Check} $ctlBit64
		${EndIf}
	${Else}
		${NSD_Check} $ctlBit32
		${NSD_UnCheck} $ctlBit64
		EnableWindow $ctlBit64 0
	${EndIf}

	${IfNot} ${UAC_IsAdmin}
		GetDlgItem $0 $HWNDPARENT 1
		SendMessage $0 ${BCM_SETSHIELD} 0 1
	${EndIf}
FunctionEnd

Function SetPortable
	EnableWindow $ctlDesktop 0
	EnableWindow $ctlAllUsers 0
	EnableWindow $ctlStartup 0
	EnableWindow $ctlBit32 1

	${If} ${RunningX64}
		EnableWindow $ctlBit64 1
	${Endif}

	${IfNot} ${UAC_IsAdmin}
		GetDlgItem $0 $HWNDPARENT 1
		SendMessage $0 ${BCM_SETSHIELD} 0 0
	${EndIf}
FunctionEnd

Function PageOptionsOnLeave
	GetDlgItem $0 $HWNDPARENT 1
	EnableWindow $0 0

	${NSD_GetState} $ctlDesktop $ctlDesktop
	${NSD_GetState} $ctlStartup $ctlStartup
	${NSD_GetState} $ctlAllUsers $ctlAllUsers

	${NSD_GetState} $ctlStandard $0
	${If} $0 == ${BST_CHECKED}
		StrCpy $instType "S"
	${Else}
		StrCpy $instType "P"
	${EndIf}

	${NSD_GetState} $ctlBit32 $0
	${If} $0 == ${BST_CHECKED}
		StrCpy $instArc "x86"
	${Else}
		StrCpy $instArc "x64"
	${EndIf}

	${If} $instType == "S"
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

Function PageDirectoryOnShow
	${If} $instType == "P"
		${GetRoot} "$WINDIR" $0
		${NSD_SetText} $mui.DirectoryPage.Directory "$0\Rainmeter"
	${Else}
		${If} $INSTDIR == ""
			; Fresh install
			${If} $instArc == "x86"
				${If} ${RunningX64}
					${NSD_SetText} $mui.DirectoryPage.Directory "$PROGRAMFILES32\Rainmeter"
				${Else}
					${NSD_SetText} $mui.DirectoryPage.Directory "$PROGRAMFILES\Rainmeter"
				${EndIf}
			${Else}
				${NSD_SetText} $mui.DirectoryPage.Directory "$PROGRAMFILES64\Rainmeter"
			${EndIf}
		${Else}
			; Upgrade install
			EnableWindow $mui.DirectoryPage.Directory 0
			EnableWindow $mui.DirectoryPage.BrowseButton 0

			; Set focus on the Install button
			GetDlgItem $0 $HWNDPARENT 1
			System::Call "user32::SetFocus(i$0)"
		${EndIf}
	${EndIf}
FunctionEnd

Function PageDirectoryOnLeave
	${If} $instType == "P"
		ClearErrors
		CreateDirectory "$INSTDIR"
		WriteINIStr "$INSTDIR\_rainmeter_writetest.tmp" "1" "1" "1"

		${If} ${Errors}
			MessageBox MB_OK|MB_ICONEXCLAMATION "$(WRITEERROR)"
			Abort
		${EndIf}

		Delete "$INSTDIR\_rainmeter_writetest.tmp"
	${EndIf}
FunctionEnd

!macro InstallFiles DIR
	File "..\..\TestBench\${DIR}\Release\Rainmeter.exe"
	File "..\..\TestBench\${DIR}\Release\Rainmeter.dll"
	File "..\..\TestBench\${DIR}\Release\SkinInstaller.exe"

	SetOutPath "$INSTDIR\Plugins"
	File /x *Example*.dll "..\..\TestBench\${DIR}\Release\Plugins\*.dll"
!macroend

!macro RemoveShortcuts
	; $1=$SMPROGRAMS, $2=$SMSTARTUP, $3=$DESKTOP
	Delete "$1\Rainmeter\Rainmeter.lnk"
	Delete "$1\Rainmeter\Rainmeter Help.lnk"
	Delete "$1\Rainmeter\Rainmeter Help.URL"
	Delete "$1\Rainmeter\Remove Rainmeter.lnk"
	Delete "$1\Rainmeter\RainThemes.lnk"
	Delete "$1\Rainmeter\RainThemes Help.lnk"
	Delete "$1\Rainmeter\RainBrowser.lnk"
	Delete "$1\Rainmeter\RainBackup.lnk"
	Delete "$1\Rainmeter\Rainstaller.lnk"
	Delete "$1\Rainmeter\Skin Installer.lnk"
	Delete "$1\Rainmeter\Rainstaller Help.lnk"
	RMDir "$1\Rainmeter"
	Delete "$2\Rainmeter.lnk"
	Delete "$3\Rainmeter.lnk"
!macroend

Section
	SetOutPath "$PLUGINSDIR"
	SetShellVarContext current

	${If} $instType == "S"
		ReadRegDWORD $0 HKLM "SOFTWARE\Microsoft\VisualStudio\10.0\VC\VCRedist\$instArc" "Bld"
		${VersionCompare} "$0" "40219" $1

		ReadRegDWORD $2 HKLM "SOFTWARE\Microsoft\VisualStudio\10.0\VC\VCRedist\$instArc" "Installed"

		; Download and install VC++ redist if required
		${If} $1 == "2"
		${OrIf} $2 != "1"
			${If} ${Silent}
				SetErrorLevel ${ERROR_NOVCREDIST}
				Quit
			${EndIf}

			${If} $instArc == "x86"
				NSISdl::download /TIMEOUT=30000 "http://download.microsoft.com/download/C/6/D/C6D0FD4E-9E53-4897-9B91-836EBA2AACD3/vcredist_x86.exe" "$PLUGINSDIR\vcredist.exe"
				Pop $0
			${Else}
				NSISdl::download /TIMEOUT=30000 "http://download.microsoft.com/download/A/8/0/A80747C3-41BD-45DF-B505-E9710D2744E0/vcredist_x64.exe" "$PLUGINSDIR\vcredist.exe"
				Pop $0
			${EndIf}

			${If} $0 != "cancel"
			${AndIf} $0 != "success"
				; download from MS failed, try from rainmter.net
				Delete "$PLUGINSDIR\vcredist.exe"

				${If} $instArc == "x86"
					NSISdl::download /TIMEOUT=30000 "http://rainmeter.net/redist/vc10SP1redist_x86.exe" "$PLUGINSDIR\vcredist.exe"
					Pop $0
				${Else}
					NSISdl::download /TIMEOUT=30000 "http://rainmeter.net/redist/vc10SP1redist_x64.exe" "$PLUGINSDIR\vcredist.exe"
					Pop $0
				${EndIf}
			${EndIf}

			${If} $0 == "success"
				ExecWait '"$PLUGINSDIR\vcredist.exe" /q /norestart' $0
				Delete "$PLUGINSDIR\vcredist.exe"

				${If} $0 == "3010"
					SetRebootFlag true
				${ElseIf} $0 != "0"
					MessageBox MB_OK|MB_ICONSTOP "$(VCINSTERROR)"
					Quit
				${EndIf}
			${ElseIf} $0 == "cancel"
				Quit
			${Else}
				MessageBox MB_OK|MB_ICONSTOP "$(VCINSTERROR)"
				Quit
			${EndIf}
		${EndIf}

		; Download and install .NET if required
		ReadRegDWORD $0 HKLM "SOFTWARE\Microsoft\NET Framework Setup\NDP\v2.0.50727" "Install"
		${If} $0 != "1"
			${If} $instArc == "x86"
				NSISdl::download /TIMEOUT=30000 "http://download.microsoft.com/download/5/6/7/567758a3-759e-473e-bf8f-52154438565a/dotnetfx.exe" "$PLUGINSDIR\dotnetfx.exe"
			${Else}
				NSISdl::download /TIMEOUT=30000 "http://download.microsoft.com/download/a/3/f/a3f1bf98-18f3-4036-9b68-8e6de530ce0a/NetFx64.exe" "$PLUGINSDIR\dotnetfx.exe"
			${EndIf}

			Pop $0

			${If} $0 != "cancel"
			${AndIf} $0 != "success"
				Delete "$PLUGINSDIR\dotnetfx.exe"

				${If} $instArc == "x86"
					NSISdl::download /TIMEOUT=30000 "http://rainmeter.net/redist/dotnetfx.exe" "$PLUGINSDIR\dotnetfx.exe"
				${Else}
					NSISdl::download /TIMEOUT=30000 "http://rainmeter.net/redist/NetFx64.exe" "$PLUGINSDIR\dotnetfx.exe"
				${EndIf}

				Pop $0
			${EndIf}

			${If} $0 == "success"
				ExecWait '"$PLUGINSDIR\dotnetfx.exe" /q:a /c:"install /q"' $0
				Delete "$PLUGINSDIR\dotnetfx.exe"

				${If} $0 == "3010"
					SetRebootFlag true
				${ElseIf} $0 != "0"
					MessageBox MB_OK|MB_ICONSTOP "$(DOTNETINSTERROR)"
					Quit
				${EndIf}
			${ElseIf} $0 == "cancel"
				Quit
			${Else}
				MessageBox MB_OK|MB_ICONSTOP "$(DOTNETINSTERROR)"
				Quit
			${EndIf}
		${EndIf}
	${EndIf}

	SetOutPath "$INSTDIR"

	FindWindow $0 "RainmeterTrayClass"
	${If} $0 != "0"
		Exec '"$INSTDIR\Rainmeter.exe" !Quit'

		; Wait up to for up to 5 seconds for Rainmeter to close
		StrCpy $1 "0"
		${DoWhile} ${ProcessExists} "Rainmeter.exe"
			IntOp $1 $1 + 1
			${If} $1 >= "10"
				${If} ${Silent}
					SetErrorLevel ${ERROR_CLOSEFAIL}
					Quit
				${Else}
					MessageBox MB_RETRYCANCEL|MB_ICONSTOP "$(RAINMETERCLOSEERROR)" IDRETRY +2
					Quit
				${EndIf}
			${EndIf}
			Sleep 500
			SendMessage $0 ${WM_CLOSE} 0 0
		${Loop}
	${EndIf}

	; Check if Rainmeter.ini is located in the installation folder and
	; if the installation folder is in Program Files
	${IfNot} ${Silent}
	${AndIf} ${FileExists} "$INSTDIR\Rainmeter.ini"
		${If} $instType == "S"
			!ifdef X64
				StrCmp $INSTDIR "$PROGRAMFILES64\Rainmeter" 0 RainmeterIniDoesntExistLabel
			!else
				StrCmp $INSTDIR "$PROGRAMFILES\Rainmeter" 0 RainmeterIniDoesntExistLabel
			!endif

			MessageBox MB_YESNO|MB_ICONEXCLAMATION "$(SETTINGSFILEERROR)" IDNO RainmeterIniDoesntExistLabel
			CreateDirectory $APPDATA\Rainmeter
			Rename "$INSTDIR\Rainmeter.ini" "$APPDATA\Rainmeter\Rainmeter.ini"
			${If} ${Errors}
				MessageBox MB_OK|MB_ICONSTOP "$(SETTINGSMOVEERROR)"
			${EndIf}
		${Else}
			ReadINIStr $0 "$INSTDIR\Rainmeter.ini" "Rainmeter" "SkinPath"
			${If} $0 == "$INSTDIR\Skins\"
				DeleteINIStr "$INSTDIR\Rainmeter.ini" "Rainmeter" "SkinPath"
			${EndIf}
		${EndIf}
	${EndIf}

RainmeterIniDoesntExistLabel:
	SetOutPath "$INSTDIR"
	Delete "$INSTDIR\Rainmeter.exe.config"
	Delete "$INSTDIR\Rainmeter.chm"
	Delete "$INSTDIR\Default.ini"

	${If} $instArc == "x86"
		!insertmacro InstallFiles "x32"
	${Else}
		!insertmacro InstallFiles "x64"
	${EndIf}

	RMDir /r "$INSTDIR\Languages"
	SetOutPath "$INSTDIR\Languages"
	File "..\..\TestBench\x32\Release\Languages\*.*"

	RMDir /r "$INSTDIR\Addons\Rainstaller"

	SetOutPath "$INSTDIR\Skins"
	RMDir /r "$INSTDIR\Skins\illustro"
	Delete "$INSTDIR\Skins\*.txt"
	File /r /x .svn ".\Skins\*.*"

	SetOutPath "$INSTDIR\Themes"
	File /r /x .svn ".\Themes\*.*"

	SetOutPath "$INSTDIR"

	${If} $instType == "S"
		ReadRegStr $0 HKLM "SOFTWARE\Rainmeter" ""
		WriteRegStr HKLM "SOFTWARE\Rainmeter" "" "$INSTDIR"
		WriteRegStr HKLM "SOFTWARE\Rainmeter" "Language" "$LANGUAGE"
		${If} $defLang == "1"
			DeleteRegValue HKLM "SOFTWARE\Rainmeter" "NonDefault"
		${Else}
			WriteRegDWORD HKLM "SOFTWARE\Rainmeter" "NonDefault" 1
		${EndIf}

		WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Rainmeter" "DisplayName" "Rainmeter"
		WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Rainmeter" "DisplayIcon" "$INSTDIR\Rainmeter.exe,0"
		WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Rainmeter" "URLInfoAbout" "http://rainmeter.net"
		WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Rainmeter" "UninstallString" "$INSTDIR\uninst.exe"

!ifdef BETA
		WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Rainmeter" "DisplayVersion" "${VER} beta r${REV}"
!else
		WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Rainmeter" "DisplayVersion" "${VER} r${REV}"
!endif

		WriteRegStr HKCR ".rmskin" "" "Rainmeter skin"
		WriteRegStr HKCR "Rainmeter skin" "" "Rainmeter skin file"
		WriteRegStr HKCR "Rainmeter skin\shell" "" "open"
		WriteRegStr HKCR "Rainmeter skin\DefaultIcon" "" "$INSTDIR\SkinInstaller.exe,0"
		WriteRegStr HKCR "Rainmeter skin\shell\open\command" "" '"$INSTDIR\SkinInstaller.exe" %1'
		WriteRegStr HKCR "Rainmeter skin\shell\edit" "" "Install Rainmeter skin"
		WriteRegStr HKCR "Rainmeter skin\shell\edit\command" "" '"$INSTDIR\SkinInstaller.exe" %1'

		; Refresh shell icons if new install
		${If} $0 == ""
			System::Call 'Shell32::SHChangeNotify(i 0x8000000, i 0, i 0, i 0)'
		${EndIf}

		; Remove all shortcuts
		${If} $ctlAllUsers == "1"
			SetShellVarContext current
			Call GetEnvPaths
			!insertmacro RemoveShortcuts
			!insertmacro UAC_AsUser_Call Function GetEnvPaths ${UAC_SYNCREGISTERS}
			!insertmacro RemoveShortcuts
			SetShellVarContext all
			Call GetEnvPaths
			!insertmacro RemoveShortcuts
		${Else}
			SetShellVarContext all
			Call GetEnvPaths
			!insertmacro RemoveShortcuts
			SetShellVarContext current
			Call GetEnvPaths
			!insertmacro RemoveShortcuts
			!insertmacro UAC_AsUser_Call Function GetEnvPaths ${UAC_SYNCREGISTERS}
			!insertmacro RemoveShortcuts
		${EndIf}

		; Create shortcuts ($1=$SMPROGRAMS, $2=$SMSTARTUP, $3=$DESKTOP)
		CreateDirectory "$1\Rainmeter"
		CreateShortCut "$1\Rainmeter\Rainmeter.lnk" "$INSTDIR\Rainmeter.exe" "" "$INSTDIR\Rainmeter.exe" 0

		SetOutPath "$INSTDIR"
		${If} $ctlStartup == "1"
			CreateShortCut  "$2\Rainmeter.lnk" "$INSTDIR\Rainmeter.exe" "" "$INSTDIR\Rainmeter.exe" 0
		${EndIf}

		${If} $ctlDesktop == "1"
			CreateShortCut  "$3\Rainmeter.lnk" "$INSTDIR\Rainmeter.exe" "" "$INSTDIR\Rainmeter.exe" 0
		${EndIf}

		WriteUninstaller "$INSTDIR\uninst.exe"
	${Else}
		${IfNot} ${FileExists} "Rainmeter.ini"
			CopyFiles /SILENT "$INSTDIR\Themes\illustro default\Rainmeter.thm" "$INSTDIR\Rainmeter.ini"
		${EndIf}

		WriteINIStr "$INSTDIR\Rainmeter.ini" "Rainmeter" "Language" "$LANGUAGE"
	${EndIf}
SectionEnd

Function GetEnvPaths
	StrCpy $1 $SMPROGRAMS
	StrCpy $2 $SMSTARTUP
	StrCpy $3 $DESKTOP
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
	Pop $ctlDelete

	${NSD_CreateLabel} 16 26u 95% 12u "$(UNSTALLSETTINGSDESC)"

	nsDialogs::Show
FunctionEnd

Function un.GetOptions
	${NSD_GetState} $ctlDelete $ctlDelete
FunctionEnd

Section Uninstall
	FindWindow $0 "RainmeterTrayClass"
	${If} $0 != "0"
		Exec '"$INSTDIR\Rainmeter.exe" !RainmeterQuit'

		; Wait up to for up to 5 seconds for Rainmeter to close
		StrCpy $1 "0"
		${DoWhile} ${ProcessExists} "Rainmeter.exe"
			IntOp $1 $1 + 1
			${If} $1 >= "10"
				MessageBox MB_RETRYCANCEL|MB_ICONSTOP "$(RAINMETERCLOSEERROR)" IDRETRY +2
				Quit
			${EndIf}
			Sleep 500
		${Loop}
	${EndIf}

	RMDir /r "$TEMP\Rainmeter-Cache"
	RMDir /r "$INSTDIR\Skins\Gnometer"
	RMDir /r "$INSTDIR\Skins\Tranquil"
	RMDir /r "$INSTDIR\Skins\Enigma"
	RMDir /r "$INSTDIR\Skins\Arcs"
	RMDir /r "$INSTDIR\Skins\illustro"
	Delete "$INSTDIR\Skins\*.txt"
	RMDir "$INSTDIR\Skins"

	RMDir /r "$INSTDIR\Addons\RainThemes"
	RMDir /r "$INSTDIR\Addons\RainBrowser"
	RMDir /r "$INSTDIR\Addons\RainBackup"
	RMDir /r "$INSTDIR\Addons\Rainstaller"
	RMDir "$INSTDIR\Addons"
	Delete "$INSTDIR\Plugins\*.*"
	Delete "$INSTDIR\Plugins\Dependencies\*.*"
	RMDir "$INSTDIR\Plugins"
	RMDir /r "$INSTDIR\Languages"
	RMDir /r "$INSTDIR\Themes"
	Delete "$INSTDIR\*.*"

	${If} $ctlDelete == "1"
		RMDir /r "$INSTDIR\Skins"
		RMDir /r "$INSTDIR\Addons"
		RMDir /r "$INSTDIR\Plugins"
		RMDir /r "$INSTDIR\Fonts"
	${EndIf}

	RMDir "$INSTDIR"

	SetShellVarContext all
	RMDir /r "$APPDATA\Rainstaller"

	SetShellVarContext current
	Call un.GetEnvPaths
	!insertmacro RemoveShortcuts
	${If} $ctlDelete == "1"
		RMDir /r "$APPDATA\Rainmeter"
		RMDir /r "$DOCUMENTS\Rainmeter\Skins"
		RMDir "$DOCUMENTS\Rainmeter"
		RMDir /r "$1\Rainmeter"
	${EndIf}
	
	!insertmacro UAC_AsUser_Call Function un.GetEnvPaths ${UAC_SYNCREGISTERS}
	!insertmacro RemoveShortcuts
	${If} $ctlDelete == "1"
		RMDir /r "$APPDATA\Rainmeter"
		RMDir /r "$DOCUMENTS\Rainmeter\Skins"
		RMDir "$DOCUMENTS\Rainmeter"
	${EndIf}

	SetShellVarContext all
	Call un.GetEnvPaths
	!insertmacro RemoveShortcuts

	DeleteRegKey HKLM "SOFTWARE\Rainmeter"
	DeleteRegKey HKCR ".rmskin"
	DeleteRegKey HKCR "Rainmeter skin"
	DeleteRegKey HKLM "SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\Rainmeter"
	System::Call 'Shell32::SHChangeNotify(i 0x8000000, i 0, i 0, i 0)'
SectionEnd

Function un.GetEnvPaths
	StrCpy $1 $SMPROGRAMS
	StrCpy $2 $SMSTARTUP
	StrCpy $3 $DESKTOP
FunctionEnd
