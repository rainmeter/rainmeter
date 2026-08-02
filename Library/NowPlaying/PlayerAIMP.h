// Copyright (c) Rainmeter Team. Source code licensed under GNU GPL v2 (see LICENSE file).

#pragma once

#include "Player.h"

class PlayerAIMP : public Player
{
public:
	virtual ~PlayerAIMP();

	static Player* Create();

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
	PlayerAIMP();

private:
	bool Initialize();
	bool CheckWindow();

	static Player* c_Player;

	HWND m_Window;				// AIMP window
	HWND m_WinampWindow;		// AIMP Winamp API window
	ULONGLONG m_LastCheckTime;
	INT64 m_LastFileSize;
	DWORD m_LastTitleSize;
	LPVOID m_FileMap;
	HANDLE m_FileMapHandle;
};
