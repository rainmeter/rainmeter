/* Copyright (C) 2013 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "StdAfx.h"
#include "Logger.h"
#include "DialogAbout.h"
#include "Util.h"
#include "Rainmeter.h"
#include "Section.h"
#include "Skin.h"
#include "System.h"
#include "resource.h"
#include "Measure.h"

namespace {

const size_t MAX_LOG_ENTIRES = 20;

}  // namespace

Logger::Logger() :
	m_LogToFile(false)
{
	System::InitializeCriticalSection(&m_CsLog);
	System::InitializeCriticalSection(&m_CsLogDelay);
}

Logger::~Logger()
{
	DeleteCriticalSection(&m_CsLog);
	DeleteCriticalSection(&m_CsLogDelay);
}

Logger& Logger::GetInstance()
{
	static Logger s_Logger;
	return s_Logger;
}

void Logger::StartLogFile()
{
	const WCHAR* filePath = m_LogFilePath.c_str();
	if (_waccess(filePath, 0) == -1)
	{
		// Create empty log file.
		HANDLE file = CreateFile(filePath, GENERIC_WRITE, 0, nullptr, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, nullptr);
		if (file != INVALID_HANDLE_VALUE)
		{
			CloseHandle(file);
		}
		else
		{
			const std::wstring text = GetFormattedString(ID_STR_LOGFILECREATEFAIL, filePath);
			GetRainmeter().ShowMessage(nullptr, text.c_str(), MB_OK | MB_ICONERROR);
			SetLogToFile(false);
			return;
		}
	}

	SetLogToFile(true);
}

void Logger::StopLogFile()
{
	SetLogToFile(false);
}

void Logger::DeleteLogFile()
{
	const WCHAR* filePath = m_LogFilePath.c_str();
	if (_waccess(filePath, 0) != -1)
	{
		const std::wstring text = GetFormattedString(ID_STR_LOGFILEDELETE, filePath);
		const int res = GetRainmeter().ShowMessage(nullptr, text.c_str(), MB_YESNO | MB_ICONQUESTION);
		if (res == IDYES)
		{
			SetLogToFile(false);
			System::RemoveFile(m_LogFilePath);
		}
	}
}

void Logger::SetLogToFile(bool logToFile)
{
	m_LogToFile = logToFile;
	WritePrivateProfileString(
		L"Rainmeter", L"Logging", logToFile ? L"1" : L"0", GetRainmeter().GetIniFile().c_str());
}

void Logger::LogInternal(Level level, std::chrono::system_clock::time_point timestamp, const WCHAR* source, const WCHAR* msg)
{
	auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(timestamp.time_since_epoch());
	auto seconds = std::chrono::duration_cast<std::chrono::seconds>(milliseconds).count();
	auto time = localtime(&seconds);

	WCHAR timestampSz[128];
	size_t len = _snwprintf_s(
		timestampSz,
		_TRUNCATE,
		L"%02i:%02i:%02i.%03llu",
		time->tm_hour,
		time->tm_min,
		time->tm_sec,
		milliseconds.count() % 1000);

	// Store up to MAX_LOG_ENTIRES entries.
	Entry entry = {level, std::wstring(timestampSz, len), source, msg};
	m_Entries.push_back(entry);
	if (m_Entries.size() > MAX_LOG_ENTIRES)
	{
		m_Entries.pop_front();
	}

	DialogAbout::AddLogItem(level, timestampSz, source, msg);
	WriteToLogFile(entry);
}

void Logger::WriteToLogFile(Entry& entry)
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
	message += entry.source;
	message += L": ";
	message += entry.message;
	message += L'\n';

#ifdef _DEBUG
	_RPTW0(_CRT_WARN, message.c_str());
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

void Logger::Log(Level level, const WCHAR* source, const WCHAR* msg)
{
	struct DelayedEntry
	{
		Level level;
		std::chrono::system_clock::time_point timestamp;
		std::wstring message;
	};
	static std::list<DelayedEntry> s_DelayedEntries;

	std::chrono::system_clock::time_point timestamp = std::chrono::system_clock::now();

	if (TryEnterCriticalSection(&m_CsLog))
	{
		// Log queued messages first.
		EnterCriticalSection(&m_CsLogDelay);

		while (!s_DelayedEntries.empty())
		{
			DelayedEntry& entry = s_DelayedEntries.front();
			LogInternal(entry.level, entry.timestamp, source, entry.message.c_str());

			s_DelayedEntries.erase(s_DelayedEntries.begin());
		}

		LeaveCriticalSection(&m_CsLogDelay);

		// Log the actual message.
		LogInternal(level, timestamp, source, msg);

		LeaveCriticalSection(&m_CsLog);
	}
	else
	{
		// Queue message.
		EnterCriticalSection(&m_CsLogDelay);

		DelayedEntry entry = {level, timestamp, msg};
		s_DelayedEntries.push_back(entry);

		LeaveCriticalSection(&m_CsLogDelay);
	}
}

void Logger::LogVF(Level level, const WCHAR* source, const WCHAR* format, va_list args)
{
	WCHAR* buffer = new WCHAR[1024];

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

	Log(level, source, buffer);
	delete [] buffer;
}

std::wstring GetSectionSourceString(Section* section)
{
	std::wstring source;
	if (section)
	{
		Skin* skin = section->GetSkin();
		if (skin)
		{
			source = skin->GetSkinPath();
			source += L" - ";
		}

		source += L'[';
		source += section->GetOriginalName();
		source += L']';
	}
	return source;
}

void Logger::LogSection(Logger::Level level, Section* section, const WCHAR* message)
{
	const std::wstring source = GetSectionSourceString(section);
	GetLogger().Log(level, source.c_str(), message);
}

void Logger::LogSectionVF(Logger::Level level, Section* section, const WCHAR* format, va_list args)
{
	const std::wstring source = GetSectionSourceString(section);
	GetLogger().LogVF(level, source.c_str(), format, args);
}

void Logger::LogSkinVF(Logger::Level level, Skin* skin, const WCHAR* format, va_list args)
{
	std::wstring source;
	if (skin)
	{
		source = skin->GetSkinPath();
	}
	GetLogger().LogVF(level, source.c_str(), format, args);
}

void Logger::LogMeasureVF(Logger::Level level, Measure* measure, const WCHAR* format, va_list args)
{
	if (!measure || !measure->IsDisabled())
	{
		const std::wstring source = GetSectionSourceString(measure);
		GetLogger().LogVF(level, source.c_str(), format, args);
	}
}
