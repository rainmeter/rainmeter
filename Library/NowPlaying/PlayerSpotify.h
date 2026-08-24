// Copyright (c) Rainmeter Team. Source code licensed under GNU GPL v2 (see LICENSE file).

#pragma once

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
	ULONGLONG m_LastCheckTime;
};
