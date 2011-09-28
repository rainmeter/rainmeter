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
#include "Rainmeter.h"
#include "DialogAbout.h"
#include "System.h"

extern CRainmeter* Rainmeter;

static CRITICAL_SECTION g_CsLog = {0};
static CRITICAL_SECTION g_CsLogDelay = {0};

static int logFound = 0;

void ResetLoggingFlag()
{
	logFound = 0;
}

void InitalizeLitestep()
{
	InitializeCriticalSection(&g_CsLog);
	InitializeCriticalSection(&g_CsLogDelay);
}

void FinalizeLitestep()
{
	DeleteCriticalSection(&g_CsLog);
	DeleteCriticalSection(&g_CsLogDelay);
}

HRGN BitmapToRegion(HBITMAP hbm, COLORREF clrTransp, COLORREF clrTolerance, int xoffset, int yoffset)
{
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
								if (*p < clrLoB || *p > clrHiB)
									break;
								p++;
								if (*p < clrLoG || *p > clrHiG)
									break;
								p++;
								if (*p < clrLoR || *p > clrHiR)
									break;

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
							if (x != x0)
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

void RunCommand(HWND Owner, LPCTSTR szCommand, int nShowCmd, bool asAdmin)
{
	// The stub implementation (some of this code is taken from lsapi.cpp)
	if (szCommand == NULL || *szCommand == 0) return;

	std::wstring args;
	std::wstring command = szCommand;

	size_t notwhite = command.find_first_not_of(L" \t\r\n");
	command.erase(0, notwhite);
	if (command.empty()) return;

	size_t quotePos = command.find(L'"');
	if (quotePos == 0)
	{
		size_t quotePos2 = command.find(L'"', quotePos + 1);
		if (quotePos2 != std::wstring::npos)
		{
			args.assign(command, quotePos2 + 1, command.length() - (quotePos2 + 1));
			command.assign(command, quotePos + 1, quotePos2 - quotePos - 1);
		}
		else
		{
			command.erase(0, 1);
		}
	}
	else
	{
		size_t spacePos = command.find(L' ');
		if (spacePos != std::wstring::npos)
		{
			args.assign(command, spacePos + 1, command.length() - (spacePos + 1));
			command.erase(spacePos);
		}
	}

	if (!command.empty())
	{
		LPCWSTR szVerb = asAdmin ? L"runas" : L"open";
		DWORD type = GetFileAttributes(command.c_str());
		if (type & FILE_ATTRIBUTE_DIRECTORY && type != 0xFFFFFFFF)
		{
			ShellExecute(Owner, szVerb, command.c_str(), NULL, NULL, nShowCmd ? nShowCmd : SW_SHOWNORMAL);
			return;
		}

		std::wstring dir = CRainmeter::ExtractPath(command);

		SHELLEXECUTEINFO si = {sizeof(SHELLEXECUTEINFO)};
		si.hwnd = Owner;
		si.lpVerb = szVerb;
		si.lpFile = command.c_str();
		si.lpParameters = args.c_str();
		si.lpDirectory = dir.c_str();
		si.nShow = nShowCmd ? nShowCmd : SW_SHOWNORMAL;
		si.fMask = SEE_MASK_DOENVSUBST | SEE_MASK_FLAG_NO_UI;
		ShellExecuteEx(&si);
	}
}

//void TransparentBltLS(HDC hdcDst, int nXDest, int nYDest, int nWidth, int nHeight, HDC hdcSrc, int nXSrc, int nYSrc, COLORREF colorTransparent)
//{
//	// Use the lsapi.dll version of the method if possible
//	if (fpTransparentBltLS)
//	{
//		fpTransparentBltLS(hdcDst, nXDest, nYDest, nWidth, nHeight, hdcSrc, nXSrc, nYSrc, colorTransparent);
//	}
//	else
//	{
//		HDC hdcMem, hdcMask, hdcDstCpy;
//		HBITMAP hbmMask, hbmMem, hbmDstCpy;
//		HBITMAP hbmOldMem, hbmOldMask, hbmOldDstCpy;
//
//		// create a destination compatble dc containing
//		// a copy of the destination dc
//		hdcDstCpy	= CreateCompatibleDC(hdcDst);
//		hbmDstCpy	= CreateCompatibleBitmap(hdcDst, nWidth, nHeight);
//		hbmOldDstCpy = (HBITMAP)SelectObject(hdcDstCpy, hbmDstCpy);
//
//		BitBlt(hdcDstCpy, 0, 0, nWidth, nHeight, hdcDst, nXDest, nYDest, SRCCOPY);
//
//		// create a destination compatble dc containing
//		// a copy of the source dc
//		hdcMem	= CreateCompatibleDC(hdcDst);
//		hbmMem	= CreateCompatibleBitmap(hdcDst, nWidth, nHeight);
//		hbmOldMem = (HBITMAP)SelectObject(hdcMem, hbmMem);
//
//		BitBlt(hdcMem, 0, 0, nWidth, nHeight, hdcSrc, nXSrc, nYSrc, SRCCOPY);
//
//		// the transparent color should be selected as
//		// bkcolor into the memory dc
//		SetBkColor(hdcMem, colorTransparent);
//
//		// Create monochrome bitmap for the mask
//		hdcMask	= CreateCompatibleDC(hdcDst);
//		hbmMask = CreateBitmap(nWidth, nHeight, 1, 1, NULL);
//		hbmOldMask = (HBITMAP)SelectObject(hdcMask, hbmMask);
//
//		// Create the mask from the memory dc
//		BitBlt(hdcMask, 0, 0, nWidth, nHeight, hdcMem, 0, 0, SRCCOPY);
//
//		// Set the background in hdcMem to black. Using SRCPAINT with black
//		// and any other color results in the other color, thus making
//		// black the transparent color
//		SetBkColor(hdcMem, RGB(0, 0, 0));
//		SetTextColor(hdcMem, RGB(255, 255, 255));
//
//		BitBlt(hdcMem, 0, 0, nWidth, nHeight, hdcMask, 0, 0, SRCAND);
//
//		// Set the foreground to black. See comment above.
//		SetBkColor(hdcDst, RGB(255, 255, 255));
//		SetTextColor(hdcDst, RGB(0, 0, 0));
//
//		BitBlt(hdcDstCpy, 0, 0, nWidth, nHeight, hdcMask, 0, 0, SRCAND);
//
//		// Combine the foreground with the background
//		BitBlt(hdcDstCpy, 0, 0, nWidth, nHeight, hdcMem, 0, 0, SRCPAINT);
//
//		// now we have created the image we want to blt
//		// in the destination copy dc
//		BitBlt(hdcDst, nXDest, nYDest, nWidth, nHeight, hdcDstCpy, 0, 0, SRCCOPY);
//
//		SelectObject(hdcMask, hbmOldMask);
//		DeleteObject(hbmMask);
//		DeleteDC(hdcMask);
//
//		SelectObject(hdcMem, hbmOldMem);
//		DeleteObject(hbmMem);
//		DeleteDC(hdcMem);
//
//		SelectObject(hdcDstCpy, hbmOldDstCpy);
//		DeleteObject(hbmDstCpy);
//		DeleteDC(hdcDstCpy);
//	}
//}

std::string ConvertToAscii(LPCTSTR str)
{
	std::string szAscii;

	if (str && *str)
	{
		int strLen = (int)wcslen(str);
		int bufLen = WideCharToMultiByte(CP_ACP, 0, str, strLen, NULL, 0, NULL, NULL);
		if (bufLen > 0)
		{
			szAscii.resize(bufLen);
			WideCharToMultiByte(CP_ACP, 0, str, strLen, &szAscii[0], bufLen, NULL, NULL);
		}
	}
	return szAscii;
}

std::wstring ConvertToWide(LPCSTR str)
{
	std::wstring szWide;

	if (str && *str)
	{
		int strLen = (int)strlen(str);
		int bufLen = MultiByteToWideChar(CP_ACP, 0, str, strLen, NULL, 0);
		if (bufLen > 0)
		{
			szWide.resize(bufLen);
			MultiByteToWideChar(CP_ACP, 0, str, strLen, &szWide[0], bufLen);
		}
	}
	return szWide;
}

std::string ConvertToUTF8(LPCWSTR str)
{
	std::string szAscii;

	if (str && *str)
	{
		int strLen = (int)wcslen(str);
		int bufLen = WideCharToMultiByte(CP_UTF8, 0, str, strLen, NULL, 0, NULL, NULL);
		if (bufLen > 0)
		{
			szAscii.resize(bufLen);
			WideCharToMultiByte(CP_UTF8, 0, str, strLen, &szAscii[0], bufLen, NULL, NULL);
		}
	}
	return szAscii;
}

std::wstring ConvertUTF8ToWide(LPCSTR str)
{
	std::wstring szWide;

	if (str && *str)
	{
		int strLen = (int)strlen(str);
		int bufLen = MultiByteToWideChar(CP_UTF8, 0, str, strLen, NULL, 0);
		if (bufLen > 0)
		{
			szWide.resize(bufLen);
			MultiByteToWideChar(CP_UTF8, 0, str, strLen, &szWide[0], bufLen);
		}
	}
	return szWide;
}

BOOL LogInternal(int nLevel, LPCTSTR pszModule, ULONGLONG elapsed, LPCTSTR pszMessage)
{
	// Add timestamp
	WCHAR buffer[128];
	_snwprintf_s(buffer, _TRUNCATE, L"%02llu:%02llu:%02llu.%03llu", elapsed / (1000 * 60 * 60), (elapsed / (1000 * 60)) % 60, (elapsed / 1000) % 60, elapsed % 1000);

	Rainmeter->AddAboutLogInfo(nLevel, buffer, pszMessage);

	std::wstring message = L"(";
	message += buffer;
	message += L") ";
	message += pszMessage;

#ifdef _DEBUG
	_RPT0(_CRT_WARN, ConvertToAscii(message.c_str()).c_str());
	_RPT0(_CRT_WARN, "\n");
#endif

	// The stub implementation
	if (Rainmeter->GetLogging())
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
					message.insert(0, L": ");

					switch (nLevel)
					{
					case LOG_ERROR:
						message.insert(0, L"ERROR");
						break;

					case LOG_WARNING:
						message.insert(0, L"WARNING");
						break;

					case LOG_NOTICE:
						message.insert(0, L"NOTICE");
						break;

					case LOG_DEBUG:
						message.insert(0, L"DEBUG");
						break;
					}

					message += L"\n";
					fputws(message.c_str(), logFile);
					fclose(logFile);
				}
			}
		}
	}

	return TRUE;
}

