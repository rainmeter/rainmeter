// Translated by Vitaliy Diatlov
// AIMP2: SDK (02.07.2009), v2.60

#ifndef AIMP2_SDK_H
#define AIMP2_SDK_H

#include <windows.h>
#include <unknwn.h>

const char  AIMP2_RemoteClass[] = "AIMP2_RemoteInfo";
const int  AIMP2_RemoteFileSize = 2048;

const int  WM_AIMP_COMMAND = WM_USER + 0x75; // WParam = One of Command, LPARAM - Parameter

const int  WM_AIMP_STATUS_GET = 1;
const int  WM_AIMP_STATUS_SET = 2;//HiWord of LParam - Command, LoWord of LParam - Parameter
const int  WM_AIMP_CALLFUNC = 3;// LPARAM - Func ID (see below Func ID for Addons)
const int  WM_AIMP_GET_VERSION = 4;
const int  WM_AIMP_STATUS_CHANGE = 5;

const int  WM_LANG = WM_USER + 101;

  // CallBack types
const int  AIMP_STATUS_CHANGE     = 1;
const int  AIMP_PLAY_FILE         = 2;
const int  AIMP_INFO_UPDATE       = 5;  // Update Info for current track
const int  AIMP_PLAYER_STATE      = 11; // Play/Pause/Stop
const int  AIMP_EFFECT_CHANGED    = 12; // Balance, Speed, Tempo, Pitch, Flanger and etc.
const int  AIMP_EQ_CHANGED        = 13; // Sliders changed
const int  AIMP_TRACK_POS_CHANGED = 14;

  // AIMP_Status_Set / AIMP_Status_Get
const int  AIMP_STS_VOLUME      = 1;
const int  AIMP_STS_BALANCE     = 2;
const int  AIMP_STS_SPEED       = 3;
const int  AIMP_STS_Player      = 4;
const int  AIMP_STS_MUTE        = 5;
const int  AIMP_STS_REVERB      = 6;
const int  AIMP_STS_ECHO        = 7;
const int  AIMP_STS_CHORUS      = 8;
const int  AIMP_STS_Flanger     = 9;

const int  AIMP_STS_EQ_STS      = 10;
const int  AIMP_STS_EQ_SLDR01   = 11;
const int  AIMP_STS_EQ_SLDR02   = 12;
const int  AIMP_STS_EQ_SLDR03   = 13;
const int  AIMP_STS_EQ_SLDR04   = 14;
const int  AIMP_STS_EQ_SLDR05   = 15;
const int  AIMP_STS_EQ_SLDR06   = 16;
const int  AIMP_STS_EQ_SLDR07   = 17;
const int  AIMP_STS_EQ_SLDR08   = 18;
const int  AIMP_STS_EQ_SLDR09   = 19;
const int  AIMP_STS_EQ_SLDR10   = 20;
const int  AIMP_STS_EQ_SLDR11   = 21;
const int  AIMP_STS_EQ_SLDR12   = 22;
const int  AIMP_STS_EQ_SLDR13   = 23;
const int  AIMP_STS_EQ_SLDR14   = 24;
const int  AIMP_STS_EQ_SLDR15   = 25;
const int  AIMP_STS_EQ_SLDR16   = 26;
const int  AIMP_STS_EQ_SLDR17   = 27;
const int  AIMP_STS_EQ_SLDR18   = 28;

const int  AIMP_STS_REPEAT      = 29;
const int  AIMP_STS_ON_STOP     = 30;
const int  AIMP_STS_POS         = 31;
const int  AIMP_STS_LENGTH      = 32;
const int  AIMP_STS_REPEATPLS   = 33;
const int  AIMP_STS_REP_PLS_1   = 34;
const int  AIMP_STS_KBPS        = 35;
const int  AIMP_STS_KHZ         = 36;
const int  AIMP_STS_MODE        = 37;
const int  AIMP_STS_RADIO       = 38;
const int  AIMP_STS_STREAM_TYPE = 39; // Music / CDA / Radio
const int  AIMP_STS_TIMER       = 40; // Reverse / Normal
const int  AIMP_STS_SHUFFLE     = 41;
  
