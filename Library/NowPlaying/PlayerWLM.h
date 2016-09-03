/* Copyright (C) 2011 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#ifndef __PLAYERWLM_H__
#define __PLAYERWLM_H__

#include "Player.h"

class PlayerWLM : public Player
{
public:
	virtual ~PlayerWLM();

	static Player* Create();

	virtual void UpdateData();

	virtual void Pause() { return Play(); }
	virtual void Play();
	virtual void Stop();
	virtual void Next();
	virtual void Previous();

protected:
	PlayerWLM();

private:
	static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
	void SendKeyInput(WORD key);

	static Player* c_Player;

	HWND m_Window;
};

#endif
