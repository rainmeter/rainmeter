/*
  Copyright (C) 2009 Elestel

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

#include <windows.h>
#include <comdef.h>
#include "iTunesCOMInterface.h"
#include "../../Library/Export.h"	// Rainmeter's exported functions
#include <map>
#include <time.h>

const int VOLUME_STEP = 5;

/* The exported functions */
extern "C"
{
__declspec( dllexport ) UINT Initialize(HMODULE instance, LPCTSTR iniFile, LPCTSTR section, UINT id);
__declspec( dllexport ) void Finalize(HMODULE instance, UINT id);
__declspec( dllexport ) LPCTSTR GetString(UINT id, UINT flags);
__declspec( dllexport ) UINT Update(UINT id);
__declspec( dllexport ) UINT GetPluginVersion();
__declspec( dllexport ) LPCTSTR GetPluginAuthor();
__declspec( dllexport ) void ExecuteBang(LPCTSTR args, UINT id);
}

enum COMMAND_TYPE
{
	COMMAND_POWER,
	COMMAND_TOGGLEITUNES,
	COMMAND_TOGGLEVISUALS,
	COMMAND_SOUNDVOLUMEUP,
	COMMAND_SOUNDVOLUMEDOWN,
// Player Controls
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
// Conversion Methods
	COMMAND_CONVERTFILE,
	COMMAND_CONVERTFILES,
	COMMAND_TRACK,
	COMMAND_TRACKS,
	COMMAND_FILE2,
	COMMAND_FILES2,
	COMMAND_TRACK2,
	COMMAND_TRACKS2,
// Miscellaneous Methods
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
// Collection Properties
	//TODO: Sources
	//TODO: Encoders
	//TODO: EQPresets
	//TODO: Visuals
	//TODO: Windows
// Sound Properties
	COMMAND_GETSOUNDVOLUME,
	COMMAND_SETSOUNDVOLUME,
	COMMAND_GETMUTE,
	COMMAND_SETMUTE,
// Player Properties
	COMMAND_GETPLAYERSTATE,
	COMMAND_GETPLAYERPOSITIONPERCENT,
	COMMAND_GETPLAYERPOSITION,
	COMMAND_SETPLAYERPOSITION,
// Encoder Properties
	//TODO: CurrentEncoder
// Visual Properties
	//TODO: VisualsEnabled
	//TODO: FullScreenVisuals
	//TODO: VisualSize
	//TODO: CurrentVisual
// EQ Properties
	COMMAND_GETEQENABLED,
	COMMAND_SETEQENABLED,
	COMMAND_GETCURRENTEQPRESET,
	COMMAND_SETCURRENTEQPRESET,
// Streaming Properties
	COMMAND_GETCURRENTSTREAMTITLE,
	COMMAND_GETCURRENTSTREAMURL,
// Miscellaneous Properties
	//TODO: BrowserWindow
	//TODO: EQWindow
	//TODO: LibrarySource
	//TODO: LibraryPlaylist
	// CurrentTrack
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
		//TODO: other CurrentTrack properties
	//TODO: CurrentPlaylist
	COMMAND_GETVERSION,
	//TODO: AppCommandMessageProcessingEnabled
	//TODO: ForceToForegroundOnDialog
	//TODO: CanSetShuffle
	//TODO: CanSetSongRepeat
	//TODO: ConvertOperationStatus
	//TODO: SoundVolumeControlEnabled
	//TODO: LibraryXMLPath
	//TODO: ITObjectPersistentIDHigh
	//TODO: ITObjectPersistentIDLow

	COMMAND_COUNT
};

