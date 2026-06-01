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

class __declspec(novtable) Task
{
public:
	static void HandleResultMessage(WPARAM wParam, LPARAM lParam);

	void AbortWhenPossible();

protected:
	Task(void* requestor);
	virtual ~Task() {}

	virtual void StartWorkOnWorkerThread() = 0;
	virtual void FinishWorkOnMainThread() = 0;

	static DWORD WINAPI ThreadProc(void* param);

	BYTE* FetchData();

	void* m_Requestor;
	std::atomic<bool> m_AbortRequested;
};

// Async task to download an URL to a file.
class DownloadTask : public Task
{
public:
	typedef void (* ResultCallback)(const Task*, void*, const std::wstring&, HRESULT result);

	static DownloadTask* Create(void* requestor, std::wstring url, std::wstring path, ResultCallback resultCallback);

private:
	DownloadTask(void* requestor, std::wstring url, std::wstring path, ResultCallback resultCallback);
	virtual ~DownloadTask() {}

	void StartWorkOnWorkerThread() override;
	void FinishWorkOnMainThread() override;

	// Request
	std::wstring m_Url;
	std::wstring m_Path;
	ResultCallback m_ResultCallback = nullptr;

	// Result
	HRESULT m_Result = E_FAIL;
};

// Async task to fetch an URL from the web.
class FetchTask : public Task
{
public:
	typedef void (* ResultCallback)(const Task*, void*, BYTE*, DWORD, DWORD);

	static FetchTask* Create(void* requestor, std::wstring url, std::wstring headers, HINTERNET internetHandle, DWORD internetFlags, ResultCallback resultCallback);

private:
	FetchTask(void* requestor, std::wstring url, std::wstring headers, HINTERNET internetHandle, DWORD internetFlags, ResultCallback resultCallback);
	virtual ~FetchTask();

	void StartWorkOnWorkerThread() override;
	void FinishWorkOnMainThread() override;

	BYTE* FetchData();

	// Request
	std::wstring m_Url;
	std::wstring m_Headers;
	HINTERNET m_InternetHandle = nullptr;
	DWORD m_InternetFlags = 0;
	ResultCallback m_ResultCallback = nullptr;

	// Result
	BYTE* m_Data = nullptr;
	DWORD m_DataSize = 0;
	DWORD m_ErrorCode = 0;
};

}  // namespace Net

#endif
