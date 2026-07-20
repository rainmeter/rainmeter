/* Copyright (C) 2026 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#ifndef RM_COMMON_DIRECTORYWATCHER_H_
#define RM_COMMON_DIRECTORYWATCHER_H_

#include <Windows.h>
#include <string>

class DirectoryWatcher
{
public:
	typedef void (*ChangeCallback)(const WCHAR* path, void* context);

	DirectoryWatcher();
	~DirectoryWatcher();
	DirectoryWatcher(const DirectoryWatcher&) = delete;
	DirectoryWatcher& operator=(const DirectoryWatcher&) = delete;

	bool Start(const std::wstring& directory, bool recursive, ChangeCallback callback, void* context);
	void Stop();

private:
	void Run();

	HANDLE m_Directory;
	HANDLE m_Thread;
	HANDLE m_StopEvent;
	std::wstring m_Path;
	bool m_Recursive;
	ChangeCallback m_Callback;
	void* m_Context;
};

#endif
