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
#include "../../Library/DisableThreadLibraryCalls.h"	// contains DllMain entry point
#include "NowPlaying.h"
#include "PlayerAIMP.h"
#include "PlayerFoobar.h"
#include "PlayerITunes.h"
#include "PlayerSpotify.h"
#include "PlayerWinamp.h"
#include "PlayerWMP.h"

CPlayer* g_AIMP = NULL;
CPlayer* g_Foobar = NULL;
CPlayer* g_iTunes = NULL;
CPlayer* g_Spotify = NULL;
CPlayer* g_Winamp = NULL;
CPlayer* g_WMP = NULL;

static MeasureMap g_Values;
static bool g_DisableLeazingZero = false;
std::wstring g_CachePath;

void SecondsToTime(UINT seconds, WCHAR* buffer)
{
	int hours = seconds;
	int mins = seconds;
	hours /= 3600;
	mins %= 3600;
	int secs = mins;
	mins /= 60;
	secs %= 60;

	if (hours)
	{
		_snwprintf_s(buffer, 32, _TRUNCATE, g_DisableLeazingZero ? L"%i:%02i:%02i" : L"%02i:%02i:%02i", hours, mins, secs);
	}
	else
	{
		_snwprintf_s(buffer, 32, _TRUNCATE, g_DisableLeazingZero ? L"%i:%02i" : L"%02i:%02i", mins, secs);
	}
}

