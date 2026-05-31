/* Copyright (C) 2026 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#ifndef RM_LIBRARY_NET_H_
#define RM_LIBRARY_NET_H_

#include <Windows.h>
#include <atomic>
#include <string>

namespace Net {

class Task
{
public:
	typedef void (* ResultCallback)(const Task*, void*, BYTE*, DWORD, DWORD);

	Task(void* requestor, std::wstring url, std::wstring headers, HINTERNET internetHandle, DWORD internetFlags, ResultCallback resultCallback);
	~Task();

	bool Start();
	void AbortWhenPossible();

	static void HandleResultMessage(WPARAM wParam, LPARAM lParam);

private:
	static DWORD WINAPI ThreadProc(void* param);

	BYTE* FetchData();

	void* m_Requestor;

	// Request
	std::wstring m_Url;
	std::wstring m_Headers;
	HINTERNET m_InternetHandle;
	DWORD m_InternetFlags;
	ResultCallback m_ResultCallback;
	std::atomic<bool> m_AbortRequested;

	// Response
	BYTE* m_Data;
	DWORD m_DataSize;
	DWORD m_ErrorCode;
};

}  // namespace Net

#endif