const static wchar_t* CommandName[COMMAND_COUNT] =
{
	L"Power",					// COMMAND_POWER
	L"ToggleiTunes",			// COMMAND_TOGGLEITUNES
	L"ToggleVisuals",			// COMMAND_TOGGLEVISUALS
	L"SoundVolumeUp",			// COMMAND_SOUNDVOLUMEUP
	L"SoundVolumeDown",			// COMMAND_SOUNDVOLUMEDOWN
// Player Controls
	L"BackTrack",				// COMMAND_BACKTRACK
	L"FastForward",				// COMMAND_FASTFORWARD
	L"NextTrack",				// COMMAND_NEXTTRACK
	L"Pause",					// COMMAND_PAUSE
	L"Play",					// COMMAND_PLAY
	L"PlayFile",				// COMMAND_PLAYFILE
	L"PlayPause",				// COMMAND_PLAYPAUSE
	L"PreviousTrack",			// COMMAND_PREVIOUSTRACK
	L"Resume",					// COMMAND_RESUME
	L"Rewind",					// COMMAND_REWIND
	L"Stop",					// COMMAND_STOP
	L"GetPlayerButtonsState",	// COMMAND_GETPLAYERBUTTONSSTATE
	L"PlayerButtonClicked",		// COMMAND_PLAYERBUTTONCLICKED
// Conversion Methods
	L"ConvertFile",				// COMMAND_CONVERTFILE
	L"ConvertFiles",			// COMMAND_CONVERTFILES
	L"ConvertTrack",			// COMMAND_TRACK
	L"ConvertTracks",			// COMMAND_TRACKS
	L"ConvertFile2",			// COMMAND_FILE2
	L"ConvertFiles2",			// COMMAND_FILES2
	L"ConvertTrack2",			// COMMAND_TRACK2
	L"ConvertTracks2",			// COMMAND_TRACKS2
// Miscellaneous Methods
	L"CheckVersion",			// COMMAND_CHECKVERSION
	L"GetITObjectByID",			// COMMAND_GETITOBJECTBYID
	L"CreatePlaylist",			// COMMAND_CREATEPLAYLIST
	L"OpenURL",					// COMMAND_OPENURL
	L"GotoMusicStoreHomePage",	// COMMAND_GOTOMUSICSTOREHOMEPAGE
	L"UpdateIPod",				// COMMAND_UPDATEIPOD
	L"Quit",					// COMMAND_QUIT
	L"CreateEQPreset",			// COMMAND_CREATEEQPRESET
	L"CreatePlaylistInSource",	// COMMAND_CREATEPLAYLISTINSOURCE
	L"SubscribeToPodcast",		// COMMAND_SUBSCRIBETOPODCAST
	L"UpdatePodcastFeeds",		// COMMAND_UPDATEPODCASTFEEDS
	L"CreateFolder",			// COMMAND_CREATEFOLDER
	L"CreateFolderInSource",	// COMMAND_CREATEFOLDERINSOURCE
	L"GetITObjectPersistentIDs",// COMMAND_GETITOBJECTPERSISTENTIDS
// Collection Properties
	//TODO: Sources
	//TODO: Encoders
	//TODO: EQPresets
	//TODO: Visuals
	//TODO: Windows
// Sound Properties
	L"GetSoundVolume",			// COMMAND_GETSOUNDVOLUME
	L"SetSoundVolume",			// COMMAND_SETSOUNDVOLUME
	L"GetMute",					// COMMAND_GETMUTE
	L"SetMute",					// COMMAND_SETMUTE
// Player Properties
	L"GetPlayerState",			// COMMAND_GETPLAYERSTATE
	L"GetPlayerPositionPercent",// COMMAND_GETPLAYERPOSITIONPERCENT
	L"GetPlayerPosition",		// COMMAND_GETPLAYERPOSITION
	L"SetPlayerPosition",		// COMMAND_SETPLAYERPOSITION
// Encoder Properties
	//TODO: CurrentEncoder
// Visual Properties
	//TODO: VisualsEnabled
	//TODO: FullScreenVisuals
	//TODO: VisualSize
	//TODO: CurrentVisual
// EQ Properties
	L"GetEQEnabled",			// COMMAND_GETEQENABLED
	L"SetEQEnabled",			// COMMAND_SETEQENABLED
	L"GetCurrentEQPreset",		// COMMAND_GETCURRENTEQPRESET
	L"SetCurrentEQPreset",		// COMMAND_SETCURRENTEQPRESET
// Streaming Properties
	L"GetCurrentStreamTitle",	// COMMAND_GETCURRENTSTREAMTITLE
	L"GetCurrentStreamURL",		// COMMAND_GETCURRENTSTREAMURL
// Miscellaneous Properties
	//TODO: BrowserWindow
	//TODO: EQWindow
	//TODO: LibrarySource
	//TODO: LibraryPlaylist
	// CurrentTrack
	L"GetCurrentTrackAlbum",		// COMMAND_GETCURRENTTRACK_ALBUM
	L"GetCurrentTrackArtist",		// COMMAND_GETCURRENTTRACK_ARTIST
	L"GetCurrentTrackBitrate",		// COMMAND_GETCURRENTTRACK_BITRATE
	L"GetCurrentTrackBPM",			// COMMAND_GETCURRENTTRACK_BPM
	L"GetCurrentTrackComment",		// COMMAND_GETCURRENTTRACK_COMMENT
	L"GetCurrentTrackComposer",		// COMMAND_GETCURRENTTRACK_COMPOSER
	L"GetCurrentTrackEQ",			// COMMAND_GETCURRENTTRACK_EQ
	L"GetCurrentTrackGenre",		// COMMAND_GETCURRENTTRACK_GENRE
	L"GetCurrentTrackKindAsString",	// COMMAND_GETCURRENTTRACK_KINDASSTRING
	L"GetCurrentTrackName",			// COMMAND_GETCURRENTTRACK_NAME
	L"GetCurrentTrackRating",		// COMMAND_GETCURRENTTRACK_RATING
	L"GetCurrentTrackSampleRate",	// COMMAND_GETCURRENTTRACK_SAMPLERATE
	L"GetCurrentTrackSize",			// COMMAND_GETCURRENTTRACK_SIZE
	L"GetCurrentTrackTime",			// COMMAND_GETCURRENTTRACK_TIME
	L"GetCurrentTrackTrackCount",	// COMMAND_GETCURRENTTRACK_TRACKCOUNT
	L"GetCurrentTrackTrackNumber",	// COMMAND_GETCURRENTTRACK_TRACKNUMBER
	L"GetCurrentTrackYear",			// COMMAND_GETCURRENTTRACK_YEAR
	L"GetCurrentTrackArtwork",		// COMMAND_GETCURRENTTRACK_ARTWORK
		//TODO: other CurrentTrack properties
	// TODO: CurrentPlaylist
	L"GetVersion",					// COMMAND_GETVERSION
	//TODO: AppCommandMessageProcessingEnabled
	//TODO: ForceToForegroundOnDialog
	//TODO: CanSetShuffle
	//TODO: CanSetSongRepeat
	//TODO: ConvertOperationStatus
	//TODO: SoundVolumeControlEnabled
	//TODO: LibraryXMLPath
	//TODO: ITObjectPersistentIDHigh
	//TODO: ITObjectPersistentIDLow
};

