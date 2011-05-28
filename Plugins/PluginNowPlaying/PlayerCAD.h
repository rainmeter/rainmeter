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

#ifndef __PLAYERIPC_H__
#define __PLAYERIPC_H__

#include "Player.h"

class CPlayerCAD : public CPlayer
{
public:
	CPlayerCAD();
	~CPlayerCAD();

	virtual void Play();
	virtual void PlayPause();
	virtual void Stop();
	virtual void Next();
	virtual void Previous();
	virtual void SetRating(int rating);
	virtual void SetVolume(int volume);
	virtual void ChangeVolume(int volume);
	virtual void ClosePlayer();
	virtual void OpenPlayer();
	virtual void TogglePlayer();

	virtual void AddInstance(MEASURETYPE type);
	virtual void RemoveInstance();
	virtual void UpdateData();

private:
	void Initialize();
	void Uninitialize();
	static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

	bool m_HasCoverMeasure;
	HWND m_Window;				// Our reciever window
	HWND m_PlayerWindow;		// CAD receiver window
};

#endif
