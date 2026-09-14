;# Copyright (c) 2008 Thomas Gaugler <thomas@dadie.net>
;# Copyright (c) 2008-2026 NSIS Contributors
;#
;# Permission is hereby granted, free of charge, to any person
;# obtaining a copy of this software and associated documentation
;# files (the "Software"), to deal in the Software without
;# restriction, including without limitation the rights to use,
;# copy, modify, merge, publish, distribute, sublicense, and/or sell
;# copies of the Software, and to permit persons to whom the
;# Software is furnished to do so, subject to the following
;# conditions:
;#
;# The above copyright notice and this permission notice shall be
;# included in all copies or substantial portions of the Software.
;#
;# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
;# EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
;# OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
;# NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
;# HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
;# WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
;# FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
;# OTHER DEALINGS IN THE SOFTWARE.
;#
;#
;# Implementation of the functions CallProc, RealCallBack and
;# CallBack of the System plugin in pure x86 assembly.
;#
;# This is a hybrid assembly source file supporting both the
;# MASM as well as the GNU assembler in one file.
;#
;#
;# MASM:
;# ml.exe /c /nologo /Fo"call.obj" /W3 /Zi /errorReport:prompt /Ta"call.S"
;#
;# For enabling debug support use:
;# ml.exe /c /nologo /D"SYSTEM_LOG_DEBUG" /Fo"call.obj" /W3 /Zi /errorReport:prompt /Ta"call.S"
;#
;# GNU toolchain:
;# gcc -x assembler-with-cpp -s call.S -c
;#
;# For enabling debug support use:
;# gcc -x assembler-with-cpp -DSYSTEM_LOG_DEBUG -s call.S -c
;#
;#

; .if 0
;# MASM specific block
.386
.model flat
OPTION casemap:none
;# SYSCALL is identical to the C calling convention,
;# but does not add an underscore prefix to symbols.
OPTION language:syscall

SECTION_DATA     equ .data
SECTION_CODE     equ .code

DATA_SUFFIX      equ

ASCII            equ DB

TEMP_LABEL       equ @@
TEMP_LABEL_AHEAD equ @f
TEMP_LABEL_BACK  equ @b

MACRO_DECL       equ

FUNC_DECL MACRO name
name PROC
ENDM

FUNC_END MACRO name
name ENDP
ENDM

;# end of MASM specific block
IF 0
; .else
;# GNU toolchain specific block
.intel_syntax noprefix
.set __GNU__,1

#ifdef SYSTEM_LOG_DEBUG
;# Disable further proprocessing of SYSTEM_LOG_DEBUG
;# and hand it over to the GNU assembler
#undef SYSTEM_LOG_DEBUG
.set SYSTEM_LOG_DEBUG,1
#endif

#ifdef _UNICODE
#undef _UNICODE
.set _UNICODE,1
#endif

#define IFDEF            .ifdef
#define ELSE             .else
#define ENDIF            .endif

#define EXTERN           .extern

#define SECTION_DATA     .data
#define SECTION_CODE     .text

#define DATA_SUFFIX      :
#define BYTE             .byte
#define DWORD            .int
#define WORD             .word
#define ASCII            .ascii

#define MACRO_DECL       .macro
#define MACRO
#define ENDM             .endm

#define TEMP_LABEL       1
#define TEMP_LABEL_AHEAD 1f
#define TEMP_LABEL_BACK  1b

.macro FUNC_DECL name
.global \name
.func   \name
\name:
.endm

.macro FUNC_END name
.endfunc
.endm

;# /*
;# http://gcc.gnu.org/ml/gcc/2006-11/msg00081.html
;# _alloca_probe <=> _chkstk <=> _alloca (in libgcc)
;#  */

#define __alloca_probe   __alloca

#define END              .end

;# end of GNU toolchain specific block
ENDIF

IFDEF SYSTEM_LOG_DEBUG
  EXTERN    _WriteToLog          : PROC
  EXTERN    _syslogbuf           : DWORD
ENDIF

EXTERN    __alloca_probe         : PROC

EXTERN    __imp__GlobalFree@4    : PROC

