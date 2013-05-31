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
};

#endif
