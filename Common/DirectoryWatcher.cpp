/* Copyright (C) 2026 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "StdAfx.h"
#include "DirectoryWatcher.h"
#include <process.h>

DirectoryWatcher::DirectoryWatcher() :
	m_Directory(INVALID_HANDLE_VALUE),
	m_Thread(nullptr),
	m_StopEvent(CreateEvent(nullptr, TRUE, FALSE, nullptr)),
	m_Recursive(false),
	m_Callback(nullptr),
	m_Context(nullptr)
{
}

DirectoryWatcher::~DirectoryWatcher()
{
	Stop();
	CloseHandle(m_StopEvent);
}

bool DirectoryWatcher::Start(const std::wstring& directory, bool recursive, ChangeCallback callback, void* context)
{
	Stop();

	m_Directory = CreateFile(
		directory.c_str(), FILE_LIST_DIRECTORY,
		FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
		nullptr, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED, nullptr);
	if (m_Directory == INVALID_HANDLE_VALUE) return false;

	m_Path = directory;
	if (!m_Path.empty() && m_Path.back() != L'\\') m_Path += L'\\';
	m_Recursive = recursive;
	m_Callback = callback;
	m_Context = context;
	ResetEvent(m_StopEvent);

	uintptr_t thread = _beginthreadex(
		nullptr, 0,
		[](void* context) -> unsigned
		{
			static_cast<DirectoryWatcher*>(context)->Run();
			return 0;
		},
		this, 0, nullptr);
	if (!thread)
	{
		CloseHandle(m_Directory);
		m_Directory = INVALID_HANDLE_VALUE;
		m_Callback = nullptr;
		m_Context = nullptr;
		return false;
	}

	m_Thread = (HANDLE)thread;
	return true;
}

void DirectoryWatcher::Stop()
{
	if (m_Thread)
	{
		SetEvent(m_StopEvent);
		WaitForSingleObject(m_Thread, INFINITE);
		CloseHandle(m_Thread);
		m_Thread = nullptr;
	}

	if (m_Directory != INVALID_HANDLE_VALUE)
	{
		CloseHandle(m_Directory);
		m_Directory = INVALID_HANDLE_VALUE;
	}

	m_Callback = nullptr;
	m_Context = nullptr;
}

void DirectoryWatcher::Run()
{
	BYTE buffer[64 * 1024];
	OVERLAPPED overlapped = { 0 };
	overlapped.hEvent = CreateEvent(nullptr, TRUE, FALSE, nullptr);
	if (!overlapped.hEvent) return;
	HANDLE events[] = { m_StopEvent, overlapped.hEvent };

	while (true)
	{
		ResetEvent(overlapped.hEvent);

		const auto flags = FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_LAST_WRITE | FILE_NOTIFY_CHANGE_SIZE;
		if (!ReadDirectoryChangesW(m_Directory, buffer, sizeof(buffer), m_Recursive, flags, nullptr, &overlapped, nullptr) &&
				GetLastError() != ERROR_IO_PENDING)
		{
			break;
		}

		const DWORD result = WaitForMultipleObjects(_countof(events), events, FALSE, INFINITE);
		if (result != WAIT_OBJECT_0 + 1)
		{
			CancelIoEx(m_Directory, &overlapped);
			DWORD ignored;
			GetOverlappedResult(m_Directory, &overlapped, &ignored, TRUE);
			break;
		}

		DWORD bytesReturned;
		if (!GetOverlappedResult(m_Directory, &overlapped, &bytesReturned, FALSE) || !bytesReturned) continue;
		FILE_NOTIFY_INFORMATION* info = (FILE_NOTIFY_INFORMATION*)buffer;
		do
		{
			if (m_Callback)
			{
				std::wstring path = m_Path;
				path.append(info->FileName, info->FileNameLength / sizeof(WCHAR));
				m_Callback(path.c_str(), m_Context);
			}

			if (!info->NextEntryOffset) break;
			info = (FILE_NOTIFY_INFORMATION*)((BYTE*)info + info->NextEntryOffset);
		}
		while (true);
	}

	CloseHandle(overlapped.hEvent);
}
