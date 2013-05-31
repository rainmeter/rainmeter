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
#include "NowPlaying.h"
#include "Internet.h"
#include "PlayerAIMP.h"
#include "PlayerCAD.h"
#include "PlayerITunes.h"
#include "PlayerSpotify.h"
#include "PlayerWinamp.h"
#include "PlayerWLM.h"
#include "PlayerWMP.h"

static std::vector<ParentMeasure*> g_ParentMeasures;
bool g_Initialized = false;
HINSTANCE g_Instance = nullptr;

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
	switch (fdwReason)
	{
	case DLL_PROCESS_ATTACH:
		g_Instance = hinstDLL;

		// Disable DLL_THREAD_ATTACH and DLL_THREAD_DETACH notification calls
		DisableThreadLibraryCalls(hinstDLL);
		break;
	}

	return TRUE;
}

PLUGIN_EXPORT void Initialize(void** data, void* rm)
{
	Measure* measure = new Measure;
	*data = measure;

	if (!g_Initialized)
	{
		Internet::Initialize();
		g_Initialized = true;
	}
}

PLUGIN_EXPORT void Reload(void* data, void* rm, double* maxValue)
{
	Measure* measure = (Measure*)data;

	// Data is stored in two structs: Measure and ParentMeasure. ParentMeasure is created for measures
	// with PlayerName=someplayer. Measure is created for all measures and points to ParentMeasure as
	// referenced in PlayerName=[section].

	// Read settings from the ini-file
	void* skin = RmGetSkin(rm);
	LPCWSTR str = RmReadString(rm, L"PlayerName", L"", FALSE);
	if (str[0] == L'[')
	{
		if (measure->parent)
		{
			// Don't let a measure measure change its parent
		}
		else
		{
			// PlayerName starts with [ so use referenced section
			++str;
			int len = wcslen(str);
			if (len > 0 && str[len - 1] == L']')
			{
				--len;

				std::vector<ParentMeasure*>::iterator iter = g_ParentMeasures.begin();
				for ( ; iter != g_ParentMeasures.end(); ++iter)
				{
					if (skin == (*iter)->skin &&
						wcsncmp(str, (*iter)->ownerName, len) == 0)
					{
						// Use same ParentMeasure as referenced section
						measure->parent = (*iter);
						++measure->parent->measureCount;

						break;
					}
				}

				if (!measure->parent)
				{
					// The referenced section doesn't exist
					std::wstring error = L"NowPlaying.dll: Invalid PlayerName=";
					error.append(str - 1, len + 2);
					error += L" in [";
					error += RmGetMeasureName(rm);
					error += L"]";
					RmLog(LOG_WARNING, error.c_str());
					return;
				}
			}
		}
	}
	else
	{
		// ParentMeasure is created when PlayerName is an actual player (and not a reference)
		ParentMeasure* parent = measure->parent;
		Player* oldPlayer = nullptr;
		if (parent)
		{
			if (parent->data != data)
			{
				// Don't let a measure-only measure become a parent measure
				return;
			}

			oldPlayer = parent->player;
		}
		else
		{
			parent = new ParentMeasure;
			g_ParentMeasures.push_back(parent);
			parent->data = data;
			parent->skin = skin;
			parent->ownerName = RmGetMeasureName(rm);
			measure->parent = parent;
		}

		if (_wcsicmp(L"AIMP", str) == 0)
		{
			parent->player = PlayerAIMP::Create();
		}
		else if (_wcsicmp(L"CAD", str) == 0)
		{
			parent->player = PlayerCAD::Create();
		}
		else if (_wcsicmp(L"foobar2000", str) == 0)
		{
			HWND fooWindow = FindWindow(L"foo_rainmeter_class", nullptr);
			if (fooWindow)
			{
				const WCHAR* error = L"Your foobar2000 plugin is out of date.\n\nDo you want to update the plugin now?";
				if (MessageBox(nullptr, error, L"Rainmeter", MB_YESNO | MB_ICONINFORMATION | MB_TOPMOST) == IDYES)
				{
					ShellExecute(nullptr, L"open", L"http://github.com/poiru/foo-cad#readme", nullptr, nullptr, SW_SHOWNORMAL);
				}
			}

			parent->player = PlayerCAD::Create();
		}
		else if (_wcsicmp(L"iTunes", str) == 0)
		{
			parent->player = PlayerITunes::Create();
		}
		else if (_wcsicmp(L"MediaMonkey", str) == 0)
		{
			parent->player = PlayerWinamp::Create(WA_MEDIAMONKEY);
		}
		else if (_wcsicmp(L"Spotify", str) == 0)
		{
			parent->player = PlayerSpotify::Create();
		}
		else if (_wcsicmp(L"WinAmp", str) == 0)
		{
			parent->player = PlayerWinamp::Create(WA_WINAMP);
		}
		else if (_wcsicmp(L"WMP", str) == 0)
		{
			parent->player = PlayerWMP::Create();
		}
		else
		{
			// Default to WLM
			parent->player = PlayerWLM::Create();

			if (_wcsicmp(L"WLM", str) != 0)
			{
				std::wstring error = L"NowPlaying.dll: Invalid PlayerName=";
				error += str;
				error += L" in [";
				error += parent->ownerName;
				error += L"]";
				RmLog(LOG_ERROR, error.c_str());
			}
		}

		parent->player->AddInstance();
		parent->playerPath = RmReadString(rm, L"PlayerPath", L"");
		parent->trackChangeAction = RmReadString(rm, L"TrackChangeAction", L"", FALSE);
		parent->disableLeadingZero = RmReadInt(rm, L"DisableLeadingZero", 0);

		if (oldPlayer)
		{
			parent->player->SetMeasures(oldPlayer->GetMeasures());

			// Remove instance here so that player doesn't have to reinitialize if PlayerName was
			// not changed.
			oldPlayer->RemoveInstance();
		}
	}

	str = RmReadString(rm, L"PlayerType", L"");
	if (_wcsicmp(L"ARTIST", str) == 0)
	{
		measure->type = MEASURE_ARTIST;
	}
	else if (_wcsicmp(L"TITLE", str) == 0)
	{
		measure->type = MEASURE_TITLE;
	}
	else if (_wcsicmp(L"ALBUM", str) == 0)
	{
		measure->type = MEASURE_ALBUM;
	}
	else if (_wcsicmp(L"COVER", str) == 0)
	{
		measure->type = MEASURE_COVER;
	}
	else if (_wcsicmp(L"DURATION", str) == 0)
	{
		measure->type = MEASURE_DURATION;
	}
	else if (_wcsicmp(L"POSITION", str) == 0)
	{
		measure->type = MEASURE_POSITION;
	}
	else if (_wcsicmp(L"PROGRESS", str) == 0)
	{
		measure->type = MEASURE_PROGRESS;
		*maxValue = 100.0;
	}
	else if (_wcsicmp(L"RATING", str) == 0)
	{
		measure->type = MEASURE_RATING;
		*maxValue = 5.0;
	}
	else if (_wcsicmp(L"STATE", str) == 0)
	{
		measure->type = MEASURE_STATE;
	}
	else if (_wcsicmp(L"STATUS", str) == 0)
	{
		measure->type = MEASURE_STATUS;
	}
	else if (_wcsicmp(L"VOLUME", str) == 0)
	{
		measure->type = MEASURE_VOLUME;
		*maxValue = 100.0;
	}
	else if (_wcsicmp(L"SHUFFLE", str) == 0)
	{
		measure->type = MEASURE_SHUFFLE;
	}
	else if (_wcsicmp(L"REPEAT", str) == 0)
	{
		measure->type = MEASURE_REPEAT;
	}
	else if (_wcsicmp(L"LYRICS", str) == 0)
	{
		RmLog(LOG_WARNING, L"NowPlaying.dll: Using undocumented PlayerType=LYRICS!");
		measure->type = MEASURE_LYRICS;
	}
	else if (_wcsicmp(L"FILE", str) == 0)
	{
		measure->type = MEASURE_FILE;
	}
	else if (_wcsicmp(L"NUMBER", str) == 0)
	{
		measure->type = MEASURE_NUMBER;
	}
	else if (_wcsicmp(L"YEAR", str) == 0)
	{
		measure->type = MEASURE_YEAR;
	}
	else
	{
		std::wstring error = L"NowPlaying.dll: Invalid PlayerType=";
		error += str;
		error += L" in [";
		error += RmGetMeasureName(rm);
		error += L"]";
		RmLog(LOG_WARNING, error.c_str());
	}

	measure->parent->player->AddMeasure(measure->type);
}

