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
#include "PlayerWLM.h"

Player* PlayerWLM::c_Player = nullptr;
extern HINSTANCE g_Instance;

// This player emulates the MSN/WLM Messenger 'Listening to' interface, which is
// supported by OpenPandora, Last.fm, Media Player Classic, TTPlayer, Zune, etc.

/*
** Constructor.
**
*/
PlayerWLM::PlayerWLM() : Player(),
	m_Window()
{
	// Create windows class
	WNDCLASS wc = {0};
	wc.hInstance = g_Instance;
	wc.lpfnWndProc = WndProc;
	wc.lpszClassName = L"MsnMsgrUIManager";
	RegisterClass(&wc);

	// Create dummy window
	m_Window = CreateWindow(L"MsnMsgrUIManager",
							L"",
							WS_DISABLED,
							CW_USEDEFAULT,
							CW_USEDEFAULT,
							CW_USEDEFAULT,
							CW_USEDEFAULT,
							nullptr,
							nullptr,
							g_Instance,
							this);

	m_Initialized = true;
}

/*
** Destructor.
**
*/
PlayerWLM::~PlayerWLM()
{
	c_Player = nullptr;
	DestroyWindow(m_Window);
	UnregisterClass(L"MsnMsgrUIManager", g_Instance);
}

/*
** Creates a shared class object.
**
*/
Player* PlayerWLM::Create()
{
	if (!c_Player)
	{
		c_Player = new PlayerWLM();
	}

	return c_Player;
}

LRESULT CALLBACK PlayerWLM::WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	static PlayerWLM* player;

	switch (msg)
	{
	case WM_CREATE:
		{
			// Get pointer to the PlayerWLM class from the CreateWindow call
			player = (PlayerWLM*)((CREATESTRUCT*)lParam)->lpCreateParams;
			return 0;
		}

	case WM_COPYDATA:
		{
			PCOPYDATASTRUCT cds = (PCOPYDATASTRUCT)lParam;
			if (cds->dwData != 1351) return 0;

			// lpData: \0Music\0<status>\0<format>\0<title>\0<artist>\0<album>\0
			std::wstring data = (WCHAR*)cds->lpData;

			// Some players include player name in the beginning. Skip that.
			std::wstring::size_type len = data.find(L"\\0Music\\0");
			len += 9;
			data.erase(0, len); // Get rid of \0Music\0

			bool playing = (data[0] == L'1');

			if (playing)
			{
				++player->m_TrackCount;
				player->m_State = STATE_PLAYING;
				data.erase(0, 3);	// Get rid of the status

				// TODO: Handle invalid
				len = data.find_first_of(L'\\');
				len += 2;
				data.erase(0, len); // Get rid of the format

				len = data.find_first_of(L'\\');
				player->m_Title.assign(data, 0, len);
				len += 2;
				data.erase(0, len);

				len = data.find_first_of(L'\\');
				player->m_Artist.assign(data, 0, len);
				len += 2;
				data.erase(0, len);

				len = data.find_first_of(L'\\');
				player->m_Album.assign(data, 0, len);

				if (player->m_Measures & MEASURE_LYRICS)
				{
					player->FindLyrics();
				}
			}
			else
			{
				player->ClearData(false);
			}

			return 0;
		}
	}

	return DefWindowProc(hWnd, msg, wParam, lParam);
}

void PlayerWLM::SendKeyInput(WORD key)
{
	KEYBDINPUT kbi = {0};
	kbi.wVk = key;
	kbi.dwExtraInfo = (ULONG_PTR)GetMessageExtraInfo();

	INPUT input = {0};
	input.type = INPUT_KEYBOARD;
	input.ki = kbi;

	SendInput(1, &input, sizeof(INPUT));
}

/*
** Called during each update of the main measure.
**
*/
void PlayerWLM::UpdateData()
{
}

/*
** Handles the Play bang.
**
*/
void PlayerWLM::Play()
{
	SendKeyInput(VK_MEDIA_PLAY_PAUSE);
}

/*
** Handles the Stop bang.
**
*/
void PlayerWLM::Stop() 
{
	SendKeyInput(VK_MEDIA_STOP);
}

/*
** Handles the Next bang.
**
*/
void PlayerWLM::Next() 
{
	SendKeyInput(VK_MEDIA_NEXT_TRACK);
}

/*
** Handles the Previous bang.
**
*/
void PlayerWLM::Previous() 
{
	SendKeyInput(VK_MEDIA_PREV_TRACK);
}
