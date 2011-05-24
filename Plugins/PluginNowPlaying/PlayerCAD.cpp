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

#include "StdAfx.h"
#include "PlayerCAD.h"

// This player emulates the CD Art Display IPC API. For now, only MusicBee
// is supported, but this can easily be extended.

extern CPlayer* g_CAD;

/*
** CPlayerCAD
**
** Constructor.
**
*/
CPlayerCAD::CPlayerCAD() : CPlayer(),
	m_HasCoverMeasure(false),
	m_Window(),
	m_PlayerWindow()
{
	Initialize();
}

/*
** ~CPlayerCAD
**
** Constructor.
**
*/
CPlayerCAD::~CPlayerCAD()
{
	Uninitialize();
}

/*
** AddInstance
**
** Called during initialization of each measure.
**
*/
void CPlayerCAD::AddInstance(MEASURETYPE type)
{
	++m_InstanceCount;

	if (type == MEASURE_COVER)
	{
		m_HasCoverMeasure = true;
	}
}

/*
** RemoveInstance
**
** Called during destruction of each measure.
**
*/
void CPlayerCAD::RemoveInstance()
{
	if (--m_InstanceCount == 0)
	{
		g_CAD = NULL;
		delete this;
	}
}

/*
** Initialize
**
** Create receiver window.
**
*/
void CPlayerCAD::Initialize()
{
	HINSTANCE hInstance = GetModuleHandle(NULL);

	// Create windows class
	WNDCLASS wc = {0};
	wc.hInstance = hInstance;
	wc.lpfnWndProc = WndProc;
	wc.lpszClassName = L"NowPlayingCADClass";
	RegisterClass(&wc);

	// Create dummy window
	m_Window = CreateWindow(L"NowPlayingCADClass",
							L"CD Art Display 1.x Class",
							WS_DISABLED,
							CW_USEDEFAULT,
							CW_USEDEFAULT,
							CW_USEDEFAULT,
							CW_USEDEFAULT,
							NULL,
							NULL,
							hInstance,
							this);

	HWND wnd = FindWindow(L"WindowsForms10.Window.8.app.0.378734a", NULL);
	if (wnd)
	{
		// Let's check if it's MusicBee
		WCHAR buffer[256];
		GetWindowText(wnd, buffer, 256);
		if (wcsstr(buffer, L"MusicBee"))
		{
			m_PlayerWindow = wnd;
			SendMessage(m_PlayerWindow, WM_USER, (WPARAM)m_Window, IPC_SET_CALLBACK_HWND);
			SendMessage(m_PlayerWindow, WM_USER, 0, IPC_GET_CURRENT_TRACK);
		}
		else
		{
			LSLog(LOG_DEBUG, L"Rainmeter", L"NowPlayingPlugin: MusicBee class found with invalid title.");
		}
	}
}

/*
** Uninitialize
**
** Destroy reciever window.
**
*/
void CPlayerCAD::Uninitialize()
{
	DestroyWindow(m_Window);
	UnregisterClass(L"NowPlayingCADClass", GetModuleHandle(NULL));
}


/*
** WndProc
**
** Window procedure for the reciever window.
**
*/
LRESULT CALLBACK CPlayerCAD::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	static CPlayerCAD* p;

	switch (msg)
	{
	case WM_CREATE:
		// Get pointer to the CPlayerCAD class from the CreateWindow call
		p = (CPlayerCAD*)((CREATESTRUCT*)lParam)->lpCreateParams;
		return 0;

	case WM_DESTROY:
		SendMessage(p->m_PlayerWindow, WM_USER, 0, IPC_SHUTDOWN_NOTIFICATION);
		return 0;

	case WM_USER:
		switch (lParam)
		{
		case IPC_TRACK_CHANGED_NOTIFICATION:
			SendMessage(p->m_PlayerWindow, WM_USER, 0, IPC_GET_CURRENT_TRACK);
			break;

		case IPC_PLAYER_STATE_CHANGED_NOTIFICATION:
			p->m_State = (PLAYERSTATE)wParam;
			if (p->m_State == PLAYER_STOPPED)
			{
				p->ClearInfo();
			}
			break;

		case IPC_SHUTDOWN_NOTIFICATION:
			p->m_PlayerWindow = NULL;
			break;
		}
		return 0;

	case WM_COPYDATA:
		{
			PCOPYDATASTRUCT cds = (PCOPYDATASTRUCT)lParam;
			if (cds->dwData == IPC_CURRENT_TRACK_INFO)
			{
				p->m_TrackChanged = true;
				std::wstring data = (WCHAR*)cds->lpData;
				std::wstring::size_type len = data.find_first_of(L'\t');
				p->m_Title.assign(data, 0, len);
				data.erase(0, ++len);

				len = data.find_first_of(L'\t');
				p->m_Artist.assign(data, 0, len);
				data.erase(0, ++len);

				len = data.find_first_of(L'\t');
				p->m_Album.assign(data, 0, len);
				data.erase(0, ++len);

				len = data.find_first_of(L'\t');			// Skip genre
				len = data.find_first_of(L'\t', ++len);		// Skip year
				len = data.find_first_of(L'\t', ++len);		// Skip comments
				len = data.find_first_of(L'\t', ++len);		// Skip track no
				data.erase(0, ++len);

				len = data.find_first_of(L'\t');
				p->m_Duration = _wtoi(data.substr(0, len).c_str());
				data.erase(0, ++len);

				len = data.find_first_of(L'\t');
				p->m_FilePath.assign(data, 0, len);
				data.erase(0, ++len);

				len = data.find_first_of(L'\t');
				UINT rating = _wtoi(data.substr(0, len).c_str()) + 1;
				rating /= 2; // From 0 - 10 to 0 - 5
				p->m_Rating = rating;
				data.erase(0, ++len);

				len = data.find_first_of(L'\t');
				p->m_CoverPath.assign(data, 0, len);
				data.erase(0, ++len);
			}
			else if (cds->dwData == IPC_REGISTER_PLAYER && !p->m_PlayerWindow)
			{
				std::wstring data = (WCHAR*)cds->lpData;
				if (data[0] == L'1')
				{
					data.erase(0, 2);	// Get rid of the 1\t at the beginning

					std::wstring::size_type len = data.find_first_of(L'\t');
					std::wstring className = data.substr(0, len);
					data.erase(0, ++len);

					len = data.find_first_of(L'\t');
					std::wstring windowName = data.substr(0, len);
					data.erase(0, ++len);

					len = data.find_first_of(L'\t');
					p->m_PlayerPath.assign(data, 0, len);
					data.erase(0, ++len);

					p->m_PlayerWindow = FindWindow(className.empty() ? NULL : className.c_str(),
												   windowName.empty() ? NULL : windowName.c_str());
				}
			}
		}
		return 0;

	default:
		return DefWindowProc(hwnd, msg, wParam, lParam);
	}
}