/*
** Initialize
**
** Called when the measure is initialized.
**
*/
UINT Initialize(HMODULE instance, LPCTSTR iniFile, LPCTSTR section, UINT id)
{
	if (g_Values.empty())
	{
		WCHAR buffer[MAX_PATH];
		GetTempPath(MAX_PATH, buffer);
		wcscat(buffer, L"Rainmeter-Cache\\");
		CreateDirectory(buffer, NULL);
		g_CachePath = buffer;
	}

	UINT maxValue = 0;
	MeasureData* data = new MeasureData;
	
	// Read settings from the ini-file
	LPCTSTR str = ReadConfigString(section, L"PlayerName", NULL);
	if (str)
	{
		if (str[0] == L'[')
		{
			int len = wcslen(str) - 2;
			if (len > 0)
			{
				MeasureMap::iterator it;
				for (it = g_Values.begin(); it != g_Values.end(); ++it)
				{
					if (wcsncmp(&str[1], it->second->section.c_str(), len) == 0 &&
						wcscmp(iniFile, it->second->iniFile.c_str()) == 0)
					{
						// Use same player instance as pointed section
						data->player = it->second->player;
					}
				}

				if (!data->player)
				{
					std::wstring error = L"NowPlayingPlugin: PlayerName=";
					error += str;
					error += L" in section [";
					error += section;
					error += L"] does not exist.";
					LSLog(LOG_WARNING, L"Rainmeter", error.c_str());
					return maxValue;
				}
			}
		}
		else
		{
			data->section = section;
			data->iniFile = iniFile;

			if (_wcsicmp(L"AIMP", str) == 0)
			{
				if (!g_AIMP)
				{
					g_AIMP = new CPlayerAIMP;
				}
				data->player = g_AIMP;
			}
			else if (_wcsicmp(L"iTunes", str) == 0)
			{
				if (!g_iTunes)
				{
					g_iTunes = new CPlayerITunes;
				}
				data->player = g_iTunes;
			}
			else if (_wcsicmp(L"foobar2000", str) == 0)
			{
				if (!g_Foobar)
				{
					g_Foobar = new CPlayerFoobar;
				}
				data->player = g_Foobar;
			}
			else if (_wcsicmp(L"Spotify", str) == 0)
			{
				if (!g_Spotify)
				{
					g_Spotify = new CPlayerSpotify;
				}
				data->player = g_Spotify;
			}
			else if (_wcsicmp(L"WinAmp", str) == 0)
			{
				if (!g_Winamp)
				{
					g_Winamp = new CPlayerWinamp;
				}
				data->player = g_Winamp;
			}
			else if (_wcsicmp(L"WMP", str) == 0)
			{
				if (!g_WMP)
				{
					g_WMP = new CPlayerWMP;
				}
				data->player = g_WMP;
			}
			else
			{
				std::wstring error = L"NowPlayingPlugin: PlayerName=";
				error += str;
				error += L" in section [";
				error += section;
				error += L"] is not valid.";
				LSLog(LOG_ERROR, L"Rainmeter", error.c_str());
				delete data;
				return maxValue;
			}

			str = ReadConfigString(section, L"PlayerPath", NULL);
			if (str && *str)
			{
				data->player->SetPlayerPath(str);
			}

			str = ReadConfigString(section, L"DisableLeadingZero", NULL);
			if (str && *str)
			{
				g_DisableLeazingZero = (1 == _wtoi(str));
			}

			str = ReadConfigString(section, L"TrackChangeAction", NULL);
			if (str && *str)
			{
				data->player->SetTrackChangeAction(str);
			}
		}
	}

	str = ReadConfigString(section, L"PlayerType", NULL);
	if (str)
	{
		if (_wcsicmp(L"ARTIST", str) == 0)
		{
			data->measure = MEASURE_ARTIST;
		}
		else if (_wcsicmp(L"TITLE", str) == 0)
		{
			data->measure = MEASURE_TITLE;
		}
		else if (_wcsicmp(L"ALBUM", str) == 0)
		{
			data->measure = MEASURE_ALBUM;
		}
		else if (_wcsicmp(L"COVER", str) == 0)
		{
			data->measure = MEASURE_COVER;
		}
		else if (_wcsicmp(L"DURATION", str) == 0)
		{
			data->measure = MEASURE_DURATION;
		}
		else if (_wcsicmp(L"POSITION", str) == 0)
		{
			data->measure = MEASURE_POSITION;
		}
		else if (_wcsicmp(L"PROGRESS", str) == 0)
		{
			data->measure = MEASURE_PROGRESS;
			maxValue = 100;
		}
		else if (_wcsicmp(L"RATING", str) == 0)
		{
			data->measure = MEASURE_RATING;
			maxValue = 5;
		}
		else if (_wcsicmp(L"STATE", str) == 0)
		{
			data->measure = MEASURE_STATE;
		}
		else if (_wcsicmp(L"VOLUME", str) == 0)
		{
			data->measure = MEASURE_VOLUME;
			maxValue = 100;
		}
		else if (_wcsicmp(L"FILE", str) == 0)
		{
			data->measure = MEASURE_FILE;
		}
	}

	data->player->AddInstance(data->measure);
	g_Values[id] = data;
	return maxValue;
}

/*
** Finalize
**
** Called when the measure is destroyed (during refresh/quit).
**
*/
void Finalize(HMODULE instance, UINT id)
{
	MeasureMap::iterator i = g_Values.find(id);
	if (i != g_Values.end())
	{
		(*i).second->player->RemoveInstance();
		delete (*i).second;
		g_Values.erase(i);
	}
}

/*
** Update
**
** Called on each update.
**
*/
UINT Update(UINT id)
{
	MeasureMap::iterator i = g_Values.find(id);
	if (i != g_Values.end())
	{
		if (!(*i).second->section.empty())
		{
			// Only allow main measure to update
			(*i).second->player->UpdateData();
		}

		CPlayer* player = (*i).second->player;

		switch ((*i).second->measure)
		{
		case MEASURE_DURATION:
			return player->GetDuration();

		case MEASURE_POSITION:
			return player->GetPosition();

		case MEASURE_PROGRESS:
			if (player->GetDuration())
			{
				return (player->GetPosition() * 100) / player->GetDuration();
			}
			return 0;

		case MEASURE_RATING:
			return player->GetRating();

		case MEASURE_STATE:
			return (int)player->GetState();

		case MEASURE_VOLUME:
			return player->GetVolume();
		}

		return 0;
	}

	return 1;
}