IFDEF _UNICODE
EXTERN    __imp__wsprintfW       : PROC
ELSE
EXTERN    __imp__wsprintfA       : PROC
ENDIF

EXTERN    _GlobalCopy            : PROC

EXTERN    _LastStackPlace        : DWORD
EXTERN    _LastStackReal         : DWORD
EXTERN    _LastProc              : DWORD
EXTERN    _CallbackIndex         : DWORD

EXTERN    _retexpr               : DWORD
EXTERN    _retaddr               : PTR

EXTERN    _GetNewStackSize       : PROC
EXTERN    _GetGenStackOption     : PROC
EXTERN    _GetCDeclOption        : PROC
EXTERN    _GetProcOffset         : PROC
EXTERN    _GetCloneOffset        : PROC
EXTERN    _GetProcNameOffset     : PROC
EXTERN    _GetArgsSizeOffset     : PROC
EXTERN    _GetParamCount         : PROC
EXTERN    _GetParamsOffset       : PROC
EXTERN    _GetSizeOfProcParam    : PROC
EXTERN    _GetSizeOffsetParam    : PROC
EXTERN    _GetValueOffsetParam   : PROC
EXTERN    _Get_valueOffsetParam  : PROC
EXTERN    _SetCloneOption        : PROC
EXTERN    _SetCallProcResultValues@12  : PROC
EXTERN    _SetProcResultCallback : PROC

SECTION_DATA

  ;# Low double word of result
  _z1 DATA_SUFFIX DWORD 0

  ;# High double word of result
  _z2 DATA_SUFFIX DWORD 0

