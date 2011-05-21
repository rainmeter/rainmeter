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
#include "PlayerSpotify.h"

/*
** CPlayerSpotify
**
** Constructor.
**
*/
CPlayerSpotify::CPlayerSpotify() :
	m_Window()
{
	GetWindow();
}

/*
** ~CPlayerSpotify
**
** Destructor.
**
*/
CPlayerSpotify::~CPlayerSpotify()
{
}

/*
** AddInstance
**
** Called during initialization of each measure.
**
*/
void CPlayerSpotify::AddInstance(MEASURETYPE type)
{
	++m_InstanceCount;
}

/*
** RemoveInstance
**
** Called during destruction of each measure.
**
*/
void CPlayerSpotify::RemoveInstance()
{
	if (--m_InstanceCount == 0)
	{
		delete this;
	}
}

/*
** UpdateData
**
** Called during each update of the main measure.
**
*/
void CPlayerSpotify::UpdateData()
{
	if (GetWindow())
	{
		if (m_TrackChanged)
		{
			ExecuteTrackChangeAction();
			m_TrackChanged = false;
		}

		// Get window text
		WCHAR buffer[256];
		buffer[0] = 0;
		GetWindowText(m_Window, buffer, 256);
		std::wstring title = buffer;

		title.erase(0, 10);	// Get rid of "Spotify - "
		std::wstring::size_type pos = title.find(L" – ");
		if (pos != std::wstring::npos)
		{
			std::wstring artist = title.substr(0, pos);
			std::wstring track = title.substr(pos + 3);

			if (track != m_Title && artist != m_Artist)
			{
				m_Title = track;
				m_Artist = artist;
				m_TrackChanged = true;
			}
			return;
		}
	}

	ClearInfo();
}

bool CPlayerSpotify::GetWindow()
{
	m_Window = FindWindow(L"SpotifyMainWindow", NULL);

	return m_Window ? true : false;
}

/*
** PlayPause
**
** Handles the PlayPause bang.
**
*/
void CPlayerSpotify::PlayPause()
{
	if (m_Window)
	{
		SendMessage(m_Window, WM_APPCOMMAND, 0, SPOTIFY_PLAYPAUSE);
	}
}

/*
** Stop
**
** Handles the Stop bang.
**
*/
void CPlayerSpotify::Stop() 
{
	if (m_Window)
	{
		SendMessage(m_Window, WM_APPCOMMAND, 0, SPOTIFY_STOP);
	}
}

/*
** Next
**
** Handles the Next bang.
**
*/
void CPlayerSpotify::Next() 
{
	if (m_Window)
	{
		SendMessage(m_Window, WM_APPCOMMAND, 0, SPOTIFY_NEXT);
	}
}

/*
** Previous
**
** Handles the Previous bang.
**
*/
void CPlayerSpotify::Previous() 
{
	if (m_Window)
	{
		SendMessage(m_Window, WM_APPCOMMAND, 0, SPOTIFY_PREV);
	}
}


/*
** ClosePlayer
**
** Handles the ClosePlayer bang.
**
*/
void CPlayerSpotify::ClosePlayer()
{
	if (m_Window)
	{
		// A little harsh...
		DWORD pID;
		GetWindowThreadProcessId(m_Window, &pID);
		HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, pID);
		if (hProcess)
		{
			TerminateProcess(hProcess, 0);
			CloseHandle(hProcess);
		}
	}
}

/*
** OpenPlayer
**
** Handles the OpenPlayer bang.
**
*/
void CPlayerSpotify::OpenPlayer()
{
	if (!m_Window)
	{
		if (m_PlayerPath.empty())
		{
			// Gotta figure out where Winamp is located at
			HKEY hKey;
			RegOpenKeyEx(HKEY_CLASSES_ROOT,
						 L"spotify\\DefaultIcon",
						 0,
						 KEY_QUERY_VALUE,
						 &hKey);

			DWORD size = 512;
			WCHAR* data = new WCHAR[size];
			DWORD type = 0;

			if (RegQueryValueEx(hKey,
								NULL,
								NULL,
								(LPDWORD)&type,
								(LPBYTE)data,
								(LPDWORD)&size) == ERROR_SUCCESS)
			{
				if (type == REG_SZ)
				{
					std::wstring path = data;
					path.erase(0, 1);				// Get rid of the leading quote
					path.resize(path.length() - 3);	// And the ",0 at the end
					ShellExecute(NULL, L"open", path.c_str(), NULL, NULL, SW_SHOW);
					m_PlayerPath = path;
				}
			}

			delete [] data;
			RegCloseKey(hKey);
		}
		else
		{
			ShellExecute(NULL, L"open", m_PlayerPath.c_str(), NULL, NULL, SW_SHOW);
		}
	}
	else
	{
		// Already active, restore the window
		ShowWindow(m_Window, SW_SHOWNORMAL);
		BringWindowToTop(m_Window);
	}
}

/*
** TogglePlayer
**
** Handles the TogglePlayer bang.
**
*/
void CPlayerSpotify::TogglePlayer()
{
	m_Window ? ClosePlayer() : OpenPlayer();
}