const int  AIMP_STS_MAIN_HWND   = 42;
const int  AIMP_STS_TC_HWND     = 43;
const int  AIMP_STS_APP_HWND    = 44;
const int  AIMP_STS_PL_HWND     = 45;
const int  AIMP_STS_EQ_HWND     = 46;
  
const int  AIMP_STS_TRAY        = 47;

  // Support Exts Flags
const int  AIMP_PLS_EXTS    = 1;
const int  AIMP_AUDIO_EXTS  = 2;

  // Menu IDs
const int  AIMP_MAIN_MENU_OPN   = 0;
const int  AIMP_MAIN_MENU_UTILS = 1;
const int  AIMP_MAIN_MENU_FNC   = 2;
const int  AIMP_MAIN_MENU_CFG   = 3;
const int  AIMP_UTILS_MENU      = 4;
const int  AIMP_PLS_MENU_ADD    = 5;
const int  AIMP_PLS_MENU_JUMP   = 6;
const int  AIMP_PLS_MENU_FNC    = 7;
const int  AIMP_PLS_MENU_SEND   = 8;
const int  AIMP_PLS_MENU_DEL    = 9;
const int  AIMP_ADD_MENU        = 10;
const int  AIMP_DEL_MENU        = 11;
const int  AIMP_FND_MENU        = 12;
const int  AIMP_SRT_MENU        = 13;
const int  AIMP_MSC_MENU        = 14;
const int  AIMP_PLS_MENU        = 15;
const int  AIMP_TRAY_UTILS      = 16;
const int  AIMP_TRAY            = 17;
const int  AIMP_EQ_LIB          = 18;
// use AIMP_UTILS_MENU overthis:
// + AIMP_MAIN_MENU_UTILS = 1;
// + AIMP_TRAY_UTILS      = 16;

  // AIMP_CallFunction
const int  AIMP_OPEN_FILES      = 0;
const int  AIMP_OPEN_DIR        = 1;
const int  AIMP_ABOUT           = 2;
const int  AIMP_SLEEP_TIMER     = 3;
const int  AIMP_UTILS_AC        = 4;
const int  AIMP_UTILS_SR        = 5;
const int  AIMP_UTILS_TE        = 6;
const int  AIMP_UTILS_CDB       = 7;
const int  AIMP_OPTIONS         = 8;
const int  AIMP_PLUGINS         = 9;
const int  AIMP_QUIT            = 10;
const int  AIMP_NEXT_VIS        = 11;
const int  AIMP_PREV_VIS        = 12;
const int AIMP_EQ_ANALOG       = 13;
const int  AIMP_TO_TRAY         = 14;
const int  AIMP_PLAY            = 15;
const int  AIMP_PAUSE           = 16;
const int  AIMP_STOP            = 17;
const int  AIMP_NEXT            = 18;
const int  AIMP_PREV            = 19;
const int  AIMP_ADD_FILES       = 20;
const int  AIMP_ADD_DIR         = 21;
const int  AIMP_ADD_PLS         = 22;
const int  AIMP_ADD_URL         = 23;
const int  AIMP_DEL_FILES       = 24;
const int  AIMP_DEL_BAD         = 25;
const int  AIMP_DEL_FROMHDD     = 26;
const int  AIMP_DEL_OFF         = 27;
const int  AIMP_DEL_OFF_HDD     = 28;
const int  AIMP_RESCAN_PLS      = 29;
const int  AIMP_SHOW_CURFILE    = 30;
const int  AIMP_SORT_INVERT     = 31;
const int  AIMP_SORT_RANDOM     = 32;
const int  AIMP_SORT_TITLE      = 33;
const int  AIMP_SORT_ARTIST     = 34;
const int  AIMP_SORT_FOLDER     = 35;
const int  AIMP_SORT_LENGTH     = 36;
const int  AIMP_SORT_RATING     = 37;
const int  AIMP_SEARCH          = 38;
const int  AIMP_OPEN_PLS        = 39;
const int  AIMP_SAVE_PLS        = 40;
const int  AIMP_PLAY_LAST       = 41;
const int  AIMP_OFF_SELECTED    = 42;
const int  AIMP_ON_SELECTED     = 43;
const int  AIMP_ADD2BOOKMARK    = 44;
const int  AIMP_EDITBOOKMARK    = 45;


  // For AIMP_GetPath