_COM_SMARTPTR_TYPEDEF(IiTunes, __uuidof(IiTunes));
_COM_SMARTPTR_TYPEDEF(IITTrack, __uuidof(IITTrack));
_COM_SMARTPTR_TYPEDEF(IITArtworkCollection, __uuidof(IITArtworkCollection));
_COM_SMARTPTR_TYPEDEF(IITArtwork, __uuidof(IITArtwork));
_COM_SMARTPTR_TYPEDEF(IITBrowserWindow, __uuidof(IITBrowserWindow));

static IiTunesPtr iTunes;
static bool CoInitialized = false;
static bool InstanceCreated = false;

typedef std::map<UINT, COMMAND_TYPE> CCommandIdMap;
static CCommandIdMap CommandIdMap;

static wchar_t BaseDir[MAX_PATH];

static IITTrackPtr CurrentTrack;
static wchar_t CurrentTrackArtworkPath[MAX_PATH];
static wchar_t DefaultTrackArtworkPath[MAX_PATH];
static bool updateCurrentTrack()
{
	static clock_t lastClock = 0;
	clock_t currentClock = clock();
	if (0 == lastClock || currentClock - lastClock > CLOCKS_PER_SEC)
	{
		wsprintf(CurrentTrackArtworkPath, L"%s%s", BaseDir, DefaultTrackArtworkPath);
		if (CurrentTrack != nullptr)
			CurrentTrack.Release();
		if (FAILED(iTunes->get_CurrentTrack(&CurrentTrack)) || !CurrentTrack)
			return false;

		lastClock = currentClock;

		IITArtworkCollectionPtr artworkCollection;
		if (SUCCEEDED(CurrentTrack->get_Artwork(&artworkCollection)))
		{
			long count;
			artworkCollection->get_Count(&count);

			IITArtworkPtr artwork;
			ITArtworkFormat artworkFormat;
			if (count > 0 &&
				SUCCEEDED(artworkCollection->get_Item(1, &artwork)) &&
				SUCCEEDED(artwork->get_Format(&artworkFormat)))
			{
				_bstr_t path;

				wsprintf(CurrentTrackArtworkPath, L"%s\\iTunesArtwork", BaseDir);
				CreateDirectory(CurrentTrackArtworkPath, nullptr);

				switch (artworkFormat)
				{
				case ITArtworkFormatJPEG:
					wcscat(CurrentTrackArtworkPath, L"\\artwork.jpg");
					break;
				case ITArtworkFormatPNG :
					wcscat(CurrentTrackArtworkPath, L"\\artwork.png");
					break;
				case ITArtworkFormatBMP:
					wcscat(CurrentTrackArtworkPath, L"\\artwork.bmp");
					break;
				}
				path = CurrentTrackArtworkPath;
				if (FAILED(artwork->SaveArtworkToFile(path)))
				{
					wsprintf(CurrentTrackArtworkPath, L"%s%s", BaseDir, DefaultTrackArtworkPath);
				}
			}
		}
	}
	return (nullptr != CurrentTrack);
}

