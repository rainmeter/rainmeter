/* Copyright (C) 2021 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

; Error levels (for silent install)
!define ERROR_UNSUPPORTED	3
!define ERROR_NOTADMIN		4
!define ERROR_NOLOGONSVC	5
!define ERROR_WRITEFAIL		6
!define ERROR_CLOSEFAIL		7

Var LogFile

; Simple log macro
; Only created on silent installs
!macro LOG_ERROR ERROR
	SetErrorLevel ${ERROR}
	${If} ${Silent}
		!insertmacro GetTime
		${GetTime} "" "L" $0 $1 $2 $3 $4 $5 $6
		StrCpy $7 "$\r$\n"  ; newline

		StrCpy $LogFile "$DESKTOP\Rainmeter.SilentInstall.log"

		${ForEach} $8 1 0 - 1
			ClearErrors
			FileOpen $9 "$LogFile" w
			${If} ${Errors}
				StrCpy $LogFile "$PROFILE\Rainmeter.SilentInstall.log"
				${Continue}
			${EndIf}

			FileWriteUTF16LE /BOM $9 ""
			FileWriteUTF16LE $9 "Timestamp   : $2.$1.$0-$4:$5:$6$7"
			${If} ${RunningX64}
				FileWriteUTF16LE $9 "IsOS64Bit   : true$7"
			${Else}
				FileWriteUTF16LE $9 "IsOS64Bit   : false$7"
			${EndIf}
			${If} ${UAC_IsAdmin}
				FileWriteUTF16LE $9 "IsUACAdmin  : true$7"
			${Else}
				FileWriteUTF16LE $9 "IsUACAdmin  : false$7"
			${EndIf}
			FileWriteUTF16LE $9 "CMD Line    : $CMDLINE$7"
			FileWriteUTF16LE $9 "Version     : Rainmeter ${VERSION_FULL}$7"
			FileWriteUTF16LE $9 "INSTDIR (D) : $INSTDIR$7"
			FileWriteUTF16LE $9 "Language    : $LANGUAGE$7"
			FileWriteUTF16LE $9 "Error (${ERROR})   : "
			${Switch} ${ERROR}
			${Case} ${ERROR_UNSUPPORTED}
				FileWriteUTF16LE $9 "Rainmeter requires at least Windows 7 (SP1) with the Platform Update installed."
				${Break}
			${Case} ${ERROR_NOTADMIN}
				FileWriteUTF16LE $9 "Adminstrative rights required."
				${Break}
			${Case} ${ERROR_NOLOGONSVC}
				FileWriteUTF16LE $9 "Logon service not running."
				${Break}
			${Case} ${ERROR_WRITEFAIL}
				FileWriteUTF16LE $9 "Cannot write file."
				${Break}
			${Case} ${ERROR_CLOSEFAIL}
				FileWriteUTF16LE $9 "Error closing Rainmeter. Please manually close Rainmeter and re-try."
				${Break}
			${Default}
				FileWriteUTF16LE $9 "Unknown error."
				${Break}
			${EndSwitch}
			FileClose $9
			${ExitFor}
		${Next}
	${EndIf}
!macroend