/*
** UpdateData
**
** Called during each update of the main measure.
**
*/
void CPlayerCAD::UpdateData()
{
	if (m_PlayerWindow)
	{
		m_Position = SendMessage(m_PlayerWindow, WM_USER, 0, IPC_GET_POSITION);
		m_Volume = SendMessage(m_PlayerWindow, WM_USER, 0, IPC_GET_VOLUME);

		if (m_TrackChanged)
		{
			ExecuteTrackChangeAction();
			m_TrackChanged = false;
		}
	}
}

/*
** Play
**
** Handles the Play bang.
**
*/
void CPlayerCAD::Play()
{
	if (m_PlayerWindow)
	{
		SendMessage(m_PlayerWindow, WM_USER, 0, IPC_PLAY);
	}

}

/*
** PlayPause
**
** Handles the PlayPause bang.
**
*/
void CPlayerCAD::PlayPause()
{
	if (m_PlayerWindow)
	{
		SendMessage(m_PlayerWindow, WM_USER, 0, IPC_PLAYPAUSE);
	}
}

/*
** Stop
**
** Handles the Stop bang.
**
*/
void CPlayerCAD::Stop() 
{
	if (m_PlayerWindow)
	{
		SendMessage(m_PlayerWindow, WM_USER, 0, IPC_STOP);
	}
}

/*
** Next
**
** Handles the Next bang.
**
*/
void CPlayerCAD::Next() 
{
	if (m_PlayerWindow)
	{
		SendMessage(m_PlayerWindow, WM_USER, 0, IPC_NEXT);
	}
}

/*
** Previous
**
** Handles the Previous bang.
**
*/
void CPlayerCAD::Previous() 
{
	if (m_PlayerWindow)
	{
		SendMessage(m_PlayerWindow, WM_USER, 0, IPC_PREVIOUS);
	}
}

/*
** SetRating
**
** Handles the SetVolume bang.
**
*/
void CPlayerCAD::SetRating(int rating) 
{
	if (m_PlayerWindow)
	{
		m_Rating = rating;
		rating *= 2; // From 0 - 5 to 0 - 10
		SendMessage(m_PlayerWindow, WM_USER, rating, IPC_RATING_CHANGED_NOTIFICATION);
	}
}

/*
** SetVolume
**
** Handles the SetVolume bang.
**
*/
void CPlayerCAD::SetVolume(int volume) 
{
	if (m_PlayerWindow)
	{
		volume += m_Volume;
		if (volume < 0)
		{
			volume = 0;
		}
		else if (volume > 100)
		{
			volume = 100;
		}
		SendMessage(m_PlayerWindow, WM_USER, volume, IPC_SET_VOLUME);
	}
}

/*
** ChangeVolume
**
** Handles the ChangeVolume bang.
**
*/
void CPlayerCAD::ChangeVolume(int volume) 
{
	volume += m_Volume;
	SetVolume(volume);
}

/*
** ClosePlayer
**
** Handles the ClosePlayer bang.
**
*/
void CPlayerCAD::ClosePlayer()
{
	SendMessage(m_PlayerWindow, WM_USER, 0, IPC_CLOSE_PLAYER);
}

/*
** OpenPlayer
**
** Handles the OpenPlayer bang.
**
*/
void CPlayerCAD::OpenPlayer()
{
}

/*
** TogglePlayer
**
** Handles the TogglePlayer bang.
**
*/
void CPlayerCAD::TogglePlayer()
{
	m_PlayerWindow ? ClosePlayer() : OpenPlayer();
}
