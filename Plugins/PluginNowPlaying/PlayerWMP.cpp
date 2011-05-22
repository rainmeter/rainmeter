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
#include "PlayerWMP.h"

/*
** CRemoteHost
**
** Constructor.
**
*/
CPlayerWMP::CRemoteHost::CRemoteHost() :
	m_WMP()
{
}

/*
** ~CRemoteHost
**
** Destructor.
**
*/
CPlayerWMP::CRemoteHost::~CRemoteHost()
{
}

HRESULT CPlayerWMP::CRemoteHost::QueryService(REFGUID guidService, REFIID riid, void** ppv)
{
	return ppv ? QueryInterface(riid, ppv) : E_POINTER;
}

HRESULT CPlayerWMP::CRemoteHost::GetServiceType(BSTR* pbstrType)
{
	HRESULT hr = E_POINTER;
	if (pbstrType)
	{
		*pbstrType = SysAllocString(L"Remote");
		hr = *pbstrType? S_OK : E_POINTER;
	}
	return hr;
}

HRESULT CPlayerWMP::CRemoteHost::GetApplicationName(BSTR* pbstrName)
{
	HRESULT hr = E_POINTER;
	if (pbstrName)
	{
		CComBSTR bstrAppName = L"Rainmeter NowPlaying";
		*pbstrName = bstrAppName.Detach();
		hr = *pbstrName? S_OK : E_POINTER;
	}
	return hr;
}

HRESULT CPlayerWMP::CRemoteHost::GetScriptableObject(BSTR* pbstrName, IDispatch** ppDispatch)
{
	if (pbstrName)
	{
		*pbstrName = NULL;
	}
	if (ppDispatch)
	{
		*ppDispatch = NULL;
	}
	return E_NOTIMPL;
}

HRESULT CPlayerWMP::CRemoteHost::GetCustomUIMode(BSTR* pbstrFile)
{
	return E_POINTER;
}

/*
** CurrentItemChange
**
** Called when playing track changes.
**
*/
void CPlayerWMP::CRemoteHost::CurrentItemChange(IDispatch* pdispMedia)
{
	m_WMP->m_TrackChanged = true;
}

/*
** PlayStateChange
**
** Called when play state changes.
**
*/
void CPlayerWMP::CRemoteHost::PlayStateChange(long NewState)
{
	switch (NewState)
	{
	case wmppsStopped:
	case wmppsMediaEnded:
		m_WMP->ClearInfo();
		break;

	case wmppsPaused:
		m_WMP->m_State = PLAYER_PAUSED;
		break;

	case wmppsPlaying:
		if (m_WMP->m_State == PLAYER_STOPPED)
		{
			m_WMP->m_TrackChanged = true;
		}
		m_WMP->m_State = PLAYER_PLAYING;
		break;

	default:
		break;
	}
}

/*
** SwitchedToControl
**
** Called when WMP quits.
**
*/
void CPlayerWMP::CRemoteHost::SwitchedToControl()
{
	m_WMP->ClearInfo();
	m_WMP->Uninitialize();
}

/*
** CPlayerWMP
**
** Constructor.
**
*/
CPlayerWMP::CPlayerWMP() : CPlayer(),
	m_Initialized(false),
	m_HasCoverMeasure(false),
	m_ComModule(),
	m_AxWindow(),
	m_IPlayer(),
	m_IControls(),
	m_ISettings(),
	m_IConnectionPoint()
{
	m_ComModule.Init(NULL, NULL, &LIBID_ATLLib);
}

/*
** ~CPlayerWMP
**
** Destructor.
**
*/
CPlayerWMP::~CPlayerWMP()
{
	Uninitialize();
	m_ComModule.Term();
}

/*
** AddInstance
**
** Called during initialization of each measure.
**
*/
void CPlayerWMP::AddInstance(MEASURETYPE type)
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
void CPlayerWMP::RemoveInstance()
{
	if (--m_InstanceCount == 0)
	{
		delete this;
	}
}