const int  AIMP_CFG_DATA     = 0;
const int  AIMP_CFG_PLS      = 1;
const int  AIMP_CFG_LNG      = 2;
const int  AIMP_CFG_SKINS    = 3;
const int  AIMP_CFG_PLUGINS  = 4;
const int  AIMP_CFG_ICONS    = 5;
const int  AIMP_CFG_ML       = 6;


  // For AIMP_QueryObject
const int  IAIMP2PlayerID           = 0x0001;
const int  IAIMP2PlaylistManagerID  = 0x0003;
const int  IAIMP2ExtendedID         = 0x0004;
const int  IAIMP2CoverArtManagerID  = 0x0005;
const int  IAIMP2PlaylistManager2ID = 0x0006;
const int  IAIMPConfigFileID        = 0x0010;
const int  IAIMPLanguageFileID      = 0x0011;



  // For AIMP_ObjectClass
const int  AIMP_EXT_LC_MESSAGE = 100;
const int  AIMP_EXT_ML_MESSAGE = 101;


  // Option Frame Position Flags
const int  AIMP_FRAME_POS_PLAY     = 1;
const int  AIMP_FRAME_POS_PLAYLIST = 2;
const int  AIMP_FRAME_POS_PLAYER   = 3;
const int  AIMP_FRAME_POS_TEMPLATE = 4;
const int  AIMP_FRAME_POS_SYSTEM   = 5;
const int  AIMP_FRAME_POS_SKINS    = 6;
const int  AIMP_FRAME_POS_LANGS    = 7;

  // AIMP_PLS_SORT_TYPE_XXX
const int  AIMP_PLS_SORT_TYPE_TITLE      = 1;
const int  AIMP_PLS_SORT_TYPE_FILENAME   = 2;
const int  AIMP_PLS_SORT_TYPE_DURATION   = 3;
const int  AIMP_PLS_SORT_TYPE_ARTIST     = 4;
const int  AIMP_PLS_SORT_TYPE_INVERSE    = 5;
const int  AIMP_PLS_SORT_TYPE_RANDOMIZE  = 6;

#pragma pack(push, 1)
struct AIMP2FileInfo
{
	DWORD cbSizeOf;
	//
	BOOL nActive;
	DWORD nBitRate;
	DWORD nChannels;
	DWORD nDuration;
	INT64 nFileSize;
	DWORD nRating;
	DWORD nSampleRate;
	DWORD nTrackID;
	//
	DWORD nAlbumLen;
	DWORD nArtistLen;
	DWORD nDateLen;
	DWORD nFileNameLen;
	DWORD nGenreLen;
	DWORD nTitleLen;
	//
	PWCHAR sAlbum;
	PWCHAR sArtist;
	PWCHAR sDate;
	PWCHAR sFileName;
	PWCHAR sGenre;
	PWCHAR sTitle;
};
#pragma pack(pop)
typedef boolean (WINAPI *AIMPPlaylistDeleteProc)(AIMP2FileInfo AFileInfo, DWORD AUserData);
typedef int (WINAPI *AIMPPlaylistSortProc)(AIMP2FileInfo AFileInfo1, AIMP2FileInfo AFileInfo2, DWORD AUserData);

typedef void (WINAPI *AIMPMenuProc)(DWORD User, void *Handle);

typedef void (WINAPI *AIMPStatusChange)(DWORD User, DWORD CallBackType);

typedef void (WINAPI *CallBackFunc)(DWORD User, DWORD dwCBType);

#pragma pack(push, 1)
struct PLSInfo
{
	PCHAR PLSName;
	DWORD FileCount;
	DWORD PLSDuration;
	INT64 PLSSize;
	int PlaylistID;
};
#pragma pack(pop)

#pragma pack(push, 1)
struct AIMPMenuInfo
{
	boolean Checkbox;
	boolean RadioItem;
	boolean Checked;
	boolean Enabled;
	int ProcPtr; // TAIMPMenuProc;
	HBITMAP Bitmap; // 0 - no bmp
	PWCHAR Caption;
	DWORD User;
};
#pragma pack(pop)