PLUGIN_EXPORT double Update(void* data)
{
	Measure* measure = (Measure*)data;
	ParentMeasure* parent = measure->parent;
	if (!parent) return 0.0;

	Player* player = parent->player;

	// Only allow parent measure to update
	if (parent->data == data)
	{
		player->UpdateMeasure();

		// Execute TrackChangeAction= if necessary
		if (!parent->trackChangeAction.empty() &&
			parent->trackCount != player->GetTrackCount())
		{
			RmExecute(parent->skin, parent->trackChangeAction.c_str());
			parent->trackCount = player->GetTrackCount();
		}
	}

	switch (measure->type)
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
		return 0.0;

	case MEASURE_RATING:
		return player->GetRating();

	case MEASURE_VOLUME:
		return player->GetVolume();

	case MEASURE_STATE:
		return player->GetState();

	case MEASURE_STATUS:
		return player->IsInitialized();

	case MEASURE_SHUFFLE:
		return player->GetShuffle();

	case MEASURE_REPEAT:
		return player->GetRepeat();

	case MEASURE_NUMBER:
		return player->GetNumber();

	case MEASURE_YEAR:
		return player->GetYear();
	}

	return 0.0;
}

PLUGIN_EXPORT LPCWSTR GetString(void* data)
{
	Measure* measure = (Measure*)data;
	ParentMeasure* parent = measure->parent;
	if (!parent) return nullptr;

	const Player* player = parent->player;
	static WCHAR buffer[32];

	switch (measure->type)
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
		_itow_s(player->GetDuration() ? ((player->GetPosition() * 100) / player->GetDuration()) : 0, buffer, 10);
		return buffer;

	case MEASURE_RATING:
		_itow_s(player->GetRating(), buffer, 10);
		return buffer;

	case MEASURE_VOLUME:
		_itow_s(player->GetVolume(), buffer, 10);
		return buffer;

	case MEASURE_STATE:
		_itow_s(player->GetState(), buffer, 10);
		return buffer;

	case MEASURE_STATUS:
		_itow_s((int)player->IsInitialized(), buffer, 10);
		return buffer;

	case MEASURE_SHUFFLE:
		_itow_s((int)player->GetShuffle(), buffer, 10);
		return buffer;

	case MEASURE_REPEAT:
		_itow_s((int)player->GetRepeat(), buffer, 10);
		return buffer;

	case MEASURE_NUMBER:
		_itow_s(player->GetNumber(), buffer, 10);
		return buffer;

	case MEASURE_YEAR:
		_itow_s(player->GetYear(), buffer, 10);
		return buffer;
	}

	return nullptr;
}