IFDEF SYSTEM_LOG_DEBUG

  IFDEF _UNICODE
    LogStack DATA_SUFFIX
      BYTE '%', 0, 's', 0, ' ', 0, ' ', 0, 'E', 0, 'S', 0, 'P', 0, ' ', 0
      BYTE ' ', 0, '0', 0, 'x', 0, '%', 0, '0', 0, '8', 0, 'X', 0, ' ', 0
      BYTE 'S', 0, 't', 0, 'a', 0, 'c', 0, 'k', 0, ' ', 0, '=', 0, ' ', 0
      BYTE 'x', 0, '%', 0, '0', 0, '8', 0, 'X', 0, ' ', 0, ' ', 0, 'R', 0
      BYTE 'a', 0, 'l', 0, ' ', 0, '=', 0, ' ', 0, '0', 0, 'x', 0, '%', 0
      BYTE '8', 0, 'X', 0
      WORD 0

    LogCall DATA_SUFFIX WORD 9,9
      BYTE 'C', 0, 'a', 0, 'l', 0, 'l', 0, ':', 0
      WORD 10,0

    LogBeforeCall DATA_SUFFIX WORD 9,9,9
      BYTE 'B', 0, 'e', 0, 'f', 0, 'o', 0, 'r', 0, 'e', 0, ' ', 0, 'c', 0
      BYTE 'l', 0, 'l', 0, ' ', 0, ' ', 0, ' ', 0, ' ', 0, ' ', 0, ' ', 0
      BYTE ' ', 0
      WORD 0

    LogNearCall DATA_SUFFIX WORD 10,9,9,9
      BYTE 'N', 0, 'e', 0, 'a', 0, 'r', 0, ' ', 0, 'c', 0, 'a', 0, 'l', 0
      BYTE ' ', 0, ' ', 0, ' ', 0, ' ', 0, ' ', 0, ' ', 0, ' ', 0, ' ', 0
      BYTE ' ', 0
      WORD 0

    LogBackFrom DATA_SUFFIX WORD 9
      BYTE 'B', 0, 'a', 0, 'c', 0, 'k', 0, ' ', 0, 'f', 0, 'r', 0, 'o', 0
      BYTE ' ', 0
      WORD 0

    LogAfterCall DATA_SUFFIX WORD 10,9,9,9
      BYTE 'A', 0, 'f', 0, 't', 0, 'e', 0, 'r', 0, ' ', 0, 'c', 0, 'a', 0
      BYTE 'l', 0, ' ', 0, ' ', 0, ' ', 0, ' ', 0, ' ', 0, ' ', 0, ' ', 0
      BYTE ' ', 0
      WORD 0

    LogReturnAfter DATA_SUFFIX WORD 10,9,9,9
      BYTE 'R', 0, 'e', 0, 't', 0, 'u', 0, 'r', 0, 'n', 0, ' ', 0, ' ', 0
      BYTE ' ', 0, ' ', 0, ' ', 0, ' ', 0, ' ', 0, ' ', 0, ' ', 0, ' ', 0
      BYTE ' ', 0, ' ', 0, ' ', 0, '0', 0, 'x', 0, '%', 0, '0', 0, '8', 0
      BYTE ' ', 0, ' ', 0, ' ', 0, ' ', 0, '0', 0, 'x', 0, '%', 0, '0', 0
      BYTE 'X', 0
      WORD 0

    LogCalled DATA_SUFFIX
      BYTE 'C', 0, 'a', 0, 'l', 0, 'l', 0, 'e', 0, 'd', 0, ' ', 0, 'c', 0
      BYTE 'l', 0, 'l', 0, 'b', 0, 'a', 0, 'c', 0, 'k', 0, ' ', 0, 'f', 0
      BYTE 'o', 0, 'm', 0, ' ', 0
      WORD 0

    LogShortAfter DATA_SUFFIX WORD 10,9,9,9
      BYTE 'S', 0, 'h', 0, 'o', 0, 'r', 0, 't', 0, '-', 0, 'A', 0, 'f', 0
      BYTE 'e', 0, 'r', 0, ' ', 0, 'c', 0, 'a', 0, 'l', 0, 'l', 0, ' ', 0
      BYTE ' ', 0
      WORD 0

    LogReturn DATA_SUFFIX WORD 9,9
      BYTE 'R', 0, 'e', 0, 't', 0, 'u', 0, 'r', 0, 'n', 0, ' ', 0, 'f', 0
      BYTE 'o', 0, 'm', 0, ' ', 0, 'c', 0, 'a', 0, 'l', 0, 'l', 0, 'b', 0
      BYTE 'c', 0, 'k', 0, ':', 0
      WORD 10,0

    LogBefore DATA_SUFFIX WORD 9,9,9
      BYTE 'B', 0, 'e', 0, 'f', 0, 'o', 0, 'r', 0, 'e', 0, ' ', 0, 'c', 0
      BYTE 'l', 0, 'l', 0, '-', 0, 'b', 0, 'a', 0, 'c', 0, 'k', 0, ' ', 0
      BYTE ' ', 0
      WORD 0

    LogShortBefore DATA_SUFFIX WORD 10,9,9,9
      BYTE 'S', 0, 'h', 0, '-', 0, 'B', 0, 'e', 0, 'f', 0, 'o', 0, 'r', 0
      BYTE ' ', 0, 'c', 0, 'a', 0, 'l', 0, 'l', 0, '-', 0, 'b', 0, 'a', 0
      BYTE 'k', 0
      WORD 0
    LogLF DATA_SUFFIX WORD 10,0
  ELSE
    LogStack DATA_SUFFIX ASCII "%s  ESP = 0x%08X  Stack = 0x%08X  Real = 0x%08X"
                    BYTE 0

    LogCall DATA_SUFFIX  BYTE 9,9
                    ASCII "Call:"
                    BYTE 10,0

    LogBeforeCall DATA_SUFFIX BYTE 9,9,9
                    ASCII "Before call        "
                    BYTE 0

    LogNearCall DATA_SUFFIX BYTE 10,9,9,9
                    ASCII "Near call          "
                    BYTE 0

    LogBackFrom DATA_SUFFIX BYTE 9
                    ASCII "Back from "
                    BYTE 0

    LogAfterCall DATA_SUFFIX BYTE 10,9,9,9
                    ASCII "After call         "
                    BYTE 0

    LogReturnAfter DATA_SUFFIX BYTE 10,9,9,9
                    ASCII "Return               0x%08X    0x%08X"
                    BYTE 0

    LogCalled DATA_SUFFIX ASCII "Called callback from "
                    BYTE 0

    LogShortAfter DATA_SUFFIX BYTE 10,9,9,9
                    ASCII "Short-After call   "
                    BYTE 0

    LogReturn DATA_SUFFIX BYTE 9,9
                    ASCII "Return from callback:"
                    BYTE 10,0

    LogBefore DATA_SUFFIX BYTE 9,9,9
                    ASCII "Before call-back   "
                    BYTE 0

    LogShortBefore DATA_SUFFIX BYTE 10,9,9,9
                    ASCII "Sh-Before call-back"
                    BYTE 0

    LogLF DATA_SUFFIX BYTE 10,0
  ENDIF