class IPLSStrings
	: public IUnknown
{
public:	
	virtual boolean WINAPI AddFile(
		PWCHAR FileName,
		AIMP2FileInfo *FileInfo
		);
	virtual boolean WINAPI DelFile(
		int ID
		);
	virtual PWCHAR WINAPI GetFileName(
		int ID
		);
	virtual boolean WINAPI GetFileInfo(
		int ID,
		AIMP2FileInfo *FileInfo
		);
	virtual DWORD WINAPI GetFileObj(
		int ID
		);
	virtual int WINAPI GetCount();
};

class IAIMP2Controller
	:public IUnknown
{
public:	
	virtual boolean WINAPI IsUnicodeVersion();
	virtual boolean WINAPI AIMP_CallBack_Set(
		DWORD dwCBType,
		CallBackFunc CallBackFuncPtr,
		DWORD User
		);
	virtual boolean WINAPI AIMP_CallBack_Remove(
		DWORD dwCBType,
		int ProcPtr
		);
	// Status
	virtual DWORD WINAPI AIMP_Status_Get(
		DWORD StatusType
		);
	virtual boolean WINAPI AIMP_Status_Set(
		DWORD StatusType,
		DWORD Value
		);
	// Playlist
	virtual boolean WINAPI AIMP_PLS_Clear(
		int ID
		);
	virtual boolean WINAPI AIMP_PLS_Delete(
		int ID
		);
	virtual boolean WINAPI AIMP_PLS_New(
		PWCHAR Name
		);	
	virtual boolean WINAPI AIMP_PLS_Info(
		int Index,
		PLSInfo *info
		);
	virtual short WINAPI AIMP_PLS_Count();
	virtual boolean WINAPI AIMP_PLS_GetFiles(
		int ID,
		IPLSStrings **Strings
		);
	virtual boolean WINAPI AIMP_PLS_GetSelFiles(
		int ID,
		IPLSStrings **Strings
		);
	virtual boolean WINAPI AIMP_PLS_AddFiles(
		int ID,
		IPLSStrings *Strings
		);
	virtual boolean WINAPI AIMP_PLS_SetPLS(
		int ID
		);	
	// System
	virtual boolean WINAPI AIMP_NewStrings(
		IPLSStrings **Strings
		);
	virtual boolean WINAPI AIMP_GetCurrentTrack(
		AIMP2FileInfo *AInfo
		);
	virtual boolean WINAPI AIMP_QueryInfo(
		PWCHAR Filename,
		AIMP2FileInfo *AInfo
		);
	virtual DWORD WINAPI AIMP_GetSystemVersion();
	virtual boolean WINAPI AIMP_CallFunction(
		DWORD FuncID
		);
	virtual int WINAPI AIMP_GetLanguage(
		PWCHAR Str,
		int ACount
		);
	virtual int WINAPI AIMP_GetCfgPath(
		PWCHAR Str,
		int ACount
		);
	virtual int WINAPI AIMP_GetSupportExts(
		DWORD Flags,
		PWCHAR Str,
		int BufSize
		);
	// Menu
	virtual DWORD WINAPI AIMP_GetSupportExts(
		DWORD Parent,
		AIMPMenuInfo *MenuInfo
		);
	virtual DWORD WINAPI AIMP_Menu_Create(
		DWORD MenuID,
		AIMPMenuInfo *MenuInfo
		);
	virtual boolean WINAPI AIMP_Menu_Update(
		int Handle,
		AIMPMenuInfo *MenuInfo
		);
	virtual boolean WINAPI AIMP_Menu_Remove(
		int Handle
		);
   // extention   
	virtual boolean WINAPI AIMP_QueryObject(
		DWORD ObjectID,
		void *Obj
		);
};

class IAIMPAddonHeader
	:public IUnknown
{
public:
	virtual BOOL WINAPI GetHasSettingsDialog() = 0;
    virtual PWCHAR WINAPI GetPluginAuthor() = 0;
    virtual PWCHAR WINAPI GetPluginName() = 0;
    virtual void WINAPI Finalize() = 0;
    virtual void WINAPI Initialize(IAIMP2Controller *AController) = 0;
    virtual void WINAPI ShowSettingsDialog(HWND AParentWindow) = 0;
};

typedef IAIMPAddonHeader *(WINAPI *AddonProc)();

