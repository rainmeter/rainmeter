/* Copyright (C) 2013 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#ifndef RM_LOGGER_H_
#define RM_LOGGER_H_

#include <Windows.h>
#include <cstdarg>
#include <string>
#include <list>
#include <chrono>

class Section;
class Skin;
class Measure;

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

	void Log(Logger::Entry* entry);
	void Log(Level level, const WCHAR* source, const WCHAR* msg);
	void LogVF(Level level, const WCHAR* source, const WCHAR* format, va_list args);
	void LogSkinVF(Logger::Level level, Skin* skin, const WCHAR* format, va_list args);
	void LogSkinSVF(Logger::Level level, Skin* skin, const WCHAR* section, const WCHAR* format, va_list args);
	void LogSection(Logger::Level level, Section* section, const WCHAR* message);
	void LogSectionVF(Logger::Level level, Section* section, const WCHAR* format, va_list args);
	void LogMeasureVF(Logger::Level level, Measure* section, const WCHAR* format, va_list args);

	const std::wstring& GetLogFilePath() { return m_LogFilePath; }

	const std::list<Entry>& GetEntries() { return m_Entries; }

private:
	void LogInternal(Level level, std::chrono::system_clock::time_point timestamp, const WCHAR* source, const WCHAR* msg);

	// Appends |entry| to the log file.
	void WriteToLogFile(Entry& entry);

	Logger();
	~Logger();

	Logger(const Logger& other) = delete;
	Logger& operator=(Logger other) = delete;

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
	inline void Log ## name ## SF(Skin* skin, const WCHAR* section, const WCHAR* format, ...) \
	{ \
		va_list args; \
		va_start(args, format); \
		GetLogger().LogSkinSVF(Logger::Level::name, skin, section, format, args); \
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
	inline void Log ## name ## F(Skin* skin, const WCHAR* format, ...) \
	{ \
		va_list args; \
		va_start(args, format); \
		GetLogger().LogSkinVF(Logger::Level::name, skin, format, args); \
		va_end(args); \
	} \
	\
	inline void Log ## name ## F(Measure* measure, const WCHAR* format, ...) \
	{ \
		va_list args; \
		va_start(args, format); \
		GetLogger().LogMeasureVF(Logger::Level::name, measure, format, args); \
		va_end(args); \
	}

RM_LOGGER_DEFINE_LOG_FUNCTIONS(Error)
RM_LOGGER_DEFINE_LOG_FUNCTIONS(Warning)
RM_LOGGER_DEFINE_LOG_FUNCTIONS(Notice)
RM_LOGGER_DEFINE_LOG_FUNCTIONS(Debug)

#endif
