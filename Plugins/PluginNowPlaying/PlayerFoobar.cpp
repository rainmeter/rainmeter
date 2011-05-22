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
#include "PlayerFoobar.h"

/*
** CPlayerFoobar
**
** Constructor.
**
*/
CPlayerFoobar::CPlayerFoobar() : CPlayer(),
	m_HasCoverMeasure(false),
	m_Window(),
	m_FooWindow()
{
	Initialize();
}

/*
** ~CPlayerFoobar
**
** Constructor.
**
*/
CPlayerFoobar::~CPlayerFoobar()
{
	Uninitialize();
}

/*
** AddInstance
**
** Called during initialization of each measure.
**
*/
void CPlayerFoobar::AddInstance(MEASURETYPE type)
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
void CPlayerFoobar::RemoveInstance()
{
	if (--m_InstanceCount == 0)
	{
		delete this;
	}
}

/*
** Initialize
**
** Create receiver window.
**
*/
void CPlayerFoobar::Initialize()
{
	HINSTANCE hInstance = GetModuleHandle(NULL);

	// Create windows class
	WNDCLASS wc = {0};
	wc.hInstance = hInstance;
	wc.lpfnWndProc = WndProc;
	wc.lpszClassName = L"NowPlayingFoobarClass";
	RegisterClass(&wc);

	// Create dummy window
	m_Window = CreateWindow(L"NowPlayingFoobarClass",
							L"ReceiverWindow",
							WS_DISABLED,
							CW_USEDEFAULT,
							CW_USEDEFAULT,
							CW_USEDEFAULT,
							CW_USEDEFAULT,
							NULL,
							NULL,
							hInstance,
							this);

	m_FooWindow = FindWindow(L"foo_rainmeter_class", NULL);
	if (m_FooWindow)
	{
		int version = (int)SendMessage(m_FooWindow, WM_USER, 0, FOO_GETVERSION);
		if (version < 100)
		{
			std::wstring error = L"Your copy of the foo_rainmeter.dll plugin is outdated.\n";
			error += L"Please download the latest version and try again.";
			MessageBox(NULL, error.c_str(), L"Rainmeter", MB_OK | MB_ICONERROR | MB_TOPMOST);
			m_FooWindow = NULL;
		}
		else
		{
			SendMessage(m_FooWindow, WM_USER, (WPARAM)m_Window, FOO_SETCALLBACK);
		}
	}
}

/*
** Uninitialize
**
** Destroy reciever window.
**
*/
void CPlayerFoobar::Uninitialize()
{
	if (m_FooWindow)
	{
		SendMessage(m_FooWindow, WM_USER, 0, FOO_REMOVECALLBACK);
	}

	DestroyWindow(m_Window);
	UnregisterClass(L"NowPlayingFoobarClass", GetModuleHandle(NULL));
}

/*
** WndProc
**
** Window procedure for the reciever window.
**
*/
LRESULT CALLBACK CPlayerFoobar::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	static CPlayerFoobar* foobar;

	switch (msg)
	{
	case WM_CREATE:
		{
			// Get pointer to the CPlayerFoobar class from the CreateWindow call
			LPVOID params = ((CREATESTRUCT*)lParam)->lpCreateParams;
			foobar = (CPlayerFoobar*)params;
			return 0;
		}

	case WM_USER:
		switch (lParam)
		{
		case FOO_GETVERSION:
			return 100;

		case FOO_STATECHANGE:
			{
				PLAYERSTATE ps = (PLAYERSTATE)wParam;
				if (ps == PLAYER_STOPPED)
				{
					foobar->ClearInfo();
				}
				else
				{
					foobar->m_State = ps;
				}
			}
			break;

		case FOO_TIMECHANGE:
			foobar->m_Position = (UINT)wParam;
			break;

		case FOO_VOLUMECHANGE:
			foobar->m_Volume = (UINT)wParam;
			break;

		case FOO_PLAYERSTART:
			foobar->m_FooWindow = (HWND)wParam;
			break;

		case FOO_PLAYERQUIT:
			foobar->m_FooWindow = NULL;
			foobar->ClearInfo();
			break;
		}
		return 0;

	case WM_COPYDATA:
		{
			PCOPYDATASTRUCT cds = (PCOPYDATASTRUCT)lParam;

			if (cds->dwData == FOO_TRACKCHANGE)
			{
				if (foobar->m_State != PLAYER_PLAYING)
				{
					foobar->m_State = PLAYER_PLAYING;
				}

				// In the format "TITLE ARTIST ALBUM LENGTH RATING" (seperated by \t)
				WCHAR buffer[1024];
				MultiByteToWideChar(CP_UTF8, 0, (char*)cds->lpData, cds->cbData, buffer, 1024);
				foobar->m_Artist = buffer;

				WCHAR* token = wcstok(buffer, L"\t");
				if (token)
				{
					foobar->m_Title = token;
				}
				token = wcstok(NULL, L"\t");
				if (token)
				{
					foobar->m_Artist = token;
				}
				token = wcstok(NULL, L"\t");
				if (token)
				{
					foobar->m_Album = token;
				}
				token = wcstok(NULL, L"\t");
				if (token)
				{
					foobar->m_Duration = _wtoi(token);
				}
				token = wcstok(NULL, L"\t");
				if (token)
				{
					foobar->m_Rating = _wtoi(token);
				}
				token = wcstok(NULL, L"\t");
				if (token)
				{
					if (wcscmp(token, foobar->m_FilePath.c_str()) != 0)
					{
						// If different file
						foobar->m_FilePath = token;
						foobar->m_TrackChanged = true;
						foobar->m_Position = 0;
						foobar->GetCoverArt(token);
					}
				}
			}
		}
		return 0;

	default:
		return DefWindowProc(hwnd, msg, wParam, lParam);
	}
}

