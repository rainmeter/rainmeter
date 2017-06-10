/* Copyright (C) 2011 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "StdAfx.h"
#include "PlayerSpotify.h"

Player* PlayerSpotify::c_Player = nullptr;

/*
** Constructor.
**
*/
PlayerSpotify::PlayerSpotify() : Player(),
	m_Window(),
	m_LastCheckTime(0)
{
}

/*
** Destructor.
**
*/
PlayerSpotify::~PlayerSpotify()
{
	c_Player = nullptr;
}

/*
** Creates a shared class object.
**
*/
Player* PlayerSpotify::Create()
{
	if (!c_Player)
	{
		c_Player = new PlayerSpotify();
	}

	return c_Player;
}

/*
** Try to find Spotify periodically.
**
*/
bool PlayerSpotify::CheckWindow()
{
	DWORD time = GetTickCount();
		
	// Try to find Spotify window every 5 seconds
	if (time - m_LastCheckTime > 5000)
	{
		m_LastCheckTime = time;

		m_Window = FindWindow(L"SpotifyMainWindow", nullptr);
		if (m_Window)
		{
			m_Initialized = true;
		}
	}

	return m_Initialized;
}

/*
** Called during each update of the main measure.
**
*/
void PlayerSpotify::UpdateData()
{
	if (m_Initialized || CheckWindow())
	{
		// Parse title and artist from window title
		WCHAR buffer[256];

		//Length of window is now 7 when not playing
		if (GetWindowText(m_Window, buffer, 256) > 7)
		{
			std::wstring title = buffer;

			std::wstring::size_type pos = title.find(L" - ");
			if (pos != std::wstring::npos)
			{
				std::wstring artist(title, 0, pos);
				pos += 3;  // Skip " - "
				std::wstring track(title, pos);

				if (track != m_Title || artist != m_Artist)
				{
					m_State = STATE_PLAYING;
					m_Title = track;
					m_Artist = artist;
					++m_TrackCount;

					if (m_Measures & MEASURE_LYRICS)
					{
						FindLyrics();
					}
				}
				return;
			}
		}
		else if (IsWindow(m_Window))
		{
			m_State = STATE_PAUSED;
		}
		else
		{
			ClearData();
			m_Initialized = false;
		}
	}
}

/*
** Handles the Play bang.
**
*/
void PlayerSpotify::Play()
{
	SendMessage(m_Window, WM_APPCOMMAND, 0, SPOTIFY_PLAYPAUSE);
}

/*
** Handles the Stop bang.
**
*/
void PlayerSpotify::Stop() 
{
	SendMessage(m_Window, WM_APPCOMMAND, 0, SPOTIFY_STOP);
}

/*
** Handles the Next bang.
**
*/
void PlayerSpotify::Next() 
{
	SendMessage(m_Window, WM_APPCOMMAND, 0, SPOTIFY_NEXT);
}

/*
** Handles the Previous bang.
**
*/
void PlayerSpotify::Previous() 
{
	SendMessage(m_Window, WM_APPCOMMAND, 0, SPOTIFY_PREV);
}


/*
** Handles the ClosePlayer bang.
**
*/
void PlayerSpotify::ClosePlayer()
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

/*
** Handles the OpenPlayer bang.
**
*/
void PlayerSpotify::OpenPlayer(std::wstring& path)
{
	if (!m_Initialized)
	{
		if (path.empty())
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
								nullptr,
								nullptr,
								(LPDWORD)&type,
								(LPBYTE)data,
								(LPDWORD)&size) == ERROR_SUCCESS)
			{
				if (type == REG_SZ)
				{
					path = data;
					path.erase(0, 1);				// Get rid of the leading quote
					path.resize(path.length() - 3);	// And the ",0 at the end
					ShellExecute(nullptr, L"open", path.c_str(), nullptr, nullptr, SW_SHOW);
				}
			}

			delete [] data;
			RegCloseKey(hKey);
		}
		else
		{
			ShellExecute(nullptr, L"open", path.c_str(), nullptr, nullptr, SW_SHOW);
		}
	}
	else
	{
		// Already active, restore the window
		ShowWindow(m_Window, SW_SHOWNORMAL);
		BringWindowToTop(m_Window);
	}
}
