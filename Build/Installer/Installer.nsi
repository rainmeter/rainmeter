; Copyright (c) Rainmeter Team. Source code licensed under GNU GPL v2 (see LICENSE file).

!verbose 2

Unicode true

!addplugindir ".\"
!addplugindir /x86-unicode "..\..\BuildOut\Installer\Plugins\x86-unicode"

!include "nsDialogs.nsh"
!include "nsDialogs_setImageOle.nsh"

; Define a custom NSD_SetStretchedImage that always uses our JPG image in order to override it
; in the MUI Finish page. This must be done before MUI2.nsh is included.
!macro __SetCustomStretchedImage CONTROL IMAGE HANDLE
	${NSD_SetStretchedImageOLE} ${CONTROL} "$PLUGINSDIR\Wizard.jpg" ${HANDLE}
!macroend
!undef NSD_SetStretchedImage
!define NSD_SetStretchedImage `!insertmacro __SetCustomStretchedImage`

!include "MUI2.nsh"
!include "x64.nsh"
!include "FileFunc.nsh"
!include "StrFunc.nsh"
!include "WordFunc.nsh"
!include "WinVer.nsh"
!include "UAC.nsh"
!include "RmError.nsh"

${Using:StrFunc} StrRep

!ifndef OUTFILE
 !define OUTFILE "Rainmeter-test.exe"
 !define VERSION_FULL "0.0.0.0"
 !define VERSION_SHORT "0.0"
 !define VERSION_REVISION "000"
 !define VERSION_MAJOR "0"
 !define VERSION_MINOR "0"
 !define BUILD_YEAR "0000"
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
VIAddVersionKey "LegalCopyright" "${U+00A9} ${BUILD_YEAR} Rainmeter Team"
VIProductVersion "${VERSION_FULL}"
BrandingText " "
!ifdef OFFICIALBUILD
SetCompressor /SOLID lzma
!else
SetCompressor zlib
!endif
RequestExecutionLevel user
InstallDirRegKey HKLM "SOFTWARE\Rainmeter" ""
ShowInstDetails nevershow
AllowSkipFiles off
XPStyle on
ManifestDPIAware true
OutFile "..\${OUTFILE}"
ReserveFile "..\..\BuildOut\Installer\Plugins\x86-unicode\LangDLL.dll"
ReserveFile "..\..\BuildOut\Installer\Plugins\x86-unicode\nsDialogs.dll"
ReserveFile "..\..\BuildOut\Installer\Plugins\x86-unicode\System.dll"
ReserveFile "..\..\BuildOut\Installer\Plugins\x86-unicode\UAC.dll"
ReserveFile ".\Wizard.jpg"
ReserveFile ".\WizardEmpty.bmp"

!define MUI_ICON ".\Installer.ico"
!define MUI_CUSTOMFUNCTION_GUIINIT InitWizardImage
!define MUI_WELCOMEFINISHPAGE_BITMAP ".\WizardEmpty.bmp"
!define MUI_FINISHPAGE_RUN
!define MUI_FINISHPAGE_RUN_FUNCTION FinishRun
!define MUI_WELCOMEPAGE ; For language strings

Page custom PageWelcome PageWelcomeOnLeave
Page custom PageOptions PageOptionsOnLeave
!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_PAGE_FINISH

; Include languages
!macro IncludeLanguage LANGUAGE CUSTOMLANGUAGE
	!insertmacro MUI_LANGUAGE ${LANGUAGE}
	!insertmacro LANGFILE_INCLUDE "..\..\BuildOut\Installer\${CUSTOMLANGUAGE}.nsh"
!macroend
!define IncludeLanguage "!insertmacro IncludeLanguage"
!include "..\..\BuildOut\Installer\Languages.nsh"

Var NonDefaultLanguage
Var AutoStartup
Var InstallPortable
Var ExistingRainmeterInstallation
Var RestartAfterInstall

${StrStr}	; Must be called before any sections or functions

