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
#include "PlayerWinamp.h"
#include "Winamp/wa_ipc.h"
#include "Winamp/wa_cmd.h"

extern CPlayer* g_Winamp;

// This player retrieves data through the Winamp IPC interface.

/*
** CPlayerWinamp
**
** Constructor.
**
*/
CPlayerWinamp::CPlayerWinamp(WINAMPTYPE type) : CPlayer(),
	m_WinampType(type),
	m_UseUnicodeAPI(false),
	m_HasCoverMeasure(false),
	m_Window()
{
	Initialize();
}

/*
** ~CPlayerWinamp
**
** Destructor.
**
*/
CPlayerWinamp::~CPlayerWinamp()
{
	if (m_WinampHandle) CloseHandle(m_WinampHandle);
}

/*
** AddInstance
**
** Called during initialization of each measure.
**
*/
void CPlayerWinamp::AddInstance(MEASURETYPE type)
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
void CPlayerWinamp::RemoveInstance()
{
	if (--m_InstanceCount == 0)
	{
		g_Winamp = NULL;
		delete this;
	}
}

/*
** Initialize
**
** Get things ready with Winamp.
**
*/
bool CPlayerWinamp::Initialize()
{
	m_Window = FindWindow(L"Winamp v1.x", NULL);

	if (m_Window)
	{
		DWORD pID;
		GetWindowThreadProcessId(m_Window, &pID);
		m_WinampHandle = OpenProcess(PROCESS_VM_READ, false, pID);

		if (m_WinampHandle)
		{
			m_WinampAddress = (LPCVOID)SendMessage(m_Window, WM_WA_IPC, 0, IPC_GET_PLAYING_FILENAME);
			m_UseUnicodeAPI = m_WinampAddress ? true : false;
			return true;
		}
	}

	return false;
}

/*
** CheckActive
**
** Check if Winamp is active.
**
*/
bool CPlayerWinamp::CheckActive()
{
	if (m_Window)
	{
		if (!IsWindow(m_Window))
		{
			m_Window = NULL;
			CloseHandle(m_WinampHandle);
			ClearInfo();

			return false;
		}
		
		return true;
	}
	else
	{
		static DWORD oldTime = 0;
		DWORD time = GetTickCount();
		
		// Try to find Winamp window every 5 seconds
		if (time - oldTime > 5000)
		{
			oldTime = time;
			return Initialize();
		}

		return false;
	}
}