/*
** GetCoverArt
**
** Try to find cover art for file.
**
*/
void CPlayerFoobar::GetCoverArt(LPTSTR filename)
{
	// TODO: Fix temp solution
	if (m_HasCoverMeasure || m_InstanceCount == 0)
	{
		std::wstring cover = CreateCoverArtPath();
		if (_waccess(cover.c_str(), 0) == 0)
		{
			// Cover is in cache, lets use the that
			m_CoverPath = cover;
			return;
		}

		TagLib::FileRef fr(filename);
		if (!fr.isNull() && fr.tag() && GetEmbeddedArt(fr, cover))
		{
			// Embedded art found
			return;
		}

		// Get rid of the name and extension from filename
		std::wstring trackFolder = filename;
		std::wstring::size_type pos = trackFolder.find_last_of(L'\\');
		if (pos == std::wstring::npos) return;
		trackFolder.resize(++pos);

		if (GetLocalArt(trackFolder, L"cover") || GetLocalArt(trackFolder, L"folder"))
		{
			// Local art found
			return;
		}

		// Nothing found
		m_CoverPath.clear();
	}
}

/*
** UpdateData
**
** Called during each update of the main measure.
**
*/
void CPlayerFoobar::UpdateData()
{
	if (m_TrackChanged)
	{
		ExecuteTrackChangeAction();
		m_TrackChanged = false;
	}
}

/*
** PlayPause
**
** Handles the PlayPause bang.
**
*/
void CPlayerFoobar::Play()
{
	if (m_FooWindow)
	{
		SendMessage(m_FooWindow, WM_USER, 0, FOO_PLAY);
	}
}

/*
** PlayPause
**
** Handles the PlayPause bang.
**
*/
void CPlayerFoobar::PlayPause()
{
	if (m_FooWindow)
	{
		SendMessage(m_FooWindow, WM_USER, 0, FOO_PLAYPAUSE);
	}
}

/*
** Stop
**
** Handles the Stop bang.
**
*/
void CPlayerFoobar::Stop() 
{
	if (m_FooWindow)
	{
		SendMessage(m_FooWindow, WM_USER, 0, FOO_STOP);
	}
}

/*
** Next
**
** Handles the Next bang.
**
*/
void CPlayerFoobar::Next() 
{
	if (m_FooWindow)
	{
		SendMessage(m_FooWindow, WM_USER, 0, FOO_NEXT);
	}
}

/*
** Previous
**
** Handles the Previous bang.
**
*/
void CPlayerFoobar::Previous() 
{
	if (m_FooWindow)
	{
		SendMessage(m_FooWindow, WM_USER, 0, FOO_PREVIOUS);
	}
}

/*
** ChangeVolume
**
** Handles the ChangeVolume bang.
**
*/
void CPlayerFoobar::ChangeVolume(int volume) 
{
	if (m_FooWindow)
	{
		volume += m_Volume;
		SendMessage(m_FooWindow, WM_USER, volume, FOO_SETVOLUME);
	}
}

/*
** SetVolume
**
** Handles the SetVolume bang.
**
*/
void CPlayerFoobar::SetVolume(int volume) 
{
	if (m_FooWindow)
	{
		SendMessage(m_FooWindow, WM_USER, volume, FOO_SETVOLUME);
	}
}

/*
** ClosePlayer
**
** Handles the ClosePlayer bang.
**
*/
void CPlayerFoobar::ClosePlayer()
{
	if (m_FooWindow)
	{
		SendMessage(m_FooWindow, WM_USER, 0, FOO_QUITPLAYER);
	}
}

/*
** OpenPlayer
**
** Handles the OpenPlayer bang.
**
*/
void CPlayerFoobar::OpenPlayer()
{
	if (!m_FooWindow)
	{
		if (m_PlayerPath.empty())
		{
			// Gotta figure out where foobar2000 is located at
			HKEY hKey;
			RegOpenKeyEx(HKEY_CLASSES_ROOT,
						 L"Applications\\foobar2000.exe\\shell\\open\\command",
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
				if (type == REG_SZ && data[0] == L'\"')
				{
					std::wstring path = data;
					path.erase(0, 1);	// Get rid of the leading quote
					std::wstring::size_type pos = path.find_first_of(L'\"');

					if (pos != std::wstring::npos)
					{
						path.resize(pos);	// Get rid the last quote and everything after it
						ShellExecute(NULL, L"open", path.c_str(), NULL, NULL, SW_SHOW);
						m_PlayerPath = path;
					}
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
		SendMessage(m_FooWindow, WM_USER, 0, FOO_SHOWPLAYER);
	}
}

/*
** TogglePlayer
**
** Handles the TogglePlayer bang.
**
*/
void CPlayerFoobar::TogglePlayer()
{
	m_FooWindow ? ClosePlayer() : OpenPlayer();
}