typedef BOOL (WINAPI *AIMPAddonHeaderProc)(IAIMPAddonHeader *AHeader);
// Export function name: AIMP_QueryAddonEx

//==============================================================================
// Old Style Addon struct - don't use for new plugins
//==============================================================================

typedef PCHAR (WINAPI *GetPlgNameFunc)();
typedef PCHAR (WINAPI *GetAutorFunc)();
typedef void (WINAPI *InitFunc)(IAIMP2Controller *AIMP);
typedef void (WINAPI *ConfigFunc)(DWORD Handle, DWORD Win);
typedef void (WINAPI *FreeFunc)();

#pragma pack(push, 1)
struct AIMPAddonHeader
{
	DWORD version;
	DWORD DllInstance;
	GetPlgNameFunc PlgNameFuncPtr;
	GetAutorFunc AutorFuncPtr;
	InitFunc InitFuncPtr;
	ConfigFunc ConfigFuncPtr;
	FreeFunc FreeFuncPtr;
};
#pragma pack(pop)

//==============================================================================

class IAIMP2OptionFrame
	:public IUnknown
{
public: 
	virtual HWND WINAPI FrameCreate(
		HWND AParent
		);
	virtual void *WINAPI FrameData(		
		);  // reserved    
	virtual int WINAPI FrameFlags(		
		);  // See FramePositionFlags
	virtual PWCHAR WINAPI FrameName(		
		);    
	virtual HWND WINAPI FrameFree(
		HWND AWindow
		);    
	virtual void WINAPI FrameLoadConfigNotify(		
		);
	virtual void WINAPI FrameSaveConfigNotify(		
		);
};

class IAIMP2Player
	: public IUnknown
{
public:
	virtual int WINAPI Version();
	virtual boolean WINAPI PlayTrack(
		int ID,
		int ATrackIndex
		);
	virtual void WINAPI PlayOrResume();
	virtual void WINAPI Pause();
	virtual void WINAPI Stop();
	virtual void WINAPI NextTrack();
	virtual void WINAPI PrevTrack();
};

class IAIMP2PlaylistManager
	: public IUnknown
{
public:
	virtual int WINAPI AIMP_PLS_CreateFromFile(
		PWCHAR AFile,
		boolean AActivate,
		boolean AStartPlay
		);
	virtual int WINAPI AIMP_PLS_ID_ActiveGet();
	virtual boolean WINAPI AIMP_PLS_ID_ActiveSet(
		int ID
		);
	virtual int WINAPI AIMP_PLS_ID_PlayingGet();	
	virtual int WINAPI AIMP_PLS_ID_PlayingGetTrackIndex(
		int ID
		);
	virtual int WINAPI AIMP_PLS_NewEx(
		PWCHAR AName,
		boolean AActivate
		);
	virtual boolean WINAPI AIMP_PLS_PlayFile(
		PWCHAR AFileName,
		boolean AFailIfNotExists
		);
	// Playlist Processing
	virtual boolean WINAPI AIMP_PLS_DeleteByFilter(
		int ID,
		DWORD AFilterProc,
		DWORD AUserData
		);
	virtual boolean WINAPI AIMP_PLS_SortByFilter(
		int ID,
		DWORD AFilterProc,
		DWORD AUserData
		);
	// Entries
	virtual boolean WINAPI AIMP_PLS_Entry_Delete(
		int ID,
		int AEntryIndex
		);
	virtual boolean WINAPI AIMP_PLS_Entry_DeleteAll(
		int ID
		);
	virtual boolean WINAPI AIMP_PLS_Entry_FileNameGet(
		int ID,
		int AEntryIndex,
		PWCHAR PBuf,
		DWORD ABufLen
		);
	virtual boolean WINAPI AIMP_PLS_Entry_FileNameSet(
		int ID,
		int AEntryIndex,
		PWCHAR PBuf
		);
	virtual int WINAPI AIMP_PLS_Entry_FocusedGet(
		int ID
		);
	virtual boolean WINAPI AIMP_PLS_Entry_FocusedSet(
		int ID,
		int AEntryIndex
		);
	virtual boolean WINAPI AIMP_PLS_Entry_InfoGet(
		int ID,
		int AEntryIndex,
		AIMP2FileInfo *PFileInfo
		);
	virtual boolean WINAPI AIMP_PLS_Entry_InfoSet(
		int ID,
		int AEntryIndex,
		AIMP2FileInfo *PFileInfo
		);
	virtual boolean WINAPI AIMP_PLS_Entry_PlayingSwitchGet(
		int ID,
		int AEntryIndex
		);
	virtual boolean WINAPI AIMP_PLS_Entry_PlayingSwitchSet(
		int ID,
		int AEntryIndex,
		boolean ASwitch
		);
	virtual boolean WINAPI AIMP_PLS_Entry_ReloadInfo(
		int ID,
		int AEntryIndex
		);
	// Load/Save Playlists
	virtual boolean WINAPI AIMP_PM_DestroyStream(
		DWORD AHandle
		);
	virtual DWORD WINAPI AIMP_PM_ReadItem(
		DWORD AHandle,
		AIMP2FileInfo *PItem
		);
	virtual DWORD WINAPI AIMP_PM_ReadStream(
		PWCHAR AFileName,
		int *Count
		);
	virtual DWORD WINAPI AIMP_PM_SaveStream(
		PWCHAR AFileName
		);
	virtual DWORD WINAPI AIMP_PM_WriteItem(
		DWORD AHandle,
		AIMP2FileInfo *PItem
		);
	// added in 2.50 B295
	virtual boolean WINAPI AIMP_PLS_ID_PlayingSetTrackIndex(
		int ID,
		int AEntryIndex
		);
};

