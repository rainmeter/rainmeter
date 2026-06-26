; ShellExecuteAsExplorer.nsh
;
; 32-bit Unicode NSIS 3.x System plug-in implementation.
;
; *** 32-BIT ONLY — will not compile under a 64-bit NSIS build. ***
;
; Public macro:
;
;   ${ShellExecuteAsExplorer} $result "open" "$INSTDIR\MyApp.exe" "--arg" "$INSTDIR" ${SEAE_SW_SHOWNORMAL}
;
; Returns:
;   $result = 0 on success
;   $result = HRESULT / error code on failure
;
; This follows the StdUtils ShellDispatch.cpp route:
;   IShellWindows
;   -> FindWindowSW(SWC_DESKTOP)
;   -> IUnknown_QueryService(SID_STopLevelBrowser, IID_IShellBrowser)
;   -> IShellBrowser::QueryActiveShellView
;   -> IShellView::GetItemObject(SVGIO_BACKGROUND)
;   -> IShellFolderViewDual::get_Application
;   -> IShellDispatch2::ShellExecute
;
; NOTE: All state is stored in global Var SEAE_* variables. This function is
; NOT re-entrant. Do not call it concurrently.

!ifndef __SHELLEXECUTEASEXPLORER_NSH__
!define __SHELLEXECUTEASEXPLORER_NSH__

!if ${NSIS_PTR_SIZE} > 4
	!error "ShellExecuteAsExplorer.nsh is 32-bit only and cannot be used with a 64-bit NSIS build."
!endif

!include LogicLib.nsh

; Public constants
!define SEAE_SW_HIDE              0
!define SEAE_SW_SHOWNORMAL        1
!define SEAE_SW_SHOW              5

; Internal constants
!define SEAE_S_OK                 0
!define SEAE_S_FALSE              1
!define SEAE_E_NO_DESKTOP_HWND    0x80004005  ; E_FAIL

!define SEAE_CLSCTX_LOCAL_SERVER  4

!define SEAE_SWC_DESKTOP          8
!define SEAE_SWFO_NEEDDISPATCH    1

!define SEAE_SVGIO_BACKGROUND     0

!define SEAE_DISPATCH_METHOD      1

!define SEAE_VT_EMPTY             0
!define SEAE_VT_I4                3
!define SEAE_VT_BSTR              8

; 32-bit VARIANT is 16 bytes  (vt:2 + reserved:6 + value:8)
!define SEAE_VARIANT_SIZE         16
; 32-bit DISPPARAMS is 16 bytes (rgvarg:4 + rgdispidNamedArgs:4 + cArgs:4 + cNamedArgs:4)
!define SEAE_DISPPARAMS_SIZE      16

!define SEAE_CLSID_ShellWindows       "{9BA05972-F6A8-11CF-A442-00A0C90A8F39}"
!define SEAE_IID_IShellWindows        "{85CB6900-4D95-11CF-960C-0080C7F4EE85}"

!define SEAE_SID_STopLevelBrowser     "{4C96BE40-915C-11CF-99D3-00AA004AE837}"
!define SEAE_IID_IShellBrowser        "{000214E2-0000-0000-C000-000000000046}"

!define SEAE_IID_IDispatch            "{00020400-0000-0000-C000-000000000046}"
!define SEAE_IID_IShellFolderViewDual "{E7A1AF80-4D96-11CF-960C-0080C7F4EE85}"
!define SEAE_IID_IShellDispatch2      "{A4C6892C-3BA9-11D2-9DEA-00C04FB16162}"

!define SEAE_IID_NULL                 "{00000000-0000-0000-0000-000000000046}"

; Public macro
!macro ShellExecuteAsExplorer _RESULT _VERB _FILE _PARAMS _DIR _SHOW
	Push `${_SHOW}`
	Push `${_VERB}`
	Push `${_DIR}`
	Push `${_PARAMS}`
	Push `${_FILE}`
	Call _SEAE_ShellExecuteAsExplorer
	Pop `${_RESULT}`
!macroend

!define ShellExecuteAsExplorer `!insertmacro ShellExecuteAsExplorer`

; Internal helper macros
!macro _SEAE_ReleaseCom PTR
	${If} ${PTR} P<> 0
		System::Call '${PTR}->2()'
		StrCpy ${PTR} 0
	${EndIf}
!macroend