ENDIF

SECTION_CODE

IFDEF SYSTEM_LOG_DEBUG

;# Sets edi to address of the end of the syslog buffer (terminating zero)
MACRO_DECL SYSTEM_LOG_INIT MACRO
  mov         edi,offset _syslogbuf
TEMP_LABEL:
  ;# End loop if terminating zero of buffer was found otherwise move
  ;# to next character and check again
  cmp         byte ptr [edi],0
  je          TEMP_LABEL_AHEAD
  inc         edi
  jmp         TEMP_LABEL_BACK
TEMP_LABEL:
ENDM

;# Appends log information and advances edi accordingly.
;#
;# edi must point to the address of the log buffer where
;# the log information should be appended.
;#
;# eax returns number of bytes appended to log buffer
MACRO_DECL SYSTEM_LOG_ADD MACRO arg1,arg2
  ;# Format string
IFDEF __GNU__
  push        \arg1 \arg2
ELSE
  push        arg1 arg2
ENDIF
  ;# Log buffer
  push        edi
IFDEF _UNICODE
  call        dword ptr [__imp__wsprintfW]
ELSE
  call        dword ptr [__imp__wsprintfA]
ENDIF
  ;# If wsprintf succeeds then advance edi by number of bytes
  ;# written to buffer
  cmp         eax,0
  jl          TEMP_LABEL_AHEAD
  add         edi,eax
TEMP_LABEL:
  add         esp,8
ENDM

;# Writes stackpointer and additional information to log
;# and advances edi accordingly (terminating zero of buffer).
;#
;# edi must point to the address of the log buffer where
;# the log information should be appended.
;#
;# eax returns number of bytes appended to log buffer
MACRO_DECL SYSTEM_EVENT MACRO arg1,arg2
  ;# Save current stack pointer in eax
  mov         eax,esp
  ;# Stackpointer information
  push        dword ptr [_LastStackReal]
  push        dword ptr [_LastStackPlace]
  push        eax
  ;# Event information
IFDEF __GNU__
  push        \arg1 \arg2
ELSE
  push        arg1 arg2
ENDIF
  SYSTEM_LOG_ADD offset LogStack
  add         esp,16
ENDM

;# Flush log information and reset log buffer.
;#
;# edi must point to the address of the log buffer where
;# the log information should be appended.
;#
;# eax returns number of bytes appended to log buffer
MACRO_DECL SYSTEM_LOG_POST MACRO
  ;# Append line feed to log information
  SYSTEM_LOG_ADD offset LogLF
  ;# Flush log information
  push        offset _syslogbuf
  call        _WriteToLog
  add         esp,4
  ;# Reset log buffer
  mov         byte ptr [_syslogbuf],0
ENDM

ENDIF

FUNC_DECL _CallProc
  ;# Save stack
  push        ebp
  mov         ebp,esp
  ;# Stack space for local variables
  ;# ebp-4  = Size of ProcParameter structure
  sub         esp,4
  ;# Save all usable registers to free our hands
  push        ebx
  push        edi
  push        esi

  IFDEF SYSTEM_LOG_DEBUG
    SYSTEM_LOG_INIT
    SYSTEM_LOG_ADD offset LogCall
    SYSTEM_EVENT offset LogBeforeCall
  ENDIF

  ;# CallbackIndex != 0
  cmp         dword ptr [_CallbackIndex],0
  je          stack_expand_done

  ;# proc->Options without POPT_GENSTACK
  push        dword ptr [ebp+8]
  call        _GetGenStackOption
  cmp         eax,0
  pop         eax
  jne         stack_expand_done

  ;# Save previous stack location
  push        ebp
  mov         dword ptr [_LastStackReal],esp
  cmp         dword ptr [_LastStackPlace],0
  jne         stack_adjust
  ;# Create new stack
  call        _GetNewStackSize
  call        __alloca_probe
  mov         dword ptr [_LastStackPlace],esp
  jmp         stack_expand_done
