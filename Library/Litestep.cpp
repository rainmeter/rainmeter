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

extern CRainmeter* Rainmeter;

UINT GetUniqueID()
{
	static UINT id = 0;
	return id++;
}

void RunCommand(std::wstring command)
{
	std::wstring args;

	size_t notwhite = command.find_first_not_of(L" \t\r\n");
	command.erase(0, notwhite);

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
		RunFile(command.c_str(), args.c_str());
	}
}

void RunFile(const WCHAR* file, const WCHAR* args)
{
	SHELLEXECUTEINFO si = {sizeof(SHELLEXECUTEINFO)};
	si.lpVerb = L"open";
	si.lpFile = file;
	si.nShow = SW_SHOWNORMAL;

	DWORD type = GetFileAttributes(si.lpFile);
	if (type & FILE_ATTRIBUTE_DIRECTORY && type != 0xFFFFFFFF)
	{
		ShellExecute(si.hwnd, si.lpVerb, si.lpFile, NULL, NULL, si.nShow);
	}
	else
	{
		std::wstring dir = CRainmeter::ExtractPath(file);
		si.lpDirectory = dir.c_str();
		si.lpParameters = args;
		si.fMask = SEE_MASK_DOENVSUBST | SEE_MASK_FLAG_NO_UI;
		ShellExecuteEx(&si);
	}
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

	std::wstring tmpSz(len ? pBuffer : L"", len);
	if (pBuffer) LocalFree(pBuffer);
	return tmpSz;
}

HICON GetIcon(UINT id, bool large)
{
	typedef HRESULT (WINAPI * FPLOADICONMETRIC)(HINSTANCE hinst, PCWSTR pszName, int lims, HICON* phico);

	HINSTANCE hExe = GetModuleHandle(NULL);
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
