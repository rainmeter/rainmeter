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
#include "PlayerWMP.h"

CPlayer* CPlayerWMP::c_Player = NULL;
extern HINSTANCE g_Instance;

/*
** Constructor.
**
*/
CPlayerWMP::CRemoteHost::CRemoteHost() :
	m_Player()
{
}

/*
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
	*pbstrType = SysAllocString(L"RemoteNoDialogs");
	return *pbstrType ? S_OK : E_POINTER;
}

HRESULT CPlayerWMP::CRemoteHost::GetApplicationName(BSTR* pbstrName)
{
	*pbstrName = SysAllocString(L"Rainmeter NowPlaying");
	return *pbstrName ? S_OK : E_POINTER;
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
** Called when playing track changes.
**
*/
void CPlayerWMP::CRemoteHost::CurrentItemChange(IDispatch* pdispMedia)
{
	m_Player->m_TrackChanged = true;
}

/*
** Called when play state changes.
**
*/
void CPlayerWMP::CRemoteHost::PlayStateChange(long NewState)
{
	switch (NewState)
	{
	case wmppsStopped:
	case wmppsMediaEnded:
		m_Player->ClearData();
		break;

	case wmppsPaused:
		m_Player->m_State = STATE_PAUSED;
		break;

	case wmppsPlaying:
		if (m_Player->m_State == STATE_STOPPED)
		{
			m_Player->m_TrackChanged = true;
		}
		m_Player->m_State = STATE_PLAYING;
		break;

	default:
		break;
	}
}

/*
** Called when WMP quits.
**
*/
void CPlayerWMP::CRemoteHost::SwitchedToControl()
{
	m_Player->ClearData();
	m_Player->Uninitialize();
}

/*
** Constructor.
**
*/
CPlayerWMP::CPlayerWMP() : CPlayer(),
	m_TrackChanged(false),
	m_Window(),
	m_LastCheckTime(0),
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
** Destructor.
**
*/
CPlayerWMP::~CPlayerWMP()
{
	c_Player = NULL;
	Uninitialize();
	m_ComModule.Term();
}

/*
** Creates a shared class object.
**
*/
CPlayer* CPlayerWMP::Create()
{
	if (!c_Player)
	{
		c_Player = new CPlayerWMP();
	}

	return c_Player;
}

/*
** Set up the COM interface with WMP.
**
*/
void CPlayerWMP::Initialize()
{
	// Create windows class
	WNDCLASS wc = {0};
	wc.hInstance = g_Instance;
	wc.lpfnWndProc = DefWindowProc;
	wc.lpszClassName = L"NowPlayingWMPClass";
	RegisterClass(&wc);

	// Create the host window
	m_Window = CreateWindow(L"NowPlayingWMPClass",
							L"HostWindow",
							WS_DISABLED,
							CW_USEDEFAULT,
							CW_USEDEFAULT,
							CW_USEDEFAULT,
							CW_USEDEFAULT,
							NULL,
							NULL,
							g_Instance,
							NULL);

	if (!m_Window) return;

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
			hr = m_AxWindow->QueryHost(IID_IObjectWithSite, (void**)&spHostObject);
			if(!spHostObject.p)
			{
				hr = E_POINTER;
			}
		}
	}
	else
	{
		return;
	}

	// Create remote host which implements IServiceProvider and IWMPRemoteMediaServices
	if (SUCCEEDED(hr))
	{
		hr = CComObject<CRemoteHost>::CreateInstance(&pRemoteHost);
		if (pRemoteHost)
		{
			pRemoteHost->AddRef();
			pRemoteHost->m_Player = this;
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
		hr = spHost->CreateControl(L"{6BF52A52-394A-11D3-B153-00C04F79FAA6}", m_AxWindow->m_hWnd, NULL);
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
		m_State = STATE_PLAYING;
	}
	else if (state ==  wmppsPaused)
	{
		m_State = STATE_PAUSED;
	}

	if (m_State != STATE_STOPPED)
	{
		m_TrackChanged = true;
	}

	m_Initialized = true;
}