/*
** Initialize
**
** Set up the COM interface with WMP.
**
*/
void CPlayerWMP::Initialize()
{
	HINSTANCE hInstance = GetModuleHandle(NULL);

	// Create windows class
	WNDCLASS wc = {0};
	wc.hInstance = hInstance;
	wc.lpfnWndProc = DefWindowProc;
	wc.lpszClassName = L"NowPlayingWMPClass";
	RegisterClass(&wc);

	// Create dummy window
	m_Window = CreateWindow(L"NowPlayingWMPClass",
							L"DummyWindow",
							WS_DISABLED,
							CW_USEDEFAULT,
							CW_USEDEFAULT,
							CW_USEDEFAULT,
							CW_USEDEFAULT,
							NULL,
							NULL,
							hInstance,
							NULL);

	if (!m_Window)
	{
		LSLog(LOG_DEBUG, L"Rainmeter", L"NowPlayingPlugin: Unable to create window (WMP).");
		return;
	}

	CComPtr<IObjectWithSite> spHostObject;
	CComPtr<IAxWinHostWindow> spHost;
	CComObject<CRemoteHost>* pRemoteHost;
	
	m_AxWindow = new CAxWindow();
	HRESULT hr = m_AxWindow ? S_OK : E_OUTOFMEMORY;

	if (SUCCEEDED(hr)) 
	{
		m_AxWindow->Create(m_Window, NULL, NULL, WS_CHILD | WS_DISABLED);
		if(IsWindow(m_AxWindow->m_hWnd))
		{
			hr = m_AxWindow->QueryHost(IID_IObjectWithSite, (void **)&spHostObject);
			if(!spHostObject.p)
			{
				hr = E_POINTER;
			}
		}
	}
	else
	{
		LSLog(LOG_DEBUG, L"Rainmeter", L"NowPlayingPlugin: Failed to initialize COM (WMP).");
		return;
	}

	// Create remote host which implements IServiceProvider and IWMPRemoteMediaServices
	if (SUCCEEDED(hr))
	{
		hr = CComObject<CRemoteHost>::CreateInstance(&pRemoteHost);
		if (pRemoteHost)
		{
			pRemoteHost->AddRef();
			pRemoteHost->m_WMP = this;
		}
		else
		{
			hr = E_POINTER;
		}
	}

	// Set site to the remote host
	if (SUCCEEDED(hr))
	{
		hr = spHostObject->SetSite((IWMPRemoteMediaServices*)pRemoteHost);
	}

	if (SUCCEEDED(hr))
	{
		hr = m_AxWindow->QueryHost(&spHost);
		if (!spHost)
		{
			hr = E_NOINTERFACE;
		}
	}

	// Create WMP control
	if (SUCCEEDED(hr))
	{
		hr = spHost->CreateControl(CComBSTR(L"{6BF52A52-394A-11D3-B153-00C04F79FAA6}"), m_AxWindow->m_hWnd, NULL);
	}

	if (SUCCEEDED(hr))
	{
		hr = m_AxWindow->QueryControl(&m_IPlayer);
		if (!m_IPlayer.p)
		{
			hr = E_NOINTERFACE;
		}
	}

	// Connect the event interface
	CComPtr<IConnectionPointContainer> spConnectionContainer;
	hr = m_IPlayer->QueryInterface(&spConnectionContainer);

	if (SUCCEEDED(hr))
	{
		hr = spConnectionContainer->FindConnectionPoint( __uuidof(IWMPEvents), &m_IConnectionPoint);
	}

	if (SUCCEEDED(hr))
	{
		DWORD adviseCookie;
		hr = m_IConnectionPoint->Advise(pRemoteHost->GetUnknown(), &adviseCookie);

		if ((FAILED(hr)) || !adviseCookie)
		{
			m_IConnectionPoint = NULL;
		}
	}

	// Release remote host object
	if (pRemoteHost)
	{
		pRemoteHost->Release();
	}
	

	hr = m_IPlayer->get_controls(&m_IControls);
	if (FAILED(hr)) return;

	hr = m_IPlayer->get_settings(&m_ISettings);
	if (FAILED(hr)) return;

	// Get player state
	WMPPlayState state;
	m_IPlayer->get_playState(&state);
	if (state == wmppsPlaying)
	{
		m_State = PLAYER_PLAYING;
	}
	else if (state ==  wmppsPaused)
	{
		m_State = PLAYER_PAUSED;
	}

	m_TrackChanged = true;
	m_Initialized = true;
}

