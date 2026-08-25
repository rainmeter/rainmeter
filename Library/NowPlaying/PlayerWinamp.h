// Copyright (c) Rainmeter Team. Source code licensed under GNU GPL v2 (see LICENSE file).

#pragma once

#include "Player.h"

enum WINAMPTYPE
{
	WA_WINAMP,
	WA_MEDIAMONKEY
};

class PlayerWinamp : public Player
{
public:
	virtual ~PlayerWinamp();

	static Player* Create(WINAMPTYPE type);

	virtual void UpdateData();

	virtual void Pause();
	virtual void Play();
	virtual void Stop();
	virtual void Next();
	virtual void Previous();
	virtual void SetPosition(int position);
	virtual void SetRating(int rating);
	virtual void SetVolume(int volume);
	virtual void SetShuffle(bool state);
	virtual void SetRepeat(bool state);
	virtual void ClosePlayer();
	virtual void OpenPlayer(std::wstring& path);

protected:
	PlayerWinamp(WINAMPTYPE type);

private:
	bool CheckWindow();

	static Player* c_Player;

	HWND m_Window;				// Winamp window
	ULONGLONG m_LastCheckTime;
	bool m_UseUnicodeAPI;
	bool m_PlayingStream;
	WINAMPTYPE m_WinampType;
	HANDLE m_WinampHandle;		// Handle to Winamp process
};
