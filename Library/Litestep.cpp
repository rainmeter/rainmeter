/*
  Copyright (C) 2002 Kimmo Pekkola + few lsapi developers

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/

#include "StdAfx.h"
#include "Litestep.h"
#include "Error.h"
#include "Rainmeter.h"
#include "System.h"

extern CRainmeter* Rainmeter;

typedef BOOL (*FPADDBANGCOMMAND)(LPCSTR command, BangCommand f);
FPADDBANGCOMMAND fpAddBangCommand = NULL;

typedef HRGN (*FPBITMAPTOREGION)(HBITMAP hBmp, COLORREF cTransparentColor, COLORREF cTolerance, int xoffset, int yoffset);
FPBITMAPTOREGION fpBitmapToRegion = NULL;

typedef HWND (*FPGETLITESTEPWND)(void);
FPGETLITESTEPWND fpGetLitestepWnd = NULL;

typedef BOOL (*FPGETRCSTRING)(LPCSTR lpKeyName, LPSTR value, LPCSTR defStr, int maxLen);
FPGETRCSTRING fpGetRCString = NULL;

typedef int (*FPGETRCINT)(LPCSTR lpKeyName, int nDefault);
FPGETRCINT fpGetRCInt = NULL;

typedef HINSTANCE (*FPLSEXECUTE)(HWND Owner, LPCSTR szCommand, int nShowCmd);
FPLSEXECUTE fpLSExecute = NULL;

typedef BOOL (*FPREMOVEBANGCOMMAND)(LPCSTR command);
FPREMOVEBANGCOMMAND fpRemoveBangCommand = NULL;

typedef void (*FPTRANSPARENTBLTLS)(HDC dc, int nXDest, int nYDest, int nWidth, int nHeight, HDC tempDC, int nXSrc, int nYSrc, COLORREF colorTransparent);
FPTRANSPARENTBLTLS fpTransparentBltLS = NULL;

typedef void (*FPVAREXPANSION)(LPSTR buffer, LPCSTR value);
FPVAREXPANSION fpVarExpansion = NULL;

typedef BOOL (WINAPI *FPLSLOG)(int nLevel, LPCSTR pszModule, LPCSTR pszMessage);
FPLSLOG fpLSLog = NULL;

static int logFound = 0;

void ResetLoggingFlag()
{
	logFound = 0;
}

void InitalizeLitestep()
{
	// Use lsapi's methods instead of the stubs
	HINSTANCE h = CSystem::RmLoadLibrary(L"lsapi.dll");
	if (h != NULL)
	{
		fpAddBangCommand = (FPADDBANGCOMMAND)GetProcAddress(h, "AddBangCommand");
		fpBitmapToRegion = (FPBITMAPTOREGION)GetProcAddress(h, "BitmapToRegion");
		fpGetLitestepWnd = (FPGETLITESTEPWND)GetProcAddress(h, "GetLitestepWnd");
		fpGetRCString = (FPGETRCSTRING)GetProcAddress(h, "GetRCString");
		fpGetRCInt = (FPGETRCINT)GetProcAddress(h, "GetRCInt");
		fpLSExecute = (FPLSEXECUTE)GetProcAddress(h, "LSExecute");
		fpRemoveBangCommand = (FPREMOVEBANGCOMMAND)GetProcAddress(h, "RemoveBangCommand");
		fpTransparentBltLS = (FPTRANSPARENTBLTLS)GetProcAddress(h, "TransparentBltLS");
		fpVarExpansion = (FPVAREXPANSION)GetProcAddress(h, "VarExpansion");
		fpLSLog = (FPLSLOG)GetProcAddress(h, "_LSLog@12");
	}
}

BOOL AddBangCommand(LPCSTR command, BangCommand f)
{
	// Use the lsapi.dll version of the method if possible
	if (fpAddBangCommand) return fpAddBangCommand(command, f);

	// The stub implementation
	return true;
}

HWND GetLitestepWnd(void)
{
	// Use the lsapi.dll version of the method if possible
	if (fpGetLitestepWnd) return fpGetLitestepWnd();

	// The stub implementation
	return NULL;
}

BOOL RemoveBangCommand(LPCSTR command)
{
	// Use the lsapi.dll version of the method if possible
	if (fpRemoveBangCommand) return fpRemoveBangCommand(command);

	// The stub implementation
	return true;
}

BOOL GetRCString(LPCSTR lpKeyName, LPSTR value, LPCSTR defStr, int maxLen)
{
	// Use the lsapi.dll version of the method if possible
	if (fpGetRCString) return fpGetRCString(lpKeyName, value, defStr, maxLen);

	// The stub implementation
	return false;
}

int GetRCInt(LPCSTR lpKeyName, int nDefault)
{
	// Use the lsapi.dll version of the method if possible
	if (fpGetRCInt) return fpGetRCInt(lpKeyName, nDefault);

	// The stub implementation
	return nDefault;
}

void VarExpansion(LPSTR buffer, LPCSTR value)
{
	// Use the lsapi.dll version of the method if possible
	if (fpVarExpansion) 
	{
		fpVarExpansion(buffer, value);
	}
	else
	{
		// The stub implementation
		if (buffer != value)
		{
			strcpy(buffer, value);
		}
	}
}

HRGN BitmapToRegion(HBITMAP hbm, COLORREF clrTransp, COLORREF clrTolerance, int xoffset, int yoffset)
{
	// Use the lsapi.dll version of the method if possible
	if (fpBitmapToRegion) return fpBitmapToRegion(hbm, clrTransp, clrTolerance, xoffset, yoffset);

	// start with a completely transparent rgn
	// this is more correct as no bmp, should render a transparent background
	HRGN hRgn = CreateRectRgn(0, 0, 0, 0);

	if (hbm)
	{
		// create a dc for the 32 bit dib
		HDC hdcMem = CreateCompatibleDC(NULL);
		if (hdcMem)
		{
			VOID *pbits32;
			HBITMAP hbm32;
			BITMAP bm;
			// get the size
			GetObject(hbm, sizeof(BITMAP), &bm);

			BITMAPINFOHEADER bmpInfo32;
			bmpInfo32.biSize	= sizeof(BITMAPINFOHEADER);
			bmpInfo32.biWidth	= bm.bmWidth;
			bmpInfo32.biHeight	= bm.bmHeight;
			bmpInfo32.biPlanes	= 1;
			bmpInfo32.biBitCount	= 32;
			bmpInfo32.biCompression	= BI_RGB;
			bmpInfo32.biSizeImage	= 0;
			bmpInfo32.biXPelsPerMeter	= 0;
			bmpInfo32.biYPelsPerMeter	= 0;
			bmpInfo32.biClrUsed	= 0;
			bmpInfo32.biClrImportant	= 0;

			hbm32 = CreateDIBSection(hdcMem, (BITMAPINFO *) & bmpInfo32, DIB_RGB_COLORS, &pbits32, NULL, 0);
			if (hbm32)
			{
				HBITMAP hbmOld32 = (HBITMAP)SelectObject(hdcMem, hbm32);

				// Create a DC just to copy the bitmap into the memory D
				HDC hdcTmp = CreateCompatibleDC(hdcMem);
				if (hdcTmp)
				{
					// Get how many bytes per row we have for the bitmap bits (rounded up to 32 bits
					int y = 0;
					BITMAP bm32;
					GetObject(hbm32, sizeof(bm32), &bm32);
					while (bm32.bmWidthBytes % 4)
						bm32.bmWidthBytes++;

					// get the limits for the colors
					BYTE clrHiR = ( 0xff - GetRValue( clrTolerance ) > GetRValue( clrTransp ) ) ? GetRValue( clrTransp ) + GetRValue( clrTolerance ) : 0xff;
					BYTE clrHiG = ( 0xff - GetGValue( clrTolerance ) > GetGValue( clrTransp ) ) ? GetGValue( clrTransp ) + GetGValue( clrTolerance ) : 0xff;
					BYTE clrHiB = ( 0xff - GetBValue( clrTolerance ) > GetBValue( clrTransp ) ) ? GetBValue( clrTransp ) + GetBValue( clrTolerance ) : 0xff;
					BYTE clrLoR = ( GetRValue( clrTolerance ) < GetRValue( clrTransp ) ) ? GetRValue( clrTransp ) - GetRValue( clrTolerance ) : 0x00;
					BYTE clrLoG = ( GetGValue( clrTolerance ) < GetGValue( clrTransp ) ) ? GetGValue( clrTransp ) - GetGValue( clrTolerance ) : 0x00;
					BYTE clrLoB = ( GetBValue( clrTolerance ) < GetBValue( clrTransp ) ) ? GetBValue( clrTransp ) - GetBValue( clrTolerance ) : 0x00;

					// Copy the bitmap into the memory D
					HBITMAP hbmOld = (HBITMAP)SelectObject(hdcTmp, hbm);

					BitBlt(hdcMem, 0, 0, bm.bmWidth, bm.bmHeight, hdcTmp, 0, 0, SRCCOPY);

					// Scan each bitmap row from bottom to top (the bitmap is inverted vertically
					BYTE *p;
					BYTE *p32 = (BYTE *)bm32.bmBits + (bm32.bmHeight - 1) * bm32.bmWidthBytes;
					while (y < bm.bmHeight)
					{
						int x = 0;
						while ( x < bm.bmWidth )
						{
							int x0 = 0;
							// loop through all transparent pixels...
							while ( x < bm.bmWidth )
							{
								p = p32 + 4 * x;

								// if the pixel is non-transparent
								{
									bool isOpaque = *p < clrLoB || *p > clrHiB;
									p++;
									isOpaque |= *p < clrLoG || *p > clrHiG;
									p++;
									isOpaque |= *p < clrLoR || *p > clrHiR;
									if (isOpaque)
										break;
								}

								x++;
							}
							// set first non transparent pixel
							x0 = x;
							// loop through all non transparent pixels
							while ( x < bm.bmWidth )
							{
								p = p32 + 4 * x;
								// if the pixel is transparent, then break
								if (*p >= clrLoB && *p <= clrHiB)
								{
									p++;
									if (*p >= clrLoG && *p <= clrHiG)
									{
										p++;
										if (*p >= clrLoR && *p <= clrHiR)
											break;
									}
								}
								x++;
							}
							// if found one or more non-transparent pixels in a row, add them to the rgn...
							if ((x - x0) > 0)
							{
								HRGN hTempRgn = CreateRectRgn(x0 + xoffset, y + yoffset, x + xoffset, y + 1 + yoffset);
								CombineRgn(hRgn, hRgn, hTempRgn, RGN_OR);
								DeleteObject(hTempRgn);
							}
							x++;
						}
						y++;
						p32 -= bm32.bmWidthBytes;
					}
					// Clean up
					SelectObject(hdcTmp, hbmOld);
					DeleteDC(hdcTmp);
				}
				SelectObject(hdcMem, hbmOld32);
				DeleteObject(hbm32);
			}
			DeleteDC(hdcMem);
		}
	}
	return hRgn;
}

HINSTANCE LSExecuteAsAdmin(HWND Owner, LPCTSTR szCommand, int nShowCmd)
{
	BOOL IsInAdminGroup = FALSE;

	SID_IDENTIFIER_AUTHORITY NtAuthority = SECURITY_NT_AUTHORITY;
	PSID AdministratorsGroup;
	// Initialize SID.
	if( !AllocateAndInitializeSid( &NtAuthority,
		2,
		SECURITY_BUILTIN_DOMAIN_RID,
		DOMAIN_ALIAS_RID_ADMINS,
		0, 0, 0, 0, 0, 0,
		&AdministratorsGroup))
	{
		// Initializing SID Failed.
		IsInAdminGroup = FALSE;
	}
	else
	{
		// Check whether the token is present in admin group.
		if( !CheckTokenMembership( NULL,
			AdministratorsGroup,
			&IsInAdminGroup ))
		{
			// Error occurred.
			IsInAdminGroup = FALSE;
		}
		// Free SID and return.
		FreeSid(AdministratorsGroup);
	}

	if (IsInAdminGroup)
	{
		return ExecuteCommand(Owner, szCommand, nShowCmd, L"open");
	}
	else
	{
		return ExecuteCommand(Owner, szCommand, nShowCmd, L"runas");
	}
}

HINSTANCE LSExecute(HWND Owner, LPCTSTR szCommand, int nShowCmd)
{
	// Use the lsapi.dll version of the method if possible
    if (fpLSExecute) 
	{
		std::string asc = ConvertToAscii(szCommand);
		return fpLSExecute(Owner, asc.c_str(), nShowCmd);
	}

	return ExecuteCommand(Owner, szCommand, nShowCmd, L"open");
}

HINSTANCE ExecuteCommand(HWND Owner, LPCTSTR szCommand, int nShowCmd, LPCTSTR szVerb)
{
	// The stub implementation (some of this code is taken from lsapi.cpp)
	if (szCommand == NULL || wcslen(szCommand) == 0) return NULL;

	std::wstring args;
	std::wstring command = szCommand;

	size_t notwhite = command.find_first_not_of(L" \t\n");
	command.erase(0, notwhite);

	size_t quotePos = command.find(L"\"");
	if (quotePos == 0)
	{
		size_t quotePos2 = command.find(L"\"", quotePos + 1);
		if (quotePos2 != std::wstring::npos)
		{
			args = command.substr(quotePos2 + 1);
			command = command.substr(quotePos + 1, quotePos2 - quotePos - 1);
		}
		else
		{
			command.erase(0, quotePos + 1);
		}
	}
	else
	{
		size_t spacePos = command.find(L" ");
		if (spacePos != std::wstring::npos)
		{
			args = command.substr(spacePos + 1);
			command = command.substr(0, spacePos);
		}
	}

	DWORD type = GetFileAttributes(command.c_str());
	if (type & FILE_ATTRIBUTE_DIRECTORY && type != 0xFFFFFFFF)
	{
		HINSTANCE instance = ShellExecute(Owner, szVerb, command.c_str(), NULL, NULL, nShowCmd ? nShowCmd : SW_SHOWNORMAL);
		return instance;
	}

	std::wstring dir = CRainmeter::ExtractPath(command);

	SHELLEXECUTEINFO si;
	memset(&si, 0, sizeof(si));
	si.cbSize = sizeof(SHELLEXECUTEINFO);
	si.hwnd = Owner;
	si.lpVerb = szVerb;
	si.lpFile = command.c_str();
	si.lpParameters = args.c_str();
	si.lpDirectory = dir.c_str();
	si.nShow = nShowCmd ? nShowCmd : SW_SHOWNORMAL;
	si.fMask = SEE_MASK_DOENVSUBST | SEE_MASK_FLAG_NO_UI;
	ShellExecuteEx(&si);
	return si.hInstApp;
}

void TransparentBltLS(HDC hdcDst, int nXDest, int nYDest, int nWidth, int nHeight, HDC hdcSrc, int nXSrc, int nYSrc, COLORREF colorTransparent)
{
	// Use the lsapi.dll version of the method if possible
	if (fpTransparentBltLS) 
	{
		fpTransparentBltLS(hdcDst, nXDest, nYDest, nWidth, nHeight, hdcSrc, nXSrc, nYSrc, colorTransparent);
	}
	else
	{
		HDC hdcMem, hdcMask, hdcDstCpy;
		HBITMAP hbmMask, hbmMem, hbmDstCpy;
		HBITMAP hbmOldMem, hbmOldMask, hbmOldDstCpy;

		// create a destination compatble dc containing
		// a copy of the destination dc
		hdcDstCpy	= CreateCompatibleDC(hdcDst);
		hbmDstCpy	= CreateCompatibleBitmap(hdcDst, nWidth, nHeight);
		hbmOldDstCpy = (HBITMAP)SelectObject(hdcDstCpy, hbmDstCpy);

		BitBlt(hdcDstCpy, 0, 0, nWidth, nHeight, hdcDst, nXDest, nYDest, SRCCOPY);

		// create a destination compatble dc containing
		// a copy of the source dc
		hdcMem	= CreateCompatibleDC(hdcDst);
		hbmMem	= CreateCompatibleBitmap(hdcDst, nWidth, nHeight);
		hbmOldMem = (HBITMAP)SelectObject(hdcMem, hbmMem);

		BitBlt(hdcMem, 0, 0, nWidth, nHeight, hdcSrc, nXSrc, nYSrc, SRCCOPY);

		// the transparent color should be selected as
		// bkcolor into the memory dc
		SetBkColor(hdcMem, colorTransparent);

		// Create monochrome bitmap for the mask
		hdcMask	= CreateCompatibleDC(hdcDst);
		hbmMask = CreateBitmap(nWidth, nHeight, 1, 1, NULL);
		hbmOldMask = (HBITMAP)SelectObject(hdcMask, hbmMask);

		// Create the mask from the memory dc
		BitBlt(hdcMask, 0, 0, nWidth, nHeight, hdcMem, 0, 0, SRCCOPY);

		// Set the background in hdcMem to black. Using SRCPAINT with black
		// and any other color results in the other color, thus making
		// black the transparent color
		SetBkColor(hdcMem, RGB(0, 0, 0));
		SetTextColor(hdcMem, RGB(255, 255, 255));

		BitBlt(hdcMem, 0, 0, nWidth, nHeight, hdcMask, 0, 0, SRCAND);

		// Set the foreground to black. See comment above.
		SetBkColor(hdcDst, RGB(255, 255, 255));
		SetTextColor(hdcDst, RGB(0, 0, 0));

		BitBlt(hdcDstCpy, 0, 0, nWidth, nHeight, hdcMask, 0, 0, SRCAND);

		// Combine the foreground with the background
		BitBlt(hdcDstCpy, 0, 0, nWidth, nHeight, hdcMem, 0, 0, SRCPAINT);

		// now we have created the image we want to blt
		// in the destination copy dc
		BitBlt(hdcDst, nXDest, nYDest, nWidth, nHeight, hdcDstCpy, 0, 0, SRCCOPY);

		SelectObject(hdcMask, hbmOldMask);
		DeleteObject(hbmMask);
		DeleteDC(hdcMask);

		SelectObject(hdcMem, hbmOldMem);
		DeleteObject(hbmMem);
		DeleteDC(hdcMem);

		SelectObject(hdcDstCpy, hbmOldDstCpy);
		DeleteObject(hbmDstCpy);
		DeleteDC(hdcDstCpy);
	}
}

std::string ConvertToAscii(LPCTSTR str)
{
	std::string szAscii;

	if (str && *str)
	{
		int strLen = (int)wcslen(str) + 1;
		int bufLen = WideCharToMultiByte(CP_ACP, 0, str, strLen, NULL, 0, NULL, NULL);
		if (bufLen > 0)
		{
			char* tmpSz = new char[bufLen];
			tmpSz[0] = 0;
			WideCharToMultiByte(CP_ACP, 0, str, strLen, tmpSz, bufLen, NULL, NULL);
			szAscii = tmpSz;
			delete [] tmpSz;
		}
	}
	return szAscii;
}

std::wstring ConvertToWide(LPCSTR str)
{
	std::wstring szWide;

	if (str && *str)
	{
		int strLen = (int)strlen(str) + 1;
		int bufLen = MultiByteToWideChar(CP_ACP, 0, str, strLen, NULL, 0);
		if (bufLen > 0)
		{
			WCHAR* wideSz = new WCHAR[bufLen];
			wideSz[0] = 0;
			MultiByteToWideChar(CP_ACP, 0, str, strLen, wideSz, bufLen);
			szWide = wideSz;
			delete [] wideSz;
		}
	}
	return szWide;
}

BOOL LSLog(int nLevel, LPCTSTR pszModule, LPCTSTR pszMessage)
{
	CRainmeter::LOG_INFO logInfo;
	logInfo.message = pszMessage;

	// Add timestamp
	static DWORD startTime = 0;
	
	DWORD time = GetTickCount();
	if (startTime == 0) 
	{
		startTime = time;
	}
	WCHAR buffer[128];
	swprintf(buffer, L"(%02i:%02i:%02i.%03i) ", (time - startTime) / (1000 * 60* 60), ((time - startTime) / (1000 * 60)) % 60, ((time - startTime) / 1000) % 60, (time - startTime) % 1000);

	std::wstring message(buffer);
	logInfo.timestamp = message; 
	message += pszMessage;

#ifdef _DEBUG
	_RPT0(_CRT_WARN, ConvertToAscii(message.c_str()).c_str());
	_RPT0(_CRT_WARN, "\n");
#endif

	switch(nLevel)
	{
	case 1:
		logInfo.type = L"ERROR";
		break;
	case 2:
		logInfo.type = L"WARNING";
		break;
	case 3:
		logInfo.type = L"NOTICE";
		break;
	case 4:
		logInfo.type = L"DEBUG";
		break;
	}

	if (Rainmeter)
	{
		Rainmeter->AddAboutLogInfo(logInfo);
	}

	// Use the lsapi.dll version of the method if possible
	if (fpLSLog) 
	{
		std::string asc = ConvertToAscii(message.c_str());
		std::string mod = ConvertToAscii(pszModule);
		return fpLSLog(nLevel, mod.c_str(), asc.c_str());
	}

	// The stub implementation
	if (Rainmeter && Rainmeter->GetLogging())
	{
		std::wstring logfile = Rainmeter->GetLogFile();
		if (logFound == 0)
		{
			// Check if the file exists
			if (_waccess(logfile.c_str(), 0) != -1)
			{
				logFound = 1;

				// Clear the file
				FILE* logFile = _wfopen(logfile.c_str(), L"w");
				fclose(logFile);
			}
			else
			{
				logFound = 2;  // not found
			}
		}

		if (logFound == 1)
		{
			if (_waccess(logfile.c_str(), 0) == -1)
			{
				// Disable logging if the file was deleted manually
				Rainmeter->StopLogging();
			}
			else
			{
				FILE* logFile = _wfopen(logfile.c_str(), L"a+, ccs=UTF-8");
				if (logFile)
				{
					fputws(logInfo.type.c_str(), logFile);
					fputws(L": ", logFile);
					fputws(message.c_str(), logFile);
					fputws(L"\n", logFile);
					fclose(logFile);
				}
			}
		}
	}
	return TRUE;
}

void DebugLog(const WCHAR* format, ... )
{
	WCHAR buffer[4096];
    va_list args;
    va_start( args, format );
    _vsnwprintf( buffer, 4096, format, args );
	LSLog(LOG_DEBUG, L"Rainmeter", buffer);
	va_end(args);
}