static bool iTunesAboutToPromptUserToQuit = false;
// from http://www.codeproject.com/KB/cs/itunestray.aspx?msg=2300786#xx2300786xx
class CiTunesEventHandler : public _IiTunesEvents
{
private:
	long m_dwRefCount;
	ITypeInfo* m_pITypeInfo; // Pointer to type information.

public:
	CiTunesEventHandler()
	{
		m_dwRefCount=0;
		ITypeLib* pITypeLib = nullptr ;
		HRESULT hr = ::LoadRegTypeLib(LIBID_iTunesLib, 1, 5, 0x00, &pITypeLib) ;
		// Get type information for the interface of the object.
		hr = pITypeLib->GetTypeInfoOfGuid(DIID__IiTunesEvents, &m_pITypeInfo) ;
		pITypeLib->Release() ;
	}
	~CiTunesEventHandler()
	{}

	HRESULT STDMETHODCALLTYPE QueryInterface(REFIID iid, void **ppvObject)
	{
		if ((iid == IID_IDispatch)||(iid == DIID__IiTunesEvents)) {
			m_dwRefCount++;
			*ppvObject = this;//(_IiTunesEvents *)this;
			return S_OK;
		}
		if (iid == IID_IUnknown) {
			m_dwRefCount++;
			*ppvObject = this;//(IUnknown *)this;
			return S_OK;
		}
		return E_NOINTERFACE;
	}
	ULONG STDMETHODCALLTYPE CiTunesEventHandler::AddRef()
	{
		InterlockedIncrement(&m_dwRefCount);
		return m_dwRefCount;
	}
	ULONG STDMETHODCALLTYPE Release()
	{
		InterlockedDecrement(&m_dwRefCount);
		if (m_dwRefCount == 0) {
			delete this;
			return 0;
		}
		return m_dwRefCount;
	}
	HRESULT STDMETHODCALLTYPE GetTypeInfoCount(UINT *){return E_NOTIMPL;};
	HRESULT STDMETHODCALLTYPE GetTypeInfo(UINT,LCID,ITypeInfo ** ){return E_NOTIMPL;};
	HRESULT STDMETHODCALLTYPE GetIDsOfNames(const IID &,LPOLESTR * ,UINT,LCID,DISPID *){return E_NOTIMPL;};
	HRESULT STDMETHODCALLTYPE Invoke(DISPID dispidMember, REFIID, LCID,WORD, DISPPARAMS* pdispparams, VARIANT*,EXCEPINFO*, UINT*)
	{
		switch (dispidMember) //look in the documentation for "enum ITEvent" to get the numbers for the functions you want to implement
		{
		case 9: // AboutToPromptUserToQuitEvent
			CurrentTrack.Release();
			iTunes->Quit();
			iTunes.Release();
			InstanceCreated = false;
			iTunesAboutToPromptUserToQuit = true;
			break;
		default:
			break;
		}
		return S_OK;
	}
};

static CiTunesEventHandler* iTunesEventHandler;
static void initEventHandler()
{
	IConnectionPointContainer* icpc;
	iTunes->QueryInterface(IID_IConnectionPointContainer, (void **)&icpc);
	IConnectionPoint* icp;
	icpc->FindConnectionPoint(DIID__IiTunesEvents, &icp);
	icpc->Release();
	DWORD dwAdvise;
	iTunesEventHandler = new CiTunesEventHandler();
	icp->Advise(iTunesEventHandler, &dwAdvise);
	icp->Release();
}

