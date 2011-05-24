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

/*
** CPlayerWinamp
**
** Constructor.
**
*/
CPlayerWinamp::CPlayerWinamp() : CPlayer(),
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
		g_Winamp = NULL;
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
		m_WinampAddress = (LPCVOID)SendMessage(m_Window, WM_WA_IPC, 0, IPC_GET_PLAYING_FILENAME);

		if (m_WinampHandle)
		{
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

	WCHAR buffer[MAX_PATH];
	if (!ReadProcessMemory(m_WinampHandle, m_WinampAddress, &buffer, MAX_PATH, NULL))
	{
		LSLog(LOG_ERROR, L"Rainmeter", L"NowPlayingPlugin: Failed to read Winamp memory");
	}
	else if (wcscmp(buffer, m_FilePath.c_str()) != 0)
	{
		m_TrackChanged = true;
		m_FilePath = buffer;
		m_Rating = SendMessage(m_Window, WM_WA_IPC, 0, IPC_GETRATING);
		m_Duration = SendMessage(m_Window, WM_WA_IPC, 1, IPC_GETOUTPUTTIME);

		TagLib::FileRef fr(buffer);
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
			LPCVOID address = (LPCVOID)SendMessage(m_Window, WM_WA_IPC, 0, IPC_GET_PLAYING_TITLE);
			if (ReadProcessMemory(m_WinampHandle, address, &buffer, MAX_PATH, NULL))
			{
				std::wstring title = buffer;
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
	if (m_Window)
	{
		SendMessage(m_Window, WM_COMMAND, (m_State == PLAYER_STOPPED) ? WINAMP_PLAY : WINAMP_PAUSE, 0);
	}
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
		if (volume > 100)
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
** ChangeVolume
**
** Handles the ChangeVolume bang.
**
*/
void CPlayerWinamp::ChangeVolume(int volume)
{
	if (m_Window)
	{
		++volume;	// For proper scaling
		volume += m_Volume;

		if (volume < 0)
		{
			volume = 0;
		}
		else if (volume > 100)
		{
			volume = 100;
		}

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
		SendMessage(m_Window, WM_CLOSE, 0, 0);
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
	ShellExecute(NULL, L"open", m_PlayerPath.empty() ? L"winamp.exe" : m_PlayerPath.c_str(), NULL, NULL, SW_SHOW);
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