!macro Elevate
UAC_TryAgain:
	!insertmacro UAC_RunElevated
	${Switch} $0
	${Case} 0
		${IfThen} $1 = 1 ${|} Quit ${|}			; This is the outer process, the inner process is done
		${IfThen} $3 <> 0 ${|} ${Break} ${|}	; We are the admin
		${If} $1 = 3							; RunAs completed successfully with a non-admin user
			MessageBox MB_OK|MB_ICONSTOP|MB_TOPMOST|MB_SETFOREGROUND "$(AdminError)" /SD IDNO IDOK UAC_TryAgain IDNO 0
			!insertmacro LOG_ERROR ${ERROR_NOTADMIN}
		${EndIf}
		; Fall-through
	${Case} 1223
		Quit
	${Case} 1062
		MessageBox MB_OK|MB_ICONSTOP|MB_TOPMOST|MB_SETFOREGROUND "$(LogonError)" /SD IDOK
		!insertmacro LOG_ERROR ${ERROR_NOLOGONSVC}
		Quit
	${Default}
		MessageBox MB_OK|MB_ICONSTOP|MB_TOPMOST|MB_SETFOREGROUND "$(UacError) ($0)" /SD IDOK
		!insertmacro LOG_ERROR ${ERROR_NOTADMIN}
		Quit
	${EndSwitch}

SetShellVarContext all
!macroend

; Creates a custom page dialog, aborting the whole installer if it cannot be created. A missing
; dialog used to go unnoticed: nsDialogs::Show returns right away, so every page silently fell
; through to the install itself with none of its variables ever set. The sentinel catches the
; nsDialogs plugin failing to load at all, in which case nothing is pushed onto the stack.
!macro CreatePageDialog RESOURCE
	Push "!"
	nsDialogs::Create ${RESOURCE}
	Pop $0
	${If} $0 == "!"
	${OrIf} $0 == "error"
	${OrIf} $0 == ""
		MessageBox MB_OK|MB_ICONSTOP "$(SetupError)" /SD IDOK
		Quit
	${EndIf}
	Pop $1
!macroend
!define CreatePageDialog "!insertmacro CreatePageDialog"

; Install
; --------------------------------------
Function .onInit
	${IfNot} ${RunningX64}
		MessageBox MB_OK|MB_ICONSTOP "This installer requires 64-bit Windows." /SD IDOK
		Quit
	${EndIf}
	${EnableX64FSRedirection}

	${IfNot} ${UAC_IsInnerInstance}
		${IfNot} ${AtLeastWin10}
		${OrIfNot} ${AtLeastBuild} 18362
			MessageBox MB_OK|MB_ICONSTOP "$(UnsupportedWindowsError)" /SD IDOK
			!insertmacro LOG_ERROR ${ERROR_UNSUPPORTED}
			Quit
		${EndIf}

		ReadRegStr $0 HKLM "SOFTWARE\Rainmeter" "Language"
		ReadRegDWORD $NonDefaultLanguage HKLM "SOFTWARE\Rainmeter" "NonDefault"

		${IfNot} ${Silent}
			${If} $0 == ""
			${OrIf} $0 <> $LANGUAGE
			${AndIf} $NonDefaultLanguage != 1
				; New install or better match. In case the default is English, strip away English string to
				; avoid showing it twice.
				StrCpy $1 "Please select the setup language."
				${StrRep} $2 "$(SelectLanguage)" "$1" ""
				LangDLL::LangDialog "$(^SetupCaption)" "$1$\n$2" AC ${LANGDLL_PARAMS} ""
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
		${Else}
			${If} $0 != ""
				StrCpy $LANGUAGE $0
			${EndIf}

			${GetParameters} $R1

			ClearErrors
			${GetOptions} $R1 "/LANGUAGE=" $0
			${IfNot} ${Errors}
				${If} $LANGUAGE != $0
					StrCpy $NonDefaultLanguage 1
				${EndIf}

				StrCpy $LANGUAGE $0
			${EndIf}

			StrCpy $RestartAfterInstall 999
			${GetOptions} $R1 "/RESTART=" $0
			${If} $0 != ""
				${If} $0 = 0
					StrCpy $RestartAfterInstall 0
				${ElseIf} $0 = 1
					StrCpy $RestartAfterInstall 1
				${EndIf}
			${EndIf}

			StrCpy $AutoStartup 0
			${GetOptions} $R1 "/AUTOSTARTUP=" $0  ; Note this value is ignored on portable installations
			${If} $0 != ""
				${If} $0 = 1
					StrCpy $AutoStartup 1
				${EndIf}
			${Else}
				ReadRegStr $0 HKCU "SOFTWARE\Microsoft\Windows\CurrentVersion\Run" "Rainmeter"
				${If} $0 != ""
					StrCpy $AutoStartup 1
				${EndIf}

				SetShellVarContext all
				${If} ${FileExists} "$SMSTARTUP\Rainmeter.lnk"
					StrCpy $AutoStartup 1
				${EndIf}

				SetShellVarContext current
				${If} ${FileExists} "$SMSTARTUP\Rainmeter.lnk"
					StrCpy $AutoStartup 1
				${EndIf}
			${EndIf}

			StrCpy $InstallPortable 0
			${GetOptions} $R1 "/PORTABLE=" $0
			${If} $0 = 1
				StrCpy $InstallPortable 1
			${EndIf}

			${If} $InstallPortable = 1
				; Check for /D= defined on the command line in case a portable
				; installation is desired after standard installation
				System::Call kernel32::GetCommandLine()t.r1
				${StrStr} $2 $1 "/D="
				${If} $2 == ""
					StrCpy $INSTDIR "$EXEDIR\Rainmeter"
				${EndIf}
			${Else}
				; Standard installation (ignore /D=)
				${If} ${FileExists} "$PROGRAMFILES64\Rainmeter\Rainmeter.exe"
					StrCpy $INSTDIR "$PROGRAMFILES64\Rainmeter"
				${ElseIf} ${FileExists} "$PROGRAMFILES\Rainmeter\Rainmeter.exe"
					StrCpy $INSTDIR "$PROGRAMFILES\Rainmeter"
				${Else}
					StrCpy $INSTDIR "$PROGRAMFILES64\Rainmeter"
				${EndIf}
			${EndIf}
		${EndIf}

		; If the language was set to a non-existent language, reset it back to English.
		${WordFind} ",${LANGUAGE_IDS}" ",$LANGUAGE," "E+1{" $0
		${If} ${Errors}
			StrCpy $LANGUAGE "1033"
		${EndIf}
	${Else}
		; Exchange settings with user instance
		!insertmacro UAC_AsUser_Call Function ExchangeSettings ${UAC_SYNCREGISTERS}
		StrCpy $AutoStartup $1
		StrCpy $NonDefaultLanguage $3
		StrCpy $RestartAfterInstall $4
		StrCpy $LANGUAGE $5
		StrCpy $INSTDIR $6
	${EndIf}
