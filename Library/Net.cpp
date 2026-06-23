/* Copyright (C) 2026 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "StdAfx.h"
#include "Net.h"
#include "Rainmeter.h"
#include "../Common/StringUtil.h"
#include "../Common/FileUtil.h"

namespace Net {

//
// DownloadTask
//

DownloadTask* DownloadTask::Create(void* requestor, std::wstring url, std::wstring path, ResultCallback resultCallback)
{
	auto* task = new DownloadTask(requestor, std::move(url), std::move(path), resultCallback);
	if (!task->Start())
	{
		delete task;
		return nullptr;
	}

	return task;
}

DownloadTask::DownloadTask(void* requestor, std::wstring url, std::wstring path, ResultCallback resultCallback) :
	AsyncTask(requestor),
	m_Url(std::move(url)),
	m_Path(std::move(path)),
	m_ResultCallback(resultCallback)
{
}

void DownloadTask::StartWorkOnWorkerThread()
{
	if (SUCCEEDED(CoInitialize(nullptr)))
	{
		m_Result = URLDownloadToFile(nullptr, m_Url.c_str(), m_Path.c_str(), 0UL, nullptr);
		CoUninitialize();
	}
}

void DownloadTask::FinishWorkOnMainThread()
{
	if (m_AbortRequested && SUCCEEDED(m_Result))
	{
		DeleteFile(m_Path.c_str());
	}
	else if (m_ResultCallback)
	{
		m_ResultCallback(this, m_Requestor, m_Path, m_Result);
	}
}

//
// FetchTask
//

FetchTask* FetchTask::Create(void* requestor, std::wstring url, std::wstring headers, HINTERNET internetHandle, DWORD internetFlags, ResultCallback resultCallback)
{
	auto* task = new FetchTask(requestor, std::move(url), std::move(headers), internetHandle, internetFlags, resultCallback);
	if (!task->Start())
	{
		delete task;
		return nullptr;
	}

	return task;
}

FetchTask::FetchTask(void* requestor, std::wstring url, std::wstring headers, HINTERNET internetHandle, DWORD internetFlags, ResultCallback resultCallback) :
	AsyncTask(requestor),
	m_Url(std::move(url)),
	m_Headers(std::move(headers)),
	m_InternetHandle(internetHandle),
	m_InternetFlags(internetFlags),
	m_ResultCallback(resultCallback)
{
}

FetchTask::~FetchTask()
{
	if (m_Data)
	{
		free(m_Data);
		m_Data = nullptr;
	}
}

void FetchTask::StartWorkOnWorkerThread()
{
	m_Data = FetchData();
	m_ErrorCode = m_Data ? ERROR_SUCCESS : GetLastError();
}

void FetchTask::FinishWorkOnMainThread()
{
	if (!m_AbortRequested && m_ResultCallback)
	{
		m_ResultCallback(this, m_Requestor, m_Data, m_DataSize, m_ErrorCode);
	}
}

BYTE* FetchTask::FetchData()
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

	DWORD contentLength = 0UL;
	DWORD contentLengthSize = sizeof(contentLength);
	if (HttpQueryInfo(hUrlDump, HTTP_QUERY_CONTENT_LENGTH | HTTP_QUERY_FLAG_NUMBER, &contentLength, &contentLengthSize, nullptr) &&
		contentLength > 0UL)
	{
		bufferSize = contentLength;
	}

	BYTE* buffer = (BYTE*)malloc(bufferSize + 3UL);
	if (!buffer)
	{
		SetLastError(ERROR_NOT_ENOUGH_MEMORY);
		InternetCloseHandle(hUrlDump);
		return nullptr;
	}

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
			free(oldBuffer);
			oldBuffer = nullptr;
			SetLastError(ERROR_NOT_ENOUGH_MEMORY);
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

}  // namespace Net