PLUGIN_EXPORT void Finalize(void* data)
{
	Measure* measure = (Measure*)data;
	ParentMeasure* parent = measure->parent;
	if (parent)
	{
		Player* player = parent->player;
		if (--parent->measureCount == 0)
		{
			player->RemoveInstance();
			delete parent;

			std::vector<ParentMeasure*>::iterator iter = std::find(g_ParentMeasures.begin(), g_ParentMeasures.end(), parent);
			g_ParentMeasures.erase(iter);

			if (g_ParentMeasures.empty())
			{
				Internet::Finalize();
			}
		}
	}

	delete measure;
}

PLUGIN_EXPORT void ExecuteBang(void* data, LPCWSTR args)
{
	Measure* measure = (Measure*)data;
	ParentMeasure* parent = measure->parent;
	if (!parent) return;

	Player* player = parent->player;

	if (!player->IsInitialized())
	{
		if (_wcsicmp(args, L"OpenPlayer") == 0 || _wcsicmp(args, L"TogglePlayer") == 0)
		{
			player->OpenPlayer(parent->playerPath);
		}
	}
	else if (_wcsicmp(args, L"Pause") == 0)
	{
		player->Pause();
	}
	else if (_wcsicmp(args, L"Play") == 0)
	{
		player->Play();
	}
	else if (_wcsicmp(args, L"PlayPause") == 0)
	{
		(player->GetState() != STATE_PLAYING) ? player->Play() : player->Pause();
	}
	else if (_wcsicmp(args, L"Next") == 0)
	{
		player->Next();
	}
	else if (_wcsicmp(args, L"Previous") == 0)
	{
		player->Previous();
	}
	else if (_wcsicmp(args, L"Stop") == 0)
	{
		player->Stop();
	}
	else if (_wcsicmp(args, L"OpenPlayer") == 0)
	{
		player->OpenPlayer(parent->playerPath);
	}
	else if (_wcsicmp(args, L"ClosePlayer") == 0 || _wcsicmp(args, L"TogglePlayer") == 0)
	{
		player->ClosePlayer();
	}
	else
	{
		LPCWSTR arg = wcschr(args, L' ');

		if (arg)
		{
			++arg;	// Skip the space

			if (wcsnicmp(args, L"SetPosition", 11) == 0)
			{
				int position = (_wtoi(arg) * (int)player->GetDuration()) / 100;
				if (arg[0] == L'+' || arg[0] == L'-')
				{
					position += player->GetPosition();
				}

				player->SetPosition(position);
			}
			else if (wcsnicmp(args, L"SetRating", 9) == 0)
			{
				int rating = _wtoi(arg);
				if (rating >= 0 && rating <= 5)
				{
					player->SetRating(rating);
				}
			}
			else if (wcsnicmp(args, L"SetVolume", 9) == 0)
			{
				int volume = _wtoi(arg);
				if (arg[0] == L'+' || arg[0] == L'-')
				{
					// Relative to current volume
					volume += player->GetVolume();
				}
					
				if (volume < 0)
				{
					volume = 0;
				}
				else if (volume > 100)
				{
					volume = 100;
				}
				player->SetVolume(volume);;
			}
			else if (wcsnicmp(args, L"SetShuffle", 9) == 0)
			{
				int state = _wtoi(arg);
				if (state == -1)
				{
					player->SetShuffle(!player->GetShuffle());
				}
				else if (state == 0 || state == 1)
				{
					player->SetShuffle((bool)state);
				}
			}
			else if (wcsnicmp(args, L"SetRepeat", 9) == 0)
			{
				int state = _wtoi(arg);
				if (state == -1)
				{
					player->SetRepeat(!player->GetRepeat());
				}
				else if (state == 0 || state == 1)
				{
					player->SetRepeat((bool)state);
				}
			}
			else
			{
				RmLog(LOG_WARNING, L"NowPlaying.dll: Unknown args");
			}
		}
		else
		{
			RmLog(LOG_WARNING, L"NowPlaying.dll: Unknown args");
		}
	}
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

	if (seconds < 0)
	{
		hours = mins = secs = 0;
	}

	if (hours)
	{
		_snwprintf_s(buffer, 32, _TRUNCATE, leadingZero ? L"%i:%02i:%02i" : L"%02i:%02i:%02i", hours, mins, secs);
	}
	else
	{
		_snwprintf_s(buffer, 32, _TRUNCATE, leadingZero ? L"%i:%02i" : L"%02i:%02i", mins, secs);
	}
}
