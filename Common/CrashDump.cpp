// Copyright (c) Rainmeter Team. Source code licensed under GNU GPL v2 (see LICENSE file).

#include "StdAfx.h"
#include "CrashDump.h"

#include <DbgHelp.h>

namespace {

std::wstring g_DumpPath;
LONG g_HandlingCrash = 0;
LPTOP_LEVEL_EXCEPTION_FILTER g_PreviousExceptionHandler = nullptr;

bool DeleteOldDumps(const std::wstring& dumpFolderPath)
{
	constexpr ULONGLONG fileTimeTicksPerWeek = 7ULL * 24 * 60 * 60 * 10000000;
	constexpr ULONGLONG fileTimeTicksPerFiveMinutes = 5ULL * 60 * 10000000;

	FILETIME currentFileTime;
	GetSystemTimeAsFileTime(&currentFileTime);

	ULARGE_INTEGER currentTime;
	currentTime.LowPart = currentFileTime.dwLowDateTime;
	currentTime.HighPart = currentFileTime.dwHighDateTime;
	const ULONGLONG cutoff = currentTime.QuadPart > fileTimeTicksPerWeek ? currentTime.QuadPart - fileTimeTicksPerWeek : 0;
	const ULONGLONG recentCutoff = currentTime.QuadPart > fileTimeTicksPerFiveMinutes ? currentTime.QuadPart - fileTimeTicksPerFiveMinutes : 0;
	UINT recentDumpCount = 0;

	WIN32_FIND_DATA findData;
	HANDLE find = FindFirstFile((dumpFolderPath + L"Crash-*.dmp").c_str(), &findData);
	if (find == INVALID_HANDLE_VALUE) return false;

	do
	{
		ULARGE_INTEGER lastWriteTime;
		lastWriteTime.LowPart = findData.ftLastWriteTime.dwLowDateTime;
		lastWriteTime.HighPart = findData.ftLastWriteTime.dwHighDateTime;
		if (!(findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
		{
			if (lastWriteTime.QuadPart >= recentCutoff && lastWriteTime.QuadPart <= currentTime.QuadPart)
			{
				++recentDumpCount;
			}
			else if (lastWriteTime.QuadPart < cutoff)
			{
				DeleteFile((dumpFolderPath + findData.cFileName).c_str());
			}
		}
	}
	while (FindNextFile(find, &findData));

	FindClose(find);
	return recentDumpCount >= 2;
}

LONG WINAPI HandleUnhandledException(EXCEPTION_POINTERS* exceptionPointers)
{
	if (exceptionPointers &&
		(exceptionPointers->ExceptionRecord->ExceptionCode == DBG_PRINTEXCEPTION_C ||
			exceptionPointers->ExceptionRecord->ExceptionCode == DBG_CONTROL_C))
	{
		return EXCEPTION_CONTINUE_SEARCH;
	}

	if (!exceptionPointers || InterlockedCompareExchange(&g_HandlingCrash, 1, 0) != 0)
	{
		return EXCEPTION_CONTINUE_SEARCH;
	}

	SYSTEMTIME localTime;
	GetLocalTime(&localTime);
	WCHAR dumpFilePath[MAX_PATH];
	swprintf_s(
		dumpFilePath,
		_countof(dumpFilePath),
		L"%sCrash-%04u-%02u-%02u_%02u-%02u-%02u.dmp",
		g_DumpPath.c_str(),
		(UINT)localTime.wYear,
		(UINT)localTime.wMonth,
		(UINT)localTime.wDay,
		(UINT)localTime.wHour,
		(UINT)localTime.wMinute,
		(UINT)localTime.wSecond);

	HANDLE file = CreateFile(dumpFilePath, GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
	if (file != INVALID_HANDLE_VALUE)
	{
		MINIDUMP_EXCEPTION_INFORMATION exceptionInformation = {};
		exceptionInformation.ThreadId = GetCurrentThreadId();
		exceptionInformation.ExceptionPointers = exceptionPointers;
		exceptionInformation.ClientPointers = FALSE;
		MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(), file, MiniDumpWithDataSegs, &exceptionInformation, nullptr, nullptr);
		CloseHandle(file);
	}

	return g_PreviousExceptionHandler ? g_PreviousExceptionHandler(exceptionPointers) : EXCEPTION_CONTINUE_SEARCH;
}

}  // namespace

namespace CrashDump {

bool Initialize(const std::wstring& dumpFolderPath)
{
	g_DumpPath = dumpFolderPath;
	if (!g_DumpPath.empty() && g_DumpPath.back() != L'\\')
	{
		g_DumpPath += L'\\';
	}

	CreateDirectory(g_DumpPath.c_str(), nullptr);
	const bool repeatedCrashesFound = DeleteOldDumps(g_DumpPath);

	g_PreviousExceptionHandler = SetUnhandledExceptionFilter(HandleUnhandledException);
	return repeatedCrashesFound;
}

}  // namespace CrashDump
