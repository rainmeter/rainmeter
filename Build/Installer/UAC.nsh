!ifndef UAC_HDR__INC
!verbose push
!verbose 3
!ifndef UAC_VERBOSE  
	!define UAC_VERBOSE 3
!endif
!verbose ${UAC_VERBOSE}

!define UAC_HDR__INC 0x00020204 ;MMmmbbrr

!include LogicLib.nsh

!macro _UAC_definemath def val1 op val2
!define /math _UAC_definemath "${val1}" ${op} ${val2}
!ifdef ${def}
	!undef ${def}
!endif
!define ${def} "${_UAC_definemath}"
!undef _UAC_definemath
!macroend

!macro _UAC_ParseDefineFlags_orin parse outflags
!searchparse /noerrors ${${parse}} "" _UAC_ParseDefineFlags_orin_f1 "|" _UAC_ParseDefineFlags_orin_f2
!define _UAC_ParseDefineFlags_orin_this ${_UAC_ParseDefineFlags_orin_f1}
!undef ${parse}
!define ${parse} ${_UAC_ParseDefineFlags_orin_f2}
!define _UAC_ParseDefineFlags_orin_saveout ${${outflags}}
!undef ${outflags}
!define /math ${outflags} "${_UAC_ParseDefineFlags_orin_saveout}" | "${_UAC_ParseDefineFlags_orin_this}"
!undef _UAC_ParseDefineFlags_orin_saveout
!undef _UAC_ParseDefineFlags_orin_this
!ifdef _UAC_ParseDefineFlags_orin_f1
	!undef _UAC_ParseDefineFlags_orin_f1
	!undef _UAC_ParseDefineFlags_orin_f2
!endif
!macroend
!macro _UAC_ParseDefineFlags_Begin _outdef _in
!define _UAC_PDF${_outdef}_parse "${_in}"
!define _UAC_PDF${_outdef}_flags ""
!define _UAC_PDF${_outdef}_r 0
!insertmacro _UAC_ParseDefineFlags_orin _UAC_PDF${_outdef}_parse _UAC_PDF${_outdef}_flags	;0x1
!insertmacro _UAC_ParseDefineFlags_orin _UAC_PDF${_outdef}_parse _UAC_PDF${_outdef}_flags	;0x2
!insertmacro _UAC_ParseDefineFlags_orin _UAC_PDF${_outdef}_parse _UAC_PDF${_outdef}_flags	;0x4
!insertmacro _UAC_ParseDefineFlags_orin _UAC_PDF${_outdef}_parse _UAC_PDF${_outdef}_flags	;0x8
!insertmacro _UAC_ParseDefineFlags_orin _UAC_PDF${_outdef}_parse _UAC_PDF${_outdef}_flags	;0x10
!macroend
!macro _UAC_ParseDefineFlags_End _outdef
!define ${_outdef} ${_UAC_PDF${_outdef}_r}
!undef _UAC_PDF${_outdef}_r
!undef _UAC_PDF${_outdef}_flags
!undef _UAC_PDF${_outdef}_parse
!macroend
!macro _UAC_ParseDefineFlags_IncludeFlag _outdef flag
!if ${_UAC_PDF${_outdef}_flags} & ${flag}
	!insertmacro _UAC_definemath _UAC_PDF${_outdef}_r ${_UAC_PDF${_outdef}_r} | ${flag}
!endif
!macroend
!macro _UAC_ParseDefineFlagsToInt _outdef _in
!insertmacro _UAC_ParseDefineFlags_Begin _UAC_ParseDefineFlagsToInt_tmp "${_in}"
!define ${_outdef} ${_UAC_PDF_UAC_ParseDefineFlagsToInt_tmp_flags}
!insertmacro _UAC_ParseDefineFlags_End _UAC_ParseDefineFlagsToInt_tmp
!undef _UAC_ParseDefineFlagsToInt_tmp
!macroend

!macro _UAC_IncL
!insertmacro _UAC_definemath __UAC_L "${__UAC_L}" + 1
!macroend


!macro _UAC_MakeLL_Cmp cmpop cmp pluginparams
!insertmacro _LOGICLIB_TEMP
UAC::_ ${pluginparams}
pop $_LOGICLIB_TEMP
!insertmacro ${cmpop} $_LOGICLIB_TEMP ${cmp} `${_t}` `${_f}`
!macroend


!macro UAC_RunElevated
UAC::_ 0
!macroend
!macro UAC_PageElevation_RunElevated
UAC::_ 0
!macroend
/*!macro UAC_OnInitElevation_RunElevated
UAC::_ 0
!macroend
!macro UAC_OnInitElevation_OnGuiInit
!macroend*/


