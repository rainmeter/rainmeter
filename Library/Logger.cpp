/*
  Copyright (C) 2013 Birunthan Mohanathas

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
#include "Logger.h"
#include "DialogAbout.h"
#include "Litestep.h"
#include "Rainmeter.h"
#include "System.h"
#include "resource.h"

extern CRainmeter* Rainmeter;

namespace {

const size_t MAX_LOG_ENTIRES = 20;

}  // namespace

CLogger::CLogger() :
	m_LogToFile(false)
{
	CSystem::InitializeCriticalSection(&m_CsLog);
	CSystem::InitializeCriticalSection(&m_CsLogDelay);
}

CLogger::~CLogger()
{
	DeleteCriticalSection(&m_CsLog);
	DeleteCriticalSection(&m_CsLogDelay);
}

CLogger& CLogger::GetInstance()
{
	static CLogger s_CLogger;
	return s_CLogger;
}

void CLogger::StartLogFile()
{
	const WCHAR* filePath = m_LogFilePath.c_str();
	if (_waccess(filePath, 0) == -1)
	{
		// Create empty log file.
		HANDLE file = CreateFile(filePath, GENERIC_WRITE, 0, NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);
		if (file != INVALID_HANDLE_VALUE)
		{
			CloseHandle(file);
		}
		else
		{
			const std::wstring text = GetFormattedString(ID_STR_LOGFILECREATEFAIL, filePath);
			Rainmeter->ShowMessage(NULL, text.c_str(), MB_OK | MB_ICONERROR);
			SetLogToFile(false);
			return;
		}
	}

	SetLogToFile(true);
}

void CLogger::StopLogFile()
{
	SetLogToFile(false);
}

void CLogger::DeleteLogFile()
{
	const WCHAR* filePath = m_LogFilePath.c_str();
	if (_waccess(filePath, 0) != -1)
	{
		const std::wstring text = GetFormattedString(ID_STR_LOGFILEDELETE, filePath);
		const int res = Rainmeter->ShowMessage(NULL, text.c_str(), MB_YESNO | MB_ICONQUESTION);
		if (res == IDYES)
		{
			SetLogToFile(false);
			CSystem::RemoveFile(m_LogFilePath);
		}
	}
}

void CLogger::SetLogToFile(bool logToFile)
{
	m_LogToFile = logToFile;
	WritePrivateProfileString(
		L"Rainmeter", L"Logging", logToFile ? L"1" : L"0", Rainmeter->GetIniFile().c_str());
}

void CLogger::LogInternal(Level level, ULONGLONG timestamp, const WCHAR* msg)
{
	WCHAR timestampSz[128];
	size_t len = _snwprintf_s(
		timestampSz,
		_TRUNCATE,
		L"%02llu:%02llu:%02llu.%03llu",
		timestamp / (1000 * 60 * 60),
		(timestamp / (1000 * 60)) % 60,
		(timestamp / 1000) % 60,
		timestamp % 1000);

	// Store up to MAX_LOG_ENTIRES entries.
	Entry entry = {level, std::wstring(timestampSz, len), msg};
	m_Entries.push_back(entry);
	if (m_Entries.size() > MAX_LOG_ENTIRES)
	{
		m_Entries.pop_front();
	}

	CDialogAbout::AddLogItem(level, timestampSz, msg);
	WriteToLogFile(entry);
}

void CLogger::WriteToLogFile(Entry& entry)
{
#ifndef _DEBUG
	if (!m_LogToFile) return;
#endif

	const WCHAR* levelSz =
		(entry.level == Level::Error) ? L"ERRO" :
		(entry.level == Level::Warning) ? L"WARN" :
		(entry.level == Level::Notice) ? L"NOTE" :
		L"DBUG";
	
	std::wstring message = levelSz;
	message += L" (";
	message.append(entry.timestamp);
	message += L") ";
	message += entry.message;
	message += L'\n';
	
#ifdef _DEBUG
	_RPT0(_CRT_WARN, StringUtil::Narrow(message).c_str());
	if (!m_LogToFile) return;
#endif

	const WCHAR* filePath = m_LogFilePath.c_str();
	if (_waccess(filePath, 0) == -1)
	{
		// The file has been deleted manually.
		StopLogFile();
	}
	else
	{
		FILE* file = _wfopen(filePath, L"a+, ccs=UTF-8");
		if (file)
		{
			fputws(message.c_str(), file);
			fclose(file);
		}
	}
}

void CLogger::Log(Level level, const WCHAR* msg)
{
	struct DelayedEntry
	{
		Level level;
		ULONGLONG elapsed;
		std::wstring message;
	};
	static std::list<DelayedEntry> s_DelayedEntries;

	static ULONGLONG s_StartTime = CSystem::GetTickCount64();
	ULONGLONG elapsed = CSystem::GetTickCount64() - s_StartTime;

	if (TryEnterCriticalSection(&m_CsLog))
	{
		// Log queued messages first.
		EnterCriticalSection(&m_CsLogDelay);

		while (!s_DelayedEntries.empty())
		{
			DelayedEntry& entry = s_DelayedEntries.front();
			LogInternal(entry.level, entry.elapsed, entry.message.c_str());

			s_DelayedEntries.erase(s_DelayedEntries.begin());
		}

		LeaveCriticalSection(&m_CsLogDelay);

		// Log the actual message.
		LogInternal(level, elapsed, msg);

		LeaveCriticalSection(&m_CsLog);
	}
	else
	{
		// Queue message.
		EnterCriticalSection(&m_CsLogDelay);

		DelayedEntry entry = {level, elapsed, msg};
		s_DelayedEntries.push_back(entry);

		LeaveCriticalSection(&m_CsLogDelay);
	}
}

void CLogger::LogF(Level level, const WCHAR* format, ...)
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
		level = Level::Error;
		_snwprintf_s(buffer, 1024, _TRUNCATE, L"Internal error: %s", format);
	}

	_set_invalid_parameter_handler(oldHandler);

	Log(level, buffer);
	va_end(args);

	delete [] buffer;
}