/*
This function is called when the measure is initialized.
The function must return the maximum value that can be measured.
The return value can also be 0, which means that Rainmeter will
track the maximum value automatically. The parameters for this
function are:

instance  The instance of this DLL
iniFile   The name of the ini-file (usually Rainmeter.ini)
section   The name of the section in the ini-file for this measure
id		The identifier for the measure. This is used to identify the measures that use the same plugin.
*/
UINT Initialize(HMODULE instance, LPCTSTR iniFile, LPCTSTR section, UINT id)
{
	if (!CoInitialized)
	{
		::CoInitialize(nullptr);
		wcsncpy(BaseDir, iniFile, MAX_PATH);
		BaseDir[MAX_PATH - 1] = 0;
		wchar_t* lastBackslash = wcsrchr(BaseDir, L'\\');
		if (lastBackslash)
			lastBackslash[1] = 0;
		CoInitialized = true;
	}

	if (CoInitialized && !InstanceCreated && (FindWindow(L"iTunesApp", L"iTunes") || FindWindow(L"iTunes", L"iTunes")))
	{
		if (SUCCEEDED(iTunes.CreateInstance(CLSID_iTunesApp, nullptr, CLSCTX_LOCAL_SERVER)))
		{
			InstanceCreated = true;
			initEventHandler();
		}
		else
		{
			LSLog(LOG_ERROR, nullptr, L"iTunesPlugin.dll: Unable to create instance");
		}
	}

	const wchar_t* type = ReadConfigString(section, L"Command", L"");
	for (int i = 0; i < COMMAND_COUNT; i++)
	{
		if (CommandName[i] && type && _wcsicmp(CommandName[i], type) == 0)
		{
			CommandIdMap[id] = (COMMAND_TYPE)i;

			if (COMMAND_GETCURRENTTRACK_ARTWORK == (COMMAND_TYPE)i)
			{
				const wchar_t* defaultArtwork = ReadConfigString(section, L"DefaultArtwork", L"");
				wcscpy(DefaultTrackArtworkPath, defaultArtwork);
			}
		}
	}

	return 0;
}

/*
This function is called when new value should be measured.
The function returns the new value.
*/
UINT Update(UINT id)
{
	if (!CoInitialized || !InstanceCreated)
	{
		// Check if the iTunes window has appeared
		if (FindWindow(L"iTunesApp", L"iTunes") || FindWindow(L"iTunes", L"iTunes"))
		{
			if (!iTunesAboutToPromptUserToQuit && SUCCEEDED(iTunes.CreateInstance(CLSID_iTunesApp, nullptr, CLSCTX_LOCAL_SERVER)))
			{
				InstanceCreated = true;
				initEventHandler();
			}
			else
			{
				LSLog(LOG_ERROR, nullptr, L"iTunesPlugin.dll: Unable to create instance");
				return 0;
			}
		}
		else
		{
			iTunesAboutToPromptUserToQuit = false;
			return 0;
		}
	}

	CCommandIdMap::const_iterator it = CommandIdMap.find(id);
	COMMAND_TYPE command = (it != CommandIdMap.end()) ? it->second : COMMAND_COUNT;

	switch (command)
	{
		case COMMAND_GETSOUNDVOLUME: // value: 0 ~ 100
		{
			long volume;
			iTunes->get_SoundVolume(&volume);
			return volume;
		}
		case COMMAND_GETPLAYERPOSITIONPERCENT: // value: 0 ~ 100
		{
			long pos;
			iTunes->get_PlayerPosition(&pos);
			if (!updateCurrentTrack())
				return 0;
			long duration;
			CurrentTrack->get_Duration(&duration);
			return duration ? pos * 100 / duration : 0;
		}
		case COMMAND_GETPLAYERPOSITION: // value: seconds
		{
			long pos;
			iTunes->get_PlayerPosition(&pos);
			return pos;
		}
	}
	return 0;
}