FunctionEnd

Function ExchangeSettings
	StrCpy $1 $AutoStartup
	StrCpy $3 $NonDefaultLanguage
	StrCpy $4 $RestartAfterInstall
	StrCpy $5 $LANGUAGE
	StrCpy $6 $INSTDIR
	HideWindow
FunctionEnd

Function InitWizardImage
	InitPluginsDir
	File /oname=$PLUGINSDIR\Wizard.jpg ".\Wizard.jpg"
FunctionEnd

Function .onInstSuccess
	${If} ${Silent}
	${AndIf} $RestartAfterInstall = 1
		Call FinishRun
	${EndIf}
FunctionEnd

Function PageWelcome
	${If} ${UAC_IsInnerInstance}
		${If} ${UAC_IsAdmin}
			; Skip page
			Abort
		${Else}
			MessageBox MB_OK|MB_ICONSTOP "$(AdminError) (Inner)" /SD IDOK
			!insertmacro LOG_ERROR ${ERROR_NOTADMIN}
			Quit
		${EndIf}
	${EndIf}

	!insertmacro MUI_HEADER_TEXT "$(InstallOptions)" "$(^ComponentsSubText1)"
	${CreatePageDialog} 1044
	nsDialogs::SetRTL $(^RTL)
	SetCtlColors $0 "" "${MUI_BGCOLOR}"

	${NSD_CreateBitmap} 0u 0u 109u 193u ""
	Pop $0
	; See NSD_SetStretchedImage definition above.
	${NSD_SetStretchedImage} $0 "" $R0

	${NSD_CreateLabel} 120u 10u 195u 38u "$(MUI_TEXT_WELCOME_INFO_TITLE)"
	Pop $0
	SetCtlColors $0 "" "${MUI_BGCOLOR}"
	CreateFont $1 "$(^Font)" "12" "700"
	SendMessage $0 ${WM_SETFONT} $1 0

	${NSD_CreateLabel} 120u 55u 195u 12u "$(^ComponentsSubText1)"
	Pop $0
	SetCtlColors $0 "" "${MUI_BGCOLOR}"

	${NSD_CreateRadioButton} 120u 70u 205u 12u "$(StandardInstall)"
	Pop $R1
	SetCtlColors $R1 "" "${MUI_BGCOLOR}"
	${NSD_AddStyle} $R1 ${WS_GROUP}
	SendMessage $R1 ${WM_SETFONT} $mui.Header.Text.Font 0

	${NSD_CreateLabel} 132u 82u 185u 24u "$(StandardInstallDescription)"
	Pop $0
	SetCtlColors $0 "" "${MUI_BGCOLOR}"

	${NSD_CreateRadioButton} 120u 106u 310u 12u "$(PortableInstall)"
	Pop $R2
	SetCtlColors $R2 "" "${MUI_BGCOLOR}"
	${NSD_AddStyle} $R2 ${WS_TABSTOP}
	SendMessage $R2 ${WM_SETFONT} $mui.Header.Text.Font 0

	${NSD_CreateLabel} 132u 118u 185u 39u "$(PortableInstallDescription)"
	Pop $0
	SetCtlColors $0 "" "${MUI_BGCOLOR}"

	${NSD_CreateLabel} 120u 176u 195u 12u "v${VERSION_FULL}"
	Pop $0
	SetCtlColors $0 "AAAAAA" "${MUI_BGCOLOR}"

	${If} $InstallPortable = 1
		${NSD_Check} $R2
	${Else}
		${NSD_Check} $R1
	${EndIf}

	; Remove UAC shield on button in case user clicked "Back" on next dialog
	GetDlgItem $0 $HWNDPARENT 1
	SendMessage $0 ${BCM_SETSHIELD} 0 0

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

	!insertmacro MUI_HEADER_TEXT "$(InstallOptions)" "$(InstallOptionsDescription)"
	${CreatePageDialog} 1018
	nsDialogs::SetRTL $(^RTL)

	${NSD_CreateGroupBox} 0 0u -1u 36u "$(^DirSubText)"

	${NSD_CreateDirRequest} 6u 14u 232u 14u ""
	Pop $R0

	${NSD_CreateBrowseButton} 242u 14u 50u 14u "$(^BrowseBtn)"
	Pop $R1
	${NSD_OnClick} $R1 PageOptionsBrowseOnClick

	StrCpy $1 0


	${If} $InstallPortable <> 1
		${If} $1 = 0
			StrCpy $0 54u
			StrCpy $1 30u
		${Else}
			StrCpy $0 66u
			StrCpy $1 42u
		${EndIf}

		${NSD_CreateCheckbox} 6u $0 285u 12u "$(AutoStartup)"
		Pop $R3

		${If} $INSTDIR == ""
			${NSD_Check} $R3
		${Else}
			ReadRegStr $0 HKCU "SOFTWARE\Microsoft\Windows\CurrentVersion\Run" "Rainmeter"
			${If} $0 != ""
				${NSD_Check} $R3
			${EndIf}

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
		${NSD_CreateGroupBox} 0 42u -1u $1 "$(AdditionalOptions)"
	${EndIf}

	; Set default directory
	${If} $InstallPortable = 1
		ReadRegStr $0 HKCU "SOFTWARE\Rainmeter" "PortableInstallPath"
		${If} $0 == ""
		${OrIfNot} ${FileExists} "$0\Rainmeter.exe"
			${GetRoot} "$WINDIR" $0
			StrCpy $0 "$0\Rainmeter"
		${EndIf}
		${NSD_SetText} $R0 "$0"
	${Else}
		; Disable Directory editbox and Browse button if already installed
		SendMessage $R0 ${EM_SETREADONLY} 1 0

		${If} $INSTDIR != ""
			EnableWindow $R1 0
			${NSD_SetText} $R0 "$INSTDIR"
		${Else}
			; Fresh install
			${NSD_SetText} $R0 "$PROGRAMFILES64\Rainmeter"
		${EndIf}
	${EndIf}

	; Show UAC shield on Install button if required
	GetDlgItem $0 $HWNDPARENT 1
	${If} $InstallPortable = 1
		SendMessage $0 ${BCM_SETSHIELD} 0 0
	${Else}
		SendMessage $0 ${BCM_SETSHIELD} 0 1
	${EndIf}

	nsDialogs::Show