stack_adjust:
  ;# Move stack pointer
  mov         esp,dword ptr [_LastStackPlace]

stack_expand_done:
  ;# Push arguments to stack
  ;#
  ;# Get number of parameters
  push        dword ptr [ebp+8]
  call        _GetParamCount
  add         esp,4

  ;# Skip if there are no parameters
  cmp         eax,0
  jle         params_loop_done

  ;# Save number of parameters on stack
  push        eax

  ;# Get offset for element Params of SystemProc structure
  call        _GetParamsOffset
  add         eax,dword ptr [ebp+8]
  push        eax

  call        _GetSizeOfProcParam
  mov         dword ptr [ebp-4],eax

  ;# Calculate offset for the last Parameter
  pop         ebx
  pop         ecx
  mul         ecx
  add         ebx,eax

  ;# Save offset of last parameter on stack
  push        ebx
  ;# Save number of parameters on stack
  push        ecx

  ;# Size offset of parameter
  call        _GetSizeOffsetParam
  push        eax

  ;# Value offset of parameter
  call        _GetValueOffsetParam
  push        eax

  ;# _value offset of parameter
  call        _Get_valueOffsetParam
  push        eax

  ;# ebx = _value offset
  pop         ebx
  ;# edx = Value offset
  pop         edx
  ;# esi = Size offset
  pop         esi
  ;# ecx = n-th parameter
  pop         ecx
  ;# eax = offset of current worked on parameter
  pop         eax

params_loop:
  ;# Check Size of param
  cmp         dword ptr [eax+esi],2
  jne         params_default

  ;# Long type 'L'/'l' (64-bit)
  push        dword ptr [eax+ebx]

params_default:
  ;# Default for all types
  push        dword ptr [eax+edx]


  ;# Continue with next parameter
  sub         eax,dword ptr[ebp-4]
  loop        params_loop

params_loop_done:
  ;# Save proc
  ;# proc->Clone
  call        _GetCloneOffset
  mov         ecx,dword ptr [ebp+8]
  add         eax,ecx

  ;# proc->Clone = LastProc
  mov         edx,dword ptr [_LastProc]
  mov         dword ptr [eax],edx

  ;# LastProc = proc
  mov         dword ptr [_LastProc],ecx

  IFDEF SYSTEM_LOG_DEBUG
    SYSTEM_EVENT offset LogNearCall
    SYSTEM_LOG_POST
  ENDIF

  ;# Get address of procedure
  call        _GetProcOffset
  mov         ecx,dword ptr [ebp+8]
  mov         ecx,dword ptr [eax+ecx]

  ;# /*
  ;# workaround for bug #1535007
  ;# http://sf.net/tracker/index.php?func=detail&aid=1535007&group_id=22049&atid=373085
  ;#
  ;# If a function returns short and doesn't clear eax in the process,
  ;# it will only set 2 bytes of eax, and the other 2 bytes remain
  ;# "random". In this case, they'll be part of the proc pointer.
  ;#
  ;# To avoid this, eax is cleared before the function is called. This
  ;# makes sure the value eax will contain is only what the function
  ;# actually sets.
  ;#  */
  xor         eax,eax

  ;# Call
  call        ecx

  ;# Save return
  mov         dword ptr [_z1],eax
  mov         dword ptr [_z2],edx

  IFDEF SYSTEM_LOG_DEBUG
    SYSTEM_LOG_INIT
    SYSTEM_LOG_ADD offset LogBackFrom
    ;# LastProc->ProcName
    call        _GetProcNameOffset
    mov         ecx,dword ptr [_LastProc]
    add         eax,ecx
    SYSTEM_LOG_ADD eax
    SYSTEM_EVENT offset LogShortAfter
  ENDIF

  cmp         dword ptr [_CallbackIndex],0
  je          stack_restore_done
  mov         eax,dword ptr [_LastProc]
  push        eax
  call        _GetGenStackOption
  cmp         eax,0
  pop         eax
  jne         stack_restore_done

  ;# Restore real stack location
  mov         dword ptr [_LastStackPlace],esp
  mov         esp,dword ptr [_LastStackReal]
  pop         ebp

