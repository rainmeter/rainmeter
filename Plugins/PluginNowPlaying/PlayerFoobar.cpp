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

#include "StdAfx.h"
#include "PlayerFoobar.h"

CPlayer* CPlayerFoobar::c_Player = NULL;
extern HINSTANCE g_Instance;

/*
** Constructor.
**
*/
CPlayerFoobar::CPlayerFoobar() : CPlayer(),
	m_Window(),
	m_FooWindow(),
	m_MaximizeOnStart(false)
{
	Initialize();
}

/*
** Constructor.
**
*/
CPlayerFoobar::~CPlayerFoobar()
{
	c_Player = NULL;
	Uninitialize();
}

/*
** Creates a shared class object.
**
*/
CPlayer* CPlayerFoobar::Create()
{
	if (!c_Player)
	{
		c_Player = new CPlayerFoobar();
	}

	return c_Player;
}

/*
** Create receiver window.
**
*/
void CPlayerFoobar::Initialize()
{
	// Create windows class
	WNDCLASS wc = {0};
	wc.hInstance = g_Instance;
	wc.lpfnWndProc = WndProc;
	wc.lpszClassName = L"NowPlayingFoobarClass";
	RegisterClass(&wc);

	// Create window
	m_Window = CreateWindow(L"NowPlayingFoobarClass",
							L"ReceiverWindow",
							WS_DISABLED,
							CW_USEDEFAULT,
							CW_USEDEFAULT,
							CW_USEDEFAULT,
							CW_USEDEFAULT,
							NULL,
							NULL,
							g_Instance,
							this);

	m_FooWindow = FindWindow(L"foo_rainmeter_class", NULL);
	if (m_FooWindow)
	{
		int version = (int)SendMessage(m_FooWindow, WM_USER, 0, FOO_GETVERSION);
		if (version < 100)
		{
			const WCHAR* error = L"Your copy of the foo_rainmeter.dll plugin for foobar2000 is outdated.\nDownload the latest version from foo-rainmeter.googlecode.com and try again.";
			MessageBox(NULL, error, L"Rainmeter", MB_OK | MB_ICONERROR | MB_TOPMOST);
			m_FooWindow = NULL;
		}
		else
		{
			m_Initialized = true;
			SendMessage(m_FooWindow, WM_USER, (WPARAM)m_Window, FOO_SETCALLBACK);
		}
	}
}