!macro UAC_IsAdmin
UAC::_ 2
!macroend
!define UAC_IsAdmin `"" UAC_IsAdmin ""`
!macro _UAC_IsAdmin _a _b _t _f
!insertmacro _UAC_MakeLL_Cmp _!= 0 2s
!macroend



!macro UAC_IsInnerInstance
UAC::_ 3
!macroend
!define UAC_IsInnerInstance `"" UAC_IsInnerInstance ""`
!macro _UAC_IsInnerInstance _a _b _t _f
!insertmacro _UAC_MakeLL_Cmp _!= 0 3s
!macroend

!macro UAC_Notify_OnGuiInit
UAC::_ 4
!macroend
!macro UAC_PageElevation_OnGuiInit
!insertmacro UAC_Notify_OnGuiInit
!macroend
!macro UAC_PageElevation_OnInit
UAC::_ 5
${IfThen} ${Errors} ${|} Quit ${|}
!macroend


!define UAC_SYNCREGISTERS 0x1
#!define UAC_SYNCSTACK     0x2
!define UAC_SYNCOUTDIR    0x4
!define UAC_SYNCINSTDIR   0x8
#!define UAC_CLEARERRFLAG  0x10
!macro UAC_AsUser_Call type name flags
push $0
Get${type}Address $0 ${name}
!verbose push
!verbose ${UAC_VERBOSE}
!insertmacro _UAC_ParseDefineFlagsToInt _UAC_AsUser_Call__flags ${flags}
!verbose pop
StrCpy $0 "1$0:${_UAC_AsUser_Call__flags}"
!undef _UAC_AsUser_Call__flags
Exch $0
UAC::_
!macroend

!macro _UAC_AsUser_GenOp outvar op opparam1 opparam2
!define _UAC_AUGOGR_ID _UAC_AUGOGR_OP${outvar}${op}${opparam1}${opparam2}
!ifndef ${_UAC_AUGOGR_ID} ;Has this exact action been done before? 
	!if ${outvar} == $0
		!define ${_UAC_AUGOGR_ID} $1
	!else
		!define ${_UAC_AUGOGR_ID} $0
	!endif
	!if "${opparam1}" == ""
		!define _UAC_AUGOGR_OPP1 ${${_UAC_AUGOGR_ID}}
		!define _UAC_AUGOGR_OPP2 ${opparam2}
	!else
		!define _UAC_AUGOGR_OPP1 ${opparam1}
		!define _UAC_AUGOGR_OPP2 ${${_UAC_AUGOGR_ID}}
	!endif	
	goto ${_UAC_AUGOGR_ID}_C
	${_UAC_AUGOGR_ID}_F:
		${op} ${_UAC_AUGOGR_OPP1} ${_UAC_AUGOGR_OPP2}
		return
	${_UAC_AUGOGR_ID}_C:
	!undef _UAC_AUGOGR_OPP1
	!undef _UAC_AUGOGR_OPP2
!endif
push ${${_UAC_AUGOGR_ID}}
!insertmacro UAC_AsUser_Call Label ${_UAC_AUGOGR_ID}_F ${UAC_SYNCREGISTERS}
StrCpy ${outvar} ${${_UAC_AUGOGR_ID}}
pop ${${_UAC_AUGOGR_ID}}
!undef _UAC_AUGOGR_ID
!macroend

!macro UAC_AsUser_GetSection datatype secidx outvar
!insertmacro _UAC_AsUser_GenOp ${outvar} SectionGet${datatype} ${secidx} ""
!macroend

!macro UAC_AsUser_GetGlobalVar var
!insertmacro _UAC_AsUser_GenOp ${var} StrCpy "" ${var}
!macroend
!macro UAC_AsUser_GetGlobal outvar srcvar
!insertmacro _UAC_AsUser_GenOp ${outvar} StrCpy "" ${srcvar}
!macroend


!macro UAC_AsUser_ExecShell verb command params workdir show
!insertmacro _UAC_IncL
goto _UAC_L_E_${__UAC_L}
_UAC_L_F_${__UAC_L}:
ExecShell "${verb}" "${command}" ${params} ${show}
return
_UAC_L_E_${__UAC_L}:
!if "${workdir}" != ""
	push $outdir
	SetOutPath "${workdir}"
!endif
!insertmacro UAC_AsUser_Call Label _UAC_L_F_${__UAC_L} ${UAC_SYNCREGISTERS}|${UAC_SYNCOUTDIR}|${UAC_SYNCINSTDIR} #|${UAC_CLEARERRFLAG}
!if "${workdir}" != ""
	pop $outdir 
	SetOutPath $outdir
!endif
!macroend

!verbose pop
!endif /* UAC_HDR__INC */