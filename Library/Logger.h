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
#include <cstdarg>
#include <string>
#include <list>

class Section;
class MeterWindow;

// Singleton class to handle and store log messages and control the log file.
class Logger
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
		std::wstring source;
		std::wstring message;
	};

	static Logger& GetInstance();

	void SetLogFilePath(std::wstring path) { m_LogFilePath = path; }

	void StartLogFile();
	void StopLogFile();
	void DeleteLogFile();

	bool IsLogToFile() { return m_LogToFile; }
	void SetLogToFile(bool logToFile);

	void Log(Level level, const WCHAR* source, const WCHAR* msg);
	void LogVF(Level level, const WCHAR* source, const WCHAR* format, va_list args);
	void LogMeterWindowVF(Logger::Level level, MeterWindow* meterWindow, const WCHAR* format, va_list args);
	void LogSectionVF(Logger::Level level, Section* section, const WCHAR* format, va_list args);

	const std::wstring& GetLogFilePath() { return m_LogFilePath; }

	const std::list<Entry>& GetEntries() { return m_Entries; }

private:
	void LogInternal(Level level, ULONGLONG timestamp, const WCHAR* source, const WCHAR* msg);

	// Appends |entry| to the log file.
	void WriteToLogFile(Entry& entry);

	Logger();
	~Logger();

	bool m_LogToFile;
	std::wstring m_LogFilePath;

	std::list<Entry> m_Entries;

	CRITICAL_SECTION m_CsLog;
	CRITICAL_SECTION m_CsLogDelay;
};

// Convenience functions.
inline Logger& GetLogger() { return Logger::GetInstance(); }

#define RM_LOGGER_DEFINE_LOG_FUNCTIONS(name) \
	inline void Log ## name(const WCHAR* msg) \
	{ \
		GetLogger().Log(Logger::Level::name, L"", msg); \
	} \
	\
	inline void Log ## name ## F(const WCHAR* format, ...) \
	{ \
		va_list args; \
		va_start(args, format); \
		GetLogger().LogVF(Logger::Level::name, L"", format, args); \
		va_end(args); \
	} \
	\
	inline void Log ## name ## F(Section* section, const WCHAR* format, ...) \
	{ \
		va_list args; \
		va_start(args, format); \
		GetLogger().LogSectionVF(Logger::Level::name, section, format, args); \
		va_end(args); \
	} \
	\
	inline void Log ## name ## F(MeterWindow* meterWindow, const WCHAR* format, ...) \
	{ \
		va_list args; \
		va_start(args, format); \
		GetLogger().LogMeterWindowVF(Logger::Level::name, meterWindow, format, args); \
		va_end(args); \
	}

RM_LOGGER_DEFINE_LOG_FUNCTIONS(Error)
RM_LOGGER_DEFINE_LOG_FUNCTIONS(Warning)
RM_LOGGER_DEFINE_LOG_FUNCTIONS(Notice)
RM_LOGGER_DEFINE_LOG_FUNCTIONS(Debug)

#endif