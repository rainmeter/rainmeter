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

#ifndef __PLAYERWLM_H__
#define __PLAYERWLM_H__

#include "Player.h"

class CPlayerWLM : public CPlayer
{
public:
	CPlayerWLM();
	~CPlayerWLM();

	virtual void Pause() { return PlayPause(); }
	virtual void Play() { return PlayPause(); }
	virtual void PlayPause();
	virtual void Stop();
	virtual void Next();
	virtual void Previous();

	virtual void AddInstance(MEASURETYPE type);
	virtual void RemoveInstance();
	virtual void UpdateData();

private:
	static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
	void SendKeyInput(WORD key);

	HWND m_Window;				// Spotify window
};

#endif