class IAIMP2PlaylistManager2
	: public IAIMP2PlaylistManager
{
public:
	// Count of loaded playlists
    virtual unsigned short WINAPI AIMP_PLS_Count();
    // Return = -1 - ID is not valid, otherthis - count of files in playlist 
    virtual int WINAPI AIMP_PLS_GetFilesCount(int ID);
	virtual HRESULT WINAPI AIMP_PLS_GetInfo(int ID, INT64 *ADuration, INT64 *ASize);
    virtual HRESULT WINAPI AIMP_PLS_GetName(int ID, PWCHAR ABuffer, int ABufferSizeInChars);
    // Custom Sorting, see AIMP_PLS_SORT_TYPE_XXX
    virtual HRESULT WINAPI AIMP_PLS_Sort(int ID, int ASortType);
    virtual HRESULT WINAPI AIMP_PLS_SortByTemplate(int ID, PWCHAR ABuffer, int ABufferSizeInChars);
    // if Index = -1 returned ID of current playlist.
    virtual HRESULT WINAPI AIMP_PLS_ID_By_Index(int Index, int *ID);
    // Get Formated title for Entry
    virtual HRESULT WINAPI AIMP_PLS_Entry_GetTitle(int ID, int AEntryIndex,
      PWCHAR ABuffer, int ABufferSizeInChars);
    // Set Entry to playback queue
    virtual HRESULT WINAPI AIMP_PLS_Entry_QueueRemove(int ID, int AEntryIndex);
	virtual HRESULT WINAPI AIMP_PLS_Entry_QueueSet(int ID, int AEntryIndex, BOOL AInsertAtQueueBegining);
    // Moving Entry
    virtual HRESULT WINAPI AIMP_PLS_Entry_SetPosition(int ID, int AEntryIndex, int ANewEntryIndex);
};

// See IAIMP2ExtendedID
class IAIMP2Extended
	:public IUnknown
{
public:
	virtual int WINAPI AIMP_GetPath(
		int ID,
		PWCHAR buffer,
		int bufSize
		);
    virtual boolean WINAPI AIMP_ObjectClass(
		int ID,
		void *AData,
		boolean ARegister
		);
	// User Option Dialogs
	virtual DWORD WINAPI AIMP_Options_FrameAdd(
		IAIMP2OptionFrame *AFrame
		);
	virtual DWORD WINAPI AIMP_Options_FrameRemove(
		IAIMP2OptionFrame *AFrame
		);
	virtual DWORD WINAPI AIMP_Options_ModifiedChanged(
		IAIMP2OptionFrame *AFrame
		);
};

