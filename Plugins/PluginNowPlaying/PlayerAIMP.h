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
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

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