/*
** UpdateData
**
** Called during each update of the main measure.
**
*/
void CPlayerWinamp::UpdateData()
{
	if (!CheckActive()) return; // Make sure Winamp is running

	if (m_TrackChanged)
	{
		ExecuteTrackChangeAction();
		m_TrackChanged = false;
	}

	int playing = SendMessage(m_Window, WM_WA_IPC, 0, IPC_ISPLAYING);
	if (playing == 0)
	{
		if (!m_FilePath.empty())
		{
			ClearInfo();
		}
		return;	// Don't continue if stopped
	}
	else
	{
		m_State = (playing == 1) ? PLAYER_PLAYING : PLAYER_PAUSED;
		m_Position = SendMessage(m_Window, WM_WA_IPC, 0, IPC_GETOUTPUTTIME) / 1000; // Returns ms, make seconds

		float volume = SendMessage(m_Window, WM_WA_IPC, -666, IPC_SETVOLUME);
		volume /= 2.55f;
		m_Volume = (UINT)volume;
	}

	BOOL ret;
	WCHAR wBuffer[MAX_PATH];
	char cBuffer[MAX_PATH];

	if (m_UseUnicodeAPI)
	{
		ret = ReadProcessMemory(m_WinampHandle, m_WinampAddress, &wBuffer, MAX_PATH, NULL);
	}
	else
	{
		// MediaMonkey doesn't support wide IPC messages
		int pos = SendMessage(m_Window, WM_WA_IPC, 0, IPC_GETLISTPOS);
		LPCVOID address = (LPCVOID)SendMessage(m_Window, WM_WA_IPC, pos, IPC_GETPLAYLISTFILE);
		ret = ReadProcessMemory(m_WinampHandle, address, &cBuffer, MAX_PATH, NULL);
		mbstowcs(wBuffer, cBuffer, MAX_PATH);
	}

	if (!ret)
	{
		LSLog(LOG_ERROR, L"Rainmeter", L"NowPlayingPlugin: Failed to read Winamp memory (file).");
		return;
	}

	if (wcscmp(wBuffer, m_FilePath.c_str()) != 0)
	{
		m_TrackChanged = true;
		m_FilePath = wBuffer;
		m_Rating = SendMessage(m_Window, WM_WA_IPC, 0, IPC_GETRATING);
		m_Duration = SendMessage(m_Window, WM_WA_IPC, 1, IPC_GETOUTPUTTIME);

		TagLib::FileRef fr(wBuffer);
		if (!fr.isNull() && fr.tag())
		{
			TagLib::Tag* tag = fr.tag();
			m_Artist = tag->artist().toWString();
			m_Album = tag->album().toWString();
			m_Title = tag->title().toWString();
		}
		else
		{
			// TagLib couldn't parse the file, try title instead
			if (m_UseUnicodeAPI)
			{
				LPCVOID address = (LPCVOID)SendMessage(m_Window, WM_WA_IPC, 0, IPC_GET_PLAYING_TITLE);
				ret = ReadProcessMemory(m_WinampHandle, address, &wBuffer, MAX_PATH, NULL);
			}
			else
			{
				int pos = SendMessage(m_Window, WM_WA_IPC, 0, IPC_GETLISTPOS);
				LPCVOID address = (LPCVOID)SendMessage(m_Window, WM_WA_IPC, pos, IPC_GETPLAYLISTTITLE);
				ReadProcessMemory(m_WinampHandle, m_WinampAddress, &cBuffer, MAX_PATH, NULL);
				ret = mbstowcs(wBuffer, cBuffer, MAX_PATH);
			}

			if (!ret)
			{
				LSLog(LOG_ERROR, L"Rainmeter", L"NowPlayingPlugin: Failed to read Winamp memory (title).");
				return;
			}

			std::wstring title = wBuffer;
			std::wstring::size_type pos = title.find(L". ");

			if (pos != std::wstring::npos && pos < 5)
			{
				pos += 2; // Skip ". "
				title.erase(0, pos);
			}

			pos = title.find(L" - ");
			if (pos != std::wstring::npos)
			{
				m_Title = title.substr(0, pos);
				pos += 3;	// Skip " - "
				m_Artist = title.substr(pos);
				m_Album.clear();
			}
			else
			{
				ClearInfo();
				return;
			}
		}

		if (m_HasCoverMeasure)
		{
			if (GetCachedArt() || GetEmbeddedArt(fr))
			{
				// Art found in cache or embedded in file
				return;
			}

			// Get rid of the name and extension from filename
			std::wstring trackFolder = m_FilePath;
			std::wstring::size_type pos = trackFolder.find_last_of(L'\\');
			if (pos == std::wstring::npos) return;
			trackFolder.resize(++pos);

			if (!m_Album.empty())
			{
				std::wstring file = m_Album;
				std::wstring::size_type end = file.length();
				for (pos = 0; pos < end; ++pos)
				{
					// Replace reserved chars according to Winamp specs
					switch (file[pos])
					{
					case L'?':
					case L'*':
					case L'|':
						file[pos] = L'_';
						break;

					case L'/':
					case L'\\':
					case L':':
						file[pos] = L'-';
						break;

					case L'\"':
						file[pos] = L'\'';
						break;

					case L'<':
						file[pos] = L'(';
						break;

					case L'>':
						file[pos] = L')';
						break;
					}
				}

				if (GetLocalArt(trackFolder, file))
				{
					// %album% art file found
					return;
				}
			}

			if (GetLocalArt(trackFolder, L"cover") || GetLocalArt(trackFolder, L"folder"))
			{
				// Local art found
				return;
			}

			// Nothing found
			m_CoverPath.clear();
		}
	}
}

/*
** Pause
**
** Handles the Pause bang.
**
*/
void CPlayerWinamp::Pause()
{
	if (m_Window)
	{
		SendMessage(m_Window, WM_COMMAND, WINAMP_PAUSE, 0);
	}
}