stack_restore_done:
  ;# Restore proc
  mov         edx,dword ptr [_LastProc]
  mov         dword ptr [ebp+8],edx

  ;# proc->Clone
  call        _GetCloneOffset
  add         eax,edx

  ;# LastProc = proc->Clone
  mov         eax,dword ptr [eax]
  mov         dword ptr [_LastProc],eax

  ;# In case of cdecl convention we should clear stack

  ;# if ((proc->Options & POPT_CDECL) != 0)
  push        edx
  call        _GetCDeclOption
  cmp         eax,0
  pop         edx
  je          stack_clear_done

  ;# Get number of parameters
  push        edx
  call        _GetParamCount
  add         esp,4

  ;# Skip if there are no parameters
  cmp         eax,0
  jle         stack_clear_done

  ;# Save number of parameters on stack
  push        eax

  ;# Get offset for element Params of SystemProc structure
  call        _GetParamsOffset
  add         eax,dword ptr [ebp+8]

  ;# Calculate offset for the Parameter 1
  add         eax,dword ptr [ebp-4]

  ;# Save offset for the Parameter 1 on stack
  push        eax

  ;# Size offset of parameter
  call        _GetSizeOffsetParam
  push        eax

  ;# if ((CallbackIndex > 0) && ((proc->Options & POPT_GENSTACK) == 0))
  cmp         dword ptr [_CallbackIndex],0
  jle         real_stack_cleanup
  push        dword ptr [ebp+8]
  call        _GetGenStackOption
  cmp         eax,0
  pop         eax
  jne         real_stack_cleanup

  ;# In case of temporary stack
  ;#
  ;# esi = Size offset
  pop         esi
  ;# eax = offset of current worked on parameter
  pop         eax
  ;# ecx = n-th parameter
  pop         ecx

temp_stack_loop:
  ;# LastStackPlace += 4* Size of current parameter;
  mov         edx,dword ptr [eax+esi]
  mov         ebx,dword ptr [_LastStackPlace]
  lea         edx,[ebx+edx*4]
  mov         dword ptr [_LastStackPlace],edx

  ;# Go to next
  add         eax,dword ptr [ebp-4]
  loop        temp_stack_loop

  ;# End of stack cleanup
  jmp         stack_clear_done

real_stack_cleanup:
  ;# In case of real stack
  ;#
  ;# esi = Size offset
  pop         esi
  ;# eax = offset of current worked on parameter
  pop         eax
  ;# ecx = Number of parameters
  pop         ecx

real_stack_loop:
  ;# Size of current parameter == 2
  cmp         dword ptr [eax+esi],2
  jne         real_stack_default
  ;# Long type
  pop         edx

real_stack_default:
  ;# Default
  pop         edx
  add         eax,dword ptr [ebp-4]
  loop        real_stack_loop

stack_clear_done:
  ;# In case of cleared call-proc-queue -> clear allocated stack place (more flexible)
  cmp         dword ptr [_LastProc],0
  jne         stack_cleanup_done
  mov         dword ptr [_LastStackPlace],0

stack_cleanup_done:

  ;# Save return
  push        dword ptr [_z2]
  push        dword ptr [_z1]
  push        dword ptr [ebp+8]
  call        _SetCallProcResultValues@12

  IFDEF SYSTEM_LOG_DEBUG
    SYSTEM_EVENT offset LogAfterCall

    push        dword ptr [_z2]
    push        dword ptr [_z1]
    SYSTEM_LOG_ADD offset LogReturnAfter
    add         esp,8

    SYSTEM_LOG_POST
  ENDIF

  ;# Return
  mov         eax,dword ptr [ebp+8]
  ;# Restore registers
  pop         esi
  pop         edi
  pop         ebx
  ;# Restore stack pointer
  mov         esp,ebp
  pop         ebp
  ret
