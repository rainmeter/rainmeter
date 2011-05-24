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
	enum IPCMESSAGE
	{
		IPC_PLAY						= 100,
		IPC_PLAYPAUSE,
		IPC_FORCEPAUSE,
		IPC_STOP,
		IPC_NEXT,
		IPC_PREVIOUS,
		IPC_SET_VOLUME					= 108,
		IPC_GET_VOLUME,
		IPC_GET_CURRENT_TRACK,
		IPC_GET_DURATION				= 113,
		IPC_SET_POSITION,
		IPC_IS_PLAYING,
		IPC_IS_PAUSED,
		IPC_GET_LIST_LENGTH,
		IPC_SET_LIST_POS,
		IPC_GET_LIST_ITEM,
		IPC_SET_CALLBACK_HWND,					// Recieved by player. wParam is handle to CAD window.
		IPC_GET_LIST_POS,
		IPC_GET_POSITION,
		IPC_TRACK_CHANGED_NOTIFICATION,			// Sent by player.
		IPC_SHOW_PLAYER_WINDOW,
		IPC_GET_PLAYER_STATE,
		IPC_PLAYER_STATE_CHANGED_NOTIFICATION,	// Sent by player.
		IPC_AUTOENQUEUE_OPTIONS,				// Ignored.
		IPC_SET_REPEAT,
		IPC_SHUTDOWN_NOTIFICATION,				// Sent by/to player on exit. Player should NULL the CAD window handle.
		IPC_GET_REPEAT,
		IPC_CLOSE_PLAYER,						// Player should exit upon receival.
		IPC_GET_SHUFFLE					= 140,
		IPC_SET_SHUFFLE,
		IPC_RATING_CHANGED_NOTIFICATION = 639,	// Sent by/to player.
		IPC_REGISTER_PLAYER				= 700,	// Sent by player to CAD on startup.
		IPC_CURRENT_TRACK_INFO,
		IPC_SEND_LYRICS,
		IPC_SEND_NEW_LYRICS,
		IPC_NEW_COVER_NOTIFICATION		= 800,	// Sent by player (ignored).
		IPC_GET_CURRENT_LYRICS,
		IPC_ADDFILE_PLAY_PLAYLIST,
		IPC_ADDFILE_QUEUE_PLAYLIST
	};

	void Initialize();
	void Uninitialize();
	static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

	bool m_HasCoverMeasure;
	HWND m_Window;				// Our reciever window
	HWND m_PlayerWindow;		// CAD receiver window
};

#endif
