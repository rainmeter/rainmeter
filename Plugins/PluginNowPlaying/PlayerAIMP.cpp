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
#include "PlayerAIMP.h"
#include "AIMP/aimp2_sdk.h"
#include "Winamp/wa_ipc.h"

CPlayer* CPlayerAIMP::c_Player = NULL;

/*
** CPlayerAIMP
**
** Constructor.
**
*/
CPlayerAIMP::CPlayerAIMP() : CPlayer(),
	m_Window(),
	m_WinampWindow(),
	m_LastCheckTime(0),
	m_LastFileSize(0),
	m_LastTitleSize(0),
	m_FileMap(),
	m_FileMapHandle()
{
}

/*
** ~CPlayerAIMP
**
** Destructor.
**
*/
CPlayerAIMP::~CPlayerAIMP()
{
	c_Player = NULL;
	if (m_FileMap) UnmapViewOfFile(m_FileMap);
	if (m_FileMapHandle) CloseHandle(m_FileMapHandle);
}

/*
** Create
**
** Creates a shared class object.
**
*/
CPlayer* CPlayerAIMP::Create()
{
	if (!c_Player)
	{
		c_Player = new CPlayerAIMP();
	}

	return c_Player;
}

/*
** CheckWindow
**
** Try to find AIMP periodically.
**
*/
bool CPlayerAIMP::CheckWindow()
{
	DWORD time = GetTickCount();

	// Try to find AIMP every 5 seconds
	if (time - m_LastCheckTime > 5000)
	{
		m_LastCheckTime = time;
		m_Window = FindWindow(L"AIMP2_RemoteInfo", L"AIMP2_RemoteInfo");

		if (m_Window)
		{
			m_WinampWindow = FindWindow(L"Winamp v1.x", NULL);

			m_FileMapHandle = OpenFileMapping(FILE_MAP_READ, FALSE, L"AIMP2_RemoteInfo");
			if (m_FileMapHandle)
			{
				m_FileMap = (LPVOID)MapViewOfFile(m_FileMapHandle, FILE_MAP_READ, 0, 0, 2048);
				if (m_FileMap)
				{
					m_Initialized = true;
				}
			}
		}
	}

	return m_Initialized;
}

/*
** UpdateData
**
** Called during each update of the main measure.
**
*/
void CPlayerAIMP::UpdateData()
{
	if (!m_Initialized)
	{
		if (m_LastTitleSize != 0)
		{
			m_LastFileSize = 0;
			m_LastTitleSize = 0;
		}

		if (!CheckWindow()) return;
	}

	// If initialized
	m_State = (PLAYSTATE)SendMessage(m_Window, WM_AIMP_COMMAND, WM_AIMP_STATUS_GET, AIMP_STS_Player);
	if (m_State == PLAYER_STOPPED)
	{
		// Make sure AIMP is still active
		if (!IsWindow(m_Window))
		{
			m_Initialized = false;
			if (m_FileMap) UnmapViewOfFile(m_FileMap);
			if (m_FileMapHandle) CloseHandle(m_FileMapHandle);
		}

		if (m_LastTitleSize != 0)
		{
			m_LastFileSize = 0;
			m_LastTitleSize = 0;
			ClearData();
		}

		// Don't continue if AIMP has quit or is stopped
		return;
	}

	m_Position = SendMessage(m_Window, WM_AIMP_COMMAND, WM_AIMP_STATUS_GET, AIMP_STS_POS);
	m_Volume = SendMessage(m_Window, WM_AIMP_COMMAND, WM_AIMP_STATUS_GET, AIMP_STS_VOLUME);

	AIMP2FileInfo* info = (AIMP2FileInfo*)m_FileMap;
	if (info->cbSizeOf > 0 &&
		info->nFileSize != m_LastFileSize	||	// Avoid reading the same file
		info->nTitleLen != m_LastTitleSize)
	{
		m_LastFileSize = info->nFileSize;
		m_LastTitleSize = info->nTitleLen;

		// 44 is sizeof(AIMP2FileInfo) / 2 (due to WCHAR being 16-bit).
		// Written explicitly due to size differences in 32bit/64bit.
		LPCTSTR stringData = (LPCTSTR)m_FileMap;
		stringData += 44;

		m_Album.assign(stringData, info->nAlbumLen);

		stringData += info->nAlbumLen;
		m_Artist.assign(stringData, info->nArtistLen);

		stringData += info->nArtistLen;
		stringData += info->nDateLen;
		std::wstring filepath(stringData, info->nFileNameLen);

		stringData += info->nFileNameLen;
		stringData += info->nGenreLen;
		m_Title.assign(stringData, info->nTitleLen);
		
		m_Duration = info->nDuration / 1000;

		m_Shuffle = (bool)SendMessage(m_Window, WM_AIMP_COMMAND, WM_AIMP_STATUS_GET, AIMP_STS_SHUFFLE);
		m_Repeat = (bool)SendMessage(m_Window, WM_AIMP_COMMAND, WM_AIMP_STATUS_GET, AIMP_STS_REPEAT);

		// Get rating through the AIMP Winamp API
		m_Rating = SendMessage(m_WinampWindow, WM_WA_IPC, 0, IPC_GETRATING);

		if (filepath != m_FilePath)
		{
			m_FilePath = filepath;
			++m_TrackCount;

			if (m_Measures & MEASURE_COVER) FindCover();

			if (m_Measures & MEASURE_LYRICS) FindLyrics();
		}
	}
}

