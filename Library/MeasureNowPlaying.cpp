// Copyright (c) Rainmeter Team. Source code licensed under GNU GPL v2 (see LICENSE file).

#include "StdAfx.h"
#include "MeasureNowPlaying.h"
#include "Rainmeter.h"
#include "../Common/StringUtil.h"
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

HINSTANCE g_Instance = nullptr;

namespace {

void DoPlayBang(Measure* measure, std::vector<std::wstring>& args, Skin* skin)
{
	((MeasureNowPlaying*)measure)->Play();
}

void DoPauseBang(Measure* measure, std::vector<std::wstring>& args, Skin* skin)
{
	((MeasureNowPlaying*)measure)->Pause();
}

void DoPlayPauseBang(Measure* measure, std::vector<std::wstring>& args, Skin* skin)
{
	((MeasureNowPlaying*)measure)->PlayPause();
}

void DoStopBang(Measure* measure, std::vector<std::wstring>& args, Skin* skin)
{
	((MeasureNowPlaying*)measure)->Stop();
}

void DoNextBang(Measure* measure, std::vector<std::wstring>& args, Skin* skin)
{
	((MeasureNowPlaying*)measure)->Next();
}

void DoPreviousBang(Measure* measure, std::vector<std::wstring>& args, Skin* skin)
{
	((MeasureNowPlaying*)measure)->Previous();
}

void DoOpenPlayerBang(Measure* measure, std::vector<std::wstring>& args, Skin* skin)
{
	((MeasureNowPlaying*)measure)->OpenPlayer();
}

void DoClosePlayerBang(Measure* measure, std::vector<std::wstring>& args, Skin* skin)
{
	((MeasureNowPlaying*)measure)->ClosePlayer();
}

void DoTogglePlayerBang(Measure* measure, std::vector<std::wstring>& args, Skin* skin)
{
	((MeasureNowPlaying*)measure)->TogglePlayer();
}

void DoSetPositionBang(Measure* measure, std::vector<std::wstring>& args, Skin* skin)
{
	((MeasureNowPlaying*)measure)->SetPosition(args[0].c_str());
}

void DoSetRatingBang(Measure* measure, std::vector<std::wstring>& args, Skin* skin)
{
	((MeasureNowPlaying*)measure)->SetRating(args[0].c_str());
}

void DoSetVolumeBang(Measure* measure, std::vector<std::wstring>& args, Skin* skin)
{
	((MeasureNowPlaying*)measure)->SetVolume(args[0].c_str());
}

void DoSetShuffleBang(Measure* measure, std::vector<std::wstring>& args, Skin* skin)
{
	((MeasureNowPlaying*)measure)->SetShuffle(args[0].c_str());
}

void DoSetRepeatBang(Measure* measure, std::vector<std::wstring>& args, Skin* skin)
{
	((MeasureNowPlaying*)measure)->SetRepeat(args[0].c_str());
}

}  // namespace

MeasureNowPlaying::MeasureNowPlaying(Skin* skin, const WCHAR* name) : Measure(skin, name),
	m_Type(MEASURE_NONE),
	m_Parent()
{
	static const bool s_BangsRegistered = []()
	{
		const UINT typeId = TypeID<MeasureNowPlaying>();
		CommandHandler::RegisterMeasureBang(typeId, L"NowPlaying:Play", 0, DoPlayBang);
		CommandHandler::RegisterMeasureBang(typeId, L"NowPlaying:Pause", 0, DoPauseBang);
		CommandHandler::RegisterMeasureBang(typeId, L"NowPlaying:TogglePlay", 0, DoPlayPauseBang);
		CommandHandler::RegisterMeasureBang(typeId, L"NowPlaying:Stop", 0, DoStopBang);
		CommandHandler::RegisterMeasureBang(typeId, L"NowPlaying:Next", 0, DoNextBang);
		CommandHandler::RegisterMeasureBang(typeId, L"NowPlaying:Previous", 0, DoPreviousBang);
		CommandHandler::RegisterMeasureBang(typeId, L"NowPlaying:OpenPlayer", 0, DoOpenPlayerBang);
		CommandHandler::RegisterMeasureBang(typeId, L"NowPlaying:ClosePlayer", 0, DoClosePlayerBang);
		CommandHandler::RegisterMeasureBang(typeId, L"NowPlaying:TogglePlayer", 0, DoTogglePlayerBang);
		CommandHandler::RegisterMeasureBang(typeId, L"NowPlaying:SetPosition", 1, DoSetPositionBang);
		CommandHandler::RegisterMeasureBang(typeId, L"NowPlaying:SetRating", 1, DoSetRatingBang);
		CommandHandler::RegisterMeasureBang(typeId, L"NowPlaying:SetVolume", 1, DoSetVolumeBang);
		CommandHandler::RegisterMeasureBang(typeId, L"NowPlaying:SetShuffle", 1, DoSetShuffleBang);
		CommandHandler::RegisterMeasureBang(typeId, L"NowPlaying:SetRepeat", 1, DoSetRepeatBang);
		return true;
	} ();
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
			m_Parent = nullptr;
		}
	}
}

