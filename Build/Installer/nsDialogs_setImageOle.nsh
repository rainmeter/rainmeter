/*

nsDialogs_setImageOLE.nsh v1.2
Header file for setting OLE-supported images as the bitmap datasource
for Bitmap controls

Usage:

  ${NSD_SetImageOLE} control_HWND image_path ipicture_interface
  Loads an OLE-supported bitmap (BMP, JPG, GIF, ICO) from image_path and
  displays it on control_HWND created by ${NSD_CreateBitmap}.

  ${NSD_FreeImageOLE} ipicture_interface
  Releases the ipicture_interface from the call to ${NSD_SetImageOLE}.

  ${NSD_SetStretchedImageOLE} control_HWND image_path image_handle
  Loads an OLE-supported bitmap (BMP, JPG, GIF, ICO) from image_path and
  resizes it to fit on control_HWND created by ${NSD_CreateBitmap}. The
  IPicture interface is stored in image_handle and should be freed using
  ${NSD_FreeImage} (not ${NSD_FreeImageOLE}!).


Changelog:

v1.0 - 8th Feb 2009 - ZeBoxx
* First version

v1.1 - 9th Feb 2009 - ZeBoxx
* Bug fix

v1.2 - 5th Dec 2014 - Afrow UK & Anders
* Added NSD_SetStretchedImageOLE based on Anders' code from
  http://stackoverflow.com/questions/13396371/how-can-i-resize-a-nsd-setimageole-image

*/

!ifndef NSDIALOGS_setImageOLE_INCLUDED
	!define NSDIALOGS_setImageOLE_INCLUDED
	!verbose push
	!verbose 3

	!ifndef IID_IPicture
		!define IID_IPicture {7BF80980-BF32-101A-8BBB-00AA00300CAB}
	!endif
	!ifndef SRCCOPY
		!define SRCCOPY 0xCC0020
	!endif

	!include LogicLib.nsh
	!include WinMessages.nsh

	!macro __NSD_SetImageOLE CONTROL IMAGE IPICTURE
		Push $0							; $0
		Push $R0						; $R0 $0
		Push $1							; $1 R$0 $0
		Push $2							; $2 $1 $R0 $0

		StrCpy $R0 ${CONTROL} # in case ${CONTROL} is $0

		System::Call 'oleaut32::OleLoadPicturePath(w "${IMAGE}", i 0, i 0, i 0, g "${IID_IPicture}", *i .r0)i.r1'
		; $0 = pointer to image interface
		; $1 = result code
		${If} $1 == 0
			System::Call "$0->3(*i.r2)i.r1" ; IPicture->get_Handle (VTable entry 3)
			${If} $1 == 0
				SendMessage $R0 ${STM_SETIMAGE} ${IMAGE_BITMAP} $2
				StrCpy ${IPICTURE} "OK"
			${Else}
				StrCpy ${IPICTURE} -1
			${EndIf}
		${Else}
			StrCpy ${IPICTURE} -1
		${EndIf}

										; $2 $1 $R0 $0
		Pop $2							; $1 $R0 $0
		Pop $1							; $R0 $0
		Pop $R0							; $0
		Exch $0							; IPicture
		Pop ${IPicture}
	!macroend
	!define NSD_SetImageOLE `!insertmacro __NSD_SetImageOLE`

	!macro __NSD_FreeImageOLE IMAGE
		${If} ${IMAGE} <> 0
			System::Call "${IMAGE}->2()" ; IPicture->release()
		${EndIf}
	!macroend
	!define NSD_FreeImageOLE `!insertmacro __NSD_FreeImageOLE`

	!macro __NSD_SetStretchedImageOLE CONTROL IMAGE HANDLE
		Push $0
		Push $1
		Push $2
		Push $3
		Push $4
		Push $5
		Push $6
		Push $7
		Push $8
		Push $9

		StrCpy $9 ${CONTROL} # in case ${CONTROL} is $0

		System::Call '*(i,i,i,i)i.r2'
		${If} $2 <> 0
			StrCpy $7 0
			StrCpy $8 0
			System::Call 'user32::GetClientRect(ir9,ir2)'
			System::Call '*$2(i,i,i.r7,i.r8)'
			System::Free $2
			${If} $7 > 0
			${AndIf} $8 > 0
				System::Call 'oleaut32::OleLoadPicturePath(w"${IMAGE}",i0,i0,i0,g"${IID_IPicture}",*i.r2)i.r1'
				${If} $1 = 0
					System::Call 'user32::GetDC(i0)i.s'
					System::Call 'gdi32::CreateCompatibleDC(iss)i.r1'
					System::Call 'gdi32::CreateCompatibleBitmap(iss,ir7,ir8)i.r0'
					System::Call 'user32::ReleaseDC(i0,is)'
					System::Call '$2->3(*i.r3)i.r4' ; IPicture->get_Handle
					${If} $4 = 0
						System::Call 'gdi32::SetStretchBltMode(ir1,i4)'
						System::Call '*(&i40,&i1024)i.r4' ; BITMAP / BITMAPINFO
						System::Call 'gdi32::GetObject(ir3,i24,ir4)'
						System::Call 'gdi32::SelectObject(ir1,ir0)i.s'
						System::Call '*$4(i40,i.r5,i.r6,i0,i,i.s)' ; Grab size and bits-ptr AND init as BITMAPINFOHEADER
						System::Call 'gdi32::GetDIBits(ir1,ir3,i0,i0,i0,ir4,i0)' ; init BITMAPINFOHEADER
						System::Call 'gdi32::GetDIBits(ir1,ir3,i0,i0,i0,ir4,i0)' ; init BITMAPINFO
						System::Call 'gdi32::StretchDIBits(ir1,i0,i0,ir7,ir8,i0,i0,ir5,ir6,is,ir4,i0,i${SRCCOPY})'
						System::Call 'gdi32::SelectObject(ir1,is)'
						System::Free $4
						SendMessage $9 ${STM_SETIMAGE} ${IMAGE_BITMAP} $0
					${EndIf}
					System::Call 'gdi32::DeleteDC(ir1)'
					System::Call '$2->2()' ; IPicture->release()
				${EndIf}
			${EndIf}
		${EndIf}

		Pop $9
		Pop $8
		Pop $7
		Pop $6
		Pop $5
		Pop $4
		Pop $3
		Pop $2
		Pop $1
		Exch $0
		Pop ${HANDLE}
	!macroend
	!define NSD_SetStretchedImageOLE `!insertmacro __NSD_SetStretchedImageOLE`

	!verbose pop
!endif