/*
** Uninitialize
**
** Close the interface with WMP.
**
*/
void CPlayerWMP::Uninitialize()
{
	if (m_Initialized)
	{
		m_Initialized = false;
		m_IControls.Release();
		m_ISettings.Release();
		m_IPlayer.Release();
		m_AxWindow->DestroyWindow();
		delete m_AxWindow;
		DestroyWindow(m_Window);
		UnregisterClass(L"NowPlayingWMPClass", GetModuleHandle(NULL));
	}
}

/*
** UpdateData
**
** Called during each update of the main measure.
**
*/
void CPlayerWMP::UpdateData()
{
	static bool clear = false;

	if (m_Initialized)
	{
		// Get the volume
		long volume;
		m_ISettings->get_volume(&volume);
		m_Volume = (UINT)volume;

		if (m_State != PLAYER_STOPPED)
		{
			double position;
			m_IControls->get_currentPosition(&position);
			m_Position = (UINT)position;
		}

		if (m_TrackChanged)
		{
			m_TrackChanged = false;
			CComPtr<IWMPMedia> spMedia;
			m_IPlayer->get_currentMedia(&spMedia);

			if (spMedia)
			{
				CComBSTR val;
				spMedia->getItemInfo(CComBSTR("Artist"), &val);
				m_Artist = val;

				spMedia->getItemInfo(CComBSTR("Title"), &val);
				m_Title = val;

				spMedia->getItemInfo(CComBSTR("Album"), &val);
				m_Album = val;

				spMedia->getItemInfo(CComBSTR("UserRating"), &val);
				int rating = _wtoi(val);

				if (rating > 75)
				{
					m_Rating = 5;
				}
				else if (rating > 50)
				{
					m_Rating = 4;
				}
				else if (rating > 25)
				{
					m_Rating = 3;
				}
				else if (rating > 1)
				{
					m_Rating = 2;
				}
				else
				{
					m_Rating = rating;
				}

				double duration;
				spMedia->get_duration(&duration);
				m_Duration = (UINT)duration;

				CComBSTR url;
				spMedia->get_sourceURL(&url);
				std::wstring targetPath = url;

				if (targetPath != m_FilePath)
				{
					m_FilePath = targetPath;

					// TODO: Better solution for this
					if (m_HasCoverMeasure || m_InstanceCount == 0)
					{
						spMedia->getItemInfo(CComBSTR("WM/WMCollectionID"), &val);
						targetPath.resize(targetPath.find_last_of(L'\\') + 1);
						targetPath += L"AlbumArt_";
						targetPath += val;
						targetPath += L"_Large.jpg";

						if (_waccess(targetPath.c_str(), 0) == 0)
						{
							m_CoverPath = targetPath;
						}
						else
						{
							std::wstring cover = CreateCoverArtPath();
							if (_waccess(cover.c_str(), 0) == 0)
							{
								// Cover is in cache, lets use the that
								m_CoverPath = cover;
								return;
							}

							TagLib::FileRef fr(url.m_str);
							if (!fr.isNull() && fr.tag() && GetEmbeddedArt(fr, cover))
							{
								// Embedded art found
								return;
							}

							// Get rid of the name and extension from filename
							std::wstring trackFolder = url;
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

					ExecuteTrackChangeAction();
				}
			}
		}
	}
	else
	{
		static DWORD oldTime = 0;
		DWORD time = GetTickCount();
		
		// Try to find WMP window every 5 seconds
		if (oldTime = 0 || time - oldTime > 5000)
		{
			oldTime = time;

			if (FindWindow(L"WMPlayerApp", NULL))
			{
				Initialize();
			}
		}
	}
}

/*
** Play
**
** Handles the Play bang.
**
*/
void CPlayerWMP::Play() 
{
	if (m_IPlayer)
	{
		m_IControls->play();
	}
}

/*
** PlayPause
**
** Handles the PlayPause bang.
**
*/
void CPlayerWMP::PlayPause() 
{
	if (m_IPlayer)
	{
		WMPPlayState state;
		m_IPlayer->get_playState(&state);

		if (state == wmppsPlaying)
		{
			m_IControls->pause();
		}
		else
		{
			m_IControls->play();
		}
	}
}

/*
** Stop
**
** Handles the Stop bang.
**
*/
void CPlayerWMP::Stop() 
{
	if (m_IPlayer)
	{
		m_IControls->stop();
		m_State = PLAYER_STOPPED;
	}
}

/*
** Next
**
** Handles the Next bang.
**
*/
void CPlayerWMP::Next() 
{
	if (m_IPlayer)
	{
		m_IControls->next();
	}
}

/*
** Previous
**
** Handles the Previous bang.
**
*/
void CPlayerWMP::Previous() 
{
	if (m_IPlayer)
	{
		m_IControls->previous();
	}
}

/*
** SetRating
**
** Handles the SetRating bang.
**
*/
void CPlayerWMP::SetRating(int rating)
{
	if (m_IPlayer && (m_State == PLAYER_PLAYING || m_State == PLAYER_PAUSED))
	{
		CComPtr<IWMPMedia> spMedia;
		m_IPlayer->get_currentMedia(&spMedia);

		if (spMedia)
		{
			CComBSTR val;
			if (rating <= 0)
			{
				rating = 0;
				val = L"0";
			}
			else if (rating == 1)
			{
				val = L"1";
			}
			else if (rating == 2)
			{
				val = L"25";
			}
			else if (rating == 3)
			{
				val = L"50";
			}
			else if (rating == 4)
			{
				val = L"75";
			}
			else if (rating >= 5)
			{
				rating = 5;
				val = L"99";
			}

			spMedia->setItemInfo(CComBSTR("UserRating"), val);
			m_Rating = rating;
		}
	}
}

/*
** SetVolume
**
** Handles the SetVolume bang.
**
*/
void CPlayerWMP::SetVolume(int volume)
{
	if (m_IPlayer)
	{
		m_ISettings->put_volume(volume);
	}
}

/*
** ClosePlayer
**
** Handles the ClosePlayer bang.
**
*/
void CPlayerWMP::ChangeVolume(int volume)
{
	if (m_IPlayer)
	{
		int newVolume = m_Volume;
		newVolume += volume;
		m_ISettings->put_volume(newVolume);
	}
}

/*
** ClosePlayer
**
** Handles the ClosePlayer bang.
**
*/
void CPlayerWMP::ClosePlayer()
{
	if (m_IPlayer)
	{
		HWND wmp = FindWindow(L"WMPlayerApp", NULL);

		if (wmp)
		{
			SendMessage(wmp, WM_CLOSE, 0, 0);
		}
	}
}

/*
** OpenPlayer
**
** Handles the OpenPlayer bang.
**
*/
void CPlayerWMP::OpenPlayer()
{
	ShellExecute(NULL, L"open", m_PlayerPath.empty() ? L"wmplayer.exe" : m_PlayerPath.c_str(), NULL, NULL, SW_SHOW);
}

/*
** TogglePlayer
**
** Handles the TogglePlayer bang.
**
*/
void CPlayerWMP::TogglePlayer()
{
	m_IPlayer ? ClosePlayer() : OpenPlayer();
}