void MeasureNowPlaying::ReadOptions(ConfigParser& parser, std::wstring_view section)
{
	Measure::ReadOptions(parser, section);

	// Data is stored in two structs: Measure and ParentMeasure. ParentMeasure is created for measures
	// with PlayerName=someplayer. Measure is created for all measures and points to ParentMeasure as
	// referenced in PlayerName=[section].

	// Read settings from the ini-file
	const std::wstring_view playerName = parser.ReadString(section, L"PlayerName", L"", { .sectionVariables = false });
	if (playerName.starts_with(L'['))
	{
		if (m_Parent)
		{
			// Don't let a measure measure change its parent
		}
		else
		{
			// PlayerName starts with [ so use the ParentMeasure of the referenced section
			if (m_Skin && playerName.length() >= 3 && playerName.back() == L']')
			{
				const std::wstring_view name = playerName.substr(1, playerName.length() - 2);
				Measure* measure = m_Skin->GetMeasure(name);
				if (measure && measure->GetTypeID() == TypeID<MeasureNowPlaying>())
				{
					auto* referenced = (MeasureNowPlaying*)measure;
					if (referenced->m_Parent && referenced->m_Parent->owner == referenced)
					{
						m_Parent = referenced->m_Parent;
						++m_Parent->measureCount;
					}
				}

				if (!m_Parent)
				{
					// The referenced section doesn't exist, or is not a player measure
					LogWarningF(this, L"Invalid PlayerName=%.*s", (int)playerName.length(), playerName.data());
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
			m_Parent->owner = this;
		}

		if (StringUtil::EqualsIgnoreCase(playerName, L"AIMP"))
		{
			m_Parent->player = PlayerAIMP::Create();
		}
		else if (StringUtil::EqualsIgnoreCase(playerName, L"CAD"))
		{
			m_Parent->player = PlayerCAD::Create();
		}
		else if (StringUtil::EqualsIgnoreCase(playerName, L"foobar2000"))
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
		else if (StringUtil::EqualsIgnoreCase(playerName, L"iTunes"))
		{
			m_Parent->player = PlayerITunes::Create();
		}
		else if (StringUtil::EqualsIgnoreCase(playerName, L"MediaMonkey"))
		{
			m_Parent->player = PlayerWinamp::Create(WA_MEDIAMONKEY);
		}
		else if (StringUtil::EqualsIgnoreCase(playerName, L"Spotify"))
		{
			m_Parent->player = PlayerSpotify::Create();
		}
		else if (StringUtil::EqualsIgnoreCase(playerName, L"WinAmp"))
		{
			m_Parent->player = PlayerWinamp::Create(WA_WINAMP);
		}
		else if (StringUtil::EqualsIgnoreCase(playerName, L"WMP"))
		{
			m_Parent->player = PlayerWMP::Create();
		}
		else
		{
			// Default to WLM
			m_Parent->player = PlayerWLM::Create();

			if (!StringUtil::EqualsIgnoreCase(playerName, L"WLM"))
			{
				LogErrorF(this, L"Invalid PlayerName=%.*s", (int)playerName.length(), playerName.data());
			}
		}

		m_Parent->player->AddInstance();
		parser.ReadString(m_Parent->playerPath, section, L"PlayerPath", L"");
		parser.ReadString(m_Parent->trackChangeAction, section, L"TrackChangeAction", L"", { .sectionVariables = false });
		m_Parent->disableLeadingZero = parser.ReadBool(section, L"DisableLeadingZero", false);

		if (oldPlayer)
		{
			m_Parent->player->SetMeasures(oldPlayer->GetMeasures());

			// Remove instance here so that player doesn't have to reinitialize if PlayerName was
			// not changed.
			oldPlayer->RemoveInstance();
		}
	}

	static constexpr ConfigParser::EnumOption<MeasureType> s_PlayerTypes[] =
	{
		{ L"ARTIST", MEASURE_ARTIST },
		{ L"TITLE", MEASURE_TITLE },
		{ L"ALBUM", MEASURE_ALBUM },
		{ L"COVER", MEASURE_COVER },
		{ L"DURATION", MEASURE_DURATION },
		{ L"POSITION", MEASURE_POSITION },
		{ L"PROGRESS", MEASURE_PROGRESS },
		{ L"RATING", MEASURE_RATING },
		{ L"STATE", MEASURE_STATE },
		{ L"STATUS", MEASURE_STATUS },
		{ L"VOLUME", MEASURE_VOLUME },
		{ L"SHUFFLE", MEASURE_SHUFFLE },
		{ L"REPEAT", MEASURE_REPEAT },
		{ L"LYRICS", MEASURE_LYRICS },
		{ L"FILE", MEASURE_FILE },
		{ L"NUMBER", MEASURE_NUMBER },
		{ L"YEAR", MEASURE_YEAR },
		{ L"GENRE", MEASURE_GENRE },
	};
	parser.ReadEnum(m_Type, section, L"PlayerType", MEASURE_NONE, s_PlayerTypes);

	if (m_Type == MEASURE_PROGRESS || m_Type == MEASURE_VOLUME)
	{
		m_MaxValue = 100.0;
	}
	else if (m_Type == MEASURE_RATING)
	{
		m_MaxValue = 5.0;
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

Player* MeasureNowPlaying::GetInitializedPlayer() const
{
	if (!m_Parent) return nullptr;

	Player* player = m_Parent->player;
	return player->IsInitialized() ? player : nullptr;
}

void MeasureNowPlaying::Play()
{
	Player* player = GetInitializedPlayer();
	if (player) player->Play();
}

void MeasureNowPlaying::Pause()
{
	Player* player = GetInitializedPlayer();
	if (player) player->Pause();
}

void MeasureNowPlaying::PlayPause()
{
	Player* player = GetInitializedPlayer();
	if (player) (player->GetState() != STATE_PLAYING) ? player->Play() : player->Pause();
}

void MeasureNowPlaying::Stop()
{
	Player* player = GetInitializedPlayer();
	if (player) player->Stop();
}

void MeasureNowPlaying::Next()
{
	Player* player = GetInitializedPlayer();
	if (player) player->Next();
}

void MeasureNowPlaying::Previous()
{
	Player* player = GetInitializedPlayer();
	if (player) player->Previous();
}

void MeasureNowPlaying::OpenPlayer()
{
	if (m_Parent) m_Parent->player->OpenPlayer(m_Parent->playerPath);
}

void MeasureNowPlaying::ClosePlayer()
{
	Player* player = GetInitializedPlayer();
	if (player) player->ClosePlayer();
}

void MeasureNowPlaying::TogglePlayer()
{
	GetInitializedPlayer() ? ClosePlayer() : OpenPlayer();
}

void MeasureNowPlaying::SetPosition(const WCHAR* arg)
{
	Player* player = GetInitializedPlayer();
	if (!player) return;

	int position = (int)(_wtof(arg) * (double)player->GetDuration()) / 100;
	if (arg[0] == L'+' || arg[0] == L'-')
	{
		position += player->GetPosition();
	}

	player->SetPosition(position);
}

void MeasureNowPlaying::SetRating(const WCHAR* arg)
{
	Player* player = GetInitializedPlayer();
	if (!player) return;

	int rating = _wtoi(arg);
	if (rating >= 0 && rating <= 5)
	{
		player->SetRating(rating);
	}
}

void MeasureNowPlaying::SetVolume(const WCHAR* arg)
{
	Player* player = GetInitializedPlayer();
	if (!player) return;

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
	player->SetVolume(volume);
}

void MeasureNowPlaying::SetShuffle(const WCHAR* arg)
{
	Player* player = GetInitializedPlayer();
	if (!player) return;

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

void MeasureNowPlaying::SetRepeat(const WCHAR* arg)
{
	Player* player = GetInitializedPlayer();
	if (!player) return;

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

void MeasureNowPlaying::Command(const std::wstring& command)
{
	const WCHAR* args = command.c_str();

	if (_wcsicmp(args, L"Pause") == 0)
	{
		Pause();
	}
	else if (_wcsicmp(args, L"Play") == 0)
	{
		Play();
	}
	else if (_wcsicmp(args, L"PlayPause") == 0)
	{
		PlayPause();
	}
	else if (_wcsicmp(args, L"Next") == 0)
	{
		Next();
	}
	else if (_wcsicmp(args, L"Previous") == 0)
	{
		Previous();
	}
	else if (_wcsicmp(args, L"Stop") == 0)
	{
		Stop();
	}
	else if (_wcsicmp(args, L"OpenPlayer") == 0)
	{
		OpenPlayer();
	}
	else if (_wcsicmp(args, L"ClosePlayer") == 0)
	{
		ClosePlayer();
	}
	else if (_wcsicmp(args, L"TogglePlayer") == 0)
	{
		TogglePlayer();
	}
	else
	{
		LPCWSTR arg = wcschr(args, L' ');

		if (arg)
		{
			++arg;	// Skip the space

			if (_wcsnicmp(args, L"SetPosition", 11) == 0)
			{
				SetPosition(arg);
			}
			else if (_wcsnicmp(args, L"SetRating", 9) == 0)
			{
				SetRating(arg);
			}
			else if (_wcsnicmp(args, L"SetVolume", 9) == 0)
			{
				SetVolume(arg);
			}
			else if (_wcsnicmp(args, L"SetShuffle", 9) == 0)
			{
				SetShuffle(arg);
			}
			else if (_wcsnicmp(args, L"SetRepeat", 9) == 0)
			{
				SetRepeat(arg);
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