/*
This function is called when the value should be
returned as a string.
*/
#define BUFFER_SIZE 256
LPCTSTR GetString(UINT id, UINT flags)
{
	//Error Check
	static wchar_t buffer[BUFFER_SIZE];
	buffer[0] = 0;
	if (!CoInitialized)
	{
		wsprintf(buffer, L"Fail to initialize");
		return buffer;
	}

	CCommandIdMap::const_iterator it = CommandIdMap.find(id);
	COMMAND_TYPE command = (it != CommandIdMap.end()) ? it->second : COMMAND_COUNT;

	if (!InstanceCreated)
	{
		//wsprintf(buffer, L"iTunes is not running");
		if (COMMAND_GETCURRENTTRACK_ARTWORK == command)
			wsprintf(buffer, L"%s%s", BaseDir, DefaultTrackArtworkPath);
		return buffer;
	}

	if (command >= COMMAND_GETCURRENTTRACK_ALBUM && command <= COMMAND_GETCURRENTTRACK_ARTWORK)
	{
		if (!updateCurrentTrack())
			return buffer;

		BSTR bstrValue;
		long longValue;
		bool bstrUsed = false;

		switch (command)
		{
			case COMMAND_GETCURRENTTRACK_ALBUM:
				bstrUsed = SUCCEEDED(CurrentTrack->get_Album(&bstrValue));
				break;
			case COMMAND_GETCURRENTTRACK_ARTIST:
				bstrUsed = SUCCEEDED(CurrentTrack->get_Artist(&bstrValue));
				break;
			case COMMAND_GETCURRENTTRACK_BITRATE:
				if (SUCCEEDED(CurrentTrack->get_BitRate(&longValue)))
				{
					wsprintf(buffer, L"%dkbps", longValue);
				}
				break;
			case COMMAND_GETCURRENTTRACK_BPM:
				if (SUCCEEDED(CurrentTrack->get_BPM(&longValue)))
				{
					wsprintf(buffer, L"%dbpm", longValue);
				}
				break;
			case COMMAND_GETCURRENTTRACK_COMMENT:
				bstrUsed = SUCCEEDED(CurrentTrack->get_Comment(&bstrValue));
				break;
			case COMMAND_GETCURRENTTRACK_COMPOSER:
				bstrUsed = SUCCEEDED(CurrentTrack->get_Composer(&bstrValue));
				break;
			case COMMAND_GETCURRENTTRACK_EQ:
				bstrUsed = SUCCEEDED(CurrentTrack->get_EQ(&bstrValue));
				break;
			case COMMAND_GETCURRENTTRACK_GENRE:
				bstrUsed = SUCCEEDED(CurrentTrack->get_Genre(&bstrValue));
				break;
			case COMMAND_GETCURRENTTRACK_KINDASSTRING:
				bstrUsed = SUCCEEDED(CurrentTrack->get_KindAsString(&bstrValue));
				break;
			case COMMAND_GETCURRENTTRACK_NAME:
				bstrUsed = SUCCEEDED(CurrentTrack->get_Name(&bstrValue));
				break;
			case COMMAND_GETCURRENTTRACK_RATING:
				if (SUCCEEDED(CurrentTrack->get_Rating(&longValue)))
				{
					wsprintf(buffer, L"%d/100", longValue);
				}
				break;
			case COMMAND_GETCURRENTTRACK_SAMPLERATE:
				if (SUCCEEDED(CurrentTrack->get_SampleRate(&longValue)))
				{
					wsprintf(buffer, L"%dHz", longValue);
				}
				break;
			case COMMAND_GETCURRENTTRACK_SIZE:
				if (SUCCEEDED(CurrentTrack->get_Size(&longValue)))
				{
					if (longValue < 1024)
						wsprintf(buffer, L"%dbytes", longValue);
					else if (longValue < 1024 * 1024)
						wsprintf(buffer, L"%d.%dKbytes", longValue/1024, (longValue*10/1024)%10);
					else
						wsprintf(buffer, L"%d.%dMbytes", longValue/1024/1024, (longValue*10/1024/1024)%10);
				}
				break;
			case COMMAND_GETCURRENTTRACK_TIME:
				bstrUsed = SUCCEEDED(CurrentTrack->get_Time(&bstrValue));
				break;
			case COMMAND_GETCURRENTTRACK_TRACKCOUNT:
				if (SUCCEEDED(CurrentTrack->get_TrackCount(&longValue)))
				{
					wsprintf(buffer, L"%d", longValue);
				}
				break;
			case COMMAND_GETCURRENTTRACK_TRACKNUMBER:
				if (SUCCEEDED(CurrentTrack->get_TrackNumber(&longValue)))
				{
					wsprintf(buffer, L"%d", longValue);
				}
				break;
			case COMMAND_GETCURRENTTRACK_YEAR:
				if (SUCCEEDED(CurrentTrack->get_Year(&longValue)))
				{
					wsprintf(buffer, L"%d", longValue);
				}
				break;
			case COMMAND_GETCURRENTTRACK_ARTWORK:
			{
				wsprintf(buffer, L"%s", CurrentTrackArtworkPath);
				break;
			}
		}

		if (bstrUsed && bstrValue)
		{
			wcsncpy(buffer, bstrValue, BUFFER_SIZE - 1);
			buffer[BUFFER_SIZE - 1] = 0;
			SysFreeString(bstrValue);
		}
		return buffer;
	}

	wsprintf(buffer, L"Type Incorrect");
	return buffer;
}

