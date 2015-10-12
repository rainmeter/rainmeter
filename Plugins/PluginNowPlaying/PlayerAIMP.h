/* Copyright (C) 2011 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#ifndef __PLAYERAIMP_H__
#define __PLAYERAIMP_H__

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
	DWORD m_LastCheckTime;
	INT64 m_LastFileSize;
	DWORD m_LastTitleSize;
	LPVOID m_FileMap;
	HANDLE m_FileMapHandle;
};

#endif
