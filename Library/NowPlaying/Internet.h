/* Copyright (C) 2011 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#ifndef __INTERNET_H__
#define __INTERNET_H__

class Internet
{
public:
	static void Initialize();
	static void Finalize();

	static std::wstring DownloadUrl(const std::wstring& url, int codepage);
	static std::wstring EncodeUrl(const std::wstring& url);
	static void DecodeReferences(std::wstring& str);
	static std::wstring ConvertToWide(LPCSTR str, int codepage);

private:
	static HINTERNET c_NetHandle;
};

#endif
