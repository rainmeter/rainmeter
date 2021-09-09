/* Copyright (C) 2011 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "StdAfx.h"
#include "PlayerITunes.h"

Player* PlayerITunes::c_Player = nullptr;
extern HINSTANCE g_Instance;

/*
** Constructor.
**
*/
PlayerITunes::CEventHandler::CEventHandler(PlayerITunes* player) :
	m_Player(player),
	m_RefCount(0UL),
	m_ConnectionPoint(),
	m_ConnectionCookie(0UL)
{
	IConnectionPointContainer* icpc;
	m_Player->m_iTunes->QueryInterface(IID_IConnectionPointContainer, (void**)&icpc);
	icpc->FindConnectionPoint(DIID__IiTunesEvents, &m_ConnectionPoint);
	m_ConnectionPoint->Advise(this, &m_ConnectionCookie);
	icpc->Release();
}

/*
** Destructor.
**
*/
PlayerITunes::CEventHandler::~CEventHandler()
{
	if (m_ConnectionPoint)
	{
		m_ConnectionPoint->Unadvise(m_ConnectionCookie);
		m_ConnectionPoint->Release();
	}
}

HRESULT STDMETHODCALLTYPE PlayerITunes::CEventHandler::QueryInterface(REFIID iid, void** ppvObject)
{
	if (iid == IID_IUnknown || iid == DIID__IiTunesEvents)
	{
		++m_RefCount;
		*ppvObject = this;
		return S_OK;
	}

	return E_NOINTERFACE;
}

ULONG STDMETHODCALLTYPE PlayerITunes::CEventHandler::AddRef()
{
	return ++m_RefCount;
}

ULONG STDMETHODCALLTYPE PlayerITunes::CEventHandler::Release()
{
	return --m_RefCount;
}

/*
** Constructor.
**
*/
PlayerITunes::PlayerITunes() : Player(),
	m_CallbackWindow(),
	m_LastCheckTime(0UL),
	m_iTunesActive(false),
	m_iTunes(),
	m_iTunesEvent()
{
	// Create windows class
	WNDCLASS wc = { 0 };
	wc.hInstance = g_Instance;
	wc.lpfnWndProc = WndProc;
	wc.lpszClassName = L"NowPlayingITunesClass";
	RegisterClass(&wc);

	// Create callback window
	m_CallbackWindow = CreateWindow(L"NowPlayingITunesClass",
									L"CallbackWindow",
									WS_DISABLED,
									CW_USEDEFAULT,
									CW_USEDEFAULT,
									CW_USEDEFAULT,
									CW_USEDEFAULT,
									HWND_MESSAGE,
									nullptr,
									g_Instance,
									this);
}

/*
** Destructor.
**
*/
PlayerITunes::~PlayerITunes()
{
	c_Player = nullptr;

	DestroyWindow(m_CallbackWindow);
	UnregisterClass(L"NowPlayingITunesClass", g_Instance);

	Uninitialize();
}

/*
** Creates a shared class object.
**
*/
Player* PlayerITunes::Create()
{
	if (!c_Player)
	{
		c_Player = new PlayerITunes();
	}

	return c_Player;
}

/*
** Initialize iTunes COM interface and event handler.
**
*/
void PlayerITunes::Initialize()
{
	while (true)
	{
		HRESULT hr = CoCreateInstance(CLSID_iTunesApp, nullptr, CLSCTX_LOCAL_SERVER, IID_IiTunes, (PVOID*)&m_iTunes);
		if (hr == CO_E_SERVER_EXEC_FAILURE)
		{
			// This seems to happen if there is a modal dialog being shown in iTunes
			// or some other delay has occurred. Retrying should do the trick.
			continue;
		}
		else if (hr != S_OK)
		{
			// Failed to get hold of iTunes instance via COM
			m_iTunes = nullptr;
		}

		break;
	}

	if (m_iTunes)
	{
		m_Initialized = true;

		//Reset last trackID
		m_TrackID = -1L;

		// Try getting track info and player state
		ITPlayerState state = ITPlayerStateStopped;
		if (SUCCEEDED(m_iTunes->get_PlayerState(&state)))
		{
			if (state == ITPlayerStateStopped)
			{
				// Determine if paused or stopped
				long position = 0L;
				m_iTunes->get_PlayerPosition(&position);

				if (SUCCEEDED(m_iTunes->get_PlayerPosition(&position)) && position != 0L)
				{
					m_State = STATE_PAUSED;
				}
			}
			else if (state == ITPlayerStatePlaying)
			{
				m_State = STATE_PLAYING;
			}
		}
	}
	else
	{
		m_Initialized = false;
	}
}

/*
** Close iTunes COM interface.
**
*/
void PlayerITunes::Uninitialize()
{
	if (m_Initialized)
	{
		m_Initialized = false;
		ClearData();

		m_iTunes->Release();
		delete m_iTunesEvent;
	}
}

