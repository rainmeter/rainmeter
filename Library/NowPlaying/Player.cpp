/* Copyright (C) 2011 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "StdAfx.h"
#include "Player.h"

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
	m_InternetThread(nullptr)
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

	if (m_InternetThread)
	{
		TerminateThread(m_InternetThread, 0UL);
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
	if (!m_InternetThread)
	{
		m_Lyrics.clear();

		unsigned int id = 0U;
		HANDLE thread = (HANDLE)_beginthreadex(nullptr, 0U, LyricsThreadProc, this, 0U, &id);
		if (thread)
		{
			m_InternetThread = thread;
		}
	}
}

/*
** Thread to download lyrics.
**
*/
unsigned __stdcall Player::LyricsThreadProc(void* pParam)
{
	Player* player = (Player*)pParam;

	std::wstring lyrics;
	bool found = false;

	while (true)
	{
		UINT beforeCount = player->GetTrackCount();
		found = Lyrics::GetFromInternet(player->m_Artist, player->m_Title, lyrics);
		UINT afterCount = player->GetTrackCount();

		if (beforeCount == afterCount)
		{
			// We're on the same track
			break;
		}

		// Track changed, try again
	}

	if (found)
	{
		player->m_Lyrics = lyrics;
	}

	CloseHandle(player->m_InternetThread);
	player->m_InternetThread = nullptr;

	return 0U;
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