void ExecuteBang(LPCTSTR args, UINT id)
{
	if (!CoInitialized)
	{
		return;
	}

	COMMAND_TYPE command;

	if (wcslen(args) == 0)
	{
		CCommandIdMap::const_iterator it = CommandIdMap.find(id);
		command = (it != CommandIdMap.end()) ? it->second : COMMAND_COUNT;
	}
	else
	{
		if (_wcsicmp(args, L"Backtrack") == 0)
		{
			command = COMMAND_BACKTRACK;
		}
		else if (_wcsicmp(args, L"FastForward") == 0)
		{
			command = COMMAND_FASTFORWARD;
		}
		else if (_wcsicmp(args, L"NextTrack") == 0)
		{
			command = COMMAND_NEXTTRACK;
		}
		else if (_wcsicmp(args, L"Pause") == 0)
		{
			command = COMMAND_PAUSE;
		}
		else if (_wcsicmp(args, L"Play") == 0)
		{
			command = COMMAND_PLAY;
		}
		else if (_wcsicmp(args, L"PlayPause") == 0)
		{
			command = COMMAND_PLAYPAUSE;
		}
		else if (_wcsicmp(args, L"Power") == 0)
		{
			command = COMMAND_POWER;
		}
		else if (_wcsicmp(args, L"PreviousTrack") == 0)
		{
			command = COMMAND_PREVIOUSTRACK;
		}
		else if (_wcsicmp(args, L"Resume") == 0)
		{
			command = COMMAND_RESUME;
		}
		else if (_wcsicmp(args, L"Rewind") == 0)
		{
			command = COMMAND_REWIND;
		}
		else if (_wcsicmp(args, L"Stop") == 0)
		{
			command = COMMAND_STOP;
		}
		else if (_wcsicmp(args, L"SoundVolumeUp") == 0)
		{
			command = COMMAND_SOUNDVOLUMEUP;
		}
		else if (_wcsicmp(args, L"SoundVolumeDown") == 0)
		{
			command = COMMAND_SOUNDVOLUMEDOWN;
		}
		else if (_wcsicmp(args, L"ToggleiTunes") == 0)
		{
			command = COMMAND_TOGGLEITUNES;
		}
		else if (_wcsicmp(args, L"ToggleVisuals") == 0)
		{
			command = COMMAND_TOGGLEVISUALS;
		}
		else if (_wcsicmp(args, L"UpdateiPod") == 0)
		{
			command = COMMAND_UPDATEIPOD;
		}
		else if (_wcsicmp(args, L"UpdatePodcastFeeds") == 0)
		{
			command = COMMAND_UPDATEPODCASTFEEDS;
		}
		else if (_wcsicmp(args, L"GotoMusicStoreHomePage") == 0)
		{
			command = COMMAND_GOTOMUSICSTOREHOMEPAGE;
		}
		else if (_wcsicmp(args, L"Quit") == 0)
		{
			command = COMMAND_QUIT;
		}
		else
		{
			LSLog(LOG_NOTICE, nullptr, L"iTunesPlugin.dll: Invalid Command");
			return;
		}
	}

	if (!InstanceCreated)
	{
		if (COMMAND_POWER == command && CoInitialized && SUCCEEDED(iTunes.CreateInstance(CLSID_iTunesApp, nullptr, CLSCTX_LOCAL_SERVER)))
		{
			IITBrowserWindowPtr browserWindow;
			if (SUCCEEDED(iTunes->get_BrowserWindow(&browserWindow)))
			{
				browserWindow->put_Minimized(VARIANT_TRUE);
			}
			InstanceCreated = true;
		}
		return;
	}

	switch (command)
	{
		case COMMAND_POWER:
		{
			iTunes->Quit();
			iTunes.Release();
			InstanceCreated = false;
			break;
		}
		case COMMAND_TOGGLEITUNES:
		{
			IITBrowserWindowPtr browserWindow;
			if (FAILED(iTunes->get_BrowserWindow(&browserWindow)))
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
			if (SUCCEEDED(iTunes->get_VisualsEnabled(&visualsEnabled)))
			{
				iTunes->put_VisualsEnabled((VARIANT_TRUE == visualsEnabled) ? VARIANT_FALSE : VARIANT_TRUE);
			}
			break;
		}
		case COMMAND_SOUNDVOLUMEUP:
		{
			long volume;
			iTunes->get_SoundVolume(&volume);
			volume += VOLUME_STEP;
			(volume > 100) ? volume = 100 : 0;
			iTunes->put_SoundVolume(volume);
			break;
		}
		case COMMAND_SOUNDVOLUMEDOWN:
		{
			long volume;
			iTunes->get_SoundVolume(&volume);
			volume -= VOLUME_STEP;
			(volume < 0) ? volume = 0 : 0;
			iTunes->put_SoundVolume(volume);
			break;
		}

		// Player Controls
		case COMMAND_BACKTRACK:
			iTunes->BackTrack();
			break;
		case COMMAND_FASTFORWARD:
			iTunes->FastForward();
			break;
		case COMMAND_NEXTTRACK:
			iTunes->NextTrack();
			break;
		case COMMAND_PAUSE:
			iTunes->Pause();
			break;
		case COMMAND_PLAY:
			iTunes->Pause();
			iTunes->Play();
			break;
		case COMMAND_PLAYFILE:
			//TODO:
			break;
		case COMMAND_PLAYPAUSE:
			iTunes->PlayPause();
			break;
		case COMMAND_PREVIOUSTRACK:
			iTunes->PreviousTrack();
			break;
		case COMMAND_RESUME:
			iTunes->Resume();
			break;
		case COMMAND_REWIND:
			iTunes->Rewind();
			break;
		case COMMAND_STOP:
			iTunes->Stop();
			break;
		case COMMAND_GETPLAYERBUTTONSSTATE:
			//TODO:
			break;
		case COMMAND_PLAYERBUTTONCLICKED:
			//TODO:
			break;

		// Conversion Methods
		case COMMAND_CONVERTFILE:
			//TODO:
			break;
		case COMMAND_CONVERTFILES:
			//TODO:
			break;
		case COMMAND_TRACK:
			//TODO:
			break;
		case COMMAND_TRACKS:
			//TODO:
			break;
		case COMMAND_FILE2:
			//TODO:
			break;
		case COMMAND_FILES2:
			//TODO:
			break;
		case COMMAND_TRACK2:
			//TODO:
			break;
		case COMMAND_TRACKS2:
			//TODO:
			break;

		// Miscellaneous Methods
		case COMMAND_CHECKVERSION:
			//TODO:
			break;
		case COMMAND_GETITOBJECTBYID:
			//TODO:
			break;
		case COMMAND_CREATEPLAYLIST:
			//TODO:
			break;
		case COMMAND_OPENURL:
			//TODO:
			break;
		case COMMAND_GOTOMUSICSTOREHOMEPAGE:
			iTunes->GotoMusicStoreHomePage();
			break;
		case COMMAND_UPDATEIPOD:
			iTunes->UpdateIPod();
			break;
		case COMMAND_QUIT:
			iTunes->Quit();
			break;
		case COMMAND_CREATEEQPRESET:
			//TODO:
			break;
		case COMMAND_CREATEPLAYLISTINSOURCE:
			//TODO:
			break;
		case COMMAND_SUBSCRIBETOPODCAST:
			//TODO:
			break;
		case COMMAND_UPDATEPODCASTFEEDS:
			iTunes->UpdatePodcastFeeds();
			break;
		case COMMAND_CREATEFOLDER:
			//TODO:
			break;
		case COMMAND_CREATEFOLDERINSOURCE:
			//TODO:
			break;
		case COMMAND_GETITOBJECTPERSISTENTIDS:
			//TODO:
			break;
	}
}

/*
If the measure needs to free resources before quitting.
The plugin can export Finalize function, which is called
when Rainmeter quits (or refreshes).
*/
void Finalize(HMODULE instance, UINT id)
{
	if (InstanceCreated)
	{
		iTunes.Release();
		InstanceCreated = false;
	}
}

UINT GetPluginVersion()
{
	return 0002;
}

LPCTSTR GetPluginAuthor()
{
	return L"Elestel";
}