/*
** Window procedure for the callback window.
**
*/
LRESULT CALLBACK PlayerITunes::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	static PlayerITunes* player;

	switch (msg)
	{
	case WM_CREATE:
		// Get pointer to the PlayerITunes class from the CreateWindow call
		player = (PlayerITunes*)(((CREATESTRUCT*)lParam)->lpCreateParams);
		return 0L;
	}

	return DefWindowProc(hwnd, msg, wParam, lParam);
}

/*
** Try to find iTunes periodically.
**
*/
bool PlayerITunes::CheckWindow()
{
	ULONGLONG time = GetTickCount64();
	if (time - m_LastCheckTime > 5000ULL)
	{
		m_LastCheckTime = time;

		if ((FindWindow(L"iTunesApp", L"iTunes") || FindWindow(L"iTunes", L"iTunes")))
		{
			m_iTunesActive = true;
			if (!m_Initialized)
			{
				Initialize();
			}
		}
		else
		{
			m_iTunesActive = false;
		}
	}
	return m_iTunesActive;
}

/*
** Called during each update of the main measure.
**
*/
void PlayerITunes::UpdateData()
{
	if (CheckWindow())
	{
		// Update player state, reset 
		ITPlayerState state = ITPlayerStateStopped;
		if (SUCCEEDED(m_iTunes->get_PlayerState(&state)))
		{
			long position = 0L;
			if (SUCCEEDED(m_iTunes->get_PlayerPosition(&position)))
			{
				m_Position = (UINT)position;

			}
			if (state == ITPlayerStateStopped)
			{
				// Determine if paused or stopped
				if (position != 0L)
				{
					m_State = STATE_PAUSED;

				}
			}
			else if (state == ITPlayerStatePlaying)
			{
				m_State = STATE_PLAYING;
			}
			else
			{
				m_State = STATE_STOPPED;
			}
		}

		// Volume onChange was removed, manually check 
		long volume = 0L;
		if (SUCCEEDED(m_iTunes->get_SoundVolume(&volume)))
		{
			m_Volume = (UINT)volume;
		}

		// Check the shuffle and repeat state since there is no onChange event
		IITPlaylist* playlist = nullptr;
		HRESULT hr = m_iTunes->get_CurrentPlaylist(&playlist);
		if (SUCCEEDED(hr) && playlist)
		{
			VARIANT_BOOL shuffle = VARIANT_FALSE;
			hr = playlist->get_Shuffle(&shuffle);
			if (SUCCEEDED(hr))
			{
				m_Shuffle = shuffle != VARIANT_FALSE;
			}

			ITPlaylistRepeatMode repeat = ITPlaylistRepeatModeOff;
			hr = playlist->get_SongRepeat(&repeat);
			if (SUCCEEDED(hr))
			{
				m_Repeat = repeat != ITPlaylistRepeatModeOff;
			}

			playlist->Release();
		}

		// If playing a song check if metadata needs updated
		if (m_State != STATE_STOPPED)
		{
			UpdateCachedData();
		}
	}
	else if (m_Initialized)
	{
		// Since iTunes no longer supports status events, we need to Uninitialize
		// the measure if the window could not be found after initalization.
		Uninitialize();
	}
}

