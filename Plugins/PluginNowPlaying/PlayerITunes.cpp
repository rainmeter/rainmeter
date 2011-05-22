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
#include "PlayerITunes.h"

extern std::wstring g_CachePath;

/*
** CEventHandler
**
** Constructor.
**
*/
CPlayerITunes::CEventHandler::CEventHandler(CPlayerITunes* player) :
	m_iTunes(player),
	m_TypeInfo(),
	m_RefCount()
{
	ITypeLib* pITypeLib = NULL;
	HRESULT hr = LoadRegTypeLib(LIBID_iTunesLib, 1, 5, 0x00, &pITypeLib);

	// Get type information for the interface of the object.
	hr = pITypeLib->GetTypeInfoOfGuid(DIID__IiTunesEvents, &m_TypeInfo);
	pITypeLib->Release();
}

/*
** ~CEventHandler
**
** Destructor.
**
*/
CPlayerITunes::CEventHandler::~CEventHandler()
{
}

HRESULT STDMETHODCALLTYPE CPlayerITunes::CEventHandler::QueryInterface(REFIID iid, void** ppvObject)
{
	if (iid == IID_IUnknown || iid == IID_IUnknown || iid == DIID__IiTunesEvents)
	{
		++m_RefCount;
		*ppvObject = this;
		return S_OK;
	}

	return E_NOINTERFACE;
}

ULONG STDMETHODCALLTYPE CPlayerITunes::CEventHandler::AddRef()
{
	InterlockedIncrement(&m_RefCount);
	return m_RefCount;
}

ULONG STDMETHODCALLTYPE CPlayerITunes::CEventHandler::Release()
{
	InterlockedDecrement(&m_RefCount);
	if (m_RefCount == 0)
	{
		delete this;
		return 0;
	}
	return m_RefCount;
}

HRESULT STDMETHODCALLTYPE CPlayerITunes::CEventHandler::Invoke(DISPID dispidMember, REFIID, LCID, WORD, DISPPARAMS* dispParams, VARIANT*, EXCEPINFO*, UINT*)
{
	switch (dispidMember)
	{
	case ITEventPlayerPlay:
		m_iTunes->OnStateChange(true);
		m_iTunes->OnTrackChange();
		break;	

	case ITEventPlayerStop:
		m_iTunes->OnStateChange(false);
		break;

	case ITEventPlayerPlayingTrackChanged:
		m_iTunes->OnTrackChange();
		break;

	case ITEventSoundVolumeChanged:
		m_iTunes->OnVolumeChange(dispParams->rgvarg[0].intVal);
		break;

	case ITEventAboutToPromptUserToQuit:
		m_iTunes->m_UserQuitPrompt = true;
		m_iTunes->Uninitialize();
		break;
	}

	return S_OK;
}

/*
** CPlayerITunes
**
** Constructor.
**
*/
CPlayerITunes::CPlayerITunes() : CPlayer(),
	m_Initialized(false),
	m_UserQuitPrompt(false),
	m_HasCoverMeasure(false),
	m_Window(),
	m_iTunes(),
	m_iTunesEvent(),
	m_ConnectionPoint(),
	m_ConnectionCookie()
{
	CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
}

/*
** ~CPlayerITunes
**
** Destructor.
**
*/
CPlayerITunes::~CPlayerITunes()
{
	Uninitialize();
	CoUninitialize();
}

/*
** AddInstance
**
** Called during initialization of each measure.
**
*/
void CPlayerITunes::AddInstance(MEASURETYPE type)
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
void CPlayerITunes::RemoveInstance()
{
	if (--m_InstanceCount == 0)
	{
		delete this;
	}
}

