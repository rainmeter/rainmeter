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

#ifndef __PLAYER_H__
#define __PLAYER_H__

#include "fileref.h"
#include "tag.h"
#include "Cover.h"
#include "Internet.h"
#include "Lyrics.h"

enum PLAYSTATE
{
	PLAYER_STOPPED,
	PLAYER_PLAYING,
	PLAYER_PAUSED
};

enum MEASURETYPE
{
	MEASURE_ARTIST,
	MEASURE_TITLE,
	MEASURE_ALBUM,
	MEASURE_LYRICS,
	MEASURE_COVER,
	MEASURE_DURATION,
	MEASURE_POSITION,
	MEASURE_PROGRESS,
	MEASURE_RATING,
	MEASURE_STATE,
	MEASURE_VOLUME,
	MEASURE_FILE
};

class CPlayer
{
public:
	CPlayer();
	virtual ~CPlayer() = 0;

	void AddInstance();
	void RemoveInstance();
	void UpdateMeasure();
	virtual void AddMeasure(MEASURETYPE measure);
	virtual void UpdateData() = 0;

	bool IsInitialized() { return m_Initialized; }
	UINT GetTrackCount() { return m_TrackCount; }
	
	std::wstring GetCacheFile();
	void FindCover();
	void FindLyrics();

	virtual void Pause() {}
	virtual void Play() {}
	virtual void Stop() {}
	virtual void Next() {}
	virtual void Previous() {}
	virtual void SetPosition(int position) {}
	virtual void SetRating(int rating) {}
	virtual void SetVolume(int volume) {}
	virtual void OpenPlayer(std::wstring& path) {}
	virtual void ClosePlayer() {}

	PLAYSTATE GetState() { return m_State; }
	LPCTSTR GetArtist() { return m_Artist.c_str(); }
	LPCTSTR GetAlbum() { return m_Album.c_str(); }
	LPCTSTR GetTitle() { return m_Title.c_str(); }
	LPCTSTR GetLyrics() { return m_Lyrics.c_str(); }
	LPCTSTR GetCoverPath() { return m_CoverPath.c_str(); }
	LPCTSTR GetFilePath() { return m_FilePath.c_str(); }
	UINT GetDuration() { return m_Duration; }
	UINT GetPosition() { return m_Position; }
	UINT GetRating() { return m_Rating; }
	UINT GetVolume() { return m_Volume; }

protected:
	void ClearData();

	bool m_Initialized;
	bool m_HasCoverMeasure;
	bool m_HasLyricsMeasure;
	UINT m_InstanceCount;
	UINT m_UpdateCount;
	UINT m_TrackCount;

	PLAYSTATE m_State;
	std::wstring m_Artist;
	std::wstring m_Title;
	std::wstring m_Album;
	std::wstring m_Lyrics;
	std::wstring m_CoverPath;		// Path to cover art image
	std::wstring m_FilePath;		// Path to playing file
	UINT m_Duration;				// Track duration in seconds
	UINT m_Position;				// Current position in seconds
	UINT m_Rating;					// Track rating from 0 to 100
	UINT m_Volume;					// Volume from 0 to 100

private:
	static unsigned __stdcall LyricsThreadProc(void* pParam);

	HANDLE m_InternetThread;
	CRITICAL_SECTION m_CriticalSection;
};

#endif