FunctionEnd

Function PageOptionsBrowseOnClick
	${NSD_GetText} $R0 $0
	nsDialogs::SelectFolderDialog "$(^DirBrowseText)" $0
	Pop $1
	${If} $1 != error
		; If the selected non-Rainmeter directory isn't empty, append \Rainmeter
		${If} $1 != ""
		${AndIf} ${FileExists} "$1\*.*"
		${AndIfNot} ${FileExists} "$1\Rainmeter.exe"
			StrCpy $1 "$1\Rainmeter"
		${EndIf}

		${NSD_SetText} $R0 $1
	${EndIf}
FunctionEnd

Function PageOptionsOnLeave
	${NSD_GetText} $R0 $0
	StrCpy $INSTDIR $0

	GetDlgItem $0 $HWNDPARENT 1
	EnableWindow $0 0

	${If} $R3 != 0
		${NSD_GetState} $R3 $AutoStartup
	${EndIf}
FunctionEnd

!macro InstallFiles OUTDIR ARCH
	SetOutPath "$INSTDIR"
	File "..\..\${OUTDIR}\Rainmeter.exe"
	File "..\..\${OUTDIR}\Rainmeter.dll"
	File "..\..\${OUTDIR}\SkinInstaller.exe"

	Delete "$INSTDIR\uninst.exe"

	; Obsolete, restarting is now handled by "Rainmeter.exe /Restart"
	Delete "$INSTDIR\RestartRainmeter.exe"
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
	${If} $INSTDIR == ""
		MessageBox MB_OK|MB_ICONSTOP "$(SetupError)" /SD IDOK
		Quit
	${EndIf}

	${If} $InstallPortable <> 1
		${IfNot} ${UAC_IsAdmin}
			; UAC_IsAdmin seems to return incorrect results sometimes, so check again before elevating.
			System::Call "shell32::IsUserAnAdmin()i.r0"
			${If} $0 = 0
				!insertmacro Elevate
			${EndIf}
		${EndIf}
	${EndIf}

	; Verify that the selected folder is writable
	ClearErrors
	CreateDirectory "$INSTDIR"
	WriteINIStr "$INSTDIR\writetest~.rm" "1" "1" "1"
	Delete "$INSTDIR\writetest~.rm"

	${If} ${Errors}
		RMDir "$INSTDIR"
		StrCpy $0 $INSTDIR	; WriteError names the directory through $0
		MessageBox MB_OK|MB_ICONEXCLAMATION "$(WriteError)" /SD IDOK
		!insertmacro LOG_ERROR ${ERROR_WRITEFAIL}
		Quit
	${EndIf}

	SetOutPath "$PLUGINSDIR"
	SetShellVarContext current

	SetOutPath "$INSTDIR"

	StrCpy $ExistingRainmeterInstallation 0
	${If} ${FileExists} "$INSTDIR\Rainmeter.exe"
		StrCpy $ExistingRainmeterInstallation 1
	${EndIf}

	; Close Rainmeter (and wait up to five seconds)
	${ForEach} $0 10 0 - 1
		FindWindow $1 "DummyRainWClass" "Rainmeter control window"
		${If} $1 <> 0
		${AndIf} $RestartAfterInstall = 999
			StrCpy $RestartAfterInstall 1
		${EndIf}

		ClearErrors
		Delete "$INSTDIR\Rainmeter.exe"
		${If} $1 = 0
		${AndIfNot} ${Errors}
			${Break}
		${EndIf}

		SendMessage $1 ${WM_CLOSE} 0 0

		${If} $0 = 0
			MessageBox MB_RETRYCANCEL|MB_ICONSTOP "$(RainmeterCloseError)" /SD IDRETRY IDRETRY Retry
			!insertmacro LOG_ERROR ${ERROR_CLOSEFAIL}
			Quit
		${EndIf}

