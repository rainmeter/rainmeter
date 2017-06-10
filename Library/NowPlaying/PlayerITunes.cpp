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
	m_RefCount(),
	m_ConnectionPoint(),
	m_ConnectionCookie()
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
	if (iid == IID_IUnknown || iid == IID_IUnknown || iid == DIID__IiTunesEvents)
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

HRESULT STDMETHODCALLTYPE PlayerITunes::CEventHandler::Invoke(DISPID dispidMember, REFIID, LCID, WORD, DISPPARAMS* dispParams, VARIANT*, EXCEPINFO*, UINT*)
{
	switch (dispidMember)
	{
	case ITEventDatabaseChanged:
		m_Player->OnDatabaseChange();
		break;

	case ITEventPlayerPlay:
		m_Player->OnStateChange(true);
		m_Player->OnTrackChange();
		break;

	case ITEventPlayerStop:
		m_Player->OnStateChange(false);
		break;

	case ITEventPlayerPlayingTrackChanged:
		m_Player->OnTrackChange();
		break;

	case ITEventSoundVolumeChanged:
		m_Player->OnVolumeChange(dispParams->rgvarg[0].intVal);
		break;

	case ITEventAboutToPromptUserToQuit:
		PostMessage(m_Player->m_CallbackWindow, WM_USER, ITEventAboutToPromptUserToQuit, 0);
		SetTimer(m_Player->m_CallbackWindow, TIMER_CHECKACTIVE, 500, nullptr);
		break;
	}

	return S_OK;
}

/*
** Constructor.
**
*/
PlayerITunes::PlayerITunes() : Player(),
	m_CallbackWindow(),
	m_LastCheckTime(0),
	m_iTunesActive(false),
	m_iTunes(),
	m_iTunesEvent()
{
	// Create windows class
	WNDCLASS wc = {0};
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

		// Set up event handler
		m_iTunesEvent = new CEventHandler(this);

		// Try getting track info and player state
		ITPlayerState state;
		if (SUCCEEDED(m_iTunes->get_PlayerState(&state)))
		{
			if (state == ITPlayerStateStopped)
			{
				// Determine if paused of stopped
				long position = 0;
				m_iTunes->get_PlayerPosition(&position);

				if (position != 0)
				{
					m_State = STATE_PAUSED;
					OnTrackChange();
				}
			}
			else if (state == ITPlayerStatePlaying)
			{
				m_State = STATE_PLAYING;
				OnTrackChange();
			}
		}

		long volume;
		m_iTunes->get_SoundVolume(&volume);
		m_Volume = (UINT)volume;

		OnDatabaseChange();
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
		return 0;

	case WM_USER:
		if (wParam == ITEventAboutToPromptUserToQuit)
		{
			// Event handler calls this through a PostMessage when iTunes quits
			player->Uninitialize();
		}
		return 0;

	case WM_TIMER:
		if (wParam == TIMER_CHECKACTIVE)
		{
			if (!FindWindow(L"iTunesApp", L"iTunes") && !FindWindow(L"iTunes", L"iTunes"))
			{
				player->m_iTunesActive = false;
				KillTimer(hwnd, TIMER_CHECKACTIVE);
			}
		}
		return 0;

	default:
		return DefWindowProc(hwnd, msg, wParam, lParam);
	}
}

/*
** Try to find iTunes periodically.
**
*/
bool PlayerITunes::CheckWindow()
{
	DWORD time = GetTickCount();
	if (time - m_LastCheckTime > 5000)
	{
		m_LastCheckTime = time;

		if ((FindWindow(L"iTunesApp", L"iTunes") || FindWindow(L"iTunes", L"iTunes")) && !m_iTunesActive)
		{
			m_iTunesActive = true;
			Initialize();
		}
	}

	return m_Initialized;
}

/*
** Called during each update of the main measure.
**
*/
void PlayerITunes::UpdateData()
{
	if ((m_Initialized || CheckWindow()) && m_State != STATE_STOPPED)
	{
		long position = 0;
		m_iTunes->get_PlayerPosition(&position);
		m_Position = (UINT)position;
	}
}

/*
** Called by iTunes event handler when the database is changed.
**
*/
void PlayerITunes::OnDatabaseChange()
{
	// Check the shuffle state. TODO: Find better way
	IITPlaylist* playlist;
	HRESULT hr = m_iTunes->get_CurrentPlaylist(&playlist);
	if (SUCCEEDED(hr) && playlist)
	{
		VARIANT_BOOL shuffle;
		hr = playlist->get_Shuffle(&shuffle);
		if (SUCCEEDED(hr))
		{
			m_Shuffle = shuffle != VARIANT_FALSE;
		}

		playlist->Release();
	}
}

