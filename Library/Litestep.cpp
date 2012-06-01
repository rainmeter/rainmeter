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

static CRITICAL_SECTION g_CsLog = {0};
static CRITICAL_SECTION g_CsLogDelay = {0};

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

	RunFile(command.c_str(), args.c_str());
}

void RunFile(const WCHAR* file, const WCHAR* args, bool asAdmin)
{
	SHELLEXECUTEINFO si = {sizeof(SHELLEXECUTEINFO)};
	si.hwnd = NULL;
	si.lpVerb = asAdmin ? L"runas" : L"open";
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

void LogInternal(int nLevel, ULONGLONG elapsed, LPCTSTR pszMessage)
{
	// Add timestamp
	WCHAR buffer[128];
	size_t len = _snwprintf_s(buffer, _TRUNCATE, L"%02llu:%02llu:%02llu.%03llu", elapsed / (1000 * 60 * 60), (elapsed / (1000 * 60)) % 60, (elapsed / 1000) % 60, elapsed % 1000);

	Rainmeter->AddAboutLogInfo(nLevel, buffer, pszMessage);

#ifndef _DEBUG
	if (!Rainmeter->GetLogging()) return;
#endif

	std::wstring message;
	switch (nLevel)
	{
	case LOG_ERROR:
		message = L"ERRO";
		break;

	case LOG_WARNING:
		message = L"WARN";
		break;

	case LOG_NOTICE:
		message = L"NOTE";
		break;

	case LOG_DEBUG:
		message = L"DBUG";
		break;
	}
	
	message += L" (";
	message.append(buffer, len);
	message += L") ";
	message += pszMessage;
	message += L'\n';
	
#ifdef _DEBUG
	_RPT0(_CRT_WARN, ConvertToAscii(message.c_str()).c_str());
	if (!Rainmeter->GetLogging()) return;
#endif

	const WCHAR* logFile = Rainmeter->GetLogFile().c_str();
	if (_waccess(logFile, 0) == -1)
	{
		// Disable logging if the file was deleted manually
		Rainmeter->StopLogging();
	}
	else
	{
		FILE* file = _wfopen(logFile, L"a+, ccs=UTF-8");
		if (file)
		{
			fputws(message.c_str(), file);
			fclose(file);
		}
	}
}

void Log(int nLevel, const WCHAR* message)
{
	struct DelayedLogInfo
	{
		int level;
		ULONGLONG elapsed;
		std::wstring message;
	};
	static std::list<DelayedLogInfo> c_LogDelay;

	static ULONGLONG startTime = CSystem::GetTickCount64();
	ULONGLONG elapsed = CSystem::GetTickCount64() - startTime;

	if (TryEnterCriticalSection(&g_CsLog))
	{
		// Log the queued messages first
		EnterCriticalSection(&g_CsLogDelay);

		while (!c_LogDelay.empty())
		{
			DelayedLogInfo& logInfo = c_LogDelay.front();
			LogInternal(logInfo.level, logInfo.elapsed, logInfo.message.c_str());

			c_LogDelay.erase(c_LogDelay.begin());
		}

		LeaveCriticalSection(&g_CsLogDelay);

		// Log the message
		LogInternal(nLevel, elapsed, message);

		LeaveCriticalSection(&g_CsLog);
	}
	else
	{
		// Queue the message
		EnterCriticalSection(&g_CsLogDelay);

		DelayedLogInfo logInfo = {nLevel, elapsed, message};
		c_LogDelay.push_back(logInfo);

		LeaveCriticalSection(&g_CsLogDelay);
	}
}

void LogWithArgs(int nLevel, const WCHAR* format, ...)
{
	WCHAR* buffer = new WCHAR[1024];
	va_list args;
	va_start(args, format);

	_invalid_parameter_handler oldHandler = _set_invalid_parameter_handler(RmNullCRTInvalidParameterHandler);
	_CrtSetReportMode(_CRT_ASSERT, 0);

	errno = 0;
	_vsnwprintf_s(buffer, 1024, _TRUNCATE, format, args);
	if (errno != 0)
	{
		nLevel = LOG_ERROR;
		_snwprintf_s(buffer, 1024, _TRUNCATE, L"Internal error: %s", format);
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