Retry:
		Sleep 500
	${Next}

	; Move Rainmeter.ini to %APPDATA% if needed
	${IfNot} ${Silent}
	${AndIf} ${FileExists} "$INSTDIR\Rainmeter.ini"
		${If} $InstallPortable <> 1
			${If} "$INSTDIR" == "$PROGRAMFILES64\Rainmeter"
			${OrIf} "$INSTDIR" == "$PROGRAMFILES\Rainmeter"
				MessageBox MB_YESNO|MB_ICONEXCLAMATION "$(SettingsFileError)" /SD IDNO IDNO SkipIniMove
				StrCpy $0 1
				!insertmacro UAC_AsUser_Call Function CopyIniToAppData ${UAC_SYNCREGISTERS}
				${If} $0 = 1
					; Copy succeeded
					Delete "$INSTDIR\Rainmeter.ini"
				${Else}
					MessageBox MB_OK|MB_ICONSTOP "$(SettingsMoveError)" /SD IDOK
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
	Delete "$INSTDIR\Rainmeter.VisualElementsManifest.xml"
	Delete "$INSTDIR\Default.ini"
	Delete "$INSTDIR\Launcher.exe"
	Delete "$INSTDIR\SkinInstaller.dll"
	Delete "$INSTDIR\Defaults\Plugins\FileView.dll"
	Delete "$INSTDIR\Defaults\Plugins\AudioLevel.dll"
	RMDir /r "$INSTDIR\Addons\Rainstaller"
	RMDir /r "$INSTDIR\Addons\RainBackup"
	RMDir /r "$INSTDIR\Runtime"
	RMDir /r "$INSTDIR\VisualElements"

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
	${EndIf}

	${Locate} "$INSTDIR\Plugins" "/L=F /M=*.dll /G=0" "HandlePlugins"