/*
** Initialize
**
** Initialize iTunes COM interface and event handler.
**
*/
void CPlayerITunes::Initialize()
{
	while (true)
	{
		HRESULT hr = CoCreateInstance(CLSID_iTunesApp, NULL, CLSCTX_LOCAL_SERVER, IID_IiTunes, (PVOID*)&m_iTunes);

		if (hr == CO_E_SERVER_EXEC_FAILURE)
		{
			// This seems to happen if there is a modal dialog being shown in iTunes
			// or some other delay has occurred. Retrying should do the trick.
			continue;
		}
		else if (hr != S_OK)
		{
			// Failed to get hold of iTunes instance via COM
			m_iTunes = NULL;
		}
			
		break;
	}

	if (m_iTunes)
	{
		m_Initialized = true;

		// Set up event handler
		IConnectionPointContainer* icpc;
		m_iTunes->QueryInterface(IID_IConnectionPointContainer, (void **)&icpc);
		icpc->FindConnectionPoint(DIID__IiTunesEvents, &m_ConnectionPoint);
		icpc->Release();
		m_iTunesEvent = new CEventHandler(this);
		m_ConnectionPoint->Advise(m_iTunesEvent, &m_ConnectionCookie);

		// Try getting track info and player state
		ITPlayerState state;
		if (SUCCEEDED(m_iTunes->get_PlayerState(&state)))
		{
			if (state == ITPlayerStateStopped)
			{
				// Determine if paused of stopped
				long position;
				m_iTunes->get_PlayerPosition(&position);

				if (position != 0)
				{
					m_State = PLAYER_PAUSED;
				}
			}
			else if (state == ITPlayerStatePlaying)
			{
				m_State = PLAYER_PLAYING;
			}

			if (m_State != PLAYER_STOPPED)
			{
				OnTrackChange();
			}

			long volume;
			m_iTunes->get_SoundVolume(&volume);
			m_Volume = (UINT)volume;
		}
	}
	else
	{
		m_Initialized = false;
		LSLog(LOG_ERROR, L"Rainmeter", L"NowPlayingPlugin: Failed to get hold of iTunes instance via COM.");
	}
}

/*
** Uninitialize
**
** Close iTunes COM interface.
**
*/
void CPlayerITunes::Uninitialize()
{
	if (m_Initialized)
	{
		m_Initialized = false;

		if (m_iTunes)
		{
			m_iTunes->Release();
			m_iTunesEvent->Release();
		}

		if (m_ConnectionPoint)
		{
			m_ConnectionPoint->Unadvise(m_ConnectionCookie);
			m_ConnectionPoint->Release();
		}

		ClearInfo();
	}
}

bool CPlayerITunes::CheckActive()
{
	static DWORD oldTime = 0;
	DWORD time = GetTickCount();

	if (time - oldTime > 5000)
	{
		oldTime = time;
		m_Window = FindWindow(L"iTunes", L"iTunes");
		return m_Window ? true : false;
	}

	return false;
}

/*
** UpdateData
**
** Called during each update of the main measure.
**
*/
void CPlayerITunes::UpdateData()
{
	if (m_Initialized)
	{
		if (m_TrackChanged)
		{
			ExecuteTrackChangeAction();
			m_TrackChanged = false;
		}

		long position;
		m_iTunes->get_PlayerPosition(&position);
		m_Position = (UINT)position;
	}
	else
	{
		if (CheckActive())
		{
			if (!m_UserQuitPrompt)
			{
				Initialize();
			}
		}
		else if (m_UserQuitPrompt)
		{
			m_UserQuitPrompt = false;
		}
	}
}

/*
** OnTrackChange
**
** Called by iTunes event handler on track change.
**
*/
void CPlayerITunes::OnTrackChange()
{
	IITTrack* track;
	HRESULT hr = m_iTunes->get_CurrentTrack(&track);

	if (SUCCEEDED(hr))
	{
		CComBSTR tmpStr;
		long tmpVal;

		// Get metadata
		track->get_Artist(&tmpStr);
		tmpStr ? (m_Artist = tmpStr) : m_Artist.clear();

		track->get_Name(&tmpStr);
		tmpStr ? (m_Title = tmpStr) : m_Title.clear();

		track->get_Album(&tmpStr);
		tmpStr ? (m_Album = tmpStr) : m_Album.clear();

		track->get_Duration(&tmpVal);
		m_Duration = (UINT)tmpVal;

		track->get_Rating(&tmpVal);
		tmpVal /= 20L;
		m_Rating = (UINT)tmpVal;

		IITFileOrCDTrack* file;
		hr = track->QueryInterface(&file);
		if (SUCCEEDED(hr))
		{
			file->get_Location(&tmpStr);
			file->Release();
			if (tmpStr && wcscmp(tmpStr, m_FilePath.c_str()) != 0)
			{
				m_FilePath = tmpStr;
				m_TrackChanged = true;

				if (m_HasCoverMeasure)
				{
					if (!GetCachedArt())
					{
						// Art not in cache, check for embedded art
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
									tmpStr = m_CoverPath.c_str();
									hr = artwork->SaveArtworkToFile(tmpStr);
									if (FAILED(hr))
									{
										m_CoverPath.clear();
									}

									artwork->Release();
								}
							}
							else
							{
								m_CoverPath.clear();
							}

							artworkCollection->Release();
						}
						else
						{
							m_CoverPath.clear();
						}
					}
				}
			}
		}

		track->Release();
	}
	else
	{
		ClearInfo();
	}
}