BOOL LSLog(int nLevel, LPCTSTR pszModule, LPCTSTR pszMessage)
{
	// Ignore LOG_DEBUG messages from plugins unless in debug mode
	if (nLevel != LOG_DEBUG || Rainmeter->GetDebug())
	{
		Log(nLevel, pszMessage, pszModule);
	}

	return TRUE;
}

void Log(int nLevel, const WCHAR* message, const WCHAR* module)
{
	struct DELAYED_LOG_INFO
	{
		int level;
		std::wstring module;
		ULONGLONG elapsed;
		std::wstring message;
	};
	static std::list<DELAYED_LOG_INFO> c_LogDelay;

	static ULONGLONG startTime = CSystem::GetTickCount64();
	ULONGLONG elapsed = CSystem::GetTickCount64() - startTime;

	if (TryEnterCriticalSection(&g_CsLog))
	{
		// Log the queued messages first
		EnterCriticalSection(&g_CsLogDelay);

		while (!c_LogDelay.empty())
		{
			DELAYED_LOG_INFO& logInfo = c_LogDelay.front();
			LogInternal(logInfo.level, logInfo.module.c_str(), logInfo.elapsed, logInfo.message.c_str());

			c_LogDelay.erase(c_LogDelay.begin());
		}

		LeaveCriticalSection(&g_CsLogDelay);

		// Log the message
		LogInternal(nLevel, module, elapsed, message);

		LeaveCriticalSection(&g_CsLog);
	}
	else
	{
		// Queue the message
		EnterCriticalSection(&g_CsLogDelay);

		DELAYED_LOG_INFO logInfo = {nLevel, module, elapsed, message};
		c_LogDelay.push_back(logInfo);

		LeaveCriticalSection(&g_CsLogDelay);
	}
}