class IAIMP2CoverArtManager
	:public IUnknown
{
public:
	// Return picture will be proportional stretched to ADisplaySize value
    virtual HBITMAP WINAPI GetCoverArtForFile(PWCHAR AFile, const SIZE *ADisplaySize);
    // Draw CoverArt of playing file, Return - cover art drawing successfuly
    // CoverArt will be proportional stretched to R value
	virtual HRESULT WINAPI CurrentCoverArtDraw(HDC DC, const RECT *R);
    // Return <> S_OK, CoverArt is empty or file are not playing
    virtual HRESULT WINAPI CurrentCoverArtGetSize(SIZE *ASize);
    // W, H - destination display sizes, function will correct sizes for proportional drawing
    // Return <> S_OK, CoverArt is empty or file are not playing
    virtual HRESULT WINAPI CurrentCoverArtCorrectSizes(int *W, int *H);
};

  // See IAIMPLanguageFileID
class IAIMPLanguageFile
	:public IUnknown
{
public:
    virtual int AIMP_Lang_Version();
    virtual int AIMP_Lang_CurrentFile(PWCHAR ABuffer, int ABufferSizeInChars);
    virtual HRESULT AIMP_Lang_IsSectionExists(PWCHAR ASectionName, int ASectionNameSizeInChars);
    virtual HRESULT AIMP_Lang_ReadString(PWCHAR ASectionName, PWCHAR AItemName, PWCHAR AValueBuffer,
      int ASectionNameSizeInChars, int AItemNameSizeInChars, int AValueBufferSizeInChars);
    // When Language changed AIMP will send to window handle "WM_LANG" message
    virtual HRESULT AIMP_Lang_Notification(HWND AWndHandle, BOOL ARegister);
};

// See IAIMPConfigFileID
class IAIMPConfigFile
	:public IUnknown
{
    // functions return null value, if value don't exists in configuration file
    virtual HRESULT AIMP_Config_ReadString(PWCHAR ASectionName, PWCHAR AItemName, PWCHAR AValueBuffer,
    	int ASectionNameSizeInChars, int AItemNameSizeInChars, int AValueBufferSizeInChars);
    virtual HRESULT AIMP_Config_ReadInteger(PWCHAR ASectionName, PWCHAR AItemName,
    	int ASectionNameSizeInChars, int AItemNameSizeInChars, int * AValue);
    //
    virtual HRESULT AIMP_Config_WriteString(PWCHAR ASectionName, PWCHAR AItemName, PWCHAR AValueBuffer,
      int ASectionNameSizeInChars, int AItemNameSizeInChars, int AValueBufferSizeInChars);
    virtual HRESULT AIMP_Config_WriteInteger(PWCHAR ASectionName, PWCHAR AItemName,
      int ASectionNameSizeInChars, int AItemNameSizeInChars, int AValue);
    //
    virtual HRESULT AIMP_Config_IsSectionExists(PWCHAR ASectionName, int ASectionNameSizeInChars);
    virtual HRESULT AIMP_Config_RemoveSection(PWCHAR ASectionName, int ASectionNameSizeInChars);
};

//==============================================================================
// V I S U A L S
//==============================================================================

const int  VIS_RQD_DATA_WAVE       = 1;
const int  VIS_RQD_DATA_SPECTRUM   = 2;
const int  VIS_RQD_NOT_SUSPEND     = 4;

typedef short WaveForm[2][512];
typedef short Spectrum[2][256];

struct AIMPVisualData
{
	int LevelR;
	int LevelL;
	Spectrum spectrum;
	WaveForm waveForm;
};

typedef AIMPVisualData *PAIMPVisualData;

class IAIMP2VisualPlugin
	:public IUnknown
{
public:
	virtual PWCHAR WINAPI AuthorName();
	virtual PWCHAR WINAPI PluginName();
    virtual PWCHAR WINAPI PluginInfo();
    virtual DWORD WINAPI PluginFlags();
    virtual BOOL WINAPI Initialize();
    virtual void WINAPI Deinitialize();
    virtual void WINAPI DisplayClick(int X, int Y);
    virtual void WINAPI DisplayRender(HDC DC, PAIMPVisualData AData);
    virtual void WINAPI DisplayResize(int AWidth, int AHeight);
};

// Export function name: AIMP_QueryVisual
typedef IAIMP2VisualPlugin *(WINAPI *AIMPVisualProc)();

#endif