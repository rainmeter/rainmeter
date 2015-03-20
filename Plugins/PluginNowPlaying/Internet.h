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

#ifndef __INTERNET_H__
#define __INTERNET_H__

class Internet
{
public:
	static void Initialize();
	static void Finalize();

	static std::wstring DownloadUrl(const std::wstring& url, int codepage, LPWSTR headers = NULL);
	static std::wstring EncodeUrl(const std::wstring& url);
	static void DecodeReferences(std::wstring& str);
	static std::wstring ConvertToWide(LPCSTR str, int codepage);

private:
	static HINTERNET c_NetHandle;
};

#endif