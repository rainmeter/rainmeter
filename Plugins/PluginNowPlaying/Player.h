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

// SHA
#include "hmac_sha2.h"
#include "sha2.h"

// TagLib
#include "apefile.h"
#include "apetag.h"
#include "asffile.h"
#include "attachedpictureframe.h"
#include "commentsframe.h"
#include "flacfile.h"
#include "id3v1genres.h"
#include "id3v2tag.h"
#include "mpcfile.h"
#include "mp4file.h"
#include "mpegfile.h"
#include "tag.h"
#include "taglib.h"
#include "textidentificationframe.h"
#include "tstring.h"
#include "vorbisfile.h"
#include "wavpackfile.h"

// Amazon keys
#define ACCESSKEY L"AKIAIDIAU7MXWSOJF3PQ"
#define SECRETKEY "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"

enum PLAYERSTATE
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
	MEASURE_COVER,
	MEASURE_DURATION,
	MEASURE_POSITION,
	MEASURE_PROGRESS,
	MEASURE_RATING,
	MEASURE_STATE,
	MEASURE_VOLUME
};

class CPlayer
{
public:
	CPlayer();
	virtual ~CPlayer();

	virtual void Play() {}
	virtual void PlayPause() {}
	virtual void Stop() {}
	virtual void Next() {}
	virtual void Previous() {}
	virtual void SetRating(int rating) {}
	virtual void SetVolume(int volume) {}
	virtual void ChangeVolume(int volume) {}
	virtual void OpenPlayer() {}
	virtual void ClosePlayer() {}
	virtual void TogglePlayer() {}

	virtual void AddInstance(MEASURETYPE type) = 0;
	virtual void RemoveInstance() = 0;
	virtual void UpdateData() = 0;

	PLAYERSTATE GetState() { return m_State; }
	LPCTSTR GetArtist() { return m_Artist.c_str(); }
	LPCTSTR GetAlbum() { return m_Album.c_str(); }
	LPCTSTR GetTitle() { return m_Title.c_str(); }
	LPCTSTR GetCoverPath() { return m_CoverPath.c_str(); }
	LPCTSTR GetPlayerPath() { return m_PlayerPath.c_str(); }
	UINT GetDuration() { return m_Duration; }
	UINT GetPosition() { return m_Position; }
	UINT GetRating() { return m_Rating; }
	UINT GetVolume() { return m_Volume; }

	void SetPlayerPath(LPCTSTR path) { m_PlayerPath = path; }
	void SetTrackChangeAction(LPCTSTR action) { m_TrackChangeAction = action; }
	void ExecuteTrackChangeAction();
	void ClearInfo();

	std::wstring CreateCoverArtPath();
	bool GetLocalArt(std::wstring& folder, std::wstring filename);
	bool GetEmbeddedArt(const TagLib::FileRef& fr, std::wstring& path);
	bool GetArtAPE(TagLib::APE::Tag* tag, std::wstring& path);
	bool GetArtID3(TagLib::ID3v2::Tag* tag, std::wstring& path);
	bool GetArtASF(TagLib::ASF::File* file, std::wstring& path);
	bool GetArtFLAC(TagLib::FLAC::File* file, std::wstring& path);
	bool GetArtMP4(TagLib::MP4::File* file, std::wstring& path);

protected:
	int m_InstanceCount;
	bool m_TrackChanged;

	PLAYERSTATE m_State;
	std::wstring m_Artist;
	std::wstring m_Album;
	std::wstring m_Title;
	std::wstring m_CoverPath;		// Path to cover art image
	std::wstring m_PlayerPath;		// Path to player executable
	UINT m_Duration;				// Track duration in seconds
	UINT m_Position;				// Current position in seconds
	UINT m_Rating;					// Track rating from 0 to 100
	UINT m_Volume;					// Volume from 0 to 100

	std::wstring m_TrackChangeAction;
};

#endif