/*
** OnStateChange
**
** Called by iTunes event handler on player state change.
**
*/
void CPlayerITunes::OnStateChange(bool playing)
{
	if (playing)
	{
		m_State = PLAYER_PLAYING;
	}
	else
	{
		// Guess if paused or stopped from track time
		m_State = (m_Position == 0) ? PLAYER_STOPPED : PLAYER_PAUSED;
	}
}

/*
** OnVolumeChange
**
** Called by iTunes event handler on volume change.
**
*/
void CPlayerITunes::OnVolumeChange(int volume)
{
	m_Volume = volume;
}

/*
** PlayPause
**
** Handles the PlayPause bang.
**
*/
void CPlayerITunes::PlayPause()
{
	if (m_Initialized)
	{
		m_iTunes->PlayPause();
	}
}

/*
** Stop
**
** Handles the Stop bang.
**
*/
void CPlayerITunes::Stop() 
{
	if (m_Initialized)
	{
		m_iTunes->Stop();
	}
}

/*
** Next
**
** Handles the Next bang.
**
*/
void CPlayerITunes::Next() 
{
	if (m_Initialized)
	{
		m_iTunes->NextTrack();
	}
}

/*
** Previous
**
** Handles the Previous bang.
**
*/
void CPlayerITunes::Previous() 
{
	if (m_Initialized)
	{
		m_iTunes->PreviousTrack();
	}
}

/*
** SetRating
**
** Handles the SetRating bang.
**
*/
void CPlayerITunes::SetRating(int rating)
{
	if (m_Initialized)
	{
		rating *= 20;
		IITTrack* track;
		HRESULT hr = m_iTunes->get_CurrentTrack(&track);

		if (SUCCEEDED(hr))
		{
			track->put_Rating((long)rating);
			track->Release();
		}
	}
}

/*
** SetVolume
**
** Handles the SetVolume bang.
**
*/
void CPlayerITunes::SetVolume(int volume)
{
	if (m_Initialized)
	{
		m_iTunes->put_SoundVolume((long)volume);
	}
}

/*
** ChangeVolume
**
** Handles the ChangeVolume bang.
**
*/
void CPlayerITunes::ChangeVolume(int volume)
{
	if (m_Initialized)
	{
		int newVolume = m_Volume;
		newVolume += volume;
		m_iTunes->put_SoundVolume(newVolume);
	}
}

/*
** ClosePlayer
**
** Handles the ClosePlayer bang.
**
*/
void CPlayerITunes::ClosePlayer()
{
	if (m_Initialized)
	{
		m_Initialized = false;
		m_iTunes->Quit();
		ClearInfo();
	}
}

/*
** OpenPlayer
**
** Handles the OpenPlayer bang.
**
*/
void CPlayerITunes::OpenPlayer()
{
	ShellExecute(NULL, L"open", m_PlayerPath.empty() ? L"iTunes.exe" : m_PlayerPath.c_str(), NULL, NULL, SW_SHOW);
}

/*
** TogglePlayer
**
** Handles the TogglePlayer bang.
**
*/
void CPlayerITunes::TogglePlayer()
{
	m_Initialized ? ClosePlayer() : OpenPlayer();
}
