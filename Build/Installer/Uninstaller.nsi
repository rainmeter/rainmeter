; Copyright (c) Rainmeter Team. Source code licensed under GNU GPL v2 (see LICENSE file).

!verbose 2

Unicode true

!addplugindir ".\"
!addplugindir /x86-unicode "..\..\BuildOut\Installer\Plugins\x86-unicode"

!include "nsDialogs.nsh"
!include "MUI2.nsh"
!include "x64.nsh"
!include "FileFunc.nsh"
!include "WordFunc.nsh"
!include "UAC.nsh"
!include "RmError.nsh"
!include "InstallerShared.nsh"

!ifndef VERSION_FULL
 !define VERSION_FULL "0.0.0.0"
 !define VERSION_SHORT "0.0"
 !define BUILD_YEAR "0000"
!endif

!insertmacro SetInstallerSettings
!insertmacro SetVersionInfo "Rainmeter Uninstaller" "uninst.exe"

SilentInstall silent
OutFile "..\..\BuildOut\Installer\UninstallerGenerator.exe"

UninstPage custom un.PageOptions un.GetOptions
!insertmacro MUI_UNPAGE_INSTFILES

!insertmacro IncludeLanguages

Var un.DeleteAll

Section
	SetOutPath "$EXEDIR"
	WriteUninstaller "$EXEDIR\uninst.exe"
SectionEnd

Function un.onInit
	!insertmacro Elevate

	ReadRegStr $0 HKLM "SOFTWARE\Rainmeter" "Language"
	${If} $0 != ""
		StrCpy $LANGUAGE $0
	${EndIf}
FunctionEnd

Function un.PageOptions
	!insertmacro MUI_HEADER_TEXT "$(UninstallOptions)" "$(UninstallOptionsDescription)"
	${CreatePageDialog} 1018
	nsDialogs::SetRTL $(^RTL)

	${NSD_CreateCheckbox} 0 0u 95% 12u "$(UninstallRainmeter)"
	Pop $0
	EnableWindow $0 0
	${NSD_Check} $0

	${NSD_CreateCheckbox} 0 15u 70% 12u "$(UninstallSettings)"
	Pop $R0

	${NSD_CreateLabel} 16 26u 95% 12u "$(UninstallSettingsDescription)"

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
			MessageBox MB_RETRYCANCEL|MB_ICONSTOP "$(RainmeterCloseError)" /SD IDRETRY IDRETRY Retry
			!insertmacro LOG_ERROR ${ERROR_CLOSEFAIL}
			Quit
		${EndIf}

Retry:
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
	RMDir /r "$INSTDIR\VisualElements"
	Delete "$INSTDIR\Rainmeter.dll"
	Delete "$INSTDIR\Rainmeter.exe"
	Delete "$INSTDIR\Rainmeter.exe.config"
	Delete "$INSTDIR\Rainmeter.VisualElementsManifest.xml"
	Delete "$INSTDIR\RestartRainmeter.exe"
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
	DeleteRegValue HKCU "SOFTWARE\Microsoft\Windows\CurrentVersion\Run" "Rainmeter"
FunctionEnd