!macro _SEAE_FreeMem PTR
	${If} ${PTR} P<> 0
		System::Free ${PTR}
		StrCpy ${PTR} 0
	${EndIf}
!macroend

!macro _SEAE_FreeBStr PTR
	${If} ${PTR} P<> 0
		System::Call 'oleaut32::SysFreeString(p ${PTR})'
		StrCpy ${PTR} 0
	${EndIf}
!macroend

; Internal variables
Var SEAE_File
Var SEAE_Params
Var SEAE_Dir
Var SEAE_Verb
Var SEAE_Show

Var SEAE_hr
Var SEAE_comInit

Var SEAE_psw
Var SEAE_desktopHwnd
Var SEAE_pdispDesktop
Var SEAE_psb
Var SEAE_psv
Var SEAE_pdispBackground
Var SEAE_psfvd
Var SEAE_pdispApp
Var SEAE_psd2

Var SEAE_vEmpty

Var SEAE_bstrName
Var SEAE_nameArray
Var SEAE_dispidMem
Var SEAE_dispid

Var SEAE_bstrFile
Var SEAE_bstrParams
Var SEAE_bstrDir
Var SEAE_bstrVerb

Var SEAE_variants
Var SEAE_dispparams
Var SEAE_tmp

Function _SEAE_ShellExecuteAsExplorer
	; Stack in:
	;   file
	;   params
	;   directory
	;   verb
	;   show
	;
	; Stack out:
	;   0 on success, otherwise HRESULT/error

	Pop $SEAE_File
	Pop $SEAE_Params
	Pop $SEAE_Dir
	Pop $SEAE_Verb
	Pop $SEAE_Show

	StrCpy $SEAE_hr -1
	StrCpy $SEAE_comInit 0

	StrCpy $SEAE_psw 0
	StrCpy $SEAE_desktopHwnd 0
	StrCpy $SEAE_pdispDesktop 0
	StrCpy $SEAE_psb 0
	StrCpy $SEAE_psv 0
	StrCpy $SEAE_pdispBackground 0
	StrCpy $SEAE_psfvd 0
	StrCpy $SEAE_pdispApp 0
	StrCpy $SEAE_psd2 0

	StrCpy $SEAE_vEmpty 0

	StrCpy $SEAE_bstrName 0
	StrCpy $SEAE_nameArray 0
	StrCpy $SEAE_dispidMem 0
	StrCpy $SEAE_dispid 0

	StrCpy $SEAE_bstrFile 0
	StrCpy $SEAE_bstrParams 0
	StrCpy $SEAE_bstrDir 0
	StrCpy $SEAE_bstrVerb 0

	StrCpy $SEAE_variants 0
	StrCpy $SEAE_dispparams 0
	StrCpy $SEAE_tmp 0

	; ----------------------------------------------------------
	; CoInitialize(NULL)
	; ----------------------------------------------------------

	System::Call 'ole32::CoInitialize(p 0) i.r0'
	StrCpy $SEAE_hr $0

	${If} $SEAE_hr = ${SEAE_S_OK}
	${OrIf} $SEAE_hr = ${SEAE_S_FALSE}
		StrCpy $SEAE_comInit 1
	${Else}
		Goto SEAE_done
	${EndIf}

	; ----------------------------------------------------------
	; CoCreateInstance(CLSID_ShellWindows, ..., IID_IShellWindows)
	; ----------------------------------------------------------

	System::Call 'ole32::CoCreateInstance( \
		g "${SEAE_CLSID_ShellWindows}", \
		p 0, \
		i ${SEAE_CLSCTX_LOCAL_SERVER}, \
		g "${SEAE_IID_IShellWindows}", \
		*p .r0 \
	) i.r1'

	StrCpy $SEAE_psw $0
	StrCpy $SEAE_hr $1

	${If} $SEAE_hr != ${SEAE_S_OK}
		Goto SEAE_done
	${EndIf}

	; ----------------------------------------------------------
	; VARIANT vEmpty = VT_EMPTY
	; ----------------------------------------------------------

	System::Alloc ${SEAE_VARIANT_SIZE}
	Pop $SEAE_vEmpty
	System::Call '*$SEAE_vEmpty(&i2 ${SEAE_VT_EMPTY}, &i2 0, &i2 0, &i2 0, p 0)'

	; ----------------------------------------------------------
	; IShellWindows::FindWindowSW(
	;   VT_EMPTY,
	;   VT_EMPTY,
	;   SWC_DESKTOP,
	;   &hwnd,
	;   SWFO_NEEDDISPATCH,
	;   &pdispDesktop
	; )
	;
	; IShellWindows vtable layout (IDispatch base occupies 0-6):
	;   7  = get_Count
	;   8  = Item
	;   9  = _NewEnum
	;   10 = Register
	;   11 = RegisterPending
	;   12 = Revoke
	;   13 = OnNavigate
	;   14 = OnActivated
	;   15 = FindWindowSW   <---
	; ----------------------------------------------------------

	System::Call '$SEAE_psw->15( \
		p $SEAE_vEmpty, \
		p $SEAE_vEmpty, \
		i ${SEAE_SWC_DESKTOP}, \
		*i .r0, \
		i ${SEAE_SWFO_NEEDDISPATCH}, \
		*p .r1 \
	) i.r2'

	StrCpy $SEAE_desktopHwnd $0
	StrCpy $SEAE_pdispDesktop $1
	StrCpy $SEAE_hr $2

	${If} $SEAE_hr != ${SEAE_S_OK}
		Goto SEAE_done
	${EndIf}

	; FindWindowSW may return S_OK with a null HWND when no Explorer
	; shell is running (e.g. custom shell replacements). Treat as failure.
	${If} $SEAE_desktopHwnd = 0
		StrCpy $SEAE_hr ${SEAE_E_NO_DESKTOP_HWND}
		Goto SEAE_done
	${EndIf}

	; Optional: let Explorer set foreground.
	System::Call 'user32::GetWindowThreadProcessId(p $SEAE_desktopHwnd, *i .r0) i.r1'
	${If} $0 != 0
		System::Call 'user32::AllowSetForegroundWindow(i r0) i.r2'
	${EndIf}

	; ----------------------------------------------------------
	; IUnknown_QueryService(
	;   pdispDesktop,
	;   SID_STopLevelBrowser,
	;   IID_IShellBrowser,
	;   &psb
	; )
	; ----------------------------------------------------------

	System::Call 'shlwapi::IUnknown_QueryService( \
		p $SEAE_pdispDesktop, \
		g "${SEAE_SID_STopLevelBrowser}", \
		g "${SEAE_IID_IShellBrowser}", \
		*p .r0 \
	) i.r1'

	StrCpy $SEAE_psb $0
	StrCpy $SEAE_hr $1

	${If} $SEAE_hr != ${SEAE_S_OK}
		Goto SEAE_done
	${EndIf}

	; ----------------------------------------------------------
	; IShellBrowser::QueryActiveShellView(&psv)
	;
	; IShellBrowser vtable layout (IOleWindow base = 0-4,
	; IShellBrowser extends from 5):
	;   5  = InsertMenusSB
	;   6  = SetMenuSB
	;   7  = RemoveMenusSB
	;   8  = SetStatusTextSB
	;   9  = EnableModelessSB
	;   10 = TranslateAcceleratorSB
	;   11 = BrowseObject
	;   12 = GetViewStateStream
	;   13 = GetControlWindow
	;   14 = SendControlMsg
	;   15 = QueryActiveShellView   <---
	; ----------------------------------------------------------

	System::Call '$SEAE_psb->15(*p .r0) i.r1'

	StrCpy $SEAE_psv $0
	StrCpy $SEAE_hr $1

	${If} $SEAE_hr != ${SEAE_S_OK}
		Goto SEAE_done
	${EndIf}

	; IShellView::GetItemObject(
	;   SVGIO_BACKGROUND,
	;   IID_IDispatch,
	;   &pdispBackground
	; )
	;
	; IShellView vtable layout (IOleWindow base = 0-4,
	; IShellView extends from 5):
	;   5  = TranslateAccelerator
	;   6  = EnableModeless
	;   7  = UIActivate
	;   8  = Refresh
	;   9  = CreateViewWindow
	;   10 = DestroyViewWindow
	;   11 = GetCurrentInfo
	;   12 = AddPropertySheetPages
	;   13 = SaveViewState
	;   14 = SelectItem
	;   15 = GetItemObject   <---
	; ----------------------------------------------------------

	System::Call '$SEAE_psv->15( \
		i ${SEAE_SVGIO_BACKGROUND}, \
		g "${SEAE_IID_IDispatch}", \
		*p .r0 \
	) i.r1'

	StrCpy $SEAE_pdispBackground $0
	StrCpy $SEAE_hr $1

	${If} $SEAE_hr != ${SEAE_S_OK}
		Goto SEAE_done
	${EndIf}

	; ----------------------------------------------------------
	; pdispBackground->QueryInterface(
	;   IID_IShellFolderViewDual,
	;   &psfvd
	; )
	; ----------------------------------------------------------

	System::Call '$SEAE_pdispBackground->0( \
		g "${SEAE_IID_IShellFolderViewDual}", \
		*p .r0 \
	) i.r1'

	StrCpy $SEAE_psfvd $0
	StrCpy $SEAE_hr $1

	${If} $SEAE_hr != ${SEAE_S_OK}
		Goto SEAE_done
	${EndIf}

	; ----------------------------------------------------------
	; IShellFolderViewDual::get_Application(&pdispApp)
	;
	; IShellFolderViewDual vtable layout
	; (IDispatch base occupies indices 0-6):
	;   0 = QueryInterface
	;   1 = AddRef
	;   2 = Release
	;   3 = GetTypeInfoCount
	;   4 = GetTypeInfo
	;   5 = GetIDsOfNames
	;   6 = Invoke
	;   7 = get_Application   <---
	; ----------------------------------------------------------

	System::Call '$SEAE_psfvd->7(*p .r0) i.r1'

	StrCpy $SEAE_pdispApp $0
	StrCpy $SEAE_hr $1

	${If} $SEAE_hr != ${SEAE_S_OK}
		Goto SEAE_done
	${EndIf}

	; ----------------------------------------------------------
	; pdispApp->QueryInterface(IID_IShellDispatch2, &psd2)
	; ----------------------------------------------------------

	System::Call '$SEAE_pdispApp->0( \
		g "${SEAE_IID_IShellDispatch2}", \
		*p .r0 \
	) i.r1'

	StrCpy $SEAE_psd2 $0
	StrCpy $SEAE_hr $1

	${If} $SEAE_hr != ${SEAE_S_OK}
		Goto SEAE_done
	${EndIf}

	; ----------------------------------------------------------
	; Get DISPID for "ShellExecute"
	; IDispatch::GetIDsOfNames = vtable index 5
	; ----------------------------------------------------------

	System::Call 'oleaut32::SysAllocString(w "ShellExecute") p.r0'
	StrCpy $SEAE_bstrName $0

	System::Alloc 4
	Pop $SEAE_nameArray
	System::Call '*$SEAE_nameArray(p $SEAE_bstrName)'

	System::Alloc 4
	Pop $SEAE_dispidMem

	System::Call '$SEAE_psd2->5( \
		g "${SEAE_IID_NULL}", \
		p $SEAE_nameArray, \
		i 1, \
		i 0, \
		p $SEAE_dispidMem \
	) i.r0'

	StrCpy $SEAE_hr $0

	${If} $SEAE_hr != ${SEAE_S_OK}
		Goto SEAE_done
	${EndIf}

	System::Call '*$SEAE_dispidMem(i .r0)'
	StrCpy $SEAE_dispid $0

	; ----------------------------------------------------------
	; Build VARIANTARG[5]
	;
	; ShellExecute(File, vArgs, vDir, vOperation, vShow)
	;
	; IDispatch::Invoke arguments are in reverse order:
	;   [0] = vShow        (last param first)
	;   [1] = vOperation / verb
	;   [2] = vDir
	;   [3] = vArgs
	;   [4] = File         (first param last)
	;
	; Each VARIANT is ${SEAE_VARIANT_SIZE} (16) bytes on 32-bit.
	; Total allocation: 5 * 16 = 80 bytes.
	; ----------------------------------------------------------

	System::Alloc 80   ; 5 * SEAE_VARIANT_SIZE
	Pop $SEAE_variants

	; [0] VT_I4 show
	System::Call '*$SEAE_variants(&i2 ${SEAE_VT_I4}, &i2 0, &i2 0, &i2 0, i $SEAE_Show)'

	; [1] VT_BSTR verb
	System::Call 'oleaut32::SysAllocString(w "$SEAE_Verb") p.r0'
	StrCpy $SEAE_bstrVerb $0
	IntOp $SEAE_tmp $SEAE_variants + 16   ; offset = 1 * SEAE_VARIANT_SIZE
	System::Call '*$SEAE_tmp(&i2 ${SEAE_VT_BSTR}, &i2 0, &i2 0, &i2 0, p $SEAE_bstrVerb)'

	; [2] VT_BSTR dir
	System::Call 'oleaut32::SysAllocString(w "$SEAE_Dir") p.r0'
	StrCpy $SEAE_bstrDir $0
	IntOp $SEAE_tmp $SEAE_variants + 32   ; offset = 2 * SEAE_VARIANT_SIZE
	System::Call '*$SEAE_tmp(&i2 ${SEAE_VT_BSTR}, &i2 0, &i2 0, &i2 0, p $SEAE_bstrDir)'

	; [3] VT_BSTR params
	System::Call 'oleaut32::SysAllocString(w "$SEAE_Params") p.r0'
	StrCpy $SEAE_bstrParams $0
	IntOp $SEAE_tmp $SEAE_variants + 48   ; offset = 3 * SEAE_VARIANT_SIZE
	System::Call '*$SEAE_tmp(&i2 ${SEAE_VT_BSTR}, &i2 0, &i2 0, &i2 0, p $SEAE_bstrParams)'

	; [4] VT_BSTR file
	System::Call 'oleaut32::SysAllocString(w "$SEAE_File") p.r0'
	StrCpy $SEAE_bstrFile $0
	IntOp $SEAE_tmp $SEAE_variants + 64   ; offset = 4 * SEAE_VARIANT_SIZE
	System::Call '*$SEAE_tmp(&i2 ${SEAE_VT_BSTR}, &i2 0, &i2 0, &i2 0, p $SEAE_bstrFile)'

	; ----------------------------------------------------------
	; Build DISPPARAMS (${SEAE_DISPPARAMS_SIZE} bytes on 32-bit):
	;   VARIANTARG *rgvarg            -> $SEAE_variants
	;   DISPID     *rgdispidNamedArgs -> NULL (no named args)
	;   UINT        cArgs             -> 5
	;   UINT        cNamedArgs        -> 0
	; ----------------------------------------------------------

	System::Alloc ${SEAE_DISPPARAMS_SIZE}
	Pop $SEAE_dispparams

	System::Call '*$SEAE_dispparams( \
		p $SEAE_variants, \
		p 0, \
		i 5, \
		i 0 \
	)'

	; ----------------------------------------------------------
	; IDispatch::Invoke = vtable index 6
	; ----------------------------------------------------------

	System::Call '$SEAE_psd2->6( \
		i $SEAE_dispid, \
		g "${SEAE_IID_NULL}", \
		i 0, \
		i ${SEAE_DISPATCH_METHOD}, \
		p $SEAE_dispparams, \
		p 0, \
		p 0, \
		p 0 \
	) i.r0'

	StrCpy $SEAE_hr $0