/*
** Called during measure update, for data that only needs updated on song change
**
*/
void PlayerITunes::UpdateCachedData()
{
	IITTrack* track = nullptr;
	HRESULT hr = m_iTunes->get_CurrentTrack(&track);
	if (SUCCEEDED(hr) && track)
	{
		BSTR tmpStr;
		long tmpVal = 0L;

		// Rating onChange was removed, manually check 
		if (SUCCEEDED(track->get_Rating(&tmpVal)))
		{
			tmpVal /= 20L;
			m_Rating = (UINT)tmpVal;
		}

		long trackID = -1L;
		hr = track->get_TrackID(&trackID);

		// If song is not current song
		if (SUCCEEDED(hr) && trackID != m_TrackID)
		{
			m_TrackID = trackID;
			//Increment song count since song count has changed
			++m_TrackCount;

			// Update various metadata
			track->get_Name(&tmpStr);
			tmpStr ? m_Title = tmpStr : m_Title.clear();
			track->get_Album(&tmpStr);
			tmpStr ? m_Album = tmpStr : m_Album.clear();
			track->get_Artist(&tmpStr);
			tmpStr ? m_Artist = tmpStr : m_Artist.clear();

			track->get_Genre(&tmpStr);;
			tmpStr ? (m_Genre = tmpStr) : m_Genre.clear();

			track->get_Duration(&tmpVal);
			m_Duration = (UINT)tmpVal;

			track->get_TrackNumber(&tmpVal);
			m_Number = (UINT)tmpVal;

			track->get_Year(&tmpVal);
			m_Year = (UINT)tmpVal;

			// Check if song still has file path
			IITFileOrCDTrack* file = nullptr;
			hr = track->QueryInterface(&file);
			if (SUCCEEDED(hr))
			{
				file->get_Location(&tmpStr);
				file->Release();
				tmpStr ? m_FilePath = tmpStr : m_FilePath.clear();
			}
			else
			{
				m_FilePath.clear();
			}

			// Update album art
			if (m_Measures & MEASURE_COVER)
			{
				m_CoverPath.clear();

				// Check for embedded art through iTunes interface
				IITArtworkCollection* artworkCollection;
				hr = track->get_Artwork(&artworkCollection);

				if (SUCCEEDED(hr))
				{
					long count = 0L;
					artworkCollection->get_Count(&count);

					if (count > 0L)
					{
						IITArtwork* artwork = nullptr;
						hr = artworkCollection->get_Item(1L, &artwork);

						if (SUCCEEDED(hr))
						{
							_bstr_t coverPath = m_TempCoverPath.c_str();
							hr = artwork->SaveArtworkToFile(coverPath);
							if (SUCCEEDED(hr))
							{
								m_CoverPath = m_TempCoverPath;
							}

							artwork->Release();
						}
					}

					artworkCollection->Release();
				}
			}

			if (m_Measures & MEASURE_LYRICS)
			{
				FindLyrics();
			}

			track->Release();
		}
	}
	else
	{
		ClearData(false);
	}
}

/*
** Handles the Pause bang.
**
*/
void PlayerITunes::Pause()
{
	m_iTunes->Pause();
}

/*
** Handles the Play bang.
**
*/
void PlayerITunes::Play()
{
	m_iTunes->Play();
}

/*
** Handles the Stop bang.
**
*/
void PlayerITunes::Stop() 
{
	m_iTunes->Stop();
}

/*
** Handles the Next bang.
**
*/
void PlayerITunes::Next() 
{
	m_iTunes->NextTrack();
}

/*
** Handles the Previous bang.
**
*/
void PlayerITunes::Previous() 
{
	m_iTunes->PreviousTrack();
}

/*
** Handles the SetPosition bang.
**
*/
void PlayerITunes::SetPosition(int position)
{
	m_iTunes->put_PlayerPosition((long)position);
}

/*
** Handles the SetRating bang.
**
*/
void PlayerITunes::SetRating(int rating)
{
	IITTrack* track = nullptr;
	HRESULT hr = m_iTunes->get_CurrentTrack(&track);
	if (SUCCEEDED(hr) && track)
	{
		rating *= 20;
		track->put_Rating((long)rating);
		track->Release();
	}
}

/*
** Handles the SetVolume bang.
**
*/
void PlayerITunes::SetVolume(int volume)
{
	m_iTunes->put_SoundVolume((long)volume);
}

/*
** Handles the SetShuffle bang.
**
*/
void PlayerITunes::SetShuffle(bool state)
{
	IITTrack* track = nullptr;
	HRESULT hr = m_iTunes->get_CurrentTrack(&track);
	if (SUCCEEDED(hr) && track)
	{
		IITPlaylist* playlist = nullptr;
		hr = track->get_Playlist(&playlist);
		if (SUCCEEDED(hr))
		{
			m_Shuffle = state;
			playlist->put_Shuffle(m_Shuffle ? VARIANT_TRUE : VARIANT_FALSE);

			playlist->Release();
		}

		track->Release();
	}
}

/*
** Handles the SetRepeat bang.
**
*/
void PlayerITunes::SetRepeat(bool state)
{
	IITTrack* track = nullptr;
	HRESULT hr = m_iTunes->get_CurrentTrack(&track);
	if (SUCCEEDED(hr) && track)
	{
		IITPlaylist* playlist = nullptr;
		hr = track->get_Playlist(&playlist);
		if (SUCCEEDED(hr))
		{
			m_Repeat = state;
			playlist->put_SongRepeat((ITPlaylistRepeatMode)m_Repeat);

			playlist->Release();
		}

		track->Release();
	}
}

/*
** Handles the ClosePlayer bang.
**
*/
void PlayerITunes::ClosePlayer()
{
	m_iTunes->Quit();
	m_iTunesActive = false;
	Uninitialize();
	SetTimer(m_CallbackWindow, TIMER_CHECKACTIVE, 500U, nullptr);
}

/*
** Handles the OpenPlayer bang.
**
*/
void PlayerITunes::OpenPlayer(std::wstring& path)
{
	ShellExecute(nullptr, L"open", path.empty() ? L"iTunes.exe" : path.c_str(), nullptr, nullptr, SW_SHOW);
}
