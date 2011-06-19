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

#ifndef __PLAYERFOOBAR_H__
#define __PLAYERFOOBAR_H__

#include "Player.h"

class CPlayerFoobar : public CPlayer
{
public:
	virtual ~CPlayerFoobar();

	static CPlayer* Create();

	virtual void UpdateData();

	virtual void Pause();
	virtual void Play();
	virtual void Stop();
	virtual void Next();
	virtual void Previous();
	virtual void SetPosition(int position);
	virtual void SetRating(int rating) {}
	virtual void SetVolume(int volume);
	virtual void ClosePlayer();
	virtual void OpenPlayer(std::wstring& path);

protected:
	CPlayerFoobar();

private:
	enum FOOMESSAGE
	{
		FOO_TRACKCHANGE = 100,
		FOO_STATECHANGE,
		FOO_TIMECHANGE,
		FOO_VOLUMECHANGE,
		FOO_PLAYERSTART,
		FOO_PLAYERQUIT
	};

	enum FOOCOMMAND
	{
		FOO_GETVERSION,
		FOO_PLAY,
		FOO_PLAYPAUSE,
		FOO_PAUSE,
		FOO_STOP,
		FOO_NEXT,
		FOO_PREVIOUS,
		FOO_SETRATING,
		FOO_SETVOLUME,
		FOO_SHOWPLAYER,
		FOO_QUITPLAYER,
		FOO_SETCALLBACK,
		FOO_REMOVECALLBACK,
		FOO_SETPOSITION
	};

	void Initialize();
	void Uninitialize();
	static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

	static CPlayer* c_Player;

	HWND m_Window;				// Our reciever window
	HWND m_FooWindow;			// Foobar receiver window
};

#endif
