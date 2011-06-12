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
#include "PlayerWLM.h"

extern CPlayer* g_WLM;

// This player emulates the MSN/WLM Messenger 'Listening to' interface, which is
// supported by OpenPandora, Last.fm, Media Player Classic, TTPlayer, etc.

/*
** CPlayerWLM
**
** Constructor.
**
*/
CPlayerWLM::CPlayerWLM() : CPlayer(),
	m_Window()
{
	HINSTANCE hInstance = GetModuleHandle(NULL);

	// Create windows class
	WNDCLASS wc = {0};
	wc.hInstance = hInstance;
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
							NULL,
							NULL,
							hInstance,
							this);
}

/*
** ~CPlayerWLM
**
** Destructor.
**
*/
CPlayerWLM::~CPlayerWLM()
{
	DestroyWindow(m_Window);
	UnregisterClass(L"MsnMsgrUIManager", GetModuleHandle(NULL));
}

/*
** AddInstance
**
** Called during initialization of each measure.
**
*/
void CPlayerWLM::AddInstance(MEASURETYPE type)
{
	++m_InstanceCount;
}

/*
** RemoveInstance
**
** Called during destruction of each measure.
**
*/
void CPlayerWLM::RemoveInstance()
{
	if (--m_InstanceCount == 0)
	{
		g_WLM = NULL;
		delete this;
	}
}

LRESULT CALLBACK CPlayerWLM::WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	static CPlayerWLM* player;

	switch (msg)
	{
	case WM_CREATE:
		{
			// Get pointer to the CPlayerWLM class from the CreateWindow call
			player = (CPlayerWLM*)((CREATESTRUCT*)lParam)->lpCreateParams;
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
				player->m_State = PLAYER_PLAYING;
				data.erase(0, 3);	// Get rid of the status

				len = data.find_first_of(L'\\');
				len += 2;
				data.erase(0, len); // Get rid of the format

				len = data.find_first_of(L'\\');
				player->m_Title = data.substr(0, len);
				len += 2;
				data.erase(0, len);

				len = data.find_first_of(L'\\');
				player->m_Artist = data.substr(0, len);
				len += 2;
				data.erase(0, len);

				len = data.find_first_of(L'\\');
				player->m_Album = data.substr(0, len);
			}
			else
			{
				player->ClearInfo();
			}

			return 0;
		}
	}

	return DefWindowProc(hWnd, msg, wParam, lParam);
}

void CPlayerWLM::SendKeyInput(WORD key)
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
** UpdateData
**
** Called during each update of the main measure.
**
*/
void CPlayerWLM::UpdateData()
{
}

/*
** PlayPause
**
** Handles the PlayPause bang.
**
*/
void CPlayerWLM::PlayPause()
{
	SendKeyInput(VK_MEDIA_PLAY_PAUSE);
}

/*
** Stop
**
** Handles the Stop bang.
**
*/
void CPlayerWLM::Stop() 
{
	SendKeyInput(VK_MEDIA_STOP);
}

/*
** Next
**
** Handles the Next bang.
**
*/
void CPlayerWLM::Next() 
{
	SendKeyInput(VK_MEDIA_NEXT_TRACK);
}

/*
** Previous
**
** Handles the Previous bang.
**
*/
void CPlayerWLM::Previous() 
{
	SendKeyInput(VK_MEDIA_PREV_TRACK);
}
