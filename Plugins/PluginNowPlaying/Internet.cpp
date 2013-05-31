/*
  Copyright (C) 2011 Birunthan Mohanathas (www.poiru.net)

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#include "StdAfx.h"
#include "Internet.h"

HINTERNET Internet::c_NetHandle = nullptr;

/*
** Initialize internet handle and crtical section.
**
*/
void Internet::Initialize()
{
	c_NetHandle = InternetOpen(L"Rainmeter NowPlaying.dll",
								INTERNET_OPEN_TYPE_PRECONFIG,
								nullptr,
								nullptr,
								0);

	if (!c_NetHandle)
	{
		RmLog(LOG_ERROR, L"NowPlaying.dll: Unable to open net handle");
	}
}

/*
** Close handles and delete critical section.
**
*/
void Internet::Finalize()
{
	if (c_NetHandle) InternetCloseHandle(c_NetHandle);
}

/*
** Downloads given url and returns it as a string.
**
*/
std::wstring Internet::DownloadUrl(const std::wstring& url, int codepage)
{
	// From WebParser.cpp
	std::wstring result;
	DWORD flags = INTERNET_FLAG_RESYNCHRONIZE;
	HINTERNET hUrlDump = InternetOpenUrl(c_NetHandle, url.c_str(), nullptr, 0, flags, 0);

	if (!hUrlDump)
	{
		return result;
	}

	// Allocate the buffer.
	const int CHUNK_SIZE = 8192;
	BYTE* lpData = new BYTE[CHUNK_SIZE];
	BYTE* lpOutPut;
	BYTE* lpHolding = nullptr;
	int nCounter = 1;
	int nBufferSize;
	DWORD dwDataSize = 0;
	DWORD dwSize = 0;

	do
	{
		// Read the data.
		if (!InternetReadFile(hUrlDump, (LPVOID)lpData, CHUNK_SIZE, &dwSize))
		{
			break;
		}
		else
		{
			// Check if all of the data has been read. This should
			// never get called on the first time through the loop.
			if (dwSize == 0)
			{
				break;
			}

			// Determine the buffer size to hold the new data and the data
			// already written (if any).
			nBufferSize = dwDataSize + dwSize;

			// Allocate the output buffer.
			lpOutPut = new BYTE[nBufferSize + 2];

			// Make sure the buffer is not the initial buffer.
			if (lpHolding != nullptr)
			{
				// Copy the data in the holding buffer.
				memcpy(lpOutPut, lpHolding, dwDataSize);

				// Delete the old buffer
				delete [] lpHolding;

				lpHolding = lpOutPut;
				lpOutPut = lpOutPut + dwDataSize;
			}
			else
			{
				lpHolding = lpOutPut;
			}

			// Copy the data buffer.
			memcpy(lpOutPut, lpData, dwSize);

			dwDataSize += dwSize;

			// End with double null
			lpOutPut[dwSize] = 0;
			lpOutPut[dwSize + 1] = 0;

			// Increment the number of buffers read.
			++nCounter;

			// Clear the buffer
			memset(lpData, 0, CHUNK_SIZE);
		}
	} while (true);

	InternetCloseHandle(hUrlDump);

	delete [] lpData;

	if (lpHolding)
	{
		result = ConvertToWide((LPCSTR)lpHolding, codepage);
		delete [] lpHolding;
	}

	return result;
}

/*
** Encode reserved characters.
**
*/
std::wstring Internet::EncodeUrl(const std::wstring& url)
{
	// Based on http://www.zedwood.com/article/111/cpp-urlencode-function
	const WCHAR* urlChars = L" !*'();:@&=+$,/?#[]";
	std::wstring ret;

	for (size_t i = 0, max = url.length(); i < max; ++i)
	{
		if (wcschr(urlChars, url[i]))
		{
			// If reserved character
			ret.append(L"%");
			WCHAR buffer[3];
			_snwprintf_s(buffer, 3, L"%.2X", url[i]);
			ret.append(buffer);
		}
		else
		{
			ret.push_back(url[i]);
		}
	}
	return ret;
}

/*
** Decodes numeric references.
**
*/
void Internet::DecodeReferences(std::wstring& str)
{
	// From WebParser.cpp
	std::wstring::size_type start = 0;

	while ((start = str.find(L'&', start)) != std::wstring::npos)
	{
		std::wstring::size_type end, pos;

		if ((end = str.find(L';', start)) == std::wstring::npos) break;
		pos = start + 1;

		if (pos == end)  // &; - skip
		{
			start = end + 1;
			continue;
		}
		else if ((end - pos) > 10)  // name (or number) is too long
		{
			++start;
			continue;
		}

		if (str[pos] == L'#')  // Numeric character reference
		{
			if (++pos == end)  // &#; - skip
			{
				start = end + 1;
				continue;
			}

			int base;
			if (str[pos] == L'x' || str[pos] == L'X')
			{
				if (++pos == end)  // &#x; or &#X; - skip
				{
					start = end + 1;
					continue;
				}
				base = 16;
			}
			else
			{
				base = 10;
			}

			std::wstring num(str, pos, end - pos);
			WCHAR* pch = nullptr;
			errno = 0;
			long ch = wcstol(num.c_str(), &pch, base);
			if (pch == nullptr || *pch != L'\0' || errno == ERANGE || ch <= 0 || ch >= 0xFFFE)  // invalid character
			{
				start = pos;
				continue;
			}
			str.replace(start, end - start + 1, 1, (WCHAR)ch);
			++start;
		}
		else  // Character entity reference
		{
			start = end + 1;
			continue;
		}
	}
}

/*
** Convert multibyte string to wide string.
**
*/
std::wstring Internet::ConvertToWide(LPCSTR str, int codepage)
{
	std::wstring szWide;

	if (str && *str)
	{
		int strLen = (int)strlen(str);
		int bufLen = MultiByteToWideChar(codepage, 0, str, strLen, nullptr, 0);
		if (bufLen > 0)
		{
			szWide.resize(bufLen);
			MultiByteToWideChar(codepage, 0, str, strLen, &szWide[0], bufLen);
		}
	}

	return szWide;
}
