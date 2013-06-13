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
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#include "StdAfx.h"
#include "Litestep.h"
#include "Rainmeter.h"
#include "DialogAbout.h"
#include "System.h"

UINT GetUniqueID()
{
	static UINT id = 0;
	return id++;
}

WCHAR* GetString(UINT id)
{
	LPWSTR pData;
	int len = LoadString(GetRainmeter().GetResourceInstance(), id, (LPWSTR)&pData, 0);
	return len ? pData : L"";
}

std::wstring GetFormattedString(UINT id, ...)
{
	LPWSTR pBuffer = nullptr;
	va_list args = nullptr;
	va_start(args, id);

	DWORD len = FormatMessage(FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_ALLOCATE_BUFFER,
		GetString(id),
		0,
		0,
		(LPWSTR)&pBuffer,
		0,
		&args);

	va_end(args);

	std::wstring tmpSz(len ? pBuffer : L"", len);
	if (pBuffer) LocalFree(pBuffer);
	return tmpSz;
}

HICON GetIcon(UINT id, bool large)
{
	typedef HRESULT (WINAPI * FPLOADICONMETRIC)(HINSTANCE hinst, PCWSTR pszName, int lims, HICON* phico);

	HINSTANCE hExe = GetModuleHandle(nullptr);
	HINSTANCE hComctl = GetModuleHandle(L"Comctl32");
	if (hComctl)
	{
		// Try LoadIconMetric for better quality with high DPI
		FPLOADICONMETRIC loadIconMetric = (FPLOADICONMETRIC)GetProcAddress(hComctl, "LoadIconMetric");
		if (loadIconMetric)
		{
			HICON icon;
			HRESULT hr = loadIconMetric(hExe, MAKEINTRESOURCE(id), large ? LIM_LARGE : LIM_SMALL, &icon);
			if (SUCCEEDED(hr))
			{
				return icon;
			}
		}
	}

	return (HICON)LoadImage(
		hExe,
		MAKEINTRESOURCE(id),
		IMAGE_ICON,
		GetSystemMetrics(large ? SM_CXICON : SM_CXSMICON),
		GetSystemMetrics(large ? SM_CYICON : SM_CYSMICON),
		LR_SHARED);
}

void RmNullCRTInvalidParameterHandler(const wchar_t* expression, const wchar_t* function, const wchar_t* file, unsigned int line, uintptr_t pReserved)
{
	// Do nothing.
}
