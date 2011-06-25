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
#include "Internet.h"
#include "PlayerAIMP.h"
#include "PlayerCAD.h"
#include "PlayerFoobar.h"
#include "PlayerITunes.h"
#include "PlayerSpotify.h"
#include "PlayerWinamp.h"
#include "PlayerWLM.h"
#include "PlayerWMP.h"

static std::map<UINT, ChildMeasure*> g_Measures;
std::wstring g_CachePath;
std::wstring g_SettingsFile;

/*
** Initialize
**
** Called when the measure is initialized.
**
*/
UINT Initialize(HMODULE instance, LPCTSTR iniFile, LPCTSTR section, UINT id)
{
	if (g_Measures.empty())
	{
		// Get path to temporary folder (for cover art cache)
		WCHAR buffer[MAX_PATH];
		GetTempPath(MAX_PATH, buffer);
		wcscat(buffer, L"Rainmeter-Cache\\");
		CreateDirectory(buffer, NULL);
		g_CachePath = buffer;

		// Get path to Plugins.ini (usually %APPDATA%\Rainmeter\Plugins.ini)
		std::wstring str = PluginBridge(L"getconfig", iniFile);
		if (!str.empty())
		{
			str += L" \"SETTINGSPATH\"";
			g_SettingsFile = PluginBridge(L"getvariable", str.c_str());
			g_SettingsFile += L"Plugins.ini";
		}
		else
		{
			LSLog(LOG_ERROR, L"Rainmeter", L"NowPlayingPlugin: Unable to get path to Plugins.ini.");
		}

		CInternet::Initialize();
	}

	// Data is stored in two structs: ChildMeasure and ParentMeasure. ParentMeasure is created for measures
	// with PlayerName=someplayer. ChildMeasure is created for all measures and points to ParentMeasure as
	// referenced in PlayerName=[section].
	ChildMeasure* child = new ChildMeasure;
	UINT maxValue = 0;

	// Read settings from the ini-file
	LPCTSTR str = ReadConfigString(section, L"PlayerName", NULL);
	if (str)
	{
		if (str[0] == L'[')
		{
			// PlayerName starts with [ so use referenced section
			int len = wcslen(str) - 2;
			if (len > 0)
			{
				std::map<UINT, ChildMeasure*>::iterator it = g_Measures.begin();
				for ( ; it != g_Measures.end(); ++it)
				{
					if (wcsncmp(&str[1], it->second->parent->name.c_str(), len) == 0 &&
						wcscmp(iniFile, it->second->parent->iniFile.c_str()) == 0)
					{
						// Use same ParentMeasure as referenced section
						child->parent = it->second->parent;
						++child->parent->childCount;
						break;
					}
				}

				if (!child->parent)
				{
					// The referenced section doesn't exist
					std::wstring error = L"NowPlayingPlugin: PlayerName=";
					error += str;
					error += L" in [";
					error += section;
					error += L"] does not exist.";
					LSLog(LOG_WARNING, L"Rainmeter", error.c_str());
					delete child;
					return maxValue;
				}
			}
		}
		else
		{
			// ParentMeasure is created when PlayerName is an actual player (and not a reference)
			ParentMeasure* parent = new ParentMeasure;
			parent->name = section;
			parent->iniFile = iniFile;

			if (_wcsicmp(L"AIMP", str) == 0)
			{
				parent->player = CPlayerAIMP::Create();
			}
			else if (_wcsicmp(L"CAD", str) == 0)
			{
				parent->player = CPlayerCAD::Create();
			}
			else if (_wcsicmp(L"foobar2000", str) == 0)
			{
				parent->player = CPlayerFoobar::Create();
			}
			else if (_wcsicmp(L"iTunes", str) == 0)
			{
				parent->player = CPlayerITunes::Create();
			}
			else if (_wcsicmp(L"MediaMonkey", str) == 0)
			{
				parent->player = CPlayerWinamp::Create(WA_MEDIAMONKEY);
			}
			else if (_wcsicmp(L"Spotify", str) == 0)
			{
				parent->player = CPlayerSpotify::Create();
			}
			else if (_wcsicmp(L"WinAmp", str) == 0)
			{
				parent->player = CPlayerWinamp::Create(WA_WINAMP);
			}
			else if (_wcsicmp(L"WLM", str) == 0)
			{
				parent->player = CPlayerWLM::Create();
			}
			else if (_wcsicmp(L"WMP", str) == 0)
			{
				parent->player = CPlayerWMP::Create();
			}
			else
			{
				if (_wcsicmp(L"MusicBee", str) == 0)
				{
					// TODO: Remove this in a few weeks (left here for MusicBee backwards compatibility)
					MessageBox(NULL, L"Due to some internal changes in the NowPlaying plugin, PlayerName=MusicBee is not valid any longer.\n\nPlease edit the skin and change to PlayerName=CAD to continue use with MusicBee.", L"NowPlaying", MB_OK | MB_ICONINFORMATION | MB_TOPMOST);
				}

				std::wstring error = L"NowPlayingPlugin: PlayerName=";
				error += str;
				error += L" in section [";
				error += section;
				error += L"] is not valid.";
				LSLog(LOG_ERROR, L"Rainmeter", error.c_str());
				delete parent;
				delete child;
				return maxValue;
			}

			parent->id = id;
			parent->childCount = 1;
			parent->player->AddInstance();
			parent->playerPath = ReadConfigString(section, L"PlayerPath", L"");
			parent->trackChangeAction = ReadConfigString(section, L"TrackChangeAction", L"");

			if (!parent->trackChangeAction.empty())
			{
				// Get window handle to send the bang later on
				parent->window = FindMeterWindow(parent->iniFile);
				parent->trackCount = 1;
			}

			str = ReadConfigString(section, L"DisableLeadingZero", L"0");
			if (str)
			{
				parent->disableLeadingZero = (1 == _wtoi(str));
			}

			child->parent = parent;
		}
	}

	str = ReadConfigString(section, L"PlayerType", NULL);
	if (str)
	{
		if (_wcsicmp(L"ARTIST", str) == 0)
		{
			child->type = MEASURE_ARTIST;
		}
		else if (_wcsicmp(L"TITLE", str) == 0)
		{
			child->type = MEASURE_TITLE;
		}
		else if (_wcsicmp(L"ALBUM", str) == 0)
		{
			child->type = MEASURE_ALBUM;
		}
		else if (_wcsicmp(L"COVER", str) == 0)
		{
			child->type = MEASURE_COVER;
		}
		else if (_wcsicmp(L"DURATION", str) == 0)
		{
			child->type = MEASURE_DURATION;
		}
		else if (_wcsicmp(L"POSITION", str) == 0)
		{
			child->type = MEASURE_POSITION;
		}
		else if (_wcsicmp(L"PROGRESS", str) == 0)
		{
			child->type = MEASURE_PROGRESS;
			maxValue = 100;
		}
		else if (_wcsicmp(L"RATING", str) == 0)
		{
			child->type = MEASURE_RATING;
			maxValue = 5;
		}
		else if (_wcsicmp(L"STATE", str) == 0)
		{
			child->type = MEASURE_STATE;
		}
		else if (_wcsicmp(L"STATUS", str) == 0)
		{
			child->type = MEASURE_STATUS;
		}
		else if (_wcsicmp(L"VOLUME", str) == 0)
		{
			child->type = MEASURE_VOLUME;
			maxValue = 100;
		}
		else if (_wcsicmp(L"LYRICS", str) == 0)
		{
			child->type = MEASURE_LYRICS;
		}
		else if (_wcsicmp(L"FILE", str) == 0)
		{
			child->type = MEASURE_FILE;
		}
		else
		{
			std::wstring error = L"NowPlayingPlugin: PlayerType=";
			error += str;
			error += L" in section [";
			error += section;
			error += L"] is not valid.";
			LSLog(LOG_WARNING, L"Rainmeter", error.c_str());
		}

		child->parent->player->AddMeasure(child->type);
	}

	g_Measures[id] = child;
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
	std::map<UINT, ChildMeasure*>::iterator i = g_Measures.find(id);
	if (i != g_Measures.end())
	{
		ChildMeasure* child = (*i).second;
		ParentMeasure* parent = child->parent;
		CPlayer* player = parent->player;

		if (--parent->childCount == 0)
		{
			player->RemoveInstance();
			delete parent;
		}

		delete child;
		g_Measures.erase(i);

		if (g_Measures.empty())
		{
			CInternet::Finalize();
		}
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
	std::map<UINT, ChildMeasure*>::iterator i = g_Measures.find(id);
	if (i != g_Measures.end())
	{
		ChildMeasure* child = (*i).second;
		ParentMeasure* parent = child->parent;
		CPlayer* player = parent->player;

		if (parent->id == id)
		{
			player->UpdateMeasure();

			// Execute TrackChangeAction= if necessary
			if (!parent->trackChangeAction.empty() &&
				parent->trackCount != player->GetTrackCount())
			{
				ExecuteCommand(parent->trackChangeAction, parent->window);
				parent->trackCount = player->GetTrackCount();
			}
		}

		switch (child->type)
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

		case MEASURE_VOLUME:
			return player->GetVolume();

		case MEASURE_STATE:
			return (UINT)player->GetState();

		case MEASURE_STATUS:
			return (UINT)player->IsInitialized();
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
	std::map<UINT, ChildMeasure*>::iterator i = g_Measures.find(id);
	if (i != g_Measures.end())
	{
		ChildMeasure* child = (*i).second;
		ParentMeasure* parent = child->parent;
		CPlayer* player = parent->player;
		static WCHAR buffer[32];

		switch (child->type)
		{
		case MEASURE_ARTIST:
			return player->GetArtist();

		case MEASURE_TITLE:
			return player->GetTitle();

		case MEASURE_ALBUM:
			return player->GetAlbum();

		case MEASURE_LYRICS:
			return player->GetLyrics();

		case MEASURE_COVER:
			return player->GetCoverPath();

		case MEASURE_FILE:
			return player->GetFilePath();

		case MEASURE_DURATION:
			SecondsToTime(player->GetDuration(), parent->disableLeadingZero, buffer);
			return buffer;

		case MEASURE_POSITION:
			SecondsToTime(player->GetPosition(), parent->disableLeadingZero, buffer);
			return buffer;

		case MEASURE_PROGRESS:
			_itow(player->GetDuration() ? ((player->GetPosition() * 100) / player->GetDuration()) : 0, buffer, 10);
			return buffer;

		case MEASURE_RATING:
			_itow(player->GetRating(), buffer, 10);
			return buffer;

		case MEASURE_VOLUME:
			_itow(player->GetVolume(), buffer, 10);
			return buffer;

		case MEASURE_STATE:
			_itow(player->GetState(), buffer, 10);
			return buffer;

		case MEASURE_STATUS:
			_itow((UINT)player->IsInitialized(), buffer, 10);
			return buffer;
		}
	}
	else
	{
		return L"Error: Invalid player name.";
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
	std::map<UINT, ChildMeasure*>::iterator i = g_Measures.find(id);
	if (i != g_Measures.end())
	{
		ChildMeasure* child = (*i).second;
		ParentMeasure* parent = child->parent;
		CPlayer* player = parent->player;

		if (!player->IsInitialized())
		{
			if (_wcsicmp(bang, L"OpenPlayer") == 0 || _wcsicmp(bang, L"TogglePlayer") == 0)
			{
				player->OpenPlayer(parent->playerPath);
			}
		}
		else if (_wcsicmp(bang, L"Pause") == 0)
		{
			player->Pause();
		}
		else if (_wcsicmp(bang, L"Play") == 0)
		{
			player->Play();
		}
		else if (_wcsicmp(bang, L"PlayPause") == 0)
		{
			(player->GetState() != PLAYER_PLAYING) ? player->Play() : player->Pause();
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
		else if (_wcsicmp(bang, L"ClosePlayer") == 0 || _wcsicmp(bang, L"TogglePlayer") == 0)
		{
			player->ClosePlayer();
		}
		else
		{
			LPCTSTR arg = wcschr(bang, L' ');

			if (arg)
			{
				++arg;	// Skip the space

				if (wcsnicmp(bang, L"SetPosition", 11) == 0)
				{
					int position = (_wtoi(arg) * player->GetDuration()) / 100;
					if (arg[0] == L'+' || arg[0] == L'-')
					{
						position += player->GetPosition();
					}

					player->SetPosition(position);
				}
				else if (wcsnicmp(bang, L"SetRating", 9) == 0)
				{
					player->SetRating(_wtoi(arg));
				}
				else if (wcsnicmp(bang, L"SetVolume", 9) == 0)
				{
					int volume = _wtoi(arg);
					if (arg[0] == L'+' || arg[0] == L'-')
					{
						// Relative to current volume
						volume += player->GetVolume();
					}

					player->SetVolume(volume);
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
	return 1001;
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

void SecondsToTime(UINT seconds, bool leadingZero, WCHAR* buffer)
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
		_snwprintf_s(buffer, 32, _TRUNCATE, leadingZero ? L"%i:%02i:%02i" : L"%02i:%02i:%02i", hours, mins, secs);
	}
	else
	{
		_snwprintf_s(buffer, 32, _TRUNCATE, leadingZero ? L"%i:%02i" : L"%02i:%02i", mins, secs);
	}
}

void ExecuteCommand(std::wstring& command, HWND wnd)
{
	COPYDATASTRUCT cds;
	cds.dwData = 1;
	cds.cbData = (DWORD)(command.size() + 1) * sizeof(WCHAR);
	cds.lpData = (void*)command.c_str();

	// Send bang to the Rainmeter window
	SendMessage(wnd, WM_COPYDATA, (WPARAM)NULL, (LPARAM)&cds);
}

bool BelongToSameProcess(HWND wnd)
{
	DWORD procId = 0;
	GetWindowThreadProcessId(wnd, &procId);

	return (procId == GetCurrentProcessId());
}

HWND FindMeterWindow(HWND parent)
{
	HWND wnd = NULL;
	while (wnd = FindWindowEx(parent, wnd, L"RainmeterMeterWindow", NULL))
	{
		if (BelongToSameProcess(wnd))
		{
			return wnd;
		}
	}

	return NULL;
}

HWND FindMeterWindow(const std::wstring& iniFile)
{
	std::wstring str = PluginBridge(L"getconfig", iniFile.c_str());
	if (!str.empty())
	{
		str = PluginBridge(L"getwindow", str.c_str());
		if (str != L"error")
		{
			return (HWND)UlongToPtr(wcstoul(str.c_str(), NULL, 10));
		}
	}

	return FindMeterWindow(NULL);  // Use old way to find
}
