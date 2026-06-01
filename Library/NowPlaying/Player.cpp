/* Copyright (C) 2011 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "StdAfx.h"
#include "Player.h"
#include "../../Common/CharacterEntityReference.h"
#include "../../Common/StringUtil.h"

std::wstring EncodeUrl(const std::wstring& url)
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
			WCHAR buffer[3] = { 0 };
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
** Constructor.
**
*/
Player::Player() :
	m_Initialized(false),
	m_InstanceCount(0U),
	m_UpdateCount(0U),
	m_TrackCount(0U),
	m_Measures(0),
	m_State(),
	m_Number(0U),
	m_Year(0U),
	m_Shuffle(false),
	m_Repeat(false),
	m_Duration(0U),
	m_Position(0U),
	m_Rating(0U),
	m_Volume(0U),
	m_FetchLyricsTask(nullptr)
{
	// Get temporary file for cover art
	WCHAR buffer[MAX_PATH] = { 0 };
	GetTempPath(_countof(buffer), buffer);
	GetTempFileName(buffer, L"jpg", 0U, buffer);
	m_TempCoverPath = buffer;
}

/*
** Destructor.
**
*/
Player::~Player()
{
	DeleteFile(m_TempCoverPath.c_str());

	if (m_FetchLyricsTask)
	{
		m_FetchLyricsTask->AbortWhenPossible();
		m_FetchLyricsTask = nullptr;
	}
}

/*
** Called during initialization of main measure.
**
*/
void Player::AddInstance()
{
	++m_InstanceCount;
}

/*
** Called during destruction of main measure.
**
*/
void Player::RemoveInstance()
{
	if (--m_InstanceCount == 0U)
	{
		delete this;
	}
}

/*
** Called during initialization of any measure.
**
*/
void Player::AddMeasure(INT type)
{
	m_Measures |= type;
}

/*
** Called during update of main measure.
**
*/
void Player::UpdateMeasure()
{
	if (++m_UpdateCount >= m_InstanceCount)
	{
		UpdateData();
		m_UpdateCount = 0U;
	}
}

/*
** Default implementation for getting cover.
**
*/
void Player::FindCover()
{
	TagLib::FileRef fr(m_FilePath.c_str(), false);
	if (!fr.isNull() && CCover::GetEmbedded(fr, m_TempCoverPath))
	{
		m_CoverPath = m_TempCoverPath;
	}
	else
	{
		std::wstring trackFolder = CCover::GetFileFolder(m_FilePath);

		if (!CCover::GetLocal(L"cover", trackFolder, m_CoverPath) &&
			!CCover::GetLocal(L"folder", trackFolder, m_CoverPath))
		{
			// Nothing found
			m_CoverPath.clear();
		}
	}
}

/*
** Default implementation for getting lyrics.
**
*/
void Player::FindLyrics()
{
	// This will be leaked on quit, but that's not a problem.
	static HINTERNET s_InternetHandle = InternetOpen(L"Rainmeter NowPlaying.dll", INTERNET_OPEN_TYPE_PRECONFIG, nullptr, nullptr, 0);

	m_Lyrics.clear();

	std::wstring url = L"https://www.letras.mus.br/winamp.php?musica=";
	url += EncodeUrl(m_Title);
	url += L"&artista=";
	url += EncodeUrl(m_Artist);

	m_FetchLyricsTask = Net::FetchTask::Create((void*)this, std::move(url), {}, s_InternetHandle, INTERNET_FLAG_RESYNCHRONIZE, LyricsFetchResultCallback);
}

void Player::LyricsFetchResultCallback(const Net::Task* fetchTask, void* requestor, BYTE* data, DWORD dataSize, DWORD errorCode)
{
	auto player = (Player*)requestor;
	if (player->m_FetchLyricsTask == fetchTask)
	{
		player->HandleLyricsFetchResult(data, dataSize, errorCode);
		player->m_FetchLyricsTask = nullptr;
	}
}

void Player::HandleLyricsFetchResult(BYTE* data, DWORD dataSize, DWORD errorCode)
{
	if (!data || dataSize < 100) return;

	std::wstring body = StringUtil::WidenUTF8((const char*)data, (int)dataSize);
	if (body.empty()) return;

	std::wstring::size_type pos = body.find(L"\"letra-cnt\"");
	pos = body.find(L"<p>", pos);
	if (pos == std::wstring::npos) return;

	pos += 6ULL;
	body.erase(0ULL, pos);

	pos = body.find(L"</div>");
	pos -= 9ULL;
	body.resize(pos);

	CharacterEntityReference::Decode(body, 2, false);

	while ((pos = body.find(L"<br/>"), pos) != std::wstring::npos)
	{
		body.replace(pos, 5ULL, L"\n");
	}

	while ((pos = body.find(L"</p><p>"), pos) != std::wstring::npos)
	{
		body.replace(pos, 7ULL, L"\n\n");
	}

	m_Lyrics = body;
}

/*
** Clear track information.
**
*/
void Player::ClearData(bool all)
{
	m_State = STATE_STOPPED;
	m_Artist.clear();
	m_Album.clear();
	m_Title.clear();
	m_Genre.clear();
	m_Lyrics.clear();
	m_FilePath.clear();
	m_CoverPath.clear();
	m_Duration = 0U;
	m_Position = 0U;
	m_Rating = 0U;
	m_Number = 0U;
	m_Year = 0U;

	if (all)
	{
		m_Volume = 0U;
		m_Shuffle = false;
		m_Repeat = false;
	}
}
