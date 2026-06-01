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

	static Task* CreateFetch(void* requestor, std::wstring url, std::wstring headers, HINTERNET internetHandle, DWORD internetFlags, ResultCallback resultCallback);

	void AbortWhenPossible();

	static void HandleResultMessage(WPARAM wParam, LPARAM lParam);

private:
	Task();
	~Task();

	static DWORD WINAPI FetchThreadProc(void* param);

	BYTE* FetchData();

	void* m_Requestor;

	// Request
	std::wstring m_Url;
	std::wstring m_Headers;
	HINTERNET m_InternetHandle = nullptr;
	DWORD m_InternetFlags = 0;
	ResultCallback m_ResultCallback = nullptr;
	std::atomic<bool> m_AbortRequested = false;

	// Response
	BYTE* m_Data = nullptr;
	DWORD m_DataSize = 0;
	DWORD m_ErrorCode = 0;
};

}  // namespace Net

#endif
