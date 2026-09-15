; Copyright (c) Rainmeter Team. Source code licensed under GNU GPL v2 (see LICENSE file).

!macro SetInstallerSettings
	Name "Rainmeter"
	BrandingText " "
	SetCompressor /SOLID lzma
	RequestExecutionLevel user
	ShowInstDetails nevershow
	AllowSkipFiles off
	XPStyle on
	ManifestDPIAware true
	!define MUI_ICON ".\Installer.ico"
	!define MUI_UNICON ".\Installer.ico"
!macroend

!macro SetVersionInfo FILEDESCRIPTION ORIGINALFILENAME
	VIAddVersionKey "CompanyName" "Rainmeter"
	VIAddVersionKey "ProductName" "Rainmeter"
	VIAddVersionKey "FileDescription" "${FILEDESCRIPTION}"
	VIAddVersionKey "FileVersion" "${VERSION_FULL}"
	VIAddVersionKey "ProductVersion" "${VERSION_FULL}"
	VIAddVersionKey "OriginalFilename" "${ORIGINALFILENAME}"
	VIAddVersionKey "LegalCopyright" "${U+00A9} ${BUILD_YEAR} Rainmeter Team"
	VIProductVersion "${VERSION_FULL}"
!macroend

!macro IncludeLanguage LANGUAGE CUSTOMLANGUAGE
	!insertmacro MUI_LANGUAGE ${LANGUAGE}
	!insertmacro LANGFILE_INCLUDE "..\..\BuildOut\Installer\${CUSTOMLANGUAGE}.nsh"
!macroend
!define IncludeLanguage "!insertmacro IncludeLanguage"

!macro IncludeLanguages
	!include "..\..\BuildOut\Installer\Languages.nsh"
!macroend

!macro Elevate
UAC_TryAgain:
	!insertmacro UAC_RunElevated
	${Switch} $0
	${Case} 0
		${IfThen} $1 = 1 ${|} Quit ${|}
		${IfThen} $3 <> 0 ${|} ${Break} ${|}
		${If} $1 = 3
			MessageBox MB_OK|MB_ICONSTOP|MB_TOPMOST|MB_SETFOREGROUND "$(AdminError)" /SD IDNO IDOK UAC_TryAgain IDNO 0
			!insertmacro LOG_ERROR ${ERROR_NOTADMIN}
		${EndIf}
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

; Creates a custom page dialog, aborting the program if it cannot be created. A missing dialog
; used to go unnoticed because nsDialogs::Show returns immediately and silently skips the page.
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
