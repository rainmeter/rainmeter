// Copyright (c) Rainmeter Team. Source code licensed under GNU GPL v2 (see LICENSE file).

#pragma once

#include <Windows.h>
#include <string>
#include "AsyncTask.h"

namespace Net {

// Async task to download an URL to a file.
class DownloadTask : public AsyncTask
{
public:
	typedef void (* ResultCallback)(const DownloadTask*, void*, const std::wstring&, HRESULT result);

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
class FetchTask : public AsyncTask
{
public:
	typedef void (* ResultCallback)(const FetchTask*, void*, BYTE*, DWORD, DWORD);

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