FUNC_END _CallProc



FUNC_DECL _RealCallBack
  ;# Save stack
  push        ebp
  mov         ebp,esp

  ;# Stack space for local variables
  ;# ebp-16 = Size of ProcParameter structure
  ;# ebp-12 = ArgsSize
  ;# ebp-8  = Arguments pointer
  ;# ebp-4  = proc
  sub         esp,16

  ;# Save all usable registers to free our hands
  push        ebx
  push        edi
  push        esi

  ;# Arguments pointer
  ;# 1-st arg (4 bytes), return (4 bytes) => add 8 bytes
  mov         dword ptr [ebp-8],ebp
  add         dword ptr [ebp-8],8

  ;# Our callback proc is in eax (set by the thunk created in CreateCallback)
  mov         dword ptr [ebp-4],eax

  IFDEF SYSTEM_LOG_DEBUG
    SYSTEM_LOG_INIT
    SYSTEM_LOG_ADD offset LogCalled
    ;# LastProc->ProcName
    call        _GetProcNameOffset
    mov         ecx,dword ptr [_LastProc]
    add         eax,ecx
    SYSTEM_LOG_ADD eax
    SYSTEM_EVENT offset LogShortAfter
    SYSTEM_LOG_POST
  ENDIF

  call        _GetCloneOffset
  mov         edx,eax
  mov         ecx,dword ptr [ebp-4]

  ;# 1. Find last unused clone
  jmp         clone_load
clone_next:
  mov         ecx,dword ptr [eax]
  mov         dword ptr [ebp-4],ecx
clone_load:
  lea         eax,[ecx+edx]
  cmp         dword ptr [eax],0
  jne         clone_next

  ;# 2. Create new clone
  push        edx
  push        ecx
  call        _GlobalCopy
  pop         ecx
  pop         edx
  ;# proc->Clone = Result of GlobalCopy
  mov         ecx,dword ptr [ebp-4]
  mov         dword ptr [ecx+edx],eax
  ;# proc = proc->Clone
  mov         dword ptr [ebp-4],eax

  ;# 3. Set clone option
  push        eax
  call        _SetCloneOption
  pop         eax

  ;# proc->ArgsSize = 0
  mov         dword ptr [ebp-12],0

  ;# Read Arguments

  ;# Get number of parameters
  push        dword ptr [ebp-4]
  call        _GetParamCount
  add         esp,4

  ;# Skip if there are no parameters
  cmp         eax,0
  jle         cb_params_loop_done

  ;# Save number of parameters on stack
  push        eax

  ;# Get size of ProcParameter structure
  call        _GetSizeOfProcParam
  mov         dword ptr [ebp-16],eax

  ;# Get offset for element Params of SystemProc structure
  call        _GetParamsOffset
  add         eax,dword ptr [ebp-4]

  ;# Calculate offset for the Parameter 1
  add         eax,dword ptr [ebp-16]

  ;# Save offset for the Parameter 1 on stack
  push        eax

  ;# Size offset of parameter
  call        _GetSizeOffsetParam
  push        eax

  ;# Value offset of parameter
  call        _GetValueOffsetParam
  push        eax

  ;# _value offset of parameter
  call        _Get_valueOffsetParam
  push        eax

  ;# ebx = _value offset
  pop         ebx
  ;# edx = Value offset
  pop         edx
  ;# esi = Size offset
  pop         esi
  ;# eax = offset of current worked on parameter
  pop         eax
  ;# ecx = n-th parameter
  pop         ecx

cb_params_loop:
  push ecx

  ;# Default
  mov         ecx,dword ptr [ebp-8]
  mov         ecx,dword ptr [ecx]
  mov         dword ptr [eax+edx],ecx
  ;# (Arguments pointer)++
  add         dword ptr [ebp-8],4
  ;# ArgsSize += 4
  add         dword ptr [ebp-12],4

  ;# Size of current parameter == 2
  cmp         dword ptr [eax+esi],2
  jne         cb_params_next
  ;# Long type
  mov         ecx,dword ptr [ebp-8]
  mov         ecx,dword ptr [ecx]
  mov         dword ptr [eax+ebx],ecx
  ;# (Arguments pointer)++
  add         dword ptr [ebp-8],4
  ;# ArgsSize += 4
  add         dword ptr [ebp-12],4