/*
** Pause
**
** Handles the Pause bang.
**
*/
void CPlayerAIMP::Pause()
{
	SendMessage(m_Window, WM_AIMP_COMMAND, WM_AIMP_CALLFUNC, AIMP_PAUSE);
}

/*
** Play
**
** Handles the Play bang.
**
*/
void CPlayerAIMP::Play()
{
	SendMessage(m_Window, WM_AIMP_COMMAND, WM_AIMP_CALLFUNC, AIMP_PLAY);
}

/*
** Stop
**
** Handles the Stop bang.
**
*/
void CPlayerAIMP::Stop()
{
	SendMessage(m_Window, WM_AIMP_COMMAND, WM_AIMP_CALLFUNC, AIMP_STOP);
}

/*
** Next
**
** Handles the Next bang.
**
*/
void CPlayerAIMP::Next() 
{
	SendMessage(m_Window, WM_AIMP_COMMAND, WM_AIMP_CALLFUNC, AIMP_NEXT);
}

/*
** Previous
**
** Handles the Previous bang.
**
*/
void CPlayerAIMP::Previous()
{
	SendMessage(m_Window, WM_AIMP_COMMAND, WM_AIMP_CALLFUNC, AIMP_PREV);
}

/*
** SetPosition
**
** Handles the SetPosition bang.
**
*/
void CPlayerAIMP::SetPosition(int position)
{
	SendMessage(m_Window, WM_AIMP_COMMAND, WM_AIMP_STATUS_SET, MAKELPARAM(position, AIMP_STS_POS));
}

/*
** SetRating
**
** Handles the SetRating bang.
**
*/
void CPlayerAIMP::SetRating(int rating)
{
	// Set rating through the AIMP Winamp API
	if (m_State != PLAYER_STOPPED)
	{
		SendMessage(m_WinampWindow, WM_WA_IPC, rating, IPC_SETRATING);
		m_Rating = rating;
	}
}

/*
** SetVolume
**
** Handles the SetVolume bang.
**
*/
void CPlayerAIMP::SetVolume(int volume)
{
	SendMessage(m_Window, WM_AIMP_COMMAND, WM_AIMP_STATUS_SET, MAKELPARAM(volume, AIMP_STS_VOLUME));
}

/*
** SetShuffle
**
** Handles the SetShuffle bang.
**
*/
void CPlayerAIMP::SetShuffle(bool state)
{
	m_Shuffle = state;
	SendMessage(m_Window, WM_AIMP_COMMAND, WM_AIMP_STATUS_SET, MAKELPARAM(m_Shuffle, AIMP_STS_SHUFFLE));
}

/*
** SetRepeat
**
** Handles the SetRepeat bang.
**
*/
void CPlayerAIMP::SetRepeat(bool state)
{
	m_Repeat = state;
	SendMessage(m_Window, WM_AIMP_COMMAND, WM_AIMP_STATUS_SET, MAKELPARAM(m_Repeat, AIMP_STS_REPEAT));
}

/*
** ClosePlayer
**
** Handles the ClosePlayer bang.
**
*/
void CPlayerAIMP::ClosePlayer()
{
	SendMessage(m_Window, WM_CLOSE, 0, 0);
}

/*
** OpenPlayer
**
** Handles the OpenPlayer bang.
**
*/
void CPlayerAIMP::OpenPlayer(std::wstring& path)
{
	if (path.empty())
	{
		// Check for AIMP2 first
		DWORD size = 512;
		WCHAR* data = new WCHAR[size];
		DWORD type = 0;
		HKEY hKey;

		RegOpenKeyEx(HKEY_LOCAL_MACHINE,
						L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\AIMP2",
						0,
						KEY_QUERY_VALUE,
						&hKey);

		if (RegQueryValueEx(hKey,
							L"DisplayIcon",
							NULL,
							(LPDWORD)&type,
							(LPBYTE)data,
							(LPDWORD)&size) == ERROR_SUCCESS)
		{
			if (type == REG_SZ)
			{
				ShellExecute(NULL, L"open", data, NULL, NULL, SW_SHOW);
				path = data;
			}
		}
		else
		{
			// Let's try AIMP3
			RegCloseKey(hKey);
			RegOpenKeyEx(HKEY_LOCAL_MACHINE,
							L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\AIMP3",
							0,
							KEY_QUERY_VALUE,
							&hKey);

			if (RegQueryValueEx(hKey,
								L"DisplayIcon",
								NULL,
								(LPDWORD)&type,
								(LPBYTE)data,
								(LPDWORD)&size) == ERROR_SUCCESS)
			{
				if (type == REG_SZ)
				{
					path = data;
					path.resize(path.find_last_of(L'\\') + 1);
					path += L"AIMP3.exe";
					ShellExecute(NULL, L"open", path.c_str(), NULL, NULL, SW_SHOW);
				}
			}
		}

		delete [] data;
		RegCloseKey(hKey);
	}
	else
	{
		ShellExecute(NULL, L"open", path.c_str(), NULL, NULL, SW_SHOW);
	}
}
