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

#ifndef RM_LOGGER_H_
#define RM_LOGGER_H_

#include <Windows.h>
#include <string>
#include <list>

// Singleton class to handle and store log messages and control the log file.
class CLogger
{
public:
	enum class Level
	{
		Error   = 1,
		Warning = 2,
		Notice  = 3,
		Debug   = 4
	};

	struct Entry
	{
		Level level;
		std::wstring timestamp;
		std::wstring message;
	};

	static CLogger& GetInstance();

	void SetLogFilePath(std::wstring path) { m_LogFilePath = path; }

	void StartLogFile();
	void StopLogFile();
	void DeleteLogFile();

	bool IsLogToFile() { return m_LogToFile; }
	void SetLogToFile(bool logToFile);

	void Log(Level level, const WCHAR* msg);
	void LogF(Level level, const WCHAR* format, ...);

	// Convenience functions.
	static void Error(const WCHAR* msg) { GetInstance().Log(Level::Error, msg); }
	static void Warning(const WCHAR* msg) { GetInstance().Log(Level::Warning, msg); }
	static void Notice(const WCHAR* msg) { GetInstance().Log(Level::Notice, msg); }
	static void Debug(const WCHAR* msg) { GetInstance().Log(Level::Debug, msg); }

	// TODO: Uncomment when VS supports variadic templates.
	/*
	template<typename... Args>
	static void ErrorF(const WCHAR* format, Args... args) { GetInstance().LogF(Level::Error, args...); }

	template<typename... Args>
	static void WarningF(const WCHAR* format, Args... args) { GetInstance().LogF(Level::Warning, args...); }

	template<typename... Args>
	static void NoticeF(const WCHAR* format, Args... args) { GetInstance().LogF(Level::Notice, args...); }

	template<typename... Args>
	static void DebugF(const WCHAR* format, Args... args) { GetInstance().LogF(Level::Debug, args...); }
	*/

	const std::wstring& GetLogFilePath() { return m_LogFilePath; }

	const std::list<Entry>& GetEntries() { return m_Entries; }

private:
	void LogInternal(Level level, ULONGLONG timestamp, const WCHAR* msg);

	// Appends |entry| to the log file.
	void WriteToLogFile(Entry& entry);

	CLogger();
	~CLogger();

	bool m_LogToFile;
	std::wstring m_LogFilePath;

	std::list<Entry> m_Entries;

	CRITICAL_SECTION m_CsLog;
	CRITICAL_SECTION m_CsLogDelay;
};

// FIXME: Temporary solution until VS support variadic macros.
#define RM_LOGGER_LOGF_HELPER(name, format, ...) \
	CLogger::GetInstance().LogF(CLogger::Level::name, format, __VA_ARGS__);
#define CLogger_ErrorF(format, ...) RM_LOGGER_LOGF_HELPER(Error, format, __VA_ARGS__)
#define CLogger_WarningF(format, ...) RM_LOGGER_LOGF_HELPER(Warning, format, __VA_ARGS__)
#define CLogger_NoticeF(format, ...) RM_LOGGER_LOGF_HELPER(Notice, format, __VA_ARGS__)
#define CLogger_DebugF(format, ...) RM_LOGGER_LOGF_HELPER(Debug, format, __VA_ARGS__)

#endif