cb_params_next:
  ;# Next parameter
  add         eax,dword ptr [ebp-16]

  pop         ecx
  loop        cb_params_loop

cb_params_loop_done:
  ;# proc->ArgsSize = ArgsSize
  call        _GetArgsSizeOffset
  add         eax,dword ptr [ebp-4]
  mov         ecx,dword ptr [ebp-12]
  mov         dword ptr [eax],ecx

  push        dword ptr [ebp-4]
  call        _SetProcResultCallback
  pop         eax

  ;# Return
  ;# eax = proc
  ;# Save temporary stack info
  push        ebp
  ;# Push LastStackPlace
  mov         dword ptr [_LastStackPlace],esp
  ;# Restore real stack
  mov         esp,dword ptr [_LastStackReal]
  ;# Pop LastStackReal
  pop         ebp

  ;# Fake return from System::Call

  ;# Restore registers
  pop         esi
  pop         edi
  pop         ebx
  ;# Restore stack pointer
  mov         esp,ebp
  pop         ebp
  ;# Return
  ret
FUNC_END _RealCallBack



FUNC_DECL _CallBack
  ;# Save stack
  push        ebp
  mov         ebp,esp

  ;# Save all usable registers to free our hands
  push        ebx
  push        edi
  push        esi

  IFDEF SYSTEM_LOG_DEBUG
    SYSTEM_LOG_INIT
    SYSTEM_LOG_ADD offset LogReturn
    SYSTEM_EVENT offset LogBefore
    SYSTEM_LOG_POST
  ENDIF

  ;# Get offset for element Params of SystemProc structure
  call        _GetParamsOffset
  add         eax,dword ptr [ebp+8]
  push        eax

  ;# Value offset
  call        _GetValueOffsetParam
  push        eax

  ;# _value offset
  call        _Get_valueOffsetParam
  mov         edx,eax

  ;# offset of Value element within SystemProc structure
  pop         ecx

  ;# offset of Params element within SystemProc structure
  pop         eax

  ;# Callback proc result
  push        dword ptr [eax+ecx]
  push        dword ptr [eax+edx]

  ;# Adjust return statement
  ;# if ((proc->Options & POPT_CDECL) != 0)
  push        dword ptr [ebp+8]
  call        _GetCDeclOption
  cmp         eax,0
  pop         edx
  jne         _retexpr_stdcall
  ;# retexpr[1] = proc->ArgsSize
  call        _GetArgsSizeOffset
  mov         ecx,dword ptr [ebp+8]
  mov         al,byte ptr [ecx+eax]
  mov         byte ptr [_retexpr+1],al
  jmp         set_return_addr
_retexpr_stdcall:
  ;# retexpr[1] = 0
  mov         byte ptr [_retexpr+1],0
set_return_addr:
  ;# Right return statement address
  mov         dword ptr [_retaddr],offset _retexpr

  ;# Remove unneeded callback proc
  push        dword ptr [ebp+8]
  call        dword ptr [__imp__GlobalFree@4]

  ;# Prepare return
  ;# Callback proc result
  pop         edx
  pop         eax

  ;# Restore temporary stack and return

  ;# Save real stack info
  ;# Save previous stack location
  ;# Push _LastStackReal
  push        ebp
  mov         dword ptr [_LastStackReal],esp
  ;# Move stack pointer
  mov         esp,dword ptr [_LastStackPlace]
  ;# Pop _LastStackPlace
  pop         ebp

  IFDEF SYSTEM_LOG_DEBUG
    push        eax
    push        edx
    SYSTEM_EVENT offset LogShortBefore
    SYSTEM_LOG_POST
    ;# Callback proc result
    pop         edx
    pop         eax
  ENDIF

  ;# Fake return from Callback

  ;# Restore registers
  pop         esi
  pop         edi
  pop         ebx
  ;# Restore stack pointer
  mov         esp,ebp
  pop         ebp
  ;# Return
  jmp         dword ptr [_retaddr]

FUNC_END _CallBack

END