/*
** Called by iTunes event handler on track change.
**
*/
void PlayerITunes::OnTrackChange()
{
	IITTrack* track;
	HRESULT hr = m_iTunes->get_CurrentTrack(&track);
	if (SUCCEEDED(hr) && track)
	{
		BSTR tmpStr;
		long tmpVal;

		// Get metadata
		track->get_Artist(&tmpStr);
		tmpStr ? (m_Artist = tmpStr) : m_Artist.clear();

		track->get_Name(&tmpStr);
		tmpStr ? (m_Title = tmpStr) : m_Title.clear();

		track->get_Album(&tmpStr);
		tmpStr ? (m_Album = tmpStr) : m_Album.clear();

		track->get_Genre(&tmpStr);;
		tmpStr ? (m_Genre = tmpStr) : m_Genre.clear();

		track->get_Duration(&tmpVal);
		m_Duration = (UINT)tmpVal;

		// Rating is 0 - 100, divide to 0 - 5
		track->get_Rating(&tmpVal);
		tmpVal /= 20L;
		m_Rating = (UINT)tmpVal;

		track->get_TrackNumber(&tmpVal);
		m_Number = (UINT)tmpVal;

		track->get_Year(&tmpVal);
		m_Year = (UINT)tmpVal;

		IITPlaylist* playlist;
		hr = track->get_Playlist(&playlist);
		if (SUCCEEDED(hr))
		{
			ITPlaylistRepeatMode repeat;
			hr = playlist->get_SongRepeat(&repeat);
			if (SUCCEEDED(hr))
			{
				m_Repeat = repeat != ITPlaylistRepeatModeOff;
			}

			playlist->Release();
		}

		IITFileOrCDTrack* file;
		hr = track->QueryInterface(&file);
		if (SUCCEEDED(hr))
		{
			file->get_Location(&tmpStr);
			file->Release();
			if (tmpStr && wcscmp(tmpStr, m_FilePath.c_str()) != 0)
			{
				++m_TrackCount;
				m_FilePath = tmpStr;
			}
		}

		if (m_Measures & MEASURE_COVER)
		{
			m_CoverPath.clear();

			// Check for embedded art through iTunes interface
			IITArtworkCollection* artworkCollection;
			hr = track->get_Artwork(&artworkCollection);

			if (SUCCEEDED(hr))
			{
				long count;
				artworkCollection->get_Count(&count);

				if (count > 0)
				{
					IITArtwork* artwork;
					hr = artworkCollection->get_Item(1, &artwork);

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
	else
	{
		ClearData(false);
	}
}

/*
** Called by iTunes event handler on player state change.
**
*/
void PlayerITunes::OnStateChange(bool playing)
{
	if (playing)
	{
		m_State = STATE_PLAYING;
	}
	else
	{
		long position = 0;
		m_iTunes->get_PlayerPosition(&position);

		// Guess if paused or stopped from track time
		m_State = (position == 0) ? STATE_STOPPED : STATE_PAUSED;
		if (m_State == STATE_STOPPED)
		{
			ClearData(false);
		}
	}
}

/*
** Called by iTunes event handler on volume change.
**
*/
void PlayerITunes::OnVolumeChange(int volume)
{
	m_Volume = volume;
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
	IITTrack* track;
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
	IITTrack* track;
	HRESULT hr = m_iTunes->get_CurrentTrack(&track);
	if (SUCCEEDED(hr) && track)
	{
		IITPlaylist* playlist;
		hr = track->get_Playlist(&playlist);
		if (SUCCEEDED(hr))
		{
			m_Shuffle = state;
			VARIANT_BOOL shuffle = m_Shuffle ? VARIANT_TRUE : VARIANT_FALSE;
			playlist->put_Shuffle(shuffle);

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
	IITTrack* track;
	HRESULT hr = m_iTunes->get_CurrentTrack(&track);
	if (SUCCEEDED(hr) && track)
	{
		IITPlaylist* playlist;
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
	Uninitialize();
	SetTimer(m_CallbackWindow, TIMER_CHECKACTIVE, 500, nullptr);
}

/*
** Handles the OpenPlayer bang.
**
*/
void PlayerITunes::OpenPlayer(std::wstring& path)
{
	ShellExecute(nullptr, L"open", path.empty() ? L"iTunes.exe" : path.c_str(), nullptr, nullptr, SW_SHOW);
}
