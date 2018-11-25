/*
  Copyright (C) 2012 Birunthan Mohanathas <http://poiru.net>

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

Unicode true

!addincludedir ".\"
!addplugindir ".\"
!include "nsDialogs.nsh"
!include "FileFunc.nsh"
!include "WordFunc.nsh"
!include "UAC.nsh"

!ifndef VERSION
 !define VERSION "0.0.0"
!endif

!define SCS_64BIT_BINARY 6

Name "RainLexer ${VERSION}"
VIAddVersionKey "ProductName" "RainLexer ${VERSION}"
VIAddVersionKey "FileDescription" "RainLexer Installer"
VIAddVersionKey "LegalCopyright" "© Birunthan Mohanathas (poiru.net)"
VIAddVersionKey "FileVersion" "${VERSION}"
VIProductVersion "${VERSION}.0"
BrandingText " "
SetCompressor /FINAL /SOLID lzma
OutFile "..\Build\RainLexer-${VERSION}.exe"
Icon ".\Icon.ico"
CRCCheck force
RequestExecutionLevel user
ShowInstDetails nevershow
AllowSkipFiles off
XPStyle on

Page custom PageOptions PageOptionsOnLeave
Page InstFiles

Var NppPath
Var NppConfigPath
Var NppExeFile
Var NppThemeName

Var ResetStyleSettings

!macro DecalareStyleVariables Key
	Var ${Key}fgColor
	Var ${Key}bgColor
	Var ${Key}fontName
	Var ${Key}fontStyle
	Var ${Key}fontSize
!macroend
!define DecalareStyleVariables "!insertmacro DecalareStyleVariables"

${DecalareStyleVariables} "DEFAULT"
${DecalareStyleVariables} "COMMENT"
${DecalareStyleVariables} "SECTION"
${DecalareStyleVariables} "OPTION"
${DecalareStyleVariables} "EQUALS"
${DecalareStyleVariables} "INVALID_VALUE"
${DecalareStyleVariables} "VALUE"
${DecalareStyleVariables} "BANG"
${DecalareStyleVariables} "VARIABLE"
${DecalareStyleVariables} "USER_VARIABLE"

!macro ReadStyleVariables Key Name
	${If} "$${Key}fgColor" == ""
		XML::select "//WordsStyle[@name='${Name}']"
		${If} $2 != "0"
			XML::getAttribute "fgColor"
			StrCpy "$${Key}fgColor" $3
			XML::getAttribute "bgColor"
			StrCpy "$${Key}bgColor" $3
			XML::getAttribute "fontName"
			StrCpy "$${Key}fontName" $3
			XML::getAttribute "fontStyle"
			StrCpy "$${Key}fontStyle" $3
			XML::getAttribute "fontSize"
			StrCpy "$${Key}fontSize" $3
		${EndIf}
	${EndIf}
!macroend
!define ReadStyleVariables "!insertmacro ReadStyleVariables"

!macro WriteStyleVariables Key Name
	${If} "$${Key}fgColor" != ""
		XML::select "//WordsStyle[@name='${Name}']"
		StrCmp "$${Key}fgColor" "" +2 0
		XML::setAttribute "fgColor" "$${Key}fgColor"
		StrCmp "$${Key}bgColor" "" +2 0
		XML::setAttribute "bgColor" "$${Key}bgColor"
		StrCmp "$${Key}fontName" "" +2 0
		XML::setAttribute "fontName" "$${Key}fontName"
		StrCmp "$${Key}fontStyle" "" +2 0
		XML::setAttribute "fontStyle" "$${Key}fontStyle"
		StrCmp "$${Key}fontSize" "" +2 0
		XML::setAttribute "fontSize" "$${Key}fontSize"
	${EndIf}
!macroend
!define WriteStyleVariables "!insertmacro WriteStyleVariables"

Function .onInit
	${IfNot} ${UAC_IsInnerInstance}
		ReadRegStr $NppPath HKLM "SOFTWARE\Notepad++" ""

retry:
		FindWindow $0 "Notepad++"
		${If} $0 != "0"
			MessageBox MB_RETRYCANCEL|MB_ICONSTOP "Notepad++ must be closed during installation.$\n$\nPlease close all instances of Notepad++ and try again." IDRETRY retry
			Quit
		${EndIf}
	${Else}
		; Exchange variables with user instance.
		!insertmacro UAC_AsUser_Call Function ExchangeVariables ${UAC_SYNCREGISTERS}
		StrCpy $NppPath $1
		StrCpy $NppConfigPath $2
		StrCpy $NppExeFile $3
		StrCpy $ResetStyleSettings $4
	${EndIf}
FunctionEnd

Function ExchangeVariables
	StrCpy $1 $NppPath
	StrCpy $2 $NppConfigPath
	StrCpy $3 $NppExeFile
	StrCpy $4 $ResetStyleSettings
	HideWindow
FunctionEnd

Function PageOptions
	${If} ${UAC_IsInnerInstance}
	${AndIf} ${UAC_IsAdmin}
		; Skip page
		Abort
	${EndIf}

	nsDialogs::Create /NOUNLOAD 1018

	${NSD_CreateIcon} 0u 0u 32 32 ""
	Pop $0
	${NSD_SetIconFromInstaller} $0 $0

	${NSD_CreateLabel} 46 0u -46 31u "Setup will install RainLexer to the following folder. Click Install to start the installation."
	Pop $0

	${NSD_CreateGroupBox} 0u 30u -1u 45u "Installation Folder"
	Pop $0

	${NSD_CreateLabel} 6u 42u -12u 9u "To change the installation folder, click Browse and locate Notepad++.exe."
	Pop $0

	${NSD_CreateDirRequest} 6u 55u 200u 13u "$NppPath"
	Pop $R1
	SendMessage $R1 ${EM_SETREADONLY} 1 0

	${NSD_CreateBrowseButton} 210u 55u 50u 13u "Browse..."
	Pop $0
	${NSD_OnClick} $0 BrowseForNppExe

	${NSD_CreateCheckbox} 6u 82u 285u 12u "Reset style settings"
	Pop $R2

	${If} $NppPath == ""
		; Disable Install button.
		GetDlgItem $0 $HWNDPARENT 1
		EnableWindow $0 0
	${EndIf}

	nsDialogs::Show
FunctionEnd

Function BrowseForNppExe
	nsDialogs::SelectFileDialog /NOUNLOAD open "$NppPath" "Notepad++ executable file|notepad++.exe"
	Pop $0

	${If} $0 != ""
		${GetParent} "$0" $0
		${NSD_SetText} $R1 "$0"

		; Enable Install button.
		GetDlgItem $0 $HWNDPARENT 1
		EnableWindow $0 1
	${EndIf}
FunctionEnd

Function PageOptionsOnLeave
	${NSD_GetText} $R1 $NppPath

	${If} ${FileExists} "$NppPath\config.xml"
		StrCpy $NppConfigPath "$NppPath"
		StrCpy $NppExeFile "$NppPath\notepad++.exe"
	${ElseIf} ${FileExists} "$NppPath\..\..\Notepad++Portable.exe"
	${AndIf} ${FileExists} "$NppPath\..\..\Data\settings\config.xml"
		; PortableApps install.
		StrCpy $NppConfigPath "$NppPath"
		StrCpy $NppExeFile "$NppPath\..\..\Notepad++Portable.exe"
	${ElseIf} ${FileExists} "$APPDATA\Notepad++\config.xml"
		StrCpy $NppConfigPath "$APPDATA\Notepad++"
		StrCpy $NppExeFile "$NppPath\notepad++.exe"
	${Else}
		MessageBox MB_OK|MB_ICONSTOP "Unable to find config.xml."
		Abort
	${EndIf}

	MoreInfo::GetProductVersion "$NppPath\notepad++.exe"
	Pop $0
	${VersionCompare} "$0" "6.0.0" $0
	${If} $0 = 2
		MessageBox MB_OK|MB_ICONSTOP "Notepad++ 6.0.0 or higher is required to install RainLexer. Try again after installing the latest version of Notepad++."
		Quit
	${EndIf}

	${NSD_GetState} $R2 $ResetStyleSettings

	; Test if $NppPath is writable. If not, try to elevate.
	ClearErrors
	CreateDirectory "$NppPath\plugins"
	WriteINIStr "$NppPath\plugins\~writetest.tmp" "1" "1" "1"
	Delete "$NppPath\plugins\~writetest.tmp"

	${If} ${Errors}
		${IfNot} ${UAC_IsAdmin}
UAC_TryAgain:
			!insertmacro UAC_RunElevated
			${Switch} $0
			${Case} 0
				${IfThen} $1 = 1 ${|} Quit ${|}
				${IfThen} $3 <> 0 ${|} ${Break} ${|}
				${If} $1 = 3
					MessageBox MB_OK|MB_ICONSTOP|MB_TOPMOST|MB_SETFOREGROUND "Elevation error." /SD IDNO IDOK UAC_TryAgain IDNO 0
				${EndIf}
			${Case} 1223
				Quit
			${Case} 1062
				MessageBox MB_OK|MB_ICONSTOP|MB_TOPMOST|MB_SETFOREGROUND "Elevation error."
				Quit
			${Default}
				MessageBox MB_OK|MB_ICONSTOP|MB_TOPMOST|MB_SETFOREGROUND "Elevation error."
				Quit
			${EndSwitch}
		${EndIf}
	${EndIf}
FunctionEnd

Function .onInstSuccess
	; Open Notepad++ through the user instance
	!insertmacro UAC_AsUser_Call Function OpenNpp ${UAC_SYNCREGISTERS}
FunctionEnd

Function OpenNpp
	Exec "$NppExeFile"
FunctionEnd

Section
	XML::create
	XML::load "$NppConfigPath\config.xml"
	${If} $0 <> 0
		XML::select "//GUIConfig[@name='stylerTheme']"
		${If} $2 <> 0
			XML::getAttribute "path"
			${GetFileName} $3 $NppThemeName
		${EndIf}
	${EndIf}

	${If} ${FileExists} "$NppConfigPath\userDefineLang.xml"
		XML::load "$NppConfigPath\userDefineLang.xml"
		XML::select "//UserLang[@name='Rainmeter']"
		${If} $2 != "0"
			MessageBox MB_YESNO|MB_ICONQUESTION "The entry for 'Rainmeter' in your userDefineLang.xml file will be renamed to 'Rainmeter uDL' to avoid conflicts with RainLexer.$\n$\nDo you want to continue?" IDYES +2
			Quit
			XML::setAttribute "name" "Rainmeter uDL"
			XML::save "$NppConfigPath\userDefineLang.xml"
		${EndIf}
	${EndIf}

	; Old versions were installed to %APPDATA%, so remove that first.
	Delete "$NppConfigPath\plugins\RainLexer.dll"
	Delete "$NppPath\plugins\RainLexer.dll"
	
	SetOutPath "$LOCALAPPDATA\Notepad++\plugins\RainLexer"
	StrCpy $0 "$NppPath\notepad++.exe"
	System::Call "kernel32::GetBinaryType(t r0, *i .r1)"
	${If} $1 = ${SCS_64BIT_BINARY}
		File "..\x64-Release\RainLexer.dll"
	${Else}
		File "..\x32-Release\RainLexer.dll"
	${EndIf}

	SetOutPath "$NppConfigPath\plugins\config"
	${If} $ResetStyleSettings <> 1
	${AndIf} ${FileExists} "$NppConfigPath\plugins\config\RainLexer.xml"
		XML::create
		XML::load "$NppConfigPath\plugins\config\RainLexer.xml"
		${If} $0 <> 0
			; For backwards compatibility
			${ReadStyleVariables} "OPTION" "KEYWORD"
			${ReadStyleVariables} "VALUE" "VALID OPTION"
			${ReadStyleVariables} "INVALID_VALUE" "INVALID OPTION"
			${ReadStyleVariables} "VARIABLE" "INT VARIABLE"
			${ReadStyleVariables} "USER_VARIABLE" "EXT VARIABLE"

			${ReadStyleVariables} "DEFAULT" "DEFAULT"
			${ReadStyleVariables} "COMMENT" "COMMENT"
			${ReadStyleVariables} "SECTION" "SECTION"
			${ReadStyleVariables} "OPTION" "OPTION"
			${ReadStyleVariables} "EQUALS" "EQUALS"
			${ReadStyleVariables} "INVALID_VALUE" "INVALID VALUE"
			${ReadStyleVariables} "VALUE" "VALUE"
			${ReadStyleVariables} "BANG" "BANG"
			${ReadStyleVariables} "VARIABLE" "VARIABLE"
			${ReadStyleVariables} "USER_VARIABLE" "USER VARIABLE"
		${EndIf}

		${If} $NppThemeName == "Zenburn.xml"
			File "..\Config\Zenburn\RainLexer.xml"
		${Else}
			File "..\Config\Default\RainLexer.xml"
		${EndIf}

		${If} $DEFAULTfgColor != ""
			XML::create
			XML::load "$NppConfigPath\plugins\config\RainLexer.xml"
			${WriteStyleVariables} "DEFAULT" "DEFAULT"
			${WriteStyleVariables} "COMMENT" "COMMENT"
			${WriteStyleVariables} "SECTION" "SECTION"
			${WriteStyleVariables} "OPTION" "OPTION"
			${WriteStyleVariables} "EQUALS" "EQUALS"
			${WriteStyleVariables} "INVALID_VALUE" "INVALID VALUE"
			${WriteStyleVariables} "VALUE" "VALUE"
			${WriteStyleVariables} "BANG" "BANG"
			${WriteStyleVariables} "VARIABLE" "VARIABLE"
			${WriteStyleVariables} "USER_VARIABLE" "USER VARIABLE"
			XML::save "$NppConfigPath\plugins\config\RainLexer.xml"
		${EndIf}
	${Else}
		${If} $NppThemeName == "Zenburn.xml"
			File "..\Config\Zenburn\RainLexer.xml"
		${Else}
			File "..\Config\Default\RainLexer.xml"
		${EndIf}
	${EndIf}

	${If} ${FileExists} "$NppConfigPath\session.xml"
		XML::create
		XML::load "$NppConfigPath\session.xml"
		${If} $0 <> 0
			${DoUntil} $2 = 0
				XML::select "//File[@lang='MS INI file']"
				${If} $2 <> 0
					XML::setAttribute "lang" "Rainmeter"
				${EndIf}
			${Loop}
			XML::save "$NppConfigPath\session.xml"
		${EndIf}
	${EndIf}
SectionEnd

