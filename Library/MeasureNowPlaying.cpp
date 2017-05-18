/* Copyright (C) 2011 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "StdAfx.h"
#include "MeasureNowPlaying.h"
#include "Rainmeter.h"
#include "NowPlaying/Internet.h"
#include "NowPlaying/PlayerAIMP.h"
#include "NowPlaying/PlayerCAD.h"
#include "NowPlaying/PlayerITunes.h"
#include "NowPlaying/PlayerSpotify.h"
#include "NowPlaying/PlayerWinamp.h"
#include "NowPlaying/PlayerWLM.h"
#include "NowPlaying/PlayerWMP.h"

struct ParentMeasure
{
	ParentMeasure() :
		player(),
		owner(),
		measureCount(1),
		trackCount(0),
		disableLeadingZero(false)
	{}

	Player* player;
	MeasureNowPlaying* owner;
	std::wstring trackChangeAction;
	std::wstring playerPath;
	UINT measureCount;
	UINT trackCount;
	bool disableLeadingZero;
};

static std::vector<ParentMeasure*> g_ParentMeasures;
bool g_Initialized = false;
HINSTANCE g_Instance = nullptr;

MeasureNowPlaying::MeasureNowPlaying(Skin* skin, const WCHAR* name) : Measure(skin, name),
	m_Type(MEASURE_NONE),
	m_Parent()
{
	if (!g_Initialized)
	{
		Internet::Initialize();
		g_Initialized = true;
	}
}

MeasureNowPlaying::~MeasureNowPlaying()
{
	if (m_Parent)
	{
		Player* player = m_Parent->player;
		if (--m_Parent->measureCount == 0)
		{
			player->RemoveInstance();
			delete m_Parent;

			auto iter = std::find(g_ParentMeasures.begin(), g_ParentMeasures.end(), m_Parent);
			g_ParentMeasures.erase(iter);

			if (g_ParentMeasures.empty())
			{
				Internet::Finalize();
				g_Initialized = false;
			}
		}
	}
}

void MeasureNowPlaying::ReadOptions(ConfigParser& parser, const WCHAR* section)
{
	Measure::ReadOptions(parser, section);

	// Data is stored in two structs: Measure and ParentMeasure. ParentMeasure is created for measures
	// with PlayerName=someplayer. Measure is created for all measures and points to ParentMeasure as
	// referenced in PlayerName=[section].

	// Read settings from the ini-file
	LPCWSTR str = parser.ReadString(section, L"PlayerName", L"", false).c_str();
	if (str[0] == L'[')
	{
		if (m_Parent)
		{
			// Don't let a measure measure change its parent
		}
		else
		{
			// PlayerName starts with [ so use referenced section
			++str;
			size_t len = wcslen(str);
			if (len > 0 && str[len - 1] == L']')
			{
				--len;

				std::vector<ParentMeasure*>::iterator iter = g_ParentMeasures.begin();
				for ( ; iter != g_ParentMeasures.end(); ++iter)
				{
					if (GetSkin() == (*iter)->owner->GetSkin() &&
						_wcsnicmp(str, (*iter)->owner->GetName(), len) == 0)
					{
						// Use same ParentMeasure as referenced section
						m_Parent = (*iter);
						++m_Parent->measureCount;

						break;
					}
				}

				if (!m_Parent)
				{
					// The referenced section doesn't exist
					LogWarningF(this, L"Invalid PlayerName=%s", str - 1);
					return;
				}
			}
		}
	}
	else
	{
		// ParentMeasure is created when PlayerName is an actual player (and not a reference)
		Player* oldPlayer = nullptr;
		if (m_Parent)
		{
			if (m_Parent->owner != this)
			{
				// Don't let a measure-only measure become a parent measure
				return;
			}

			oldPlayer = m_Parent->player;
		}
		else
		{
			m_Parent = new ParentMeasure;
			g_ParentMeasures.push_back(m_Parent);
			m_Parent->owner = this;
		}

		if (_wcsicmp(L"AIMP", str) == 0)
		{
			m_Parent->player = PlayerAIMP::Create();
		}
		else if (_wcsicmp(L"CAD", str) == 0)
		{
			m_Parent->player = PlayerCAD::Create();
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

			m_Parent->player = PlayerCAD::Create();
		}
		else if (_wcsicmp(L"iTunes", str) == 0)
		{
			m_Parent->player = PlayerITunes::Create();
		}
		else if (_wcsicmp(L"MediaMonkey", str) == 0)
		{
			m_Parent->player = PlayerWinamp::Create(WA_MEDIAMONKEY);
		}
		else if (_wcsicmp(L"Spotify", str) == 0)
		{
			m_Parent->player = PlayerSpotify::Create();
		}
		else if (_wcsicmp(L"WinAmp", str) == 0)
		{
			m_Parent->player = PlayerWinamp::Create(WA_WINAMP);
		}
		else if (_wcsicmp(L"WMP", str) == 0)
		{
			m_Parent->player = PlayerWMP::Create();
		}
		else
		{
			// Default to WLM
			m_Parent->player = PlayerWLM::Create();

			if (_wcsicmp(L"WLM", str) != 0)
			{
				LogErrorF(this, L"Invalid PlayerName=%s", str);
			}
		}

		m_Parent->player->AddInstance();
		m_Parent->playerPath = parser.ReadString(section, L"PlayerPath", L"");
		m_Parent->trackChangeAction = parser.ReadString(section, L"TrackChangeAction", L"", false);
		m_Parent->disableLeadingZero = parser.ReadInt(section, L"DisableLeadingZero", 0) != 0;

		if (oldPlayer)
		{
			m_Parent->player->SetMeasures(oldPlayer->GetMeasures());

			// Remove instance here so that player doesn't have to reinitialize if PlayerName was
			// not changed.
			oldPlayer->RemoveInstance();
		}
	}

	str = parser.ReadString(section, L"PlayerType", L"").c_str();
	if (_wcsicmp(L"ARTIST", str) == 0)
	{
		m_Type = MEASURE_ARTIST;
	}
	else if (_wcsicmp(L"TITLE", str) == 0)
	{
		m_Type = MEASURE_TITLE;
	}
	else if (_wcsicmp(L"ALBUM", str) == 0)
	{
		m_Type = MEASURE_ALBUM;
	}
	else if (_wcsicmp(L"COVER", str) == 0)
	{
		m_Type = MEASURE_COVER;
	}
	else if (_wcsicmp(L"DURATION", str) == 0)
	{
		m_Type = MEASURE_DURATION;
	}
	else if (_wcsicmp(L"POSITION", str) == 0)
	{
		m_Type = MEASURE_POSITION;
	}
	else if (_wcsicmp(L"PROGRESS", str) == 0)
	{
		m_Type = MEASURE_PROGRESS;
		m_MaxValue = 100.0;
	}
	else if (_wcsicmp(L"RATING", str) == 0)
	{
		m_Type = MEASURE_RATING;
		m_MaxValue = 5.0;
	}
	else if (_wcsicmp(L"STATE", str) == 0)
	{
		m_Type = MEASURE_STATE;
	}
	else if (_wcsicmp(L"STATUS", str) == 0)
	{
		m_Type = MEASURE_STATUS;
	}
	else if (_wcsicmp(L"VOLUME", str) == 0)
	{
		m_Type = MEASURE_VOLUME;
		m_MaxValue = 100.0;
	}
	else if (_wcsicmp(L"SHUFFLE", str) == 0)
	{
		m_Type = MEASURE_SHUFFLE;
	}
	else if (_wcsicmp(L"REPEAT", str) == 0)
	{
		m_Type = MEASURE_REPEAT;
	}
	else if (_wcsicmp(L"LYRICS", str) == 0)
	{
		LogWarningF(this, L"Using undocumented PlayerType=LYRICS!");
		m_Type = MEASURE_LYRICS;
	}
	else if (_wcsicmp(L"FILE", str) == 0)
	{
		m_Type = MEASURE_FILE;
	}
	else if (_wcsicmp(L"NUMBER", str) == 0)
	{
		m_Type = MEASURE_NUMBER;
	}
	else if (_wcsicmp(L"YEAR", str) == 0)
	{
		m_Type = MEASURE_YEAR;
	}
	else if (_wcsicmp(L"GENRE", str) == 0)
	{
		m_Type = MEASURE_GENRE;
	}
	else
	{
		LogErrorF(this, L"Invalid PlayerType=%s", str);
	}

	m_Parent->player->AddMeasure(m_Type);
}

void MeasureNowPlaying::UpdateValue()
{
	m_Value = 0.0;
	if (!m_Parent)
	{
		return;
	}

	Player* player = m_Parent->player;

	// Only allow parent measure to update
	if (m_Parent->owner == this)
	{
		player->UpdateMeasure();

		// Execute TrackChangeAction= if necessary
		if (!m_Parent->trackChangeAction.empty() &&
			m_Parent->trackCount != player->GetTrackCount())
		{
			GetRainmeter().DelayedExecuteCommand(m_Parent->trackChangeAction.c_str(), GetSkin());
			m_Parent->trackCount = player->GetTrackCount();
		}
	}

	switch (m_Type)
	{
	case MEASURE_DURATION:
		m_Value = player->GetDuration();
		break;
	case MEASURE_POSITION:
		m_Value = player->GetPosition();
		break;
	case MEASURE_PROGRESS:
		if (player->GetDuration())
		{
			m_Value = ((double)player->GetPosition() * 100.0) / player->GetDuration();
		}
		break;
	case MEASURE_RATING:
		m_Value = player->GetRating();
		break;
	case MEASURE_VOLUME:
		m_Value = player->GetVolume();
		break;
	case MEASURE_STATE:
		m_Value = player->GetState();
		break;
	case MEASURE_STATUS:
		m_Value = player->IsInitialized();
		break;
	case MEASURE_SHUFFLE:
		m_Value = player->GetShuffle();
		break;
	case MEASURE_REPEAT:
		m_Value = player->GetRepeat();
		break;
	case MEASURE_NUMBER:
		m_Value = player->GetNumber();
		break;
	case MEASURE_YEAR:
		m_Value = player->GetYear();
		break;
	}
}

const WCHAR* MeasureNowPlaying::GetStringValue()
{
	if (!m_Parent) return nullptr;

	const Player* player = m_Parent->player;
	static WCHAR buffer[32];
	const WCHAR* str = nullptr;
	switch (m_Type)
	{
	case MEASURE_ARTIST:
		str = player->GetArtist();
		break;

	case MEASURE_TITLE:
		str = player->GetTitle();
		break;

	case MEASURE_ALBUM:
		str = player->GetAlbum();
		break;

	case MEASURE_LYRICS:
		str = player->GetLyrics();
		break;

	case MEASURE_COVER:
		str = player->GetCoverPath();
		break;

	case MEASURE_FILE:
		str = player->GetFilePath();
		break;

	case MEASURE_DURATION:
		SecondsToTime(player->GetDuration(), m_Parent->disableLeadingZero, buffer);
		str = buffer;
		break;

	case MEASURE_POSITION:
		SecondsToTime(player->GetPosition(), m_Parent->disableLeadingZero, buffer);
		str = buffer;
		break;

	case MEASURE_GENRE:
		str = player->GetGenre();
		break;
	}

	return str ? CheckSubstitute(str) : nullptr;
}

void MeasureNowPlaying::Command(const std::wstring& command)
{
	const WCHAR* args = command.c_str();

	if (!m_Parent) return;

	Player* player = m_Parent->player;

	if (!player->IsInitialized())
	{
		if (_wcsicmp(args, L"OpenPlayer") == 0 || _wcsicmp(args, L"TogglePlayer") == 0)
		{
			player->OpenPlayer(m_Parent->playerPath);
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
		player->OpenPlayer(m_Parent->playerPath);
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

			if (_wcsnicmp(args, L"SetPosition", 11) == 0)
			{
				int position = (int)(_wtof(arg) * (double)player->GetDuration()) / 100;
				if (arg[0] == L'+' || arg[0] == L'-')
				{
					position += player->GetPosition();
				}

				player->SetPosition(position);
			}
			else if (_wcsnicmp(args, L"SetRating", 9) == 0)
			{
				int rating = _wtoi(arg);
				if (rating >= 0 && rating <= 5)
				{
					player->SetRating(rating);
				}
			}
			else if (_wcsnicmp(args, L"SetVolume", 9) == 0)
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
			else if (_wcsnicmp(args, L"SetShuffle", 9) == 0)
			{
				int state = _wtoi(arg);
				if (state == -1)
				{
					player->SetShuffle(!player->GetShuffle());
				}
				else if (state == 0 || state == 1)
				{
					player->SetShuffle(state != 0);
				}
			}
			else if (_wcsnicmp(args, L"SetRepeat", 9) == 0)
			{
				int state = _wtoi(arg);
				if (state == -1)
				{
					player->SetRepeat(!player->GetRepeat());
				}
				else if (state == 0 || state == 1)
				{
					player->SetRepeat(state != 0);
				}
			}
			else
			{
				LogWarningF(this, L"Invalid !CommandMeasure");
			}
		}
		else
		{
			LogWarningF(this, L"Invalid !CommandMeasure");
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
