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

	bool found = GetFromWikia(encArtist, encTitle, out) ||
				 GetFromLYRDB(encArtist, encTitle, out) ||
				 GetFromLetras(encArtist, encTitle, out);

	return found;
}

/*
** Download lyrics from LyricWiki.
**
*/
bool Lyrics::GetFromWikia(const std::wstring& artist, const std::wstring& title, std::wstring& data)
{
	bool ret = false;
	
	std::wstring url = L"http://lyrics.wikia.com/api.php?func=getSong&fmt=json&artist=" + artist;
	url += L"&song=";
	url += title;

	data = Internet::DownloadUrl(url, CP_UTF8);
	if (!data.empty())
	{
		// First we get the URL to the actual wiki page
		std::wstring::size_type pos = data.find(L"http://");
		if (pos != std::wstring::npos)
		{
			data.erase(0, pos);
			pos = data.find_first_of(L'\'');
			url.assign(data, 0, pos);

			// Fetch the wiki page
			data = Internet::DownloadUrl(url, CP_UTF8);
			if (!data.empty())
			{
				pos = data.find(L"'lyricbox'");
				pos = data.find(L"&#", pos);
				if (pos != std::wstring::npos)
				{
					// Get and decode lyrics
					data.erase(0, pos);
					pos = data.find(L"<!");
					data.resize(pos);
					Internet::DecodeReferences(data);

					pos = data.find(L"[...]");
					if (pos != std::wstring::npos)
					{
						// Skip incomplete lyrics
						return ret;
					}

					pos = data.find(L"<p>");
					if (pos != std::wstring::npos)
					{
						// Skip unavailable lyrics
						return ret;
					}

					while ((pos = data.find(L"<br />"), pos) != std::wstring::npos)
					{
						data.replace(pos, 6, L"\n");
					}

					// Get rid of all HTML tags
					std::wstring::size_type len = 0;
					while ((pos = data.find_first_of(L'<'), pos) != std::wstring::npos)
					{
						len = data.find_first_of(L'>', pos);
						len -= pos;
						data.erase(pos, ++len);
					}

					ret = true;
				}
			}
		}
	}

	return ret;
}

/*
** Download lyrics from LYRDB.
**
*/
bool Lyrics::GetFromLYRDB(const std::wstring& artist, const std::wstring& title, std::wstring& data)
{
	bool ret = false;

	std::wstring query = artist + L"|";
	query += title;

	// LYRDB doesn't like apostrophes
	std::wstring::size_type pos = 0;
	while ((pos = query.find(L"%27", pos)) != std::wstring::npos)
	{
		query.erase(pos, 3);
	}

	std::wstring url = L"http://webservices.lyrdb.com/lookup.php?q=" + query;
	url += L"&for=match&agent=RainmeterNowPlaying";

	data = Internet::DownloadUrl(url, CP_ACP);
	if (!data.empty())
	{
		pos = data.find(L"\\");
		if (pos != std::wstring::npos)
		{
			// Grab the first match
			url.assign(data, 0, pos);
			url.insert(0, L"http://webservices.lyrdb.com/getlyr.php?q=");

			data = Internet::DownloadUrl(url, CP_ACP);
			if (!data.empty())
			{
				ret = true;
			}
		}
	}

	return ret;
}

/*
** Download lyrics from Letras.
**
*/
bool Lyrics::GetFromLetras(const std::wstring& artist, const std::wstring& title, std::wstring& data)
{
	bool ret = false;

	std::wstring url = L"http://letras.terra.com.br/winamp.php?musica=" + title;
	url += L"&artista=";
	url += artist;
	data = Internet::DownloadUrl(url, CP_ACP);
	if (!data.empty())
	{
		std::wstring::size_type pos = data.find(L"\"letra\"");
		pos = data.find(L"<p>", pos);
		if (pos != std::wstring::npos)
		{
			pos += 3;
			data.erase(0, pos);

			pos = data.find(L"</p>");
			data.resize(pos);

			Internet::DecodeReferences(data);

			while ((pos = data.find(L"<br/>"), pos) != std::wstring::npos)
			{
				data.erase(pos, 5);
			}

			ret = true;
		}
	}

	return ret;
}
