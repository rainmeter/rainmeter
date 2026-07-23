/* Copyright (C) 2002 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "StdAfx.h"
#include "Util.h"
#include "Rainmeter.h"
#include "DialogDebug.h"
#include "System.h"

UINT GetUniqueID()
{
	static UINT id = 0;
	return id++;
}

const WCHAR* GetString(UINT id)
{
	switch (id)
	{
		case IDS_Ellipsis: return L"...";
		case IDS_AddResources: return L"@Resources";
		case IDS_0Percent: return L"0%";
		case IDS_10Percent: return L"10%";
		case IDS_20Percent: return L"20%";
		case IDS_30Percent: return L"30%";
		case IDS_40Percent: return L"40%";
		case IDS_50Percent: return L"50%";
		case IDS_60Percent: return L"60%";
		case IDS_70Percent: return L"70%";
		case IDS_80Percent: return L"80%";
		case IDS_90Percent: return L"90%";
		case IDS_Approx100Percent: return L"~100%";
		case IDS_100Percent: return L"100%";
		case IDS_110Percent: return L"110%";
		case IDS_120Percent: return L"120%";
		case IDS_130Percent: return L"130%";
		case IDS_140Percent: return L"140%";
		case IDS_150Percent: return L"150%";
	}

	return GetRainmeter().GetLanguageString(id);
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
	HINSTANCE hExe = GetModuleHandle(nullptr);
	HINSTANCE hComctl = GetModuleHandle(L"Comctl32");
	if (hComctl)
	{
		// Try LoadIconMetric for better quality with high DPI
		auto loadIconMetric = (decltype(LoadIconMetric)*)GetProcAddress(hComctl, "LoadIconMetric");
		if (loadIconMetric)
		{
			HICON icon = { 0 };
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

HICON GetIconBySize(UINT id, int size)
{
	return (HICON)LoadImage(
		GetModuleHandle(nullptr),
		MAKEINTRESOURCE(id),
		IMAGE_ICON,
		size,
		size,
		LR_SHARED);
}

void RmNullCRTInvalidParameterHandler(const wchar_t* expression, const wchar_t* function, const wchar_t* file, unsigned int line, uintptr_t pReserved)
{

#ifdef _DEBUG
	if (function && file)
	{
		LogErrorF(L"Invalid parameter detected. Function: \"%s\" File: \"%s:%d\" Expression: \"%s\"", function, file, line, expression);
		return;
	}
#endif

	if (GetRainmeter().GetDebug())
	{
		LogError(L"Invalid parameter detected");
	}
}
