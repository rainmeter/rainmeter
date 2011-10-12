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
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/

#include "StdAfx.h"
#include "Player.h"

extern std::wstring g_CachePath;

/*
** CPlayer
**
** Constructor.
**
*/
CPlayer::CPlayer() :
	m_Initialized(false),
	m_InstanceCount(),
	m_UpdateCount(),
	m_TrackCount(),
	m_Measures(),
	m_State(),
	m_Shuffle(false),
	m_Repeat(false),
	m_Duration(),
	m_Position(),
	m_Rating(),
	m_Volume(),
	m_InternetThread()
{
}

/*
** ~CPlayer
**
** Destructor.
**
*/
CPlayer::~CPlayer()
{
	if (m_InternetThread)
	{
		TerminateThread(m_InternetThread, 0);
	}
}

/*
** AddInstance
**
** Called during initialization of main measure.
**
*/
void CPlayer::AddInstance()
{
	++m_InstanceCount;
}

/*
** RemoveInstance
**
** Called during destruction of main measure.
**
*/
void CPlayer::RemoveInstance()
{
	m_UpdateCount = 0;

	if (--m_InstanceCount == 0)
	{
		delete this;
	}
}

/*
** AddMeasure
**
** Called during initialization of any measure.
**
*/
void CPlayer::AddMeasure(INT type)
{
	m_Measures |= type;
}

/*
** UpdateMeasure
**
** Called during update of main measure.
**
*/
void CPlayer::UpdateMeasure()
{
	if (++m_UpdateCount == m_InstanceCount)
	{
		UpdateData();
		m_UpdateCount = 0;
	}
}

/*
** GetCacheFile
**
** Creates escaped path to cache file (without extension)
**
*/
std::wstring CPlayer::GetCacheFile()
{
	std::wstring path = g_CachePath;

	if (m_Artist.empty() || m_Title.empty())
	{
		path += L"temp";
	}
	else
	{
		std::wstring name = m_Artist + L" - ";
		name += m_Title;

		// Replace reserved chars with _
		std::wstring::size_type pos = 0;
		while ((pos = name.find_first_of(L"\\/:*?\"<>|", pos)) != std::wstring::npos) name[pos] = L'_';

		path += name;
	}

	return path;
}

/*
** FindCover
**
** Default implementation for getting cover.
**
*/
void CPlayer::FindCover()
{
	m_CoverPath = GetCacheFile();

	if (!CCover::GetCached(m_CoverPath))
	{
		TagLib::FileRef fr(m_FilePath.c_str());
		if (fr.isNull() || !CCover::GetEmbedded(fr, m_CoverPath))
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
}

/*
** FindLyrics
**
** Default implementation for getting lyrics.
**
*/
void CPlayer::FindLyrics()
{
	if (!m_InternetThread)
	{
		m_Lyrics.clear();

		unsigned int id;
		HANDLE thread = (HANDLE)_beginthreadex(NULL, 0, LyricsThreadProc, this, 0, &id);
		if (thread)
		{
			m_InternetThread = thread;
		}
		else
		{
			LSLog(LOG_DEBUG, NULL, L"NowPlaying.dll: Failed to start lyrics thread");
		}
	}
}

/*
** LyricsThreadProc
**
** Thread to download lyrics.
**
*/
unsigned __stdcall CPlayer::LyricsThreadProc(void* pParam)
{
	CPlayer* player = (CPlayer*)pParam;

	std::wstring lyrics;
	bool found;

	while (true)
	{
		UINT beforeCount = player->GetTrackCount();
		found = CLyrics::GetFromInternet(player->m_Artist, player->m_Title, lyrics);
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
	player->m_InternetThread = NULL;

	return 0;
}

/*
** ClearData
**
** Clear track information.
**
*/
void CPlayer::ClearData()
{
	m_State = PLAYER_STOPPED;
	m_Artist.clear();
	m_Album.clear();
	m_Title.clear();
	m_Lyrics.clear();
	m_FilePath.clear();
	m_CoverPath.clear();
	m_Duration = 0;
	m_Position = 0;
	m_Rating = 0;
}
