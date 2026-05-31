/* Copyright (C) 2026 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "StdAfx.h"
#include "AsyncNet.h"
#include "Rainmeter.h"
#include "../Common/StringUtil.h"
#include "../Common/FileUtil.h"

AsyncFetch::AsyncFetch(void* requestor, std::wstring url, std::wstring headers, HINTERNET internetHandle, DWORD internetFlags, ResultCallback resultCallback) :
	m_Requestor(requestor),
	m_Url(std::move(url)),
	m_Headers(std::move(headers)),
	m_InternetHandle(internetHandle),
	m_InternetFlags(internetFlags),
	m_ResultCallback(resultCallback),
	m_Data(nullptr),
	m_DataSize(0),
	m_ErrorCode(0)
{
}

AsyncFetch::~AsyncFetch()
{
	if (m_Data)
	{
		free(m_Data);
		m_Data = nullptr;
	}
}

bool AsyncFetch::Start()
{
	return QueueUserWorkItem(AsyncFetch::ThreadProc, this, 0);
}

void AsyncFetch::AbortWhenPossible()
{
	m_AbortRequested = true;
}

DWORD WINAPI AsyncFetch::ThreadProc(void* param)
{
	auto fetch = (AsyncFetch*)param;
	fetch->m_Data = fetch->FetchData();
	fetch->m_ErrorCode = fetch->m_Data ? ERROR_SUCCESS : GetLastError();

	// Continue processing the request on the main thread in AsyncFetch::HandleAsyncFetchResult.
	PostMessage(GetRainmeter().GetWindow(), WM_RAINMETER_HANDLE_ASYNC_FETCH_RESULT, (WPARAM)fetch, 0);

	return 0;
}

void AsyncFetch::HandleAsyncFetchResult(WPARAM wParam, LPARAM lParam)
{
	auto fetch = (AsyncFetch*)wParam;
	if (fetch && fetch->m_ResultCallback)
	{
		fetch->m_ResultCallback(fetch, fetch->m_Requestor, fetch->m_Data, fetch->m_DataSize, fetch->m_ErrorCode);
	}

	delete fetch;
}

BYTE* AsyncFetch::FetchData()
{
	m_DataSize = 0UL;

	if (m_AbortRequested)
	{
		return nullptr;
	}

	if (_wcsnicmp(m_Url.c_str(), L"file://", 7ULL) == 0)  // Local file
	{
		WCHAR path[MAX_PATH] = { 0 };
		DWORD pathLength = _countof(path);
		HRESULT hr = PathCreateFromUrl(m_Url.c_str(), path, &pathLength, 0);
		if (FAILED(hr))
		{
			return nullptr;
		}

		size_t fileSize = 0ULL;
		BYTE* buffer = FileUtil::ReadFullFile(path, &fileSize).release();
		m_DataSize = (DWORD)fileSize;

		return buffer;
	}

	{
		URL_COMPONENTS components = { 0 };
		components.dwStructSize = sizeof(components);
		components.dwExtraInfoLength = ULONG_MAX;
		if (InternetCrackUrl(m_Url.c_str(), static_cast<DWORD>(m_Url.size()), 0, &components))
		{
			if (components.lpszExtraInfo && components.dwExtraInfoLength > 0ULL)
			{
				size_t position = m_Url.find(components.lpszExtraInfo);  // Only percent encode characters in the query or fragment part of the URL
				if (position != std::wstring::npos)
				{
					std::wstring extra = m_Url.substr(position);
					StringUtil::EncodeUrl(extra, false);  // Only percent encode spaces, control characters, and non-ascii characters (HEX: 80-255)

					m_Url.erase(position);
					m_Url.append(extra);
				}
			}
		}
	}

	HINTERNET hUrlDump = InternetOpenUrl(m_InternetHandle, m_Url.c_str(), m_Headers.c_str(), -1L, m_InternetFlags, 0);
	if (!hUrlDump)
	{
		return nullptr;
	}

	// Allocate buffer with 3 extra bytes for triple null termination in case the string is
	// invalid (e.g. when incorrectly using the UTF-16LE codepage for the data).
	const DWORD CHUNK_SIZE = 8192UL;
	DWORD bufferSize = CHUNK_SIZE;
	BYTE* buffer = (BYTE*)malloc(bufferSize + 3UL);

	// Read the data.
	do
	{
		DWORD readSize = 0UL;
		if (m_AbortRequested || !InternetReadFile(hUrlDump, buffer + m_DataSize, bufferSize - m_DataSize, &readSize))
		{
			free(buffer);
			buffer = nullptr;
			InternetCloseHandle(hUrlDump);
			return nullptr;
		}
		else if (readSize == 0UL)
		{
			// All data read.
			break;
		}

		m_DataSize += readSize;

		bufferSize += CHUNK_SIZE;

		BYTE* oldBuffer = buffer;
		if ((buffer = (BYTE*)realloc(buffer, bufferSize + 3UL)) == nullptr)
		{
			free(oldBuffer);  // In case realloc fails
			oldBuffer = nullptr;
			InternetCloseHandle(hUrlDump);
			return nullptr;
		}
	}
	while (true);

	InternetCloseHandle(hUrlDump);

	// Triple null terminate the buffer.
	buffer[m_DataSize] = 0;
	buffer[m_DataSize + 1] = 0;
	buffer[m_DataSize + 2] = 0;

	return buffer;
}
