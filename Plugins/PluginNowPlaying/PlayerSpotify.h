/* Copyright (C) 2011 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#ifndef __PLAYERSPOTIFY_H__
#define __PLAYERSPOTIFY_H__

#include "Player.h"

class PlayerSpotify : public Player
{
public:
	virtual ~PlayerSpotify();

	static Player* Create();

	virtual void Pause() { return Play(); }
	virtual void Play();
	virtual void Stop();
	virtual void Next();
	virtual void Previous();
	virtual void ClosePlayer();
	virtual void OpenPlayer(std::wstring& path);
	virtual void UpdateData();

protected:
	PlayerSpotify();

private:
	enum SPOTIFYCOMMAND
	{
		SPOTIFY_MUTE		= 524288,
		SPOTIFY_VOLUMEDOWN	= 589824,
		SPOTIFY_VOLUMEUP	= 655360,
		SPOTIFY_NEXT		= 720896,
		SPOTIFY_PREV		= 786432,
		SPOTIFY_STOP		= 851968,
		SPOTIFY_PLAYPAUSE	= 917504
	};

	bool CheckWindow();

	static Player* c_Player;

	HWND m_Window;
	DWORD m_LastCheckTime;
	std::wstring m_baseURL = L"http://127.0.0.1:4380/";
	std::wstring m_csrfURL = L"simplecsrf/token.json";
	std::wstring m_statusURL = L"remote/status.json";
	std::wstring m_openidURL = L"https://open.spotify.com/token";
	LPWSTR originHeader = L"Origin: https://open.spotify.com\r\n\r\n\r\n";
	std::wstring csrfToken, openidToken;
};

#endif
