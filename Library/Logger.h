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

// Convenience functions.
#define RM_LOGGER_DEFINE_LOG_FUNCTION(name) \
	inline void Log ## name(const WCHAR* msg) \
	{ \
		CLogger::GetInstance().Log(CLogger::Level::name, msg); \
	} \
/*	\
	template<typename... Args> \
	inline void Log ## name ## F(const WCHAR* format, Args... args) \
	{ \
		GetInstance().LogF(CLogger::Level::name, args...); \
	}
*/

RM_LOGGER_DEFINE_LOG_FUNCTION(Error)
RM_LOGGER_DEFINE_LOG_FUNCTION(Warning)
RM_LOGGER_DEFINE_LOG_FUNCTION(Notice)
RM_LOGGER_DEFINE_LOG_FUNCTION(Debug)

// FIXME: Temporary solution until VS support variadic templates.
#define RM_LOGGER_LOGF_HELPER(name, format, ...) \
	CLogger::GetInstance().LogF(CLogger::Level::name, format, __VA_ARGS__)
#define LogErrorF(format, ...) RM_LOGGER_LOGF_HELPER(Error, format, __VA_ARGS__)
#define LogWarningF(format, ...) RM_LOGGER_LOGF_HELPER(Warning, format, __VA_ARGS__)
#define LogNoticeF(format, ...) RM_LOGGER_LOGF_HELPER(Notice, format, __VA_ARGS__)
#define LogDebugF(format, ...) RM_LOGGER_LOGF_HELPER(Debug, format, __VA_ARGS__)

#endif