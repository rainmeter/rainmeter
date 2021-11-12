/* Copyright (C) 2011 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#ifndef __PLAYERWINAMP_H__
#define __PLAYERWINAMP_H__

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

#endif
