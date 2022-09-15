/* Copyright (C) 2011 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "StdAfx.h"
#include "Player.h"
#include "Internet.h"
#include "Lyrics.h"

/*
** Download lyrics from various serivces.
**
*/
bool Lyrics::GetFromInternet(const std::wstring& artist, const std::wstring& title, std::wstring& out)
{
	std::wstring encArtist = Internet::EncodeUrl(artist);
	std::wstring encTitle = Internet::EncodeUrl(title);

	bool found = GetFromLetras(encArtist, encTitle, out);

	return found;
}

/*
** Download lyrics from Letras.
**
*/
bool Lyrics::GetFromLetras(const std::wstring& artist, const std::wstring& title, std::wstring& data)
{
	bool ret = false;

	std::wstring url = L"https://www.letras.mus.br/winamp.php?musica=";
	url += title;
	url += L"&artista=";
	url += artist;
	data = Internet::DownloadUrl(url, CP_UTF8);
	if (!data.empty())
	{
		std::wstring::size_type pos = data.find(L"\"letra-cnt\"");
		pos = data.find(L"<p>", pos);
		if (pos != std::wstring::npos)
		{
			pos += 6ULL;
			data.erase(0ULL, pos);

			pos = data.find(L"</div>");
			pos -= 9ULL;
			data.resize(pos);

			Internet::DecodeReferences(data);

			while ((pos = data.find(L"<br/>"), pos) != std::wstring::npos)
			{
				data.replace(pos, 5ULL, L"\n");
			}

			while ((pos = data.find(L"</p><p>"), pos) != std::wstring::npos)
			{
				data.replace(pos, 7ULL, L"\n\n");
			}

			ret = true;
		}
	}

	return ret;
}
