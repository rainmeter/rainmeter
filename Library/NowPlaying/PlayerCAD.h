/* Copyright (C) 2011 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#ifndef __PLAYERCAD_H__
#define __PLAYERCAD_H__

#include "Player.h"

class PlayerCAD : public Player
{
public:
	virtual ~PlayerCAD();

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
	PlayerCAD();

private:
	void Initialize();
	void Uninitialize();
	static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

	static Player* c_Player;

	HWND m_Window;
	HWND m_PlayerWindow;
	std::wstring m_PlayerPath;
	bool m_ExtendedAPI;
	bool m_Open;
};

#endif
