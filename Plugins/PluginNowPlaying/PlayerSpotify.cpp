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
#include "PlayerSpotify.h"
#include "../../Library/rapidjson/rapidjson.h"
#include "../../Library/rapidjson/document.h"
#include "../../Library/rapidjson/stringbuffer.h"

Player* PlayerSpotify::c_Player = nullptr;

using namespace rapidjson;
typedef GenericDocument<UTF16<>> WDocument;
typedef GenericValue<UTF16<>> WValue;

/*
** Constructor.
**
*/
PlayerSpotify::PlayerSpotify() : Player(),
	m_Window(),
	m_LastCheckTime(0)
{
}

/*
** Destructor.
**
*/
PlayerSpotify::~PlayerSpotify()
{
	c_Player = nullptr;
}

/*
** Creates a shared class object.
**
*/
Player* PlayerSpotify::Create()
{
	if (!c_Player)
	{
		c_Player = new PlayerSpotify();
	}

	return c_Player;
}

/*
** Try to find Spotify periodically.
**
*/
bool PlayerSpotify::CheckWindow()
{
	DWORD time = GetTickCount();
		
	// Try to find Spotify window every 5 seconds
	if (time - m_LastCheckTime > 5000)
	{
		m_Window = FindWindow(L"SpotifyMainWindow", nullptr);
		if ((!m_Initialized || csrfToken.empty() || openidToken.empty()) && m_Window) {
			//Grab CSRF token, requires Origin header set
			csrfToken = Internet::DownloadUrl(m_baseURL + m_csrfURL, CP_UTF8, originHeader);
			WDocument csrfDocument;
			csrfDocument.Parse(csrfToken.c_str());

			if (!csrfDocument[L"token"].IsNull()) {
				csrfToken = csrfDocument[L"token"].GetString();
			}
			else {
				RmLog(LOG_ERROR, L"Spotify: Cannot get CSRF token");
			}

			//Grab OpenID token
			openidToken = Internet::DownloadUrl(m_openidURL, CP_UTF8);
			WDocument tokenDocument;
			tokenDocument.Parse(openidToken.c_str());

			if (!tokenDocument[L"t"].IsNull()) {
				openidToken = tokenDocument[L"t"].GetString();
			}
			else {
				RmLog(LOG_ERROR, L"Spotify: Cannot get OpenID token");
			}
		}
		
		m_LastCheckTime = time;

		if (m_Window)
		{
			m_Initialized = true;
		}
	}

	return m_Initialized;
}

/*
** Called during each update of the main measure.
**
*/
void PlayerSpotify::UpdateData()
{
	if (m_Initialized || CheckWindow())
	{
		std::wstring statusParams = L"?oauth=" + openidToken + L"&csrf=" + csrfToken;
		std::wstring statusJson = Internet::DownloadUrl(m_baseURL + m_statusURL + statusParams, CP_UTF8, originHeader);

		if (!statusJson.empty())
		{
			WDocument statusDocument;
			statusDocument.Parse(statusJson.c_str());
			if (statusDocument[L"error"].IsNull()) {
				WValue& runningValue = statusDocument[L"running"];
				if (!runningValue.IsNull() && runningValue.GetBool()) {

					//Client does not tell us what's playing in private mode
					WValue& openGraphState = statusDocument[L"open_graph_state"];
					if (openGraphState[L"private_session"].GetBool()) {
						ClearData();
						return;
					}

					//Client does not really have a "stopped" state, and holds on to tracks
					m_State = statusDocument[L"playing"].GetBool() ? STATE_PLAYING : STATE_PAUSED;
					m_Shuffle = statusDocument[L"shuffle"].GetBool();
					m_Repeat = statusDocument[L"repeat"].GetBool();
					m_Position = int(statusDocument[L"playing_position"].GetDouble());

					WValue& trackValue = statusDocument[L"track"];

					if (!trackValue.IsNull()) {

						m_Duration = trackValue[L"length"].GetInt();

						WValue& trackResource = trackValue[L"track_resource"];
						m_Title = trackResource[L"name"].GetString();

						WValue& artistResource = trackValue[L"artist_resource"];
						m_Artist = artistResource[L"name"].GetString();

						WValue& albumResouce = trackValue[L"album_resource"];
						m_Album = albumResouce[L"name"].GetString();
					}
				}
			}
			else {
				ClearData();
				m_Initialized = false;
			}
		}
	}
}

/*
** Handles the Play bang.
**
*/
void PlayerSpotify::Play()
{
	SendMessage(m_Window, WM_APPCOMMAND, 0, SPOTIFY_PLAYPAUSE);
}

/*
** Handles the Stop bang.
**
*/
void PlayerSpotify::Stop() 
{
	SendMessage(m_Window, WM_APPCOMMAND, 0, SPOTIFY_STOP);
}

/*
** Handles the Next bang.
**
*/
void PlayerSpotify::Next() 
{
	SendMessage(m_Window, WM_APPCOMMAND, 0, SPOTIFY_NEXT);
}

/*
** Handles the Previous bang.
**
*/
void PlayerSpotify::Previous() 
{
	SendMessage(m_Window, WM_APPCOMMAND, 0, SPOTIFY_PREV);
}


/*
** Handles the ClosePlayer bang.
**
*/
void PlayerSpotify::ClosePlayer()
{
	// A little harsh...
	DWORD pID;
	GetWindowThreadProcessId(m_Window, &pID);
	HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, pID);
	if (hProcess)
	{
		TerminateProcess(hProcess, 0);
		CloseHandle(hProcess);
	}
}

/*
** Handles the OpenPlayer bang.
**
*/
void PlayerSpotify::OpenPlayer(std::wstring& path)
{
	if (!m_Initialized)
	{
		if (path.empty())
		{
			// Gotta figure out where Spotify is located at
			HKEY hKey;
			RegOpenKeyEx(HKEY_CLASSES_ROOT,
						 L"spotify\\DefaultIcon",
						 0,
						 KEY_QUERY_VALUE,
						 &hKey);

			DWORD size = 512;
			WCHAR* data = new WCHAR[size];
			DWORD type = 0;

			if (RegQueryValueEx(hKey,
								nullptr,
								nullptr,
								(LPDWORD)&type,
								(LPBYTE)data,
								(LPDWORD)&size) == ERROR_SUCCESS)
			{
				if (type == REG_SZ)
				{
					path = data;
					path.erase(0, 1);				// Get rid of the leading quote
					path.resize(path.length() - 3);	// And the ",0 at the end
					ShellExecute(nullptr, L"open", path.c_str(), nullptr, nullptr, SW_SHOW);
				}
			}

			delete [] data;
			RegCloseKey(hKey);
		}
		else
		{
			ShellExecute(nullptr, L"open", path.c_str(), nullptr, nullptr, SW_SHOW);
		}
	}
	else
	{
		// Already active, restore the window
		ShowWindow(m_Window, SW_SHOWNORMAL);
		BringWindowToTop(m_Window);
	}
}