/*
** GetString
**
** Called when a string value is needed.
**
*/
LPCTSTR GetString(UINT id, UINT flags)
{
	MeasureMap::iterator i = g_Values.find(id);
	if (i != g_Values.end())
	{
		CPlayer* player = (*i).second->player;
		static WCHAR buffer[32];

		switch ((*i).second->measure)
		{
		case MEASURE_ARTIST:
			return player->GetArtist();

		case MEASURE_TITLE:
			return player->GetTitle();

		case MEASURE_ALBUM:
			return player->GetAlbum();

		case MEASURE_COVER:
			return player->GetCoverPath();

		case MEASURE_DURATION:
			SecondsToTime(player->GetDuration(), buffer);
			return buffer;

		case MEASURE_POSITION:
			SecondsToTime(player->GetPosition(), buffer);
			return buffer;

		case MEASURE_PROGRESS:
			if (player->GetDuration())
			{
				UINT res = (player->GetPosition() * 100) / player->GetDuration();
				_itow(res, buffer, 10);
				return buffer;
			}
			return L"0";

		case MEASURE_RATING:
			_itow(player->GetRating(), buffer, 10);
			return buffer;

		case MEASURE_STATE:
			_itow(player->GetState(), buffer, 10);
			return buffer;

		case MEASURE_VOLUME:
			_itow(player->GetVolume(), buffer, 10);
			return buffer;

		case MEASURE_FILE:
			return player->GetFilePath();
		}
	}
	else
	{
		// For invalid PlayerName=
		return L"0";
	}

	return L"";
}

/*
** ExecuteBang
**
** Called when a !RainmeterPluginBang is executed.
**
*/
void ExecuteBang(LPCTSTR bang, UINT id)
{
	MeasureMap::iterator i = g_Values.find(id);
	if (i != g_Values.end())
	{
		CPlayer* player = (*i).second->player;

		if (_wcsicmp(bang, L"Play") == 0)
		{
			player->Play();
		}
		else if (_wcsicmp(bang, L"PlayPause") == 0)
		{
			player->PlayPause();
		}
		else if (_wcsicmp(bang, L"Stop") == 0)
		{
			player->Stop();
		}
		else if (_wcsicmp(bang, L"Next") == 0)
		{
			player->Next();
		}
		else if (_wcsicmp(bang, L"Previous") == 0)
		{
			player->Previous();
		}
		else if (_wcsicmp(bang, L"ClosePlayer") == 0)
		{
			player->ClosePlayer();
		}
		else if (_wcsicmp(bang, L"OpenPlayer") == 0)
		{
			player->OpenPlayer();
		}
		else if (_wcsicmp(bang, L"TogglePlayer") == 0)
		{
			player->TogglePlayer();
		}
		else
		{
			LPCTSTR arg = wcschr(bang, L' ');

			if (++arg)	// Skip the space
			{
				if (wcsnicmp(bang, L"SetRating", 9) == 0)
				{
					player->SetRating(_wtoi(arg));
				}
				else if (wcsnicmp(bang, L"SetVolume", 9) == 0)
				{
					if (arg[0] == L'+' || arg[0] == L'-')
					{
						player->ChangeVolume(_wtoi(arg));
					}
					else
					{
						player->SetVolume(_wtoi(arg));
					}
				}
				else
				{
					LSLog(LOG_WARNING, L"Rainmeter", L"NowPlayingPlugin: Unknown bang!");
				}
			}
		}
	}
}

/*
** GetPluginVersion
**
** Returns the version number of the plugin.
**
*/
UINT GetPluginVersion()
{
	// Major * 1000 + Minor
	return 1000;
}

/*
** GetPluginAuthor
**
** Returns the author of the plugin for the About dialog.
**
*/
LPCTSTR GetPluginAuthor()
{
	return L"Birunthan Mohanathas (www.poiru.net)";
}