/*
** Play
**
** Handles the Play bang.
**
*/
void CPlayerWinamp::Play()
{
	if (m_Window)
	{
		SendMessage(m_Window, WM_COMMAND, WINAMP_PLAY, 0);
	}
}

/*
** PlayPause
**
** Handles the PlayPause bang.
**
*/
void CPlayerWinamp::PlayPause()
{
	(m_State == PLAYER_PLAYING) ? Pause() : Play();
}

/*
** Stop
**
** Handles the Stop bang.
**
*/
void CPlayerWinamp::Stop()
{
	if (m_Window)
	{
		SendMessage(m_Window, WM_COMMAND, WINAMP_STOP, 0);
	}
}

/*
** Next
**
** Handles the Next bang.
**
*/
void CPlayerWinamp::Next() 
{
	if (m_Window)
	{
		SendMessage(m_Window, WM_COMMAND, WINAMP_FASTFWD, 0);
	}
}

/*
** Previous
**
** Handles the Previous bang.
**
*/
void CPlayerWinamp::Previous()
{
	if (m_Window)
	{
		SendMessage(m_Window, WM_COMMAND, WINAMP_REWIND, 0);
	}
}

/*
** SetPosition
**
** Handles the SetPosition bang.
**
*/
void CPlayerWinamp::SetPosition(int position)
{
	if (m_Window)
	{
		position *= 1000; // To milliseconds
		SendMessage(m_Window, WM_WA_IPC, position, IPC_JUMPTOTIME);
	}
}

/*
** SetRating
**
** Handles the SetRating bang.
**
*/
void CPlayerWinamp::SetRating(int rating)
{
	if (m_Window && (m_State != PLAYER_STOPPED))
	{
		if (rating < 0)
		{
			rating = 0;
		}
		else if (rating > 5)
		{
			rating = 5;
		}

		SendMessage(m_Window, WM_WA_IPC, rating, IPC_SETRATING);
		m_Rating = rating;
	}
}

/*
** SetVolume
**
** Handles the SetVolume bang.
**
*/
void CPlayerWinamp::SetVolume(int volume)
{
	if (m_Window)
	{
		++volume;	// For proper scaling
		if (volume < 0)
		{
			volume = 0;
		}
		else if (volume > 100)
		{
			volume = 100;
		}

		// Winamp accepts volume in 0 - 255 range
		float fVolume = (float)volume;
		fVolume *= 2.55f;
		volume = (UINT)fVolume;

		SendMessage(m_Window, WM_WA_IPC, volume, IPC_SETVOLUME);
	}
}

/*
** ClosePlayer
**
** Handles the ClosePlayer bang.
**
*/
void CPlayerWinamp::ClosePlayer()
{
	if (m_Window)
	{
		if (m_WinampType == WA_WINAMP)
		{
			SendMessage(m_Window, WM_CLOSE, 0, 0);
		}
		else // if (m_WinampType == WA_MEDIAMONKEY)
		{
			HWND wnd = FindWindow(L"TFMainWindow", L"MediaMonkey");
			if (wnd)
			{
				SendMessage(wnd, WM_CLOSE, 0, 0);
			}
		}
	}
}

/*
** OpenPlayer
**
** Handles the OpenPlayer bang.
**
*/
void CPlayerWinamp::OpenPlayer()
{
	if (m_WinampType == WA_WINAMP)
	{
		ShellExecute(NULL, L"open", m_PlayerPath.empty() ? L"winamp.exe" : m_PlayerPath.c_str(), NULL, NULL, SW_SHOW);
	}
	else // if (m_WinampType == WA_MEDIAMONKEY)
	{
		if (m_PlayerPath.empty())
		{
			// Gotta figure out where Winamp is located at
			HKEY hKey;
			RegOpenKeyEx(HKEY_LOCAL_MACHINE,
						 L"SOFTWARE\\Clients\\Media\\MediaMonkey\\shell\\open\\command",
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
					ShellExecute(NULL, L"open", data, NULL, NULL, SW_SHOW);
					m_PlayerPath = data;
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
}

/*
** TogglePlayer
**
** Handles the TogglePlayer bang.
**
*/
void CPlayerWinamp::TogglePlayer()
{
	m_Window ? ClosePlayer() : OpenPlayer();
}