!ifdef INCLUDEFILES
	File "..\..\Application\Rainmeter.exe.config"

	!insertmacro InstallFiles "BuildOut\Release64" "x64"

	RMDir /r "$INSTDIR\Languages"
	SetOutPath "$INSTDIR\Languages"
	File "..\..\BuildOut\Release64\Languages\*.rmlang"

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

		WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Rainmeter" "Publisher" "Rainmeter"
		WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Rainmeter" "DisplayIcon" "$INSTDIR\Rainmeter.exe,0"
		WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Rainmeter" "Comments" "Rainmeter - desktop customization tool"
		WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Rainmeter" "HelpLink" "https://docs.rainmeter.net/manual/"
		WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Rainmeter" "URLUpdateInfo" "https://rainmeter.net"
		WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Rainmeter" "URLInfoAbout" "https://rainmeter.net"
		WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Rainmeter" "UninstallString" '$\"$INSTDIR\Rainmeter.exe$\" /Uninstall'
		WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Rainmeter" "InstallLocation" "$INSTDIR"
		WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Rainmeter" "NoModify" "1"
		WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Rainmeter" "NoRepair" "1"
		WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Rainmeter" "VersionMajor" "${VERSION_MAJOR}"
		WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Rainmeter" "VersionMinor" "${VERSION_MINOR}"

		; Get the current date (runtime) [YMD]
		${GetTime} "" "L" $0 $1 $2 $3 $4 $5 $6
		WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Rainmeter" "InstallDate" "$2$1$0"

		; Get rid of approximate install size, which we wrote out in the past.
		DeleteRegValue HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Rainmeter" "EstimatedSize"

		WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Rainmeter" "DisplayName" "Rainmeter"
		WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Rainmeter" "DisplayVersion" "${VERSION_SHORT}"
		WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Rainmeter" "ReleaseType" "Release"

		; Create .rmskin association
		WriteRegStr HKCR ".rmskin" "" "Rainmeter.SkinInstaller"
		DeleteRegKey HKCR "Rainmeter skin"	; Old key
		WriteRegStr HKCR "Rainmeter.SkinInstaller" "" "Rainmeter Skin Installer"
		WriteRegStr HKCR "Rainmeter.SkinInstaller" "FriendlyTypeName" "Rainmeter Skin Package"
		WriteRegStr HKCR "Rainmeter.SkinInstaller\shell" "" "open"
		WriteRegStr HKCR "Rainmeter.SkinInstaller\DefaultIcon" "" "$INSTDIR\SkinInstaller.exe,0"
		WriteRegStr HKCR "Rainmeter.SkinInstaller\shell\open\command" "" '"$INSTDIR\SkinInstaller.exe" "%1"'
		WriteRegStr HKCR "Rainmeter.SkinInstaller\shell\edit" "" "Install Rainmeter skin"
		WriteRegStr HKCR "Rainmeter.SkinInstaller\shell\edit\command" "" '"$INSTDIR\SkinInstaller.exe" "%1"'

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

		Delete "$SMSTARTUP\Rainmeter.lnk"
		!insertmacro UAC_AsUser_Call Function UpdateUserStartup ${UAC_SYNCREGISTERS}

		SetShellVarContext current
		Call RemoveStartMenuShortcuts

		!insertmacro UAC_AsUser_Call Function RemoveStartMenuShortcuts ${UAC_SYNCREGISTERS}
	${Else}
		WriteRegStr HKCU "SOFTWARE\Rainmeter" "PortableInstallPath" "$INSTDIR"

		${IfNot} ${FileExists} "Rainmeter.ini"
		${AndIf} $ExistingRainmeterInstallation <> 1
			CopyFiles /SILENT "$INSTDIR\Defaults\Layouts\illustro default\Rainmeter.ini" "$INSTDIR\Rainmeter.ini"
		${EndIf}

		${If} ${FileExists} "$INSTDIR\Rainmeter.ini"
			WriteINIStr "$INSTDIR\Rainmeter.ini" "Rainmeter" "Language" "$LANGUAGE"
		${EndIf}
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
	${If} $R7 == "ActionTimer.dll"
	${OrIf} $R7 == "AdvancedCPU.dll"
	${OrIf} $R7 == "AudioLevel.dll"
	${OrIf} $R7 == "CoreTemp.dll"
	${OrIf} $R7 == "FileView.dll"
	${OrIf} $R7 == "FolderInfo.dll"
	${OrIf} $R7 == "InputText.dll"
	${OrIf} $R7 == "iTunesPlugin.dll"
	${OrIf} $R7 == "MediaKey.dll"
	${OrIf} $R7 == "NowPlaying.dll"
	${OrIf} $R7 == "PerfMon.dll"
	${OrIf} $R7 == "PingPlugin.dll"
	${OrIf} $R7 == "PowerPlugin.dll"
	${OrIf} $R7 == "Process.dll"
	${OrIf} $R7 == "QuotePlugin.dll"
	${OrIf} $R7 == "RecycleManager.dll"
	${OrIf} $R7 == "ResMon.dll"
	${OrIf} $R7 == "RunCommand.dll"
	${OrIf} $R7 == "SpeedFanPlugin.dll"
	${OrIf} $R7 == "SysInfo.dll"
	${OrIf} $R7 == "UsageMonitor.dll"
	${OrIf} $R7 == "WebParser.dll"
	${OrIf} $R7 == "WifiStatus.dll"
	${OrIf} $R7 == "Win7AudioPlugin.dll"
	${OrIf} $R7 == "WindowMessagePlugin.dll"
		Delete "$R9"
	${ElseIf} $InstallPortable <> 1
	${AndIf} $R7 != "VirtualDesktops.dll"
		CreateDirectory "$INSTDIR\Defaults\Plugins"
		Delete "$INSTDIR\Defaults\Plugins\$R7"
		Rename "$R9" "$INSTDIR\Defaults\Plugins\$R7"
	${EndIf}

	Push $0
FunctionEnd

Function RemoveStartMenuShortcuts
	!insertmacro RemoveStartMenuShortcuts "$SMPROGRAMS\Rainmeter"
FunctionEnd

Function UpdateUserStartup
	SetShellVarContext current
	Delete "$SMSTARTUP\Rainmeter.lnk"
	${If} $AutoStartup = 1
		WriteRegStr HKCU "SOFTWARE\Microsoft\Windows\CurrentVersion\Run" "Rainmeter" "$INSTDIR\Rainmeter.exe"
	${Else}
		DeleteRegValue HKCU "SOFTWARE\Microsoft\Windows\CurrentVersion\Run" "Rainmeter"
	${EndIf}
FunctionEnd

Function FinishRun
	; Explorer launches Rainmeter with the desktop user's token instead of the elevated installer's token.
	ExecShell "" "$WINDIR\explorer.exe" '$\"$INSTDIR\Rainmeter.exe$\"'
FunctionEnd