/*
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
		UnregisterClass(L"NowPlayingWMPClass", g_Instance);
	}
}

/*
** Called during each update of the main measure.
**
*/
void CPlayerWMP::UpdateData()
{
	if (m_Initialized)
	{
		// Get the volume
		long volume;
		m_ISettings->get_volume(&volume);
		m_Volume = (UINT)volume;

		if (m_State != STATE_STOPPED)
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
				BSTR val;
				spMedia->getItemInfo(CComBSTR(L"Artist"), &val);
				m_Artist = val;

				spMedia->getItemInfo(CComBSTR(L"Title"), &val);
				m_Title = val;

				spMedia->getItemInfo(CComBSTR(L"Album"), &val);
				m_Album = val;

				spMedia->getItemInfo(CComBSTR(L"UserRating"), &val);
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

				spMedia->getItemInfo(CComBSTR(L"WM/TrackNumber"), &val);
				m_Number = (UINT)_wtoi(val);

				spMedia->getItemInfo(CComBSTR(L"WM/Year"), &val);
				m_Year = (UINT)_wtoi(val);

				double duration;
				spMedia->get_duration(&duration);
				m_Duration = (UINT)duration;

				BSTR url;
				spMedia->get_sourceURL(&url);
				std::wstring targetPath = url;

				if (targetPath != m_FilePath)
				{
					++m_TrackCount;
					m_FilePath = targetPath;

					// Find cover if needed
					// TODO: Fix temp solution
					if (m_Measures & MEASURE_COVER || m_InstanceCount == 0)
					{
						spMedia->getItemInfo(CComBSTR(L"WM/WMCollectionID"), &val);
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
							FindCover();
						}
					}

					if (m_Measures & MEASURE_LYRICS)
					{
						FindLyrics();
					}
				}
			}
		}
	}
	else
	{
		DWORD time = GetTickCount();
		
		// Try to find WMP window every 5 seconds
		if (m_LastCheckTime = 0 || time - m_LastCheckTime > 5000)
		{
			m_LastCheckTime = time;

			if (FindWindow(L"WMPlayerApp", NULL))
			{
				Initialize();
			}
		}
	}
}

/*
** Handles the Pause bang.
**
*/
void CPlayerWMP::Pause() 
{
	m_IControls->pause();
}

/*
** Handles the Play bang.
**
*/
void CPlayerWMP::Play() 
{
	m_IControls->play();
}

/*
** Handles the Stop bang.
**
*/
void CPlayerWMP::Stop() 
{
	m_IControls->stop();
	// TODO: FIXME
	m_State = STATE_STOPPED;
}

/*
** Handles the Next bang.
**
*/
void CPlayerWMP::Next() 
{
	m_IControls->next();
}

/*
** Handles the Previous bang.
**
*/
void CPlayerWMP::Previous() 
{
	m_IControls->previous();
}

/*
** Handles the SetPosition bang.
**
*/
void CPlayerWMP::SetPosition(int position)
{
	m_IControls->put_currentPosition((double)position);
}

/*
** Handles the SetRating bang.
**
*/
void CPlayerWMP::SetRating(int rating)
{
	if (m_State != STATE_STOPPED)
	{
		CComPtr<IWMPMedia> spMedia;
		m_IPlayer->get_currentMedia(&spMedia);

		if (spMedia)
		{
			BSTR val;
			switch (rating)
			{
			case 0:
				val = L"0";
				break;

			case 1:
				val = L"1";
				break;

			case 2:
				val = L"25";
				break;

			case 3:
				val = L"50";
				break;

			case 4:
				val = L"75";
				break;

			default: // case 5:
				val = L"99";
				break;
			}

			spMedia->setItemInfo(CComBSTR(L"UserRating"), val);
			m_Rating = rating;
		}
	}
}

/*
** Handles the SetVolume bang.
**
*/
void CPlayerWMP::SetVolume(int volume)
{
	m_ISettings->put_volume(volume);
}

/*
** Handles the ClosePlayer bang.
**
*/
void CPlayerWMP::ClosePlayer()
{
	HWND wnd = FindWindow(L"WMPlayerApp", NULL);

	if (wnd)
	{
		SendMessage(wnd, WM_CLOSE, 0, 0);
	}
}

/*
** Handles the OpenPlayer bang.
**
*/
void CPlayerWMP::OpenPlayer(std::wstring& path)
{
	ShellExecute(NULL, L"open", path.empty() ? L"wmplayer.exe" : path.c_str(), NULL, NULL, SW_SHOW);
}