/*
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
	UnregisterClass(L"NowPlayingFoobarClass", g_Instance);
}

/*
** Window procedure for the reciever window.
**
*/
LRESULT CALLBACK CPlayerFoobar::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	static CPlayerFoobar* player;

	switch (msg)
	{
	case WM_CREATE:
		{
			// Get pointer to the CPlayerFoobar class from the CreateWindow call
			LPVOID params = ((CREATESTRUCT*)lParam)->lpCreateParams;
			player = (CPlayerFoobar*)params;
			return 0;
		}

	case WM_USER:
		switch (lParam)
		{
		case FOO_GETVERSION:
			return 100;

		case FOO_STATECHANGE:
			{
				StateType ps = (StateType)wParam;
				if (ps == STATE_STOPPED)
				{
					player->ClearData();
				}
				else
				{
					player->m_State = ps;
				}
			}
			break;

		case FOO_TIMECHANGE:
			player->m_Position = (UINT)wParam;
			break;

		case FOO_VOLUMECHANGE:
			player->m_Volume = (UINT)wParam;
			break;

		case FOO_PLAYERSTART:
			player->m_Initialized = true;
			player->m_FooWindow = (HWND)wParam;

			if (player->m_MaximizeOnStart)
			{
				SendMessage(player->m_FooWindow, WM_USER, 0, FOO_SHOWPLAYER);
				player->m_MaximizeOnStart = false;
			}
			break;

		case FOO_PLAYERQUIT:
			player->m_Initialized = false;
			player->ClearData();
			break;
		}
		return 0;

	case WM_COPYDATA:
		{
			PCOPYDATASTRUCT cds = (PCOPYDATASTRUCT)lParam;

			if (cds->dwData == FOO_TRACKCHANGE)
			{
				if (player->m_State != STATE_PLAYING)
				{
					player->m_State = STATE_PLAYING;
				}

				// In the format "TITLE ARTIST ALBUM LENGTH RATING" (seperated by \t)
				WCHAR buffer[1024];
				MultiByteToWideChar(CP_UTF8, 0, (char*)cds->lpData, cds->cbData, buffer, 1024);
				player->m_Artist = buffer;

				WCHAR* token = wcstok(buffer, L"\t");
				if (token)
				{
					if (wcscmp(token, L" ") == 0)
					{
						player->m_Title.clear();
					}
					else
					{
						player->m_Title = token;
					}
				}
				token = wcstok(NULL, L"\t");
				if (token)
				{
					if (wcscmp(token, L" ") == 0)
					{
						player->m_Artist.clear();
					}
					else
					{
						player->m_Artist = token;
					}
				}
				token = wcstok(NULL, L"\t");
				if (token)
				{
					if (wcscmp(token, L" ") == 0)
					{
						player->m_Album.clear();
					}
					else
					{
						player->m_Album = token;
					}
				}
				token = wcstok(NULL, L"\t");
				if (token)
				{
					player->m_Duration = _wtoi(token);
				}
				token = wcstok(NULL, L"\t");
				if (token)
				{
					player->m_Rating = _wtoi(token);
				}
				token = wcstok(NULL, L"\t");
				if (token && wcscmp(token, player->m_FilePath.c_str()) != 0)
				{
					// If different file
					++player->m_TrackCount;
					player->m_FilePath = token;
					player->m_Position = 0;

					if (player->m_Measures & MEASURE_COVER || player->m_InstanceCount == 0)
					{
						player->FindCover();
					}

					if (player->m_Measures & MEASURE_LYRICS)
					{
						player->FindLyrics();
					}
				}
				token = wcstok(NULL, L"\t");
				if (token)
				{
					switch (_wtoi(token))
					{
					case 0:
						player->m_Shuffle = player->m_Repeat = false;
						break;

					case 1:
						player->m_Shuffle = true;
						player->m_Repeat = false;
						break;

					case 2:
						player->m_Shuffle = false;
						player->m_Repeat = true;
						break;
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
** Called during each update of the main measure.
**
*/
void CPlayerFoobar::UpdateData()
{
}

/*
** Handles the Pause bang.
**
*/
void CPlayerFoobar::Pause()
{
	SendMessage(m_FooWindow, WM_USER, 0, FOO_PAUSE);
}

/*
** Handles the Play bang.
**
*/
void CPlayerFoobar::Play()
{
	SendMessage(m_FooWindow, WM_USER, 0, (m_State == STATE_PAUSED) ? FOO_PLAYPAUSE : FOO_PLAY);
}

/*
** Handles the Stop bang.
**
*/
void CPlayerFoobar::Stop() 
{
	SendMessage(m_FooWindow, WM_USER, 0, FOO_STOP);
}

/*
** Handles the Next bang.
**
*/
void CPlayerFoobar::Next() 
{
	SendMessage(m_FooWindow, WM_USER, 0, FOO_NEXT);
}

/*
** Handles the Previous bang.
**
*/
void CPlayerFoobar::Previous() 
{
	SendMessage(m_FooWindow, WM_USER, 0, FOO_PREVIOUS);
}

/*
** Handles the SetPosition bang.
**
*/
void CPlayerFoobar::SetPosition(int position)
{
	SendMessage(m_FooWindow, WM_USER, position, FOO_SETPOSITION);
}

/*
** Handles the SetShuffle bang.
**
*/
void CPlayerFoobar::SetShuffle(bool state)
{
	m_Shuffle = state;
	m_Repeat = state ? !m_Shuffle : m_Shuffle;
	SendMessage(m_FooWindow, WM_USER, state ? 1 : 0, FOO_SETORDER);
}

/*
** Handles the SetRepeat bang.
**
*/
void CPlayerFoobar::SetRepeat(bool state)
{
	m_Repeat = state;
	m_Shuffle = state ? !m_Repeat : m_Repeat;
	SendMessage(m_FooWindow, WM_USER, state ? 2 : 0, FOO_SETORDER);
}

/*
** Handles the SetVolume bang.
**
*/
void CPlayerFoobar::SetVolume(int volume) 
{
	SendMessage(m_FooWindow, WM_USER, volume, FOO_SETVOLUME);
}

/*
** Handles the ClosePlayer bang.
**
*/
void CPlayerFoobar::ClosePlayer()
{
	SendMessage(m_FooWindow, WM_USER, 0, FOO_QUITPLAYER);
}

/*
** Handles the OpenPlayer bang.
**
*/
void CPlayerFoobar::OpenPlayer(std::wstring& path)
{
	if (!m_Initialized)
	{
		if (path.empty())
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
					path = data;
					path.erase(0, 1);	// Get rid of the leading quote
					std::wstring::size_type pos = path.find_first_of(L'\"');

					if (pos != std::wstring::npos)
					{
						path.resize(pos);	// Get rid the last quote and everything after it
						ShellExecute(NULL, L"open", path.c_str(), NULL, NULL, SW_SHOW);
						path = path;
					}
					else
					{
						path.clear();
					}
				}
			}

			delete [] data;
			RegCloseKey(hKey);
		}
		else
		{
			ShellExecute(NULL, L"open", path.c_str(), NULL, NULL, SW_SHOW);
			m_MaximizeOnStart = true;
		}
	}
	else
	{
		SendMessage(m_FooWindow, WM_USER, 0, FOO_SHOWPLAYER);
	}
}