void LogWithArgs(int nLevel, const WCHAR* format, ...)
{
	WCHAR* buffer = new WCHAR[4096];
	va_list args;
	va_start( args, format );

	_invalid_parameter_handler oldHandler = _set_invalid_parameter_handler(RmNullCRTInvalidParameterHandler);
	_CrtSetReportMode(_CRT_ASSERT, 0);

	errno = 0;
	_vsnwprintf_s( buffer, 4096, _TRUNCATE, format, args );
	if (errno != 0)
	{
		nLevel = LOG_ERROR;
		_snwprintf_s(buffer, 4096, _TRUNCATE, L"LogWithArgs internal error: %s", format);
	}

	_set_invalid_parameter_handler(oldHandler);

	Log(nLevel, buffer);
	va_end(args);

	delete [] buffer;
}

void LogError(CError& error)
{
	Log(LOG_ERROR, error.GetString().c_str());
	CDialogAbout::ShowAboutLog();
}

WCHAR* GetString(UINT id)
{
	LPWSTR pData;
	int len = LoadString(Rainmeter->GetResourceInstance(), id, (LPWSTR)&pData, 0);
	return len ? pData : L"";
}

std::wstring GetFormattedString(UINT id, ...)
{
	LPWSTR pBuffer = NULL;
	va_list args = NULL;
	va_start(args, id);

	DWORD len = FormatMessage(FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_ALLOCATE_BUFFER,
				  GetString(id),
				  0,
				  0,
				  (LPWSTR)&pBuffer,
				  0,
				  &args);

	va_end(args);

	std::wstring tmpSz(len ? pBuffer : L"");
	if (pBuffer) LocalFree(pBuffer);
	return tmpSz;
}

void RmNullCRTInvalidParameterHandler(const wchar_t* expression, const wchar_t* function,  const wchar_t* file, unsigned int line, uintptr_t pReserved)
{
	// Do nothing.
}