SEAE_done:
	; Cleanup — in reverse order of acquisition
	!insertmacro _SEAE_FreeMem  $SEAE_dispparams
	!insertmacro _SEAE_FreeMem  $SEAE_variants

	!insertmacro _SEAE_FreeBStr $SEAE_bstrFile
	!insertmacro _SEAE_FreeBStr $SEAE_bstrParams
	!insertmacro _SEAE_FreeBStr $SEAE_bstrDir
	!insertmacro _SEAE_FreeBStr $SEAE_bstrVerb

	!insertmacro _SEAE_FreeMem  $SEAE_dispidMem
	!insertmacro _SEAE_FreeMem  $SEAE_nameArray
	!insertmacro _SEAE_FreeBStr $SEAE_bstrName

	!insertmacro _SEAE_FreeMem  $SEAE_vEmpty

	!insertmacro _SEAE_ReleaseCom $SEAE_psd2
	!insertmacro _SEAE_ReleaseCom $SEAE_pdispApp
	!insertmacro _SEAE_ReleaseCom $SEAE_psfvd
	!insertmacro _SEAE_ReleaseCom $SEAE_pdispBackground
	!insertmacro _SEAE_ReleaseCom $SEAE_psv
	!insertmacro _SEAE_ReleaseCom $SEAE_psb
	!insertmacro _SEAE_ReleaseCom $SEAE_pdispDesktop
	!insertmacro _SEAE_ReleaseCom $SEAE_psw

	${If} $SEAE_comInit = 1
		System::Call 'ole32::CoUninitialize()'
	${EndIf}

	Push $SEAE_hr
FunctionEnd

!endif
