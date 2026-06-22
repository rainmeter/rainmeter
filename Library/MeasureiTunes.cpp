/* Copyright (C) 2026 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "StdAfx.h"
#include "MeasureiTunes.h"
#include "ConfigParser.h"
#include "Logger.h"
#include "NowPlaying/SDKs/iTunes/iTunesCOMInterface.h"
#include "Skin.h"

const int VOLUME_STEP = 5;
const size_t BUFFER_SIZE = 256;

enum COMMAND_TYPE : int
{
	COMMAND_POWER,
	COMMAND_TOGGLEITUNES,
	COMMAND_TOGGLEVISUALS,
	COMMAND_SOUNDVOLUMEUP,
	COMMAND_SOUNDVOLUMEDOWN,
	COMMAND_BACKTRACK,
	COMMAND_FASTFORWARD,
	COMMAND_NEXTTRACK,
	COMMAND_PAUSE,
	COMMAND_PLAY,
	COMMAND_PLAYFILE,
	COMMAND_PLAYPAUSE,
	COMMAND_PREVIOUSTRACK,
	COMMAND_RESUME,
	COMMAND_REWIND,
	COMMAND_STOP,
	COMMAND_GETPLAYERBUTTONSSTATE,
	COMMAND_PLAYERBUTTONCLICKED,
	COMMAND_CONVERTFILE,
	COMMAND_CONVERTFILES,
	COMMAND_TRACK,
	COMMAND_TRACKS,
	COMMAND_FILE2,
	COMMAND_FILES2,
	COMMAND_TRACK2,
	COMMAND_TRACKS2,
	COMMAND_CHECKVERSION,
	COMMAND_GETITOBJECTBYID,
	COMMAND_CREATEPLAYLIST,
	COMMAND_OPENURL,
	COMMAND_GOTOMUSICSTOREHOMEPAGE,
	COMMAND_UPDATEIPOD,
	COMMAND_QUIT,
	COMMAND_CREATEEQPRESET,
	COMMAND_CREATEPLAYLISTINSOURCE,
	COMMAND_SUBSCRIBETOPODCAST,
	COMMAND_UPDATEPODCASTFEEDS,
	COMMAND_CREATEFOLDER,
	COMMAND_CREATEFOLDERINSOURCE,
	COMMAND_GETITOBJECTPERSISTENTIDS,
	COMMAND_GETSOUNDVOLUME,
	COMMAND_SETSOUNDVOLUME,
	COMMAND_GETMUTE,
	COMMAND_SETMUTE,
	COMMAND_GETPLAYERSTATE,
	COMMAND_GETPLAYERPOSITIONPERCENT,
	COMMAND_GETPLAYERPOSITION,
	COMMAND_SETPLAYERPOSITION,
	COMMAND_GETEQENABLED,
	COMMAND_SETEQENABLED,
	COMMAND_GETCURRENTEQPRESET,
	COMMAND_SETCURRENTEQPRESET,
	COMMAND_GETCURRENTSTREAMTITLE,
	COMMAND_GETCURRENTSTREAMURL,
	COMMAND_GETCURRENTTRACK_ALBUM,
	COMMAND_GETCURRENTTRACK_ARTIST,
	COMMAND_GETCURRENTTRACK_BITRATE,
	COMMAND_GETCURRENTTRACK_BPM,
	COMMAND_GETCURRENTTRACK_COMMENT,
	COMMAND_GETCURRENTTRACK_COMPOSER,
	COMMAND_GETCURRENTTRACK_EQ,
	COMMAND_GETCURRENTTRACK_GENRE,
	COMMAND_GETCURRENTTRACK_KINDASSTRING,
	COMMAND_GETCURRENTTRACK_NAME,
	COMMAND_GETCURRENTTRACK_RATING,
	COMMAND_GETCURRENTTRACK_SAMPLERATE,
	COMMAND_GETCURRENTTRACK_SIZE,
	COMMAND_GETCURRENTTRACK_TIME,
	COMMAND_GETCURRENTTRACK_TRACKCOUNT,
	COMMAND_GETCURRENTTRACK_TRACKNUMBER,
	COMMAND_GETCURRENTTRACK_YEAR,
	COMMAND_GETCURRENTTRACK_ARTWORK,
	COMMAND_GETVERSION,
	COMMAND_COUNT
};

const static WCHAR* CommandName[COMMAND_COUNT] =
{
	L"Power",
	L"ToggleiTunes",
	L"ToggleVisuals",
	L"SoundVolumeUp",
	L"SoundVolumeDown",
	L"BackTrack",
	L"FastForward",
	L"NextTrack",
	L"Pause",
	L"Play",
	L"PlayFile",
	L"PlayPause",
	L"PreviousTrack",
	L"Resume",
	L"Rewind",
	L"Stop",
	L"GetPlayerButtonsState",
	L"PlayerButtonClicked",
	L"ConvertFile",
	L"ConvertFiles",
	L"ConvertTrack",
	L"ConvertTracks",
	L"ConvertFile2",
	L"ConvertFiles2",
	L"ConvertTrack2",
	L"ConvertTracks2",
	L"CheckVersion",
	L"GetITObjectByID",
	L"CreatePlaylist",
	L"OpenURL",
	L"GotoMusicStoreHomePage",
	L"UpdateIPod",
	L"Quit",
	L"CreateEQPreset",
	L"CreatePlaylistInSource",
	L"SubscribeToPodcast",
	L"UpdatePodcastFeeds",
	L"CreateFolder",
	L"CreateFolderInSource",
	L"GetITObjectPersistentIDs",
	L"GetSoundVolume",
	L"SetSoundVolume",
	L"GetMute",
	L"SetMute",
	L"GetPlayerState",
	L"GetPlayerPositionPercent",
	L"GetPlayerPosition",
	L"SetPlayerPosition",
	L"GetEQEnabled",
	L"SetEQEnabled",
	L"GetCurrentEQPreset",
	L"SetCurrentEQPreset",
	L"GetCurrentStreamTitle",
	L"GetCurrentStreamURL",
	L"GetCurrentTrackAlbum",
	L"GetCurrentTrackArtist",
	L"GetCurrentTrackBitrate",
	L"GetCurrentTrackBPM",
	L"GetCurrentTrackComment",
	L"GetCurrentTrackComposer",
	L"GetCurrentTrackEQ",
	L"GetCurrentTrackGenre",
	L"GetCurrentTrackKindAsString",
	L"GetCurrentTrackName",
	L"GetCurrentTrackRating",
	L"GetCurrentTrackSampleRate",
	L"GetCurrentTrackSize",
	L"GetCurrentTrackTime",
	L"GetCurrentTrackTrackCount",
	L"GetCurrentTrackTrackNumber",
	L"GetCurrentTrackYear",
	L"GetCurrentTrackArtwork",
	L"GetVersion"
};

_COM_SMARTPTR_TYPEDEF(IiTunes, __uuidof(IiTunes));
_COM_SMARTPTR_TYPEDEF(IITTrack, __uuidof(IITTrack));
_COM_SMARTPTR_TYPEDEF(IITArtworkCollection, __uuidof(IITArtworkCollection));
_COM_SMARTPTR_TYPEDEF(IITArtwork, __uuidof(IITArtwork));
_COM_SMARTPTR_TYPEDEF(IITBrowserWindow, __uuidof(IITBrowserWindow));

namespace {

IiTunesPtr g_iTunes;
IITTrackPtr g_CurrentTrack;
bool g_CoInitialized = false;
bool g_InstanceCreated = false;
bool g_iTunesAboutToPromptUserToQuit = false;
int g_MeasureCount = 0;
clock_t g_LastTrackClock = 0;

bool IsITunesRunning()
{
	return FindWindow(L"iTunesApp", L"iTunes") || FindWindow(L"iTunes", L"iTunes");
}

class CiTunesEventHandler : public _IiTunesEvents
{
public:
	CiTunesEventHandler() : m_RefCount(0), m_TypeInfo(nullptr)
	{
		ITypeLib* typeLib = nullptr;
		if (SUCCEEDED(::LoadRegTypeLib(LIBID_iTunesLib, 1, 5, 0x00, &typeLib)) && typeLib)
		{
			typeLib->GetTypeInfoOfGuid(DIID__IiTunesEvents, &m_TypeInfo);
			typeLib->Release();
		}
	}

	~CiTunesEventHandler()
	{
		if (m_TypeInfo)
		{
			m_TypeInfo->Release();
		}
	}

	HRESULT STDMETHODCALLTYPE QueryInterface(REFIID iid, void** object)
	{
		if (iid == IID_IDispatch || iid == DIID__IiTunesEvents || iid == IID_IUnknown)
		{
			*object = this;
			AddRef();
			return S_OK;
		}
		return E_NOINTERFACE;
	}

	ULONG STDMETHODCALLTYPE AddRef()
	{
		return InterlockedIncrement(&m_RefCount);
	}

	ULONG STDMETHODCALLTYPE Release()
	{
		long count = InterlockedDecrement(&m_RefCount);
		if (count == 0)
		{
			delete this;
			return 0;
		}
		return count;
	}

	HRESULT STDMETHODCALLTYPE GetTypeInfoCount(UINT*) { return E_NOTIMPL; }
	HRESULT STDMETHODCALLTYPE GetTypeInfo(UINT, LCID, ITypeInfo**) { return E_NOTIMPL; }
	HRESULT STDMETHODCALLTYPE GetIDsOfNames(const IID&, LPOLESTR*, UINT, LCID, DISPID*) { return E_NOTIMPL; }
	HRESULT STDMETHODCALLTYPE Invoke(DISPID dispidMember, REFIID, LCID, WORD, DISPPARAMS*, VARIANT*, EXCEPINFO*, UINT*)
	{
		switch (dispidMember)
		{
		case 9:
			g_CurrentTrack.Release();
			g_iTunes->Quit();
			g_iTunes.Release();
			g_InstanceCreated = false;
			g_iTunesAboutToPromptUserToQuit = true;
			break;
		default:
			break;
		}

		return S_OK;
	}

private:
	long m_RefCount;
	ITypeInfo* m_TypeInfo;
};

void InitEventHandler()
{
	IConnectionPointContainer* connectionPointContainer = nullptr;
	if (FAILED(g_iTunes->QueryInterface(IID_IConnectionPointContainer, (void**)&connectionPointContainer)) || !connectionPointContainer)
	{
		return;
	}

	IConnectionPoint* connectionPoint = nullptr;
	if (SUCCEEDED(connectionPointContainer->FindConnectionPoint(DIID__IiTunesEvents, &connectionPoint)) && connectionPoint)
	{
		DWORD advise = 0;
		CiTunesEventHandler* eventHandler = new CiTunesEventHandler();
		connectionPoint->Advise(eventHandler, &advise);
		connectionPoint->Release();
	}

	connectionPointContainer->Release();
}

bool EnsureCOM(MeasureiTunes* measure)
{
	if (!g_CoInitialized)
	{
		g_CoInitialized = SUCCEEDED(CoInitialize(nullptr));
		if (!g_CoInitialized)
		{
			LogErrorF(measure, L"iTunes: Unable to initialize COM");
		}
	}

	return g_CoInitialized;
}

bool CreateITunesInstance(MeasureiTunes* measure, bool logError)
{
	if (g_InstanceCreated)
	{
		return true;
	}

	if (SUCCEEDED(g_iTunes.CreateInstance(CLSID_iTunesApp, nullptr, CLSCTX_LOCAL_SERVER)))
	{
		g_InstanceCreated = true;
		InitEventHandler();
		return true;
	}

	if (logError)
	{
		LogErrorF(measure, L"iTunes: Unable to create instance");
	}

	return false;
}

bool EnsureITunes(MeasureiTunes* measure)
{
	if (!EnsureCOM(measure))
	{
		return false;
	}

	if (g_InstanceCreated)
	{
		return true;
	}

	if (!IsITunesRunning())
	{
		g_iTunesAboutToPromptUserToQuit = false;
		return false;
	}

	return !g_iTunesAboutToPromptUserToQuit && CreateITunesInstance(measure, true);
}

bool UpdateCurrentTrack(const std::wstring& baseDir, const std::wstring& defaultArtworkPath, std::wstring& currentArtworkPath)
{
	clock_t currentClock = clock();
	if (g_LastTrackClock == 0 || currentClock - g_LastTrackClock > CLOCKS_PER_SEC)
	{
		currentArtworkPath = baseDir + defaultArtworkPath;
		if (g_CurrentTrack != nullptr)
		{
			g_CurrentTrack.Release();
		}
		if (FAILED(g_iTunes->get_CurrentTrack(&g_CurrentTrack)) || !g_CurrentTrack)
		{
			return false;
		}

		g_LastTrackClock = currentClock;

		IITArtworkCollectionPtr artworkCollection;
		if (SUCCEEDED(g_CurrentTrack->get_Artwork(&artworkCollection)))
		{
			long count = 0;
			artworkCollection->get_Count(&count);

			IITArtworkPtr artwork;
			ITArtworkFormat artworkFormat;
			if (count > 0 &&
				SUCCEEDED(artworkCollection->get_Item(1, &artwork)) &&
				SUCCEEDED(artwork->get_Format(&artworkFormat)))
			{
				std::wstring artworkPath = baseDir + L"\\iTunesArtwork";
				CreateDirectory(artworkPath.c_str(), nullptr);

				switch (artworkFormat)
				{
				case ITArtworkFormatJPEG:
					artworkPath += L"\\artwork.jpg";
					break;
				case ITArtworkFormatPNG:
					artworkPath += L"\\artwork.png";
					break;
				case ITArtworkFormatBMP:
					artworkPath += L"\\artwork.bmp";
					break;
				default:
					artworkPath.clear();
					break;
				}

				if (!artworkPath.empty())
				{
					_bstr_t path = artworkPath.c_str();
					if (SUCCEEDED(artwork->SaveArtworkToFile(path)))
					{
						currentArtworkPath = artworkPath;
					}
				}
			}
		}
	}

	return g_CurrentTrack != nullptr;
}

COMMAND_TYPE ParseCommand(const WCHAR* command)
{
	for (int i = 0; i < COMMAND_COUNT; ++i)
	{
		if (CommandName[i] && _wcsicmp(CommandName[i], command) == 0)
		{
			return (COMMAND_TYPE)i;
		}
	}

	return COMMAND_COUNT;
}

COMMAND_TYPE ParseBang(const WCHAR* args)
{
	if (_wcsicmp(args, L"Backtrack") == 0) return COMMAND_BACKTRACK;
	if (_wcsicmp(args, L"FastForward") == 0) return COMMAND_FASTFORWARD;
	if (_wcsicmp(args, L"NextTrack") == 0) return COMMAND_NEXTTRACK;
	if (_wcsicmp(args, L"Pause") == 0) return COMMAND_PAUSE;
	if (_wcsicmp(args, L"Play") == 0) return COMMAND_PLAY;
	if (_wcsicmp(args, L"PlayPause") == 0) return COMMAND_PLAYPAUSE;
	if (_wcsicmp(args, L"Power") == 0) return COMMAND_POWER;
	if (_wcsicmp(args, L"PreviousTrack") == 0) return COMMAND_PREVIOUSTRACK;
	if (_wcsicmp(args, L"Resume") == 0) return COMMAND_RESUME;
	if (_wcsicmp(args, L"Rewind") == 0) return COMMAND_REWIND;
	if (_wcsicmp(args, L"Stop") == 0) return COMMAND_STOP;
	if (_wcsicmp(args, L"SoundVolumeUp") == 0) return COMMAND_SOUNDVOLUMEUP;
	if (_wcsicmp(args, L"SoundVolumeDown") == 0) return COMMAND_SOUNDVOLUMEDOWN;
	if (_wcsicmp(args, L"ToggleiTunes") == 0) return COMMAND_TOGGLEITUNES;
	if (_wcsicmp(args, L"ToggleVisuals") == 0) return COMMAND_TOGGLEVISUALS;
	if (_wcsicmp(args, L"UpdateiPod") == 0) return COMMAND_UPDATEIPOD;
	if (_wcsicmp(args, L"UpdatePodcastFeeds") == 0) return COMMAND_UPDATEPODCASTFEEDS;
	if (_wcsicmp(args, L"GotoMusicStoreHomePage") == 0) return COMMAND_GOTOMUSICSTOREHOMEPAGE;
	if (_wcsicmp(args, L"Quit") == 0) return COMMAND_QUIT;

	return COMMAND_COUNT;
}

void ExecuteITunesCommand(COMMAND_TYPE command)
{
	switch (command)
	{
	case COMMAND_POWER:
		g_iTunes->Quit();
		g_iTunes.Release();
		g_InstanceCreated = false;
		break;
	case COMMAND_TOGGLEITUNES:
	{
		IITBrowserWindowPtr browserWindow;
		if (FAILED(g_iTunes->get_BrowserWindow(&browserWindow)))
		{
			break;
		}
		VARIANT_BOOL minimized;
		if (SUCCEEDED(browserWindow->get_Minimized(&minimized)))
		{
			browserWindow->put_Minimized((VARIANT_TRUE == minimized) ? VARIANT_FALSE : VARIANT_TRUE);
		}
		break;
	}
	case COMMAND_TOGGLEVISUALS:
	{
		VARIANT_BOOL visualsEnabled;
		if (SUCCEEDED(g_iTunes->get_VisualsEnabled(&visualsEnabled)))
		{
			g_iTunes->put_VisualsEnabled((VARIANT_TRUE == visualsEnabled) ? VARIANT_FALSE : VARIANT_TRUE);
		}
		break;
	}
	case COMMAND_SOUNDVOLUMEUP:
	{
		long volume;
		g_iTunes->get_SoundVolume(&volume);
		volume += VOLUME_STEP;
		(volume > 100) ? volume = 100 : 0;
		g_iTunes->put_SoundVolume(volume);
		break;
	}
	case COMMAND_SOUNDVOLUMEDOWN:
	{
		long volume;
		g_iTunes->get_SoundVolume(&volume);
		volume -= VOLUME_STEP;
		(volume < 0) ? volume = 0 : 0;
		g_iTunes->put_SoundVolume(volume);
		break;
	}
	case COMMAND_BACKTRACK:
		g_iTunes->BackTrack();
		break;
	case COMMAND_FASTFORWARD:
		g_iTunes->FastForward();
		break;
	case COMMAND_NEXTTRACK:
		g_iTunes->NextTrack();
		break;
	case COMMAND_PAUSE:
		g_iTunes->Pause();
		break;
	case COMMAND_PLAY:
		g_iTunes->Pause();
		g_iTunes->Play();
		break;
	case COMMAND_PLAYPAUSE:
		g_iTunes->PlayPause();
		break;
	case COMMAND_PREVIOUSTRACK:
		g_iTunes->PreviousTrack();
		break;
	case COMMAND_RESUME:
		g_iTunes->Resume();
		break;
	case COMMAND_REWIND:
		g_iTunes->Rewind();
		break;
	case COMMAND_STOP:
		g_iTunes->Stop();
		break;
	case COMMAND_GOTOMUSICSTOREHOMEPAGE:
		g_iTunes->GotoMusicStoreHomePage();
		break;
	case COMMAND_UPDATEIPOD:
		g_iTunes->UpdateIPod();
		break;
	case COMMAND_QUIT:
		g_iTunes->Quit();
		break;
	case COMMAND_UPDATEPODCASTFEEDS:
		g_iTunes->UpdatePodcastFeeds();
		break;
	default:
		break;
	}
}

}  // namespace

MeasureiTunes::MeasureiTunes(Skin* skin, const WCHAR* name) : Measure(skin, name),
	m_Command(COMMAND_COUNT)
{
	++g_MeasureCount;
}

MeasureiTunes::~MeasureiTunes()
{
	--g_MeasureCount;
	if (g_MeasureCount == 0)
	{
		g_CurrentTrack.Release();
		if (g_InstanceCreated)
		{
			g_iTunes.Release();
			g_InstanceCreated = false;
		}
		if (g_CoInitialized)
		{
			CoUninitialize();
			g_CoInitialized = false;
		}
	}
}

void MeasureiTunes::ReadOptions(ConfigParser& parser, const WCHAR* section)
{
	Measure::ReadOptions(parser, section);

	size_t pos = m_Skin->GetFilePath().find_last_of(L"\\/");
	m_BaseDir = (pos != std::wstring::npos) ? m_Skin->GetFilePath().substr(0, pos + 1) : L"";

	const std::wstring& command = parser.ReadString(section, L"Command", L"");
	m_Command = ParseCommand(command.c_str());

	if (m_Command == COMMAND_GETCURRENTTRACK_ARTWORK)
	{
		m_DefaultTrackArtworkPath = parser.ReadString(section, L"DefaultArtwork", L"");
		m_CurrentTrackArtworkPath = m_BaseDir + m_DefaultTrackArtworkPath;
	}

	if (EnsureCOM(this) && !g_InstanceCreated && IsITunesRunning())
	{
		CreateITunesInstance(this, true);
	}
}

void MeasureiTunes::UpdateValue()
{
	if (!EnsureITunes(this))
	{
		m_Value = 0.0;
		return;
	}

	switch (m_Command)
	{
	case COMMAND_GETSOUNDVOLUME:
	{
		long volume = 0;
		g_iTunes->get_SoundVolume(&volume);
		m_Value = volume;
		return;
	}
	case COMMAND_GETPLAYERPOSITIONPERCENT:
	{
		long pos = 0;
		g_iTunes->get_PlayerPosition(&pos);
		if (!UpdateCurrentTrack(m_BaseDir, m_DefaultTrackArtworkPath, m_CurrentTrackArtworkPath))
		{
			m_Value = 0.0;
			return;
		}
		long duration = 0;
		g_CurrentTrack->get_Duration(&duration);
		m_Value = duration ? pos * 100 / duration : 0;
		return;
	}
	case COMMAND_GETPLAYERPOSITION:
	{
		long pos = 0;
		g_iTunes->get_PlayerPosition(&pos);
		m_Value = pos;
		return;
	}
	default:
		m_Value = 0.0;
		return;
	}
}

const WCHAR* MeasureiTunes::GetStringValue()
{
	m_StringValue.clear();
	if (!g_CoInitialized)
	{
		m_StringValue = L"Fail to initialize";
		return m_StringValue.c_str();
	}

	if (!g_InstanceCreated)
	{
		if (m_Command == COMMAND_GETCURRENTTRACK_ARTWORK)
		{
			m_StringValue = m_BaseDir + m_DefaultTrackArtworkPath;
		}
		return m_StringValue.c_str();
	}

	if (m_Command >= COMMAND_GETCURRENTTRACK_ALBUM && m_Command <= COMMAND_GETCURRENTTRACK_ARTWORK)
	{
		if (!UpdateCurrentTrack(m_BaseDir, m_DefaultTrackArtworkPath, m_CurrentTrackArtworkPath))
		{
			return m_StringValue.c_str();
		}

		BSTR bstrValue = nullptr;
		long longValue = 0L;
		bool bstrUsed = false;

		switch (m_Command)
		{
		case COMMAND_GETCURRENTTRACK_ALBUM:
			bstrUsed = SUCCEEDED(g_CurrentTrack->get_Album(&bstrValue));
			break;
		case COMMAND_GETCURRENTTRACK_ARTIST:
			bstrUsed = SUCCEEDED(g_CurrentTrack->get_Artist(&bstrValue));
			break;
		case COMMAND_GETCURRENTTRACK_BITRATE:
			if (SUCCEEDED(g_CurrentTrack->get_BitRate(&longValue)))
			{
				WCHAR buffer[BUFFER_SIZE];
				_snwprintf_s(buffer, _TRUNCATE, L"%dkbps", longValue);
				m_StringValue = buffer;
			}
			break;
		case COMMAND_GETCURRENTTRACK_BPM:
			if (SUCCEEDED(g_CurrentTrack->get_BPM(&longValue)))
			{
				WCHAR buffer[BUFFER_SIZE];
				_snwprintf_s(buffer, _TRUNCATE, L"%dbpm", longValue);
				m_StringValue = buffer;
			}
			break;
		case COMMAND_GETCURRENTTRACK_COMMENT:
			bstrUsed = SUCCEEDED(g_CurrentTrack->get_Comment(&bstrValue));
			break;
		case COMMAND_GETCURRENTTRACK_COMPOSER:
			bstrUsed = SUCCEEDED(g_CurrentTrack->get_Composer(&bstrValue));
			break;
		case COMMAND_GETCURRENTTRACK_EQ:
			bstrUsed = SUCCEEDED(g_CurrentTrack->get_EQ(&bstrValue));
			break;
		case COMMAND_GETCURRENTTRACK_GENRE:
			bstrUsed = SUCCEEDED(g_CurrentTrack->get_Genre(&bstrValue));
			break;
		case COMMAND_GETCURRENTTRACK_KINDASSTRING:
			bstrUsed = SUCCEEDED(g_CurrentTrack->get_KindAsString(&bstrValue));
			break;
		case COMMAND_GETCURRENTTRACK_NAME:
			bstrUsed = SUCCEEDED(g_CurrentTrack->get_Name(&bstrValue));
			break;
		case COMMAND_GETCURRENTTRACK_RATING:
			if (SUCCEEDED(g_CurrentTrack->get_Rating(&longValue)))
			{
				WCHAR buffer[BUFFER_SIZE];
				_snwprintf_s(buffer, _TRUNCATE, L"%d/100", longValue);
				m_StringValue = buffer;
			}
			break;
		case COMMAND_GETCURRENTTRACK_SAMPLERATE:
			if (SUCCEEDED(g_CurrentTrack->get_SampleRate(&longValue)))
			{
				WCHAR buffer[BUFFER_SIZE];
				_snwprintf_s(buffer, _TRUNCATE, L"%dHz", longValue);
				m_StringValue = buffer;
			}
			break;
		case COMMAND_GETCURRENTTRACK_SIZE:
			if (SUCCEEDED(g_CurrentTrack->get_Size(&longValue)))
			{
				WCHAR buffer[BUFFER_SIZE];
				if (longValue < 1024L)
				{
					_snwprintf_s(buffer, _TRUNCATE, L"%dbytes", longValue);
				}
				else if (longValue < 1024L * 1024L)
				{
					_snwprintf_s(buffer, _TRUNCATE, L"%d.%dKbytes", longValue / 1024, (longValue * 10 / 1024) % 10);
				}
				else
				{
					_snwprintf_s(buffer, _TRUNCATE, L"%d.%dMbytes", longValue / 1024 / 1024, (longValue * 10 / 1024 / 1024) % 10);
				}
				m_StringValue = buffer;
			}
			break;
		case COMMAND_GETCURRENTTRACK_TIME:
			bstrUsed = SUCCEEDED(g_CurrentTrack->get_Time(&bstrValue));
			break;
		case COMMAND_GETCURRENTTRACK_TRACKCOUNT:
			if (SUCCEEDED(g_CurrentTrack->get_TrackCount(&longValue)))
			{
				WCHAR buffer[BUFFER_SIZE];
				_snwprintf_s(buffer, _TRUNCATE, L"%d", longValue);
				m_StringValue = buffer;
			}
			break;
		case COMMAND_GETCURRENTTRACK_TRACKNUMBER:
			if (SUCCEEDED(g_CurrentTrack->get_TrackNumber(&longValue)))
			{
				WCHAR buffer[BUFFER_SIZE];
				_snwprintf_s(buffer, _TRUNCATE, L"%d", longValue);
				m_StringValue = buffer;
			}
			break;
		case COMMAND_GETCURRENTTRACK_YEAR:
			if (SUCCEEDED(g_CurrentTrack->get_Year(&longValue)))
			{
				WCHAR buffer[BUFFER_SIZE];
				_snwprintf_s(buffer, _TRUNCATE, L"%d", longValue);
				m_StringValue = buffer;
			}
			break;
		case COMMAND_GETCURRENTTRACK_ARTWORK:
			m_StringValue = m_CurrentTrackArtworkPath;
			break;
		default:
			break;
		}

		if (bstrUsed && bstrValue)
		{
			m_StringValue = bstrValue;
			SysFreeString(bstrValue);
		}

		return CheckSubstitute(m_StringValue.c_str());
	}

	m_StringValue = L"Type Incorrect";
	return m_StringValue.c_str();
}

void MeasureiTunes::Command(const std::wstring& command)
{
	if (!EnsureCOM(this))
	{
		return;
	}

	COMMAND_TYPE type = command.empty() ? m_Command : ParseBang(command.c_str());
	if (type == COMMAND_COUNT)
	{
		LogNoticeF(this, L"iTunes: Invalid Command");
		return;
	}

	if (!g_InstanceCreated)
	{
		if (type == COMMAND_POWER && CreateITunesInstance(this, false))
		{
			IITBrowserWindowPtr browserWindow;
			if (SUCCEEDED(g_iTunes->get_BrowserWindow(&browserWindow)))
			{
				browserWindow->put_Minimized(VARIANT_TRUE);
			}
		}
		return;
	}

	ExecuteITunesCommand(type);
}
