

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 6.00.0366 */
/* at Wed Nov 05 13:21:00 2008
 */
/* Compiler settings for iTunesCOMInterface.idl:
    Oicf, W1, Zp8, env=Win32 (32b run)
    protocol : dce , ms_ext, c_ext, robust
    error checks: allocation ref bounds_check enum stub_data 
    VC __declspec() decoration level: 
         __declspec(uuid()), __declspec(selectany), __declspec(novtable)
         DECLSPEC_UUID(), MIDL_INTERFACE()
*/
//@@MIDL_FILE_HEADING(  )

#pragma warning( disable: 4049 )  /* more than 64k source lines */


/* verify that the <rpcndr.h> version is high enough to compile this file*/
#ifndef __REQUIRED_RPCNDR_H_VERSION__
#define __REQUIRED_RPCNDR_H_VERSION__ 475
#endif

#include "rpc.h"
#include "rpcndr.h"

#ifndef __RPCNDR_H_VERSION__
#error this stub requires an updated version of <rpcndr.h>
#endif // __RPCNDR_H_VERSION__


#ifndef __iTunesCOMInterface_h__
#define __iTunesCOMInterface_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

/* Forward Declarations */ 

#ifndef __IITObject_FWD_DEFINED__
#define __IITObject_FWD_DEFINED__
typedef interface IITObject IITObject;
#endif 	/* __IITObject_FWD_DEFINED__ */


#ifndef __IITSource_FWD_DEFINED__
#define __IITSource_FWD_DEFINED__
typedef interface IITSource IITSource;
#endif 	/* __IITSource_FWD_DEFINED__ */


#ifndef __IITSourceCollection_FWD_DEFINED__
#define __IITSourceCollection_FWD_DEFINED__
typedef interface IITSourceCollection IITSourceCollection;
#endif 	/* __IITSourceCollection_FWD_DEFINED__ */


#ifndef __IITEncoder_FWD_DEFINED__
#define __IITEncoder_FWD_DEFINED__
typedef interface IITEncoder IITEncoder;
#endif 	/* __IITEncoder_FWD_DEFINED__ */


#ifndef __IITEncoderCollection_FWD_DEFINED__
#define __IITEncoderCollection_FWD_DEFINED__
typedef interface IITEncoderCollection IITEncoderCollection;
#endif 	/* __IITEncoderCollection_FWD_DEFINED__ */


#ifndef __IITEQPreset_FWD_DEFINED__
#define __IITEQPreset_FWD_DEFINED__
typedef interface IITEQPreset IITEQPreset;
#endif 	/* __IITEQPreset_FWD_DEFINED__ */


#ifndef __IITEQPresetCollection_FWD_DEFINED__
#define __IITEQPresetCollection_FWD_DEFINED__
typedef interface IITEQPresetCollection IITEQPresetCollection;
#endif 	/* __IITEQPresetCollection_FWD_DEFINED__ */


#ifndef __IITPlaylist_FWD_DEFINED__
#define __IITPlaylist_FWD_DEFINED__
typedef interface IITPlaylist IITPlaylist;
#endif 	/* __IITPlaylist_FWD_DEFINED__ */


#ifndef __IITOperationStatus_FWD_DEFINED__
#define __IITOperationStatus_FWD_DEFINED__
typedef interface IITOperationStatus IITOperationStatus;
#endif 	/* __IITOperationStatus_FWD_DEFINED__ */


#ifndef __IITConvertOperationStatus_FWD_DEFINED__
#define __IITConvertOperationStatus_FWD_DEFINED__
typedef interface IITConvertOperationStatus IITConvertOperationStatus;
#endif 	/* __IITConvertOperationStatus_FWD_DEFINED__ */


#ifndef __IITLibraryPlaylist_FWD_DEFINED__
#define __IITLibraryPlaylist_FWD_DEFINED__
typedef interface IITLibraryPlaylist IITLibraryPlaylist;
#endif 	/* __IITLibraryPlaylist_FWD_DEFINED__ */


#ifndef __IITUserPlaylist_FWD_DEFINED__
#define __IITUserPlaylist_FWD_DEFINED__
typedef interface IITUserPlaylist IITUserPlaylist;
#endif 	/* __IITUserPlaylist_FWD_DEFINED__ */


#ifndef __IITTrack_FWD_DEFINED__
#define __IITTrack_FWD_DEFINED__
typedef interface IITTrack IITTrack;
#endif 	/* __IITTrack_FWD_DEFINED__ */


#ifndef __IITTrackCollection_FWD_DEFINED__
#define __IITTrackCollection_FWD_DEFINED__
typedef interface IITTrackCollection IITTrackCollection;
#endif 	/* __IITTrackCollection_FWD_DEFINED__ */


#ifndef __IITVisual_FWD_DEFINED__
#define __IITVisual_FWD_DEFINED__
typedef interface IITVisual IITVisual;
#endif 	/* __IITVisual_FWD_DEFINED__ */


#ifndef __IITVisualCollection_FWD_DEFINED__
#define __IITVisualCollection_FWD_DEFINED__
typedef interface IITVisualCollection IITVisualCollection;
#endif 	/* __IITVisualCollection_FWD_DEFINED__ */


#ifndef __IITWindow_FWD_DEFINED__
#define __IITWindow_FWD_DEFINED__
typedef interface IITWindow IITWindow;
#endif 	/* __IITWindow_FWD_DEFINED__ */


#ifndef __IITBrowserWindow_FWD_DEFINED__
#define __IITBrowserWindow_FWD_DEFINED__
typedef interface IITBrowserWindow IITBrowserWindow;
#endif 	/* __IITBrowserWindow_FWD_DEFINED__ */


#ifndef __IITWindowCollection_FWD_DEFINED__
#define __IITWindowCollection_FWD_DEFINED__
typedef interface IITWindowCollection IITWindowCollection;
#endif 	/* __IITWindowCollection_FWD_DEFINED__ */


#ifndef __IiTunes_FWD_DEFINED__
#define __IiTunes_FWD_DEFINED__
typedef interface IiTunes IiTunes;
#endif 	/* __IiTunes_FWD_DEFINED__ */


#ifndef ___IiTunesEvents_FWD_DEFINED__
#define ___IiTunesEvents_FWD_DEFINED__
typedef interface _IiTunesEvents _IiTunesEvents;
#endif 	/* ___IiTunesEvents_FWD_DEFINED__ */


#ifndef ___IITConvertOperationStatusEvents_FWD_DEFINED__
#define ___IITConvertOperationStatusEvents_FWD_DEFINED__
typedef interface _IITConvertOperationStatusEvents _IITConvertOperationStatusEvents;
#endif 	/* ___IITConvertOperationStatusEvents_FWD_DEFINED__ */


#ifndef __iTunesApp_FWD_DEFINED__
#define __iTunesApp_FWD_DEFINED__

#ifdef __cplusplus
typedef class iTunesApp iTunesApp;
#else
typedef struct iTunesApp iTunesApp;
#endif /* __cplusplus */

#endif 	/* __iTunesApp_FWD_DEFINED__ */


#ifndef __iTunesConvertOperationStatus_FWD_DEFINED__
#define __iTunesConvertOperationStatus_FWD_DEFINED__

#ifdef __cplusplus
typedef class iTunesConvertOperationStatus iTunesConvertOperationStatus;
#else
typedef struct iTunesConvertOperationStatus iTunesConvertOperationStatus;
#endif /* __cplusplus */

#endif 	/* __iTunesConvertOperationStatus_FWD_DEFINED__ */


#ifndef __IITArtwork_FWD_DEFINED__
#define __IITArtwork_FWD_DEFINED__
typedef interface IITArtwork IITArtwork;
#endif 	/* __IITArtwork_FWD_DEFINED__ */


#ifndef __IITArtworkCollection_FWD_DEFINED__
#define __IITArtworkCollection_FWD_DEFINED__
typedef interface IITArtworkCollection IITArtworkCollection;
#endif 	/* __IITArtworkCollection_FWD_DEFINED__ */


#ifndef __IITURLTrack_FWD_DEFINED__
#define __IITURLTrack_FWD_DEFINED__
typedef interface IITURLTrack IITURLTrack;
#endif 	/* __IITURLTrack_FWD_DEFINED__ */


#ifndef __IITAudioCDPlaylist_FWD_DEFINED__
#define __IITAudioCDPlaylist_FWD_DEFINED__
typedef interface IITAudioCDPlaylist IITAudioCDPlaylist;
#endif 	/* __IITAudioCDPlaylist_FWD_DEFINED__ */


#ifndef __IITPlaylistCollection_FWD_DEFINED__
#define __IITPlaylistCollection_FWD_DEFINED__
typedef interface IITPlaylistCollection IITPlaylistCollection;
#endif 	/* __IITPlaylistCollection_FWD_DEFINED__ */


#ifndef __IITIPodSource_FWD_DEFINED__
#define __IITIPodSource_FWD_DEFINED__
typedef interface IITIPodSource IITIPodSource;
#endif 	/* __IITIPodSource_FWD_DEFINED__ */


#ifndef __IITFileOrCDTrack_FWD_DEFINED__
#define __IITFileOrCDTrack_FWD_DEFINED__
typedef interface IITFileOrCDTrack IITFileOrCDTrack;
#endif 	/* __IITFileOrCDTrack_FWD_DEFINED__ */


#ifndef __IITPlaylistWindow_FWD_DEFINED__
#define __IITPlaylistWindow_FWD_DEFINED__
typedef interface IITPlaylistWindow IITPlaylistWindow;
#endif 	/* __IITPlaylistWindow_FWD_DEFINED__ */


/* header files for imported files */
#include "oaidl.h"
#include "ocidl.h"
#include "DispEx.h"

#ifdef __cplusplus
extern "C"{
#endif 

void * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void * ); 

/* interface __MIDL_itf_iTunesCOMInterface_0000 */
/* [local] */ 

typedef /* [public][v1_enum][uuid] */  DECLSPEC_UUID("4B73428D-2F56-4833-8E5D-65590E45FEAD") 
enum __MIDL___MIDL_itf_iTunesCOMInterface_0000_0001
    {	kITTypeLibrary_MajorVersion	= 1,
	kITTypeLibrary_MinorVersion	= 12
    } 	ITVersion;

typedef /* [public][v1_enum][uuid] */  DECLSPEC_UUID("4C25623B-F990-4ebd-8970-F29A70084B8C") 
enum __MIDL___MIDL_itf_iTunesCOMInterface_0000_0002
    {	ITUNES_E_USERCANCEL	= 0xa0040201,
	ITUNES_E_OBJECTDELETED	= 0xa0040202,
	ITUNES_E_OBJECTLOCKED	= 0xa0040203,
	ITUNES_E_CONVERSIONINPROGRESS	= 0xa0040204,
	ITUNES_E_MUSICSTOREDISABLED	= 0xa0040205,
	ITUNES_E_OBJECTEXISTS	= 0xa0040206,
	ITUNES_E_PODCASTSDISABLED	= 0xa0040207
    } 	ITErrors;



extern RPC_IF_HANDLE __MIDL_itf_iTunesCOMInterface_0000_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_iTunesCOMInterface_0000_v0_0_s_ifspec;


#ifndef __iTunesLib_LIBRARY_DEFINED__
#define __iTunesLib_LIBRARY_DEFINED__

/* library iTunesLib */
/* [helpstring][uuid][version] */ 



















typedef /* [public][public][v1_enum][uuid] */  DECLSPEC_UUID("3D502ACA-B474-4640-A2A4-C149538345EC") 
enum __MIDL___MIDL_itf_iTunesCOMInterface_0272_0001
    {	ITPlayerStateStopped	= 0,
	ITPlayerStatePlaying	= ITPlayerStateStopped + 1,
	ITPlayerStateFastForward	= ITPlayerStatePlaying + 1,
	ITPlayerStateRewind	= ITPlayerStateFastForward + 1
    } 	ITPlayerState;

typedef /* [public][public][public][v1_enum][uuid] */  DECLSPEC_UUID("5319FADA-0F39-4015-82A0-48B8B871C63C") 
enum __MIDL___MIDL_itf_iTunesCOMInterface_0272_0002
    {	ITVisualSizeSmall	= 0,
	ITVisualSizeMedium	= ITVisualSizeSmall + 1,
	ITVisualSizeLarge	= ITVisualSizeMedium + 1
    } 	ITVisualSize;

typedef /* [public][public][v1_enum][uuid] */  DECLSPEC_UUID("C8128C8D-EDE0-4f0e-AEB1-08D24A91C551") 
enum __MIDL___MIDL_itf_iTunesCOMInterface_0272_0003
    {	ITCOMDisabledReasonOther	= 0,
	ITCOMDisabledReasonDialog	= ITCOMDisabledReasonOther + 1,
	ITCOMDisabledReasonQuitting	= ITCOMDisabledReasonDialog + 1
    } 	ITCOMDisabledReason;

typedef /* [public][public][v1_enum][uuid] */  DECLSPEC_UUID("6B1BD814-CA6E-4063-9EDA-4128D31068C1") 
enum __MIDL___MIDL_itf_iTunesCOMInterface_0272_0004
    {	ITPlayButtonStatePlayDisabled	= 0,
	ITPlayButtonStatePlayEnabled	= ITPlayButtonStatePlayDisabled + 1,
	ITPlayButtonStatePauseEnabled	= ITPlayButtonStatePlayEnabled + 1,
	ITPlayButtonStatePauseDisabled	= ITPlayButtonStatePauseEnabled + 1,
	ITPlayButtonStateStopEnabled	= ITPlayButtonStatePauseDisabled + 1,
	ITPlayButtonStateStopDisabled	= ITPlayButtonStateStopEnabled + 1
    } 	ITPlayButtonState;

typedef /* [public][public][v1_enum][uuid] */  DECLSPEC_UUID("8AF85488-2154-4e46-B65B-1972A43493EF") 
enum __MIDL___MIDL_itf_iTunesCOMInterface_0272_0005
    {	ITPlayerButtonPrevious	= 0,
	ITPlayerButtonPlay	= ITPlayerButtonPrevious + 1,
	ITPlayerButtonNext	= ITPlayerButtonPlay + 1
    } 	ITPlayerButton;

typedef /* [public][v1_enum][uuid] */  DECLSPEC_UUID("2129AB11-F23F-485e-B15A-3F8573294F9A") 
enum __MIDL___MIDL_itf_iTunesCOMInterface_0272_0006
    {	ITPlayerButtonModifierKeyNone	= 0,
	ITPlayerButtonModifierKeyShift	= 1,
	ITPlayerButtonModifierKeyControl	= 2,
	ITPlayerButtonModifierKeyAlt	= 4,
	ITPlayerButtonModifierKeyCapsLock	= 8
    } 	ITPlayerButtonModifierKey;

typedef /* [public][v1_enum][uuid] */  DECLSPEC_UUID("3194F5F4-8F52-41e6-AB8E-4221CFE29550") 
enum __MIDL___MIDL_itf_iTunesCOMInterface_0275_0001
    {	ITEventDatabaseChanged	= 1,
	ITEventPlayerPlay	= 2,
	ITEventPlayerStop	= 3,
	ITEventPlayerPlayingTrackChanged	= 4,
	ITEventUserInterfaceEnabled	= 5,
	ITEventCOMCallsDisabled	= 6,
	ITEventCOMCallsEnabled	= 7,
	ITEventQuitting	= 8,
	ITEventAboutToPromptUserToQuit	= 9,
	ITEventSoundVolumeChanged	= 10
    } 	ITEvent;

typedef /* [public][v1_enum][uuid] */  DECLSPEC_UUID("2E4D55FA-1CD3-4831-8751-0C11EC4FF6FD") 
enum __MIDL___MIDL_itf_iTunesCOMInterface_0276_0001
    {	ITConvertOperationStatusChanged	= 1,
	ITConvertOperationComplete	= 2
    } 	ITConvertOperationStatusEvent;

typedef /* [public][public][v1_enum][uuid] */  DECLSPEC_UUID("269E36A5-1728-46e4-BF04-93032C3DD51C") 
enum __MIDL___MIDL_itf_iTunesCOMInterface_0277_0001
    {	ITArtworkFormatUnknown	= 0,
	ITArtworkFormatJPEG	= ITArtworkFormatUnknown + 1,
	ITArtworkFormatPNG	= ITArtworkFormatJPEG + 1,
	ITArtworkFormatBMP	= ITArtworkFormatPNG + 1
    } 	ITArtworkFormat;




typedef /* [public][public][v1_enum][uuid] */  DECLSPEC_UUID("DDE76D6E-5F8C-4bda-AFA6-69E82218CFF3") 
enum __MIDL___MIDL_itf_iTunesCOMInterface_0283_0001
    {	ITPlaylistKindUnknown	= 0,
	ITPlaylistKindLibrary	= ITPlaylistKindUnknown + 1,
	ITPlaylistKindUser	= ITPlaylistKindLibrary + 1,
	ITPlaylistKindCD	= ITPlaylistKindUser + 1,
	ITPlaylistKindDevice	= ITPlaylistKindCD + 1,
	ITPlaylistKindRadioTuner	= ITPlaylistKindDevice + 1
    } 	ITPlaylistKind;

typedef /* [public][public][public][v1_enum][uuid] */  DECLSPEC_UUID("4E1D67A4-6C7A-4c7d-821C-03AF7EB10C35") 
enum __MIDL___MIDL_itf_iTunesCOMInterface_0283_0002
    {	ITPlaylistRepeatModeOff	= 0,
	ITPlaylistRepeatModeOne	= ITPlaylistRepeatModeOff + 1,
	ITPlaylistRepeatModeAll	= ITPlaylistRepeatModeOne + 1
    } 	ITPlaylistRepeatMode;

typedef /* [public][public][v1_enum][uuid] */  DECLSPEC_UUID("BB8E7701-1E77-4972-B6C4-C70AC216F468") 
enum __MIDL___MIDL_itf_iTunesCOMInterface_0283_0003
    {	ITPlaylistPrintKindPlaylist	= 0,
	ITPlaylistPrintKindAlbumlist	= ITPlaylistPrintKindPlaylist + 1,
	ITPlaylistPrintKindInsert	= ITPlaylistPrintKindAlbumlist + 1
    } 	ITPlaylistPrintKind;

typedef /* [public][public][v1_enum][uuid] */  DECLSPEC_UUID("58765E77-E34A-4d67-AC12-5B5BA33EA08F") 
enum __MIDL___MIDL_itf_iTunesCOMInterface_0283_0004
    {	ITPlaylistSearchFieldAll	= 0,
	ITPlaylistSearchFieldVisible	= ITPlaylistSearchFieldAll + 1,
	ITPlaylistSearchFieldArtists	= ITPlaylistSearchFieldVisible + 1,
	ITPlaylistSearchFieldAlbums	= ITPlaylistSearchFieldArtists + 1,
	ITPlaylistSearchFieldComposers	= ITPlaylistSearchFieldAlbums + 1,
	ITPlaylistSearchFieldSongNames	= ITPlaylistSearchFieldComposers + 1
    } 	ITPlaylistSearchField;

typedef /* [public][public][v1_enum][uuid] */  DECLSPEC_UUID("62BC24E6-5C77-4fb7-AA6C-B7FA40C6095D") 
enum __MIDL___MIDL_itf_iTunesCOMInterface_0285_0001
    {	ITUserPlaylistSpecialKindNone	= 0,
	ITUserPlaylistSpecialKindPurchasedMusic	= ITUserPlaylistSpecialKindNone + 1,
	ITUserPlaylistSpecialKindPartyShuffle	= ITUserPlaylistSpecialKindPurchasedMusic + 1,
	ITUserPlaylistSpecialKindPodcasts	= ITUserPlaylistSpecialKindPartyShuffle + 1,
	ITUserPlaylistSpecialKindFolder	= ITUserPlaylistSpecialKindPodcasts + 1,
	ITUserPlaylistSpecialKindVideos	= ITUserPlaylistSpecialKindFolder + 1,
	ITUserPlaylistSpecialKindMusic	= ITUserPlaylistSpecialKindVideos + 1,
	ITUserPlaylistSpecialKindMovies	= ITUserPlaylistSpecialKindMusic + 1,
	ITUserPlaylistSpecialKindTVShows	= ITUserPlaylistSpecialKindMovies + 1,
	ITUserPlaylistSpecialKindAudiobooks	= ITUserPlaylistSpecialKindTVShows + 1
    } 	ITUserPlaylistSpecialKind;


typedef /* [public][public][v1_enum][uuid] */  DECLSPEC_UUID("5F35912B-E633-4930-9E25-09489BAED75A") 
enum __MIDL___MIDL_itf_iTunesCOMInterface_0288_0001
    {	ITSourceKindUnknown	= 0,
	ITSourceKindLibrary	= ITSourceKindUnknown + 1,
	ITSourceKindIPod	= ITSourceKindLibrary + 1,
	ITSourceKindAudioCD	= ITSourceKindIPod + 1,
	ITSourceKindMP3CD	= ITSourceKindAudioCD + 1,
	ITSourceKindDevice	= ITSourceKindMP3CD + 1,
	ITSourceKindRadioTuner	= ITSourceKindDevice + 1,
	ITSourceKindSharedLibrary	= ITSourceKindRadioTuner + 1
    } 	ITSourceKind;


typedef /* [public][public][v1_enum][uuid] */  DECLSPEC_UUID("ACA133C5-4697-4d5f-98B1-D9881B85FE98") 
enum __MIDL___MIDL_itf_iTunesCOMInterface_0291_0001
    {	ITTrackKindUnknown	= 0,
	ITTrackKindFile	= ITTrackKindUnknown + 1,
	ITTrackKindCD	= ITTrackKindFile + 1,
	ITTrackKindURL	= ITTrackKindCD + 1,
	ITTrackKindDevice	= ITTrackKindURL + 1,
	ITTrackKindSharedLibrary	= ITTrackKindDevice + 1
    } 	ITTrackKind;

typedef /* [public][public][public][v1_enum][uuid] */  DECLSPEC_UUID("735ECC17-38CC-4d4d-A838-24AF7DCB440E") 
enum __MIDL___MIDL_itf_iTunesCOMInterface_0291_0002
    {	ITVideoKindNone	= 0,
	ITVideoKindMovie	= ITVideoKindNone + 1,
	ITVideoKindMusicVideo	= ITVideoKindMovie + 1,
	ITVideoKindTVShow	= ITVideoKindMusicVideo + 1
    } 	ITVideoKind;

typedef /* [public][public][public][public][public][v1_enum][uuid] */  DECLSPEC_UUID("5C75B72C-D066-4faa-8732-D9ED71A6CBD9") 
enum __MIDL___MIDL_itf_iTunesCOMInterface_0291_0003
    {	ITRatingKindUser	= 0,
	ITRatingKindComputed	= ITRatingKindUser + 1
    } 	ITRatingKind;

typedef /* [public][public][v1_enum][uuid] */  DECLSPEC_UUID("C20CE920-EFD9-4c1a-8036-95A895741214") 
enum __MIDL___MIDL_itf_iTunesCOMInterface_0297_0001
    {	ITWindowKindUnknown	= 0,
	ITWindowKindBrowser	= ITWindowKindUnknown + 1,
	ITWindowKindPlaylist	= ITWindowKindBrowser + 1,
	ITWindowKindEQ	= ITWindowKindPlaylist + 1,
	ITWindowKindArtwork	= ITWindowKindEQ + 1,
	ITWindowKindNowPlaying	= ITWindowKindArtwork + 1
    } 	ITWindowKind;


EXTERN_C const IID LIBID_iTunesLib;

#ifndef __IITObject_INTERFACE_DEFINED__
#define __IITObject_INTERFACE_DEFINED__

/* interface IITObject */
/* [hidden][unique][helpstring][dual][uuid][object] */ 


EXTERN_C const IID IID_IITObject;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("9FAB0E27-70D7-4e3a-9965-B0C8B8869BB6")
    IITObject : public IDispatch
    {
    public:
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE GetITObjectIDs( 
            /* [out] */ long *sourceID,
            /* [out] */ long *playlistID,
            /* [out] */ long *trackID,
            /* [out] */ long *databaseID) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_Name( 
            /* [retval][out] */ BSTR *name) = 0;
        
        virtual /* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE put_Name( 
            /* [in] */ BSTR name) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_Index( 
            /* [retval][out] */ long *index) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_SourceID( 
            /* [retval][out] */ long *sourceID) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_PlaylistID( 
            /* [retval][out] */ long *playlistID) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_TrackID( 
            /* [retval][out] */ long *trackID) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_TrackDatabaseID( 
            /* [retval][out] */ long *databaseID) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IITObjectVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IITObject * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IITObject * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IITObject * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IITObject * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IITObject * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IITObject * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IITObject * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *GetITObjectIDs )( 
            IITObject * This,
            /* [out] */ long *sourceID,
            /* [out] */ long *playlistID,
            /* [out] */ long *trackID,
            /* [out] */ long *databaseID);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Name )( 
            IITObject * This,
            /* [retval][out] */ BSTR *name);
        
        /* [helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Name )( 
            IITObject * This,
            /* [in] */ BSTR name);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Index )( 
            IITObject * This,
            /* [retval][out] */ long *index);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_SourceID )( 
            IITObject * This,
            /* [retval][out] */ long *sourceID);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_PlaylistID )( 
            IITObject * This,
            /* [retval][out] */ long *playlistID);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_TrackID )( 
            IITObject * This,
            /* [retval][out] */ long *trackID);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_TrackDatabaseID )( 
            IITObject * This,
            /* [retval][out] */ long *databaseID);
        
        END_INTERFACE
    } IITObjectVtbl;

    interface IITObject
    {
        CONST_VTBL struct IITObjectVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IITObject_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IITObject_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IITObject_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IITObject_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IITObject_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IITObject_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IITObject_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IITObject_GetITObjectIDs(This,sourceID,playlistID,trackID,databaseID)	\
    (This)->lpVtbl -> GetITObjectIDs(This,sourceID,playlistID,trackID,databaseID)

#define IITObject_get_Name(This,name)	\
    (This)->lpVtbl -> get_Name(This,name)

#define IITObject_put_Name(This,name)	\
    (This)->lpVtbl -> put_Name(This,name)

#define IITObject_get_Index(This,index)	\
    (This)->lpVtbl -> get_Index(This,index)

#define IITObject_get_SourceID(This,sourceID)	\
    (This)->lpVtbl -> get_SourceID(This,sourceID)

#define IITObject_get_PlaylistID(This,playlistID)	\
    (This)->lpVtbl -> get_PlaylistID(This,playlistID)

#define IITObject_get_TrackID(This,trackID)	\
    (This)->lpVtbl -> get_TrackID(This,trackID)

#define IITObject_get_TrackDatabaseID(This,databaseID)	\
    (This)->lpVtbl -> get_TrackDatabaseID(This,databaseID)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring] */ HRESULT STDMETHODCALLTYPE IITObject_GetITObjectIDs_Proxy( 
    IITObject * This,
    /* [out] */ long *sourceID,
    /* [out] */ long *playlistID,
    /* [out] */ long *trackID,
    /* [out] */ long *databaseID);


void __RPC_STUB IITObject_GetITObjectIDs_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IITObject_get_Name_Proxy( 
    IITObject * This,
    /* [retval][out] */ BSTR *name);


void __RPC_STUB IITObject_get_Name_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE IITObject_put_Name_Proxy( 
    IITObject * This,
    /* [in] */ BSTR name);


void __RPC_STUB IITObject_put_Name_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IITObject_get_Index_Proxy( 
    IITObject * This,
    /* [retval][out] */ long *index);


void __RPC_STUB IITObject_get_Index_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IITObject_get_SourceID_Proxy( 
    IITObject * This,
    /* [retval][out] */ long *sourceID);


void __RPC_STUB IITObject_get_SourceID_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IITObject_get_PlaylistID_Proxy( 
    IITObject * This,
    /* [retval][out] */ long *playlistID);


void __RPC_STUB IITObject_get_PlaylistID_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IITObject_get_TrackID_Proxy( 
    IITObject * This,
    /* [retval][out] */ long *trackID);


void __RPC_STUB IITObject_get_TrackID_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IITObject_get_TrackDatabaseID_Proxy( 
    IITObject * This,
    /* [retval][out] */ long *databaseID);


void __RPC_STUB IITObject_get_TrackDatabaseID_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IITObject_INTERFACE_DEFINED__ */


#ifndef __IITSource_INTERFACE_DEFINED__
#define __IITSource_INTERFACE_DEFINED__

/* interface IITSource */
/* [hidden][unique][helpstring][dual][uuid][object] */ 


EXTERN_C const IID IID_IITSource;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("AEC1C4D3-AEF1-4255-B892-3E3D13ADFDF9")
    IITSource : public IITObject
    {
    public:
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_Kind( 
            /* [retval][out] */ ITSourceKind *kind) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_Capacity( 
            /* [retval][out] */ double *capacity) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_FreeSpace( 
            /* [retval][out] */ double *freespace) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_Playlists( 
            /* [retval][out] */ IITPlaylistCollection **iPlaylistCollection) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IITSourceVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IITSource * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IITSource * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IITSource * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IITSource * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IITSource * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IITSource * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IITSource * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *GetITObjectIDs )( 
            IITSource * This,
            /* [out] */ long *sourceID,
            /* [out] */ long *playlistID,
            /* [out] */ long *trackID,
            /* [out] */ long *databaseID);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Name )( 
            IITSource * This,
            /* [retval][out] */ BSTR *name);
        
        /* [helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Name )( 
            IITSource * This,
            /* [in] */ BSTR name);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Index )( 
            IITSource * This,
            /* [retval][out] */ long *index);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_SourceID )( 
            IITSource * This,
            /* [retval][out] */ long *sourceID);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_PlaylistID )( 
            IITSource * This,
            /* [retval][out] */ long *playlistID);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_TrackID )( 
            IITSource * This,
            /* [retval][out] */ long *trackID);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_TrackDatabaseID )( 
            IITSource * This,
            /* [retval][out] */ long *databaseID);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Kind )( 
            IITSource * This,
            /* [retval][out] */ ITSourceKind *kind);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Capacity )( 
            IITSource * This,
            /* [retval][out] */ double *capacity);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_FreeSpace )( 
            IITSource * This,
            /* [retval][out] */ double *freespace);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Playlists )( 
            IITSource * This,
            /* [retval][out] */ IITPlaylistCollection **iPlaylistCollection);
        
        END_INTERFACE
    } IITSourceVtbl;

    interface IITSource
    {
        CONST_VTBL struct IITSourceVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IITSource_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IITSource_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IITSource_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IITSource_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IITSource_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IITSource_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IITSource_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IITSource_GetITObjectIDs(This,sourceID,playlistID,trackID,databaseID)	\
    (This)->lpVtbl -> GetITObjectIDs(This,sourceID,playlistID,trackID,databaseID)

#define IITSource_get_Name(This,name)	\
    (This)->lpVtbl -> get_Name(This,name)

#define IITSource_put_Name(This,name)	\
    (This)->lpVtbl -> put_Name(This,name)

#define IITSource_get_Index(This,index)	\
    (This)->lpVtbl -> get_Index(This,index)

#define IITSource_get_SourceID(This,sourceID)	\
    (This)->lpVtbl -> get_SourceID(This,sourceID)

#define IITSource_get_PlaylistID(This,playlistID)	\
    (This)->lpVtbl -> get_PlaylistID(This,playlistID)

#define IITSource_get_TrackID(This,trackID)	\
    (This)->lpVtbl -> get_TrackID(This,trackID)

#define IITSource_get_TrackDatabaseID(This,databaseID)	\
    (This)->lpVtbl -> get_TrackDatabaseID(This,databaseID)


#define IITSource_get_Kind(This,kind)	\
    (This)->lpVtbl -> get_Kind(This,kind)

#define IITSource_get_Capacity(This,capacity)	\
    (This)->lpVtbl -> get_Capacity(This,capacity)

#define IITSource_get_FreeSpace(This,freespace)	\
    (This)->lpVtbl -> get_FreeSpace(This,freespace)

#define IITSource_get_Playlists(This,iPlaylistCollection)	\
    (This)->lpVtbl -> get_Playlists(This,iPlaylistCollection)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IITSource_get_Kind_Proxy( 
    IITSource * This,
    /* [retval][out] */ ITSourceKind *kind);


void __RPC_STUB IITSource_get_Kind_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IITSource_get_Capacity_Proxy( 
    IITSource * This,
    /* [retval][out] */ double *capacity);


void __RPC_STUB IITSource_get_Capacity_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IITSource_get_FreeSpace_Proxy( 
    IITSource * This,
    /* [retval][out] */ double *freespace);


void __RPC_STUB IITSource_get_FreeSpace_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IITSource_get_Playlists_Proxy( 
    IITSource * This,
    /* [retval][out] */ IITPlaylistCollection **iPlaylistCollection);


void __RPC_STUB IITSource_get_Playlists_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IITSource_INTERFACE_DEFINED__ */


#ifndef __IITSourceCollection_INTERFACE_DEFINED__
#define __IITSourceCollection_INTERFACE_DEFINED__

/* interface IITSourceCollection */
/* [unique][helpstring][dual][uuid][object] */ 


EXTERN_C const IID IID_IITSourceCollection;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("2FF6CE20-FF87-4183-B0B3-F323D047AF41")
    IITSourceCollection : public IDispatch
    {
    public:
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_Count( 
            /* [retval][out] */ long *count) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Item( 
            /* [in] */ long index,
            /* [retval][out] */ IITSource **iSource) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_ItemByName( 
            /* [in] */ BSTR name,
            /* [retval][out] */ IITSource **iSource) = 0;
        
        virtual /* [helpstring][restricted][id][propget] */ HRESULT STDMETHODCALLTYPE get__NewEnum( 
            /* [retval][out] */ IUnknown **iEnumerator) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_ItemByPersistentID( 
            /* [in] */ long highID,
            /* [in] */ long lowID,
            /* [retval][out] */ IITSource **iSource) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IITSourceCollectionVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IITSourceCollection * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IITSourceCollection * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IITSourceCollection * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IITSourceCollection * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IITSourceCollection * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IITSourceCollection * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IITSourceCollection * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Count )( 
            IITSourceCollection * This,
            /* [retval][out] */ long *count);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Item )( 
            IITSourceCollection * This,
            /* [in] */ long index,
            /* [retval][out] */ IITSource **iSource);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_ItemByName )( 
            IITSourceCollection * This,
            /* [in] */ BSTR name,
            /* [retval][out] */ IITSource **iSource);
        
        /* [helpstring][restricted][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get__NewEnum )( 
            IITSourceCollection * This,
            /* [retval][out] */ IUnknown **iEnumerator);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_ItemByPersistentID )( 
            IITSourceCollection * This,
            /* [in] */ long highID,
            /* [in] */ long lowID,
            /* [retval][out] */ IITSource **iSource);
        
        END_INTERFACE
    } IITSourceCollectionVtbl;

    interface IITSourceCollection
    {
        CONST_VTBL struct IITSourceCollectionVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IITSourceCollection_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IITSourceCollection_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IITSourceCollection_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IITSourceCollection_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IITSourceCollection_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IITSourceCollection_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IITSourceCollection_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IITSourceCollection_get_Count(This,count)	\
    (This)->lpVtbl -> get_Count(This,count)

#define IITSourceCollection_get_Item(This,index,iSource)	\
    (This)->lpVtbl -> get_Item(This,index,iSource)

#define IITSourceCollection_get_ItemByName(This,name,iSource)	\
    (This)->lpVtbl -> get_ItemByName(This,name,iSource)

#define IITSourceCollection_get__NewEnum(This,iEnumerator)	\
    (This)->lpVtbl -> get__NewEnum(This,iEnumerator)

#define IITSourceCollection_get_ItemByPersistentID(This,highID,lowID,iSource)	\
    (This)->lpVtbl -> get_ItemByPersistentID(This,highID,lowID,iSource)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IITSourceCollection_get_Count_Proxy( 
    IITSourceCollection * This,
    /* [retval][out] */ long *count);


void __RPC_STUB IITSourceCollection_get_Count_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IITSourceCollection_get_Item_Proxy( 
    IITSourceCollection * This,
    /* [in] */ long index,
    /* [retval][out] */ IITSource **iSource);


void __RPC_STUB IITSourceCollection_get_Item_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IITSourceCollection_get_ItemByName_Proxy( 
    IITSourceCollection * This,
    /* [in] */ BSTR name,
    /* [retval][out] */ IITSource **iSource);


void __RPC_STUB IITSourceCollection_get_ItemByName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][restricted][id][propget] */ HRESULT STDMETHODCALLTYPE IITSourceCollection_get__NewEnum_Proxy( 
    IITSourceCollection * This,
    /* [retval][out] */ IUnknown **iEnumerator);


void __RPC_STUB IITSourceCollection_get__NewEnum_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IITSourceCollection_get_ItemByPersistentID_Proxy( 
    IITSourceCollection * This,
    /* [in] */ long highID,
    /* [in] */ long lowID,
    /* [retval][out] */ IITSource **iSource);


void __RPC_STUB IITSourceCollection_get_ItemByPersistentID_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IITSourceCollection_INTERFACE_DEFINED__ */


#ifndef __IITEncoder_INTERFACE_DEFINED__
#define __IITEncoder_INTERFACE_DEFINED__

/* interface IITEncoder */
/* [hidden][unique][helpstring][dual][uuid][object] */ 


EXTERN_C const IID IID_IITEncoder;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("1CF95A1C-55FE-4f45-A2D3-85AC6C504A73")
    IITEncoder : public IDispatch
    {
    public:
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_Name( 
            /* [retval][out] */ BSTR *name) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_Format( 
            /* [retval][out] */ BSTR *format) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IITEncoderVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IITEncoder * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IITEncoder * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IITEncoder * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IITEncoder * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IITEncoder * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IITEncoder * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IITEncoder * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Name )( 
            IITEncoder * This,
            /* [retval][out] */ BSTR *name);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Format )( 
            IITEncoder * This,
            /* [retval][out] */ BSTR *format);
        
        END_INTERFACE
    } IITEncoderVtbl;

    interface IITEncoder
    {
        CONST_VTBL struct IITEncoderVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IITEncoder_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IITEncoder_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IITEncoder_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IITEncoder_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IITEncoder_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IITEncoder_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IITEncoder_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IITEncoder_get_Name(This,name)	\
    (This)->lpVtbl -> get_Name(This,name)

#define IITEncoder_get_Format(This,format)	\
    (This)->lpVtbl -> get_Format(This,format)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IITEncoder_get_Name_Proxy( 
    IITEncoder * This,
    /* [retval][out] */ BSTR *name);


void __RPC_STUB IITEncoder_get_Name_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IITEncoder_get_Format_Proxy( 
    IITEncoder * This,
    /* [retval][out] */ BSTR *format);


void __RPC_STUB IITEncoder_get_Format_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IITEncoder_INTERFACE_DEFINED__ */


#ifndef __IITEncoderCollection_INTERFACE_DEFINED__
#define __IITEncoderCollection_INTERFACE_DEFINED__

/* interface IITEncoderCollection */
/* [unique][helpstring][dual][uuid][object] */ 


EXTERN_C const IID IID_IITEncoderCollection;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("8862BCA9-168D-4549-A9D5-ADB35E553BA6")
    IITEncoderCollection : public IDispatch
    {
    public:
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_Count( 
            /* [retval][out] */ long *count) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Item( 
            /* [in] */ long index,
            /* [retval][out] */ IITEncoder **iEncoder) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_ItemByName( 
            /* [in] */ BSTR name,
            /* [retval][out] */ IITEncoder **iEncoder) = 0;
        
        virtual /* [helpstring][restricted][id][propget] */ HRESULT STDMETHODCALLTYPE get__NewEnum( 
            /* [retval][out] */ IUnknown **iEnumerator) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IITEncoderCollectionVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IITEncoderCollection * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IITEncoderCollection * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IITEncoderCollection * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IITEncoderCollection * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IITEncoderCollection * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IITEncoderCollection * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IITEncoderCollection * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Count )( 
            IITEncoderCollection * This,
            /* [retval][out] */ long *count);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Item )( 
            IITEncoderCollection * This,
            /* [in] */ long index,
            /* [retval][out] */ IITEncoder **iEncoder);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_ItemByName )( 
            IITEncoderCollection * This,
            /* [in] */ BSTR name,
            /* [retval][out] */ IITEncoder **iEncoder);
        
        /* [helpstring][restricted][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get__NewEnum )( 
            IITEncoderCollection * This,
            /* [retval][out] */ IUnknown **iEnumerator);
        
        END_INTERFACE
    } IITEncoderCollectionVtbl;

    interface IITEncoderCollection
    {
        CONST_VTBL struct IITEncoderCollectionVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IITEncoderCollection_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IITEncoderCollection_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IITEncoderCollection_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IITEncoderCollection_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IITEncoderCollection_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IITEncoderCollection_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IITEncoderCollection_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IITEncoderCollection_get_Count(This,count)	\
    (This)->lpVtbl -> get_Count(This,count)

#define IITEncoderCollection_get_Item(This,index,iEncoder)	\
    (This)->lpVtbl -> get_Item(This,index,iEncoder)

#define IITEncoderCollection_get_ItemByName(This,name,iEncoder)	\
    (This)->lpVtbl -> get_ItemByName(This,name,iEncoder)

#define IITEncoderCollection_get__NewEnum(This,iEnumerator)	\
    (This)->lpVtbl -> get__NewEnum(This,iEnumerator)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IITEncoderCollection_get_Count_Proxy( 
    IITEncoderCollection * This,
    /* [retval][out] */ long *count);


void __RPC_STUB IITEncoderCollection_get_Count_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IITEncoderCollection_get_Item_Proxy( 
    IITEncoderCollection * This,
    /* [in] */ long index,
    /* [retval][out] */ IITEncoder **iEncoder);


void __RPC_STUB IITEncoderCollection_get_Item_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IITEncoderCollection_get_ItemByName_Proxy( 
    IITEncoderCollection * This,
    /* [in] */ BSTR name,
    /* [retval][out] */ IITEncoder **iEncoder);


void __RPC_STUB IITEncoderCollection_get_ItemByName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][restricted][id][propget] */ HRESULT STDMETHODCALLTYPE IITEncoderCollection_get__NewEnum_Proxy( 
    IITEncoderCollection * This,
    /* [retval][out] */ IUnknown **iEnumerator);


void __RPC_STUB IITEncoderCollection_get__NewEnum_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IITEncoderCollection_INTERFACE_DEFINED__ */


#ifndef __IITEQPreset_INTERFACE_DEFINED__
#define __IITEQPreset_INTERFACE_DEFINED__

/* interface IITEQPreset */
/* [hidden][unique][helpstring][dual][uuid][object] */ 


EXTERN_C const IID IID_IITEQPreset;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("5BE75F4F-68FA-4212-ACB7-BE44EA569759")
    IITEQPreset : public IDispatch
    {
    public:
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_Name( 
            /* [retval][out] */ BSTR *name) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_Modifiable( 
            /* [retval][out] */ VARIANT_BOOL *isModifiable) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_Preamp( 
            /* [retval][out] */ double *level) = 0;
        
        virtual /* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE put_Preamp( 
            /* [in] */ double level) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_Band1( 
            /* [retval][out] */ double *level) = 0;
        
        virtual /* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE put_Band1( 
            /* [in] */ double level) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_Band2( 
            /* [retval][out] */ double *level) = 0;
        
        virtual /* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE put_Band2( 
            /* [in] */ double level) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_Band3( 
            /* [retval][out] */ double *level) = 0;
        
        virtual /* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE put_Band3( 
            /* [in] */ double level) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_Band4( 
            /* [retval][out] */ double *level) = 0;
        
        virtual /* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE put_Band4( 
            /* [in] */ double level) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_Band5( 
            /* [retval][out] */ double *level) = 0;
        
        virtual /* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE put_Band5( 
            /* [in] */ double level) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_Band6( 
            /* [retval][out] */ double *level) = 0;
        
        virtual /* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE put_Band6( 
            /* [in] */ double level) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_Band7( 
            /* [retval][out] */ double *level) = 0;
        
        virtual /* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE put_Band7( 
            /* [in] */ double level) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_Band8( 
            /* [retval][out] */ double *level) = 0;
        
        virtual /* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE put_Band8( 
            /* [in] */ double level) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_Band9( 
            /* [retval][out] */ double *level) = 0;
        
        virtual /* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE put_Band9( 
            /* [in] */ double level) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_Band10( 
            /* [retval][out] */ double *level) = 0;
        
        virtual /* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE put_Band10( 
            /* [in] */ double level) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE Delete( 
            /* [in] */ VARIANT_BOOL updateAllTracks) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE Rename( 
            /* [in] */ BSTR newName,
            /* [in] */ VARIANT_BOOL updateAllTracks) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IITEQPresetVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IITEQPreset * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IITEQPreset * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IITEQPreset * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IITEQPreset * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IITEQPreset * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IITEQPreset * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IITEQPreset * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Name )( 
            IITEQPreset * This,
            /* [retval][out] */ BSTR *name);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Modifiable )( 
            IITEQPreset * This,
            /* [retval][out] */ VARIANT_BOOL *isModifiable);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Preamp )( 
            IITEQPreset * This,
            /* [retval][out] */ double *level);
        
        /* [helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Preamp )( 
            IITEQPreset * This,
            /* [in] */ double level);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Band1 )( 
            IITEQPreset * This,
            /* [retval][out] */ double *level);
        
        /* [helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Band1 )( 
            IITEQPreset * This,
            /* [in] */ double level);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Band2 )( 
            IITEQPreset * This,
            /* [retval][out] */ double *level);
        
        /* [helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Band2 )( 
            IITEQPreset * This,
            /* [in] */ double level);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Band3 )( 
            IITEQPreset * This,
            /* [retval][out] */ double *level);
        
        /* [helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Band3 )( 
            IITEQPreset * This,
            /* [in] */ double level);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Band4 )( 
            IITEQPreset * This,
            /* [retval][out] */ double *level);
        
        /* [helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Band4 )( 
            IITEQPreset * This,
            /* [in] */ double level);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Band5 )( 
            IITEQPreset * This,
            /* [retval][out] */ double *level);
        
        /* [helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Band5 )( 
            IITEQPreset * This,
            /* [in] */ double level);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Band6 )( 
            IITEQPreset * This,
            /* [retval][out] */ double *level);
        
        /* [helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Band6 )( 
            IITEQPreset * This,
            /* [in] */ double level);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Band7 )( 
            IITEQPreset * This,
            /* [retval][out] */ double *level);
        
        /* [helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Band7 )( 
            IITEQPreset * This,
            /* [in] */ double level);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Band8 )( 
            IITEQPreset * This,
            /* [retval][out] */ double *level);
        
        /* [helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Band8 )( 
            IITEQPreset * This,
            /* [in] */ double level);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Band9 )( 
            IITEQPreset * This,
            /* [retval][out] */ double *level);
        
        /* [helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Band9 )( 
            IITEQPreset * This,
            /* [in] */ double level);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Band10 )( 
            IITEQPreset * This,
            /* [retval][out] */ double *level);
        
        /* [helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Band10 )( 
            IITEQPreset * This,
            /* [in] */ double level);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *Delete )( 
            IITEQPreset * This,
            /* [in] */ VARIANT_BOOL updateAllTracks);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *Rename )( 
            IITEQPreset * This,
            /* [in] */ BSTR newName,
            /* [in] */ VARIANT_BOOL updateAllTracks);
        
        END_INTERFACE
    } IITEQPresetVtbl;

    interface IITEQPreset
    {
        CONST_VTBL struct IITEQPresetVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IITEQPreset_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IITEQPreset_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IITEQPreset_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IITEQPreset_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IITEQPreset_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IITEQPreset_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IITEQPreset_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IITEQPreset_get_Name(This,name)	\
    (This)->lpVtbl -> get_Name(This,name)

#define IITEQPreset_get_Modifiable(This,isModifiable)	\
    (This)->lpVtbl -> get_Modifiable(This,isModifiable)

#define IITEQPreset_get_Preamp(This,level)	\
    (This)->lpVtbl -> get_Preamp(This,level)

#define IITEQPreset_put_Preamp(This,level)	\
    (This)->lpVtbl -> put_Preamp(This,level)

#define IITEQPreset_get_Band1(This,level)	\
    (This)->lpVtbl -> get_Band1(This,level)

#define IITEQPreset_put_Band1(This,level)	\
    (This)->lpVtbl -> put_Band1(This,level)

#define IITEQPreset_get_Band2(This,level)	\
    (This)->lpVtbl -> get_Band2(This,level)

#define IITEQPreset_put_Band2(This,level)	\
    (This)->lpVtbl -> put_Band2(This,level)

#define IITEQPreset_get_Band3(This,level)	\
    (This)->lpVtbl -> get_Band3(This,level)

#define IITEQPreset_put_Band3(This,level)	\
    (This)->lpVtbl -> put_Band3(This,level)

#define IITEQPreset_get_Band4(This,level)	\
    (This)->lpVtbl -> get_Band4(This,level)

#define IITEQPreset_put_Band4(This,level)	\
    (This)->lpVtbl -> put_Band4(This,level)

#define IITEQPreset_get_Band5(This,level)	\
    (This)->lpVtbl -> get_Band5(This,level)

#define IITEQPreset_put_Band5(This,level)	\
    (This)->lpVtbl -> put_Band5(This,level)

#define IITEQPreset_get_Band6(This,level)	\
    (This)->lpVtbl -> get_Band6(This,level)

#define IITEQPreset_put_Band6(This,level)	\
    (This)->lpVtbl -> put_Band6(This,level)

#define IITEQPreset_get_Band7(This,level)	\
    (This)->lpVtbl -> get_Band7(This,level)

#define IITEQPreset_put_Band7(This,level)	\
    (This)->lpVtbl -> put_Band7(This,level)

#define IITEQPreset_get_Band8(This,level)	\
    (This)->lpVtbl -> get_Band8(This,level)

#define IITEQPreset_put_Band8(This,level)	\
    (This)->lpVtbl -> put_Band8(This,level)

#define IITEQPreset_get_Band9(This,level)	\
    (This)->lpVtbl -> get_Band9(This,level)

#define IITEQPreset_put_Band9(This,level)	\
    (This)->lpVtbl -> put_Band9(This,level)

#define IITEQPreset_get_Band10(This,level)	\
    (This)->lpVtbl -> get_Band10(This,level)

#define IITEQPreset_put_Band10(This,level)	\
    (This)->lpVtbl -> put_Band10(This,level)

#define IITEQPreset_Delete(This,updateAllTracks)	\
    (This)->lpVtbl -> Delete(This,updateAllTracks)

#define IITEQPreset_Rename(This,newName,updateAllTracks)	\
    (This)->lpVtbl -> Rename(This,newName,updateAllTracks)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IITEQPreset_get_Name_Proxy( 
    IITEQPreset * This,
    /* [retval][out] */ BSTR *name);


void __RPC_STUB IITEQPreset_get_Name_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IITEQPreset_get_Modifiable_Proxy( 
    IITEQPreset * This,
    /* [retval][out] */ VARIANT_BOOL *isModifiable);


void __RPC_STUB IITEQPreset_get_Modifiable_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IITEQPreset_get_Preamp_Proxy( 
    IITEQPreset * This,
    /* [retval][out] */ double *level);


void __RPC_STUB IITEQPreset_get_Preamp_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE IITEQPreset_put_Preamp_Proxy( 
    IITEQPreset * This,
    /* [in] */ double level);


void __RPC_STUB IITEQPreset_put_Preamp_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IITEQPreset_get_Band1_Proxy( 
    IITEQPreset * This,
    /* [retval][out] */ double *level);


void __RPC_STUB IITEQPreset_get_Band1_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE IITEQPreset_put_Band1_Proxy( 
    IITEQPreset * This,
    /* [in] */ double level);


void __RPC_STUB IITEQPreset_put_Band1_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IITEQPreset_get_Band2_Proxy( 
    IITEQPreset * This,
    /* [retval][out] */ double *level);


void __RPC_STUB IITEQPreset_get_Band2_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE IITEQPreset_put_Band2_Proxy( 
    IITEQPreset * This,
    /* [in] */ double level);


void __RPC_STUB IITEQPreset_put_Band2_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IITEQPreset_get_Band3_Proxy( 
    IITEQPreset * This,
    /* [retval][out] */ double *level);


void __RPC_STUB IITEQPreset_get_Band3_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE IITEQPreset_put_Band3_Proxy( 
    IITEQPreset * This,
    /* [in] */ double level);


void __RPC_STUB IITEQPreset_put_Band3_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IITEQPreset_get_Band4_Proxy( 
    IITEQPreset * This,
    /* [retval][out] */ double *level);


void __RPC_STUB IITEQPreset_get_Band4_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE IITEQPreset_put_Band4_Proxy( 
    IITEQPreset * This,
    /* [in] */ double level);


void __RPC_STUB IITEQPreset_put_Band4_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IITEQPreset_get_Band5_Proxy( 
    IITEQPreset * This,
    /* [retval][out] */ double *level);


void __RPC_STUB IITEQPreset_get_Band5_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE IITEQPreset_put_Band5_Proxy( 
    IITEQPreset * This,
    /* [in] */ double level);


void __RPC_STUB IITEQPreset_put_Band5_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IITEQPreset_get_Band6_Proxy( 
    IITEQPreset * This,
    /* [retval][out] */ double *level);


void __RPC_STUB IITEQPreset_get_Band6_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE IITEQPreset_put_Band6_Proxy( 
    IITEQPreset * This,
    /* [in] */ double level);


void __RPC_STUB IITEQPreset_put_Band6_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IITEQPreset_get_Band7_Proxy( 
    IITEQPreset * This,
    /* [retval][out] */ double *level);


void __RPC_STUB IITEQPreset_get_Band7_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE IITEQPreset_put_Band7_Proxy( 
    IITEQPreset * This,
    /* [in] */ double level);


void __RPC_STUB IITEQPreset_put_Band7_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IITEQPreset_get_Band8_Proxy( 
    IITEQPreset * This,
    /* [retval][out] */ double *level);


void __RPC_STUB IITEQPreset_get_Band8_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE IITEQPreset_put_Band8_Proxy( 
    IITEQPreset * This,
    /* [in] */ double level);


void __RPC_STUB IITEQPreset_put_Band8_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IITEQPreset_get_Band9_Proxy( 
    IITEQPreset * This,
    /* [retval][out] */ double *level);


void __RPC_STUB IITEQPreset_get_Band9_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE IITEQPreset_put_Band9_Proxy( 
    IITEQPreset * This,
    /* [in] */ double level);


void __RPC_STUB IITEQPreset_put_Band9_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IITEQPreset_get_Band10_Proxy( 
    IITEQPreset * This,
    /* [retval][out] */ double *level);


void __RPC_STUB IITEQPreset_get_Band10_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE IITEQPreset_put_Band10_Proxy( 
    IITEQPreset * This,
    /* [in] */ double level);


void __RPC_STUB IITEQPreset_put_Band10_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IITEQPreset_Delete_Proxy( 
    IITEQPreset * This,
    /* [in] */ VARIANT_BOOL updateAllTracks);


void __RPC_STUB IITEQPreset_Delete_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IITEQPreset_Rename_Proxy( 
    IITEQPreset * This,
    /* [in] */ BSTR newName,
    /* [in] */ VARIANT_BOOL updateAllTracks);


void __RPC_STUB IITEQPreset_Rename_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IITEQPreset_INTERFACE_DEFINED__ */


#ifndef __IITEQPresetCollection_INTERFACE_DEFINED__
#define __IITEQPresetCollection_INTERFACE_DEFINED__

/* interface IITEQPresetCollection */
/* [unique][helpstring][dual][uuid][object] */ 


EXTERN_C const IID IID_IITEQPresetCollection;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("AEF4D111-3331-48da-B0C2-B468D5D61D08")
    IITEQPresetCollection : public IDispatch
    {
    public:
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_Count( 
            /* [retval][out] */ long *count) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Item( 
            /* [in] */ long index,
            /* [retval][out] */ IITEQPreset **iEQPreset) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_ItemByName( 
            /* [in] */ BSTR name,
            /* [retval][out] */ IITEQPreset **iEQPreset) = 0;
        
        virtual /* [helpstring][restricted][id][propget] */ HRESULT STDMETHODCALLTYPE get__NewEnum( 
            /* [retval][out] */ IUnknown **iEnumerator) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IITEQPresetCollectionVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IITEQPresetCollection * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IITEQPresetCollection * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IITEQPresetCollection * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IITEQPresetCollection * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IITEQPresetCollection * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IITEQPresetCollection * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IITEQPresetCollection * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Count )( 
            IITEQPresetCollection * This,
            /* [retval][out] */ long *count);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Item )( 
            IITEQPresetCollection * This,
            /* [in] */ long index,
            /* [retval][out] */ IITEQPreset **iEQPreset);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_ItemByName )( 
            IITEQPresetCollection * This,
            /* [in] */ BSTR name,
            /* [retval][out] */ IITEQPreset **iEQPreset);
        
        /* [helpstring][restricted][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get__NewEnum )( 
            IITEQPresetCollection * This,
            /* [retval][out] */ IUnknown **iEnumerator);
        
        END_INTERFACE
    } IITEQPresetCollectionVtbl;

    interface IITEQPresetCollection
    {
        CONST_VTBL struct IITEQPresetCollectionVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IITEQPresetCollection_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IITEQPresetCollection_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IITEQPresetCollection_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IITEQPresetCollection_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IITEQPresetCollection_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IITEQPresetCollection_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IITEQPresetCollection_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IITEQPresetCollection_get_Count(This,count)	\
    (This)->lpVtbl -> get_Count(This,count)

#define IITEQPresetCollection_get_Item(This,index,iEQPreset)	\
    (This)->lpVtbl -> get_Item(This,index,iEQPreset)

#define IITEQPresetCollection_get_ItemByName(This,name,iEQPreset)	\
    (This)->lpVtbl -> get_ItemByName(This,name,iEQPreset)

#define IITEQPresetCollection_get__NewEnum(This,iEnumerator)	\
    (This)->lpVtbl -> get__NewEnum(This,iEnumerator)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IITEQPresetCollection_get_Count_Proxy( 
    IITEQPresetCollection * This,
    /* [retval][out] */ long *count);


void __RPC_STUB IITEQPresetCollection_get_Count_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IITEQPresetCollection_get_Item_Proxy( 
    IITEQPresetCollection * This,
    /* [in] */ long index,
    /* [retval][out] */ IITEQPreset **iEQPreset);


void __RPC_STUB IITEQPresetCollection_get_Item_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IITEQPresetCollection_get_ItemByName_Proxy( 
    IITEQPresetCollection * This,
    /* [in] */ BSTR name,
    /* [retval][out] */ IITEQPreset **iEQPreset);


void __RPC_STUB IITEQPresetCollection_get_ItemByName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][restricted][id][propget] */ HRESULT STDMETHODCALLTYPE IITEQPresetCollection_get__NewEnum_Proxy( 
    IITEQPresetCollection * This,
    /* [retval][out] */ IUnknown **iEnumerator);


void __RPC_STUB IITEQPresetCollection_get__NewEnum_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IITEQPresetCollection_INTERFACE_DEFINED__ */


#ifndef __IITPlaylist_INTERFACE_DEFINED__
#define __IITPlaylist_INTERFACE_DEFINED__

/* interface IITPlaylist */
/* [hidden][unique][helpstring][dual][uuid][object] */ 


EXTERN_C const IID IID_IITPlaylist;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("3D5E072F-2A77-4b17-9E73-E03B77CCCCA9")
    IITPlaylist : public IITObject
    {
    public:
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE Delete( void) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE PlayFirstTrack( void) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE Print( 
            /* [in] */ VARIANT_BOOL showPrintDialog,
            /* [in] */ ITPlaylistPrintKind printKind,
            /* [in] */ BSTR theme) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE Search( 
            /* [in] */ BSTR searchText,
            /* [in] */ ITPlaylistSearchField searchFields,
            /* [retval][out] */ IITTrackCollection **iTrackCollection) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_Kind( 
            /* [retval][out] */ ITPlaylistKind *kind) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_Source( 
            /* [retval][out] */ IITSource **iSource) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_Duration( 
            /* [retval][out] */ long *duration) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_Shuffle( 
            /* [retval][out] */ VARIANT_BOOL *isShuffle) = 0;
        
        virtual /* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE put_Shuffle( 
            /* [in] */ VARIANT_BOOL shouldShuffle) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_Size( 
            /* [retval][out] */ double *size) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_SongRepeat( 
            /* [retval][out] */ ITPlaylistRepeatMode *repeatMode) = 0;
        
        virtual /* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE put_SongRepeat( 
            /* [in] */ ITPlaylistRepeatMode repeatMode) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_Time( 
            /* [retval][out] */ BSTR *time) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_Visible( 
            /* [retval][out] */ VARIANT_BOOL *isVisible) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_Tracks( 
            /* [retval][out] */ IITTrackCollection **iTrackCollection) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IITPlaylistVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IITPlaylist * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IITPlaylist * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IITPlaylist * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IITPlaylist * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IITPlaylist * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IITPlaylist * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IITPlaylist * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *GetITObjectIDs )( 
            IITPlaylist * This,
            /* [out] */ long *sourceID,
            /* [out] */ long *playlistID,
            /* [out] */ long *trackID,
            /* [out] */ long *databaseID);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Name )( 
            IITPlaylist * This,
            /* [retval][out] */ BSTR *name);
        
        /* [helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Name )( 
            IITPlaylist * This,
            /* [in] */ BSTR name);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Index )( 
            IITPlaylist * This,
            /* [retval][out] */ long *index);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_SourceID )( 
            IITPlaylist * This,
            /* [retval][out] */ long *sourceID);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_PlaylistID )( 
            IITPlaylist * This,
            /* [retval][out] */ long *playlistID);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_TrackID )( 
            IITPlaylist * This,
            /* [retval][out] */ long *trackID);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_TrackDatabaseID )( 
            IITPlaylist * This,
            /* [retval][out] */ long *databaseID);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *Delete )( 
            IITPlaylist * This);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *PlayFirstTrack )( 
            IITPlaylist * This);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *Print )( 
            IITPlaylist * This,
            /* [in] */ VARIANT_BOOL showPrintDialog,
            /* [in] */ ITPlaylistPrintKind printKind,
            /* [in] */ BSTR theme);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *Search )( 
            IITPlaylist * This,
            /* [in] */ BSTR searchText,
            /* [in] */ ITPlaylistSearchField searchFields,
            /* [retval][out] */ IITTrackCollection **iTrackCollection);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Kind )( 
            IITPlaylist * This,
            /* [retval][out] */ ITPlaylistKind *kind);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Source )( 
            IITPlaylist * This,
            /* [retval][out] */ IITSource **iSource);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Duration )( 
            IITPlaylist * This,
            /* [retval][out] */ long *duration);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Shuffle )( 
            IITPlaylist * This,
            /* [retval][out] */ VARIANT_BOOL *isShuffle);
        
        /* [helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Shuffle )( 
            IITPlaylist * This,
            /* [in] */ VARIANT_BOOL shouldShuffle);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Size )( 
            IITPlaylist * This,
            /* [retval][out] */ double *size);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_SongRepeat )( 
            IITPlaylist * This,
            /* [retval][out] */ ITPlaylistRepeatMode *repeatMode);
        
        /* [helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE *put_SongRepeat )( 
            IITPlaylist * This,
            /* [in] */ ITPlaylistRepeatMode repeatMode);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Time )( 
            IITPlaylist * This,
            /* [retval][out] */ BSTR *time);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Visible )( 
            IITPlaylist * This,
            /* [retval][out] */ VARIANT_BOOL *isVisible);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Tracks )( 
            IITPlaylist * This,
            /* [retval][out] */ IITTrackCollection **iTrackCollection);
        
        END_INTERFACE
    } IITPlaylistVtbl;

    interface IITPlaylist
    {
        CONST_VTBL struct IITPlaylistVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IITPlaylist_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IITPlaylist_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IITPlaylist_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IITPlaylist_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IITPlaylist_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IITPlaylist_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IITPlaylist_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IITPlaylist_GetITObjectIDs(This,sourceID,playlistID,trackID,databaseID)	\
    (This)->lpVtbl -> GetITObjectIDs(This,sourceID,playlistID,trackID,databaseID)

#define IITPlaylist_get_Name(This,name)	\
    (This)->lpVtbl -> get_Name(This,name)

#define IITPlaylist_put_Name(This,name)	\
    (This)->lpVtbl -> put_Name(This,name)

#define IITPlaylist_get_Index(This,index)	\
    (This)->lpVtbl -> get_Index(This,index)

#define IITPlaylist_get_SourceID(This,sourceID)	\
    (This)->lpVtbl -> get_SourceID(This,sourceID)

#define IITPlaylist_get_PlaylistID(This,playlistID)	\
    (This)->lpVtbl -> get_PlaylistID(This,playlistID)

#define IITPlaylist_get_TrackID(This,trackID)	\
    (This)->lpVtbl -> get_TrackID(This,trackID)

#define IITPlaylist_get_TrackDatabaseID(This,databaseID)	\
    (This)->lpVtbl -> get_TrackDatabaseID(This,databaseID)


#define IITPlaylist_Delete(This)	\
    (This)->lpVtbl -> Delete(This)

#define IITPlaylist_PlayFirstTrack(This)	\
    (This)->lpVtbl -> PlayFirstTrack(This)

#define IITPlaylist_Print(This,showPrintDialog,printKind,theme)	\
    (This)->lpVtbl -> Print(This,showPrintDialog,printKind,theme)

#define IITPlaylist_Search(This,searchText,searchFields,iTrackCollection)	\
    (This)->lpVtbl -> Search(This,searchText,searchFields,iTrackCollection)

#define IITPlaylist_get_Kind(This,kind)	\
    (This)->lpVtbl -> get_Kind(This,kind)

#define IITPlaylist_get_Source(This,iSource)	\
    (This)->lpVtbl -> get_Source(This,iSource)

#define IITPlaylist_get_Duration(This,duration)	\
    (This)->lpVtbl -> get_Duration(This,duration)

#define IITPlaylist_get_Shuffle(This,isShuffle)	\
    (This)->lpVtbl -> get_Shuffle(This,isShuffle)

#define IITPlaylist_put_Shuffle(This,shouldShuffle)	\
    (This)->lpVtbl -> put_Shuffle(This,shouldShuffle)

#define IITPlaylist_get_Size(This,size)	\
    (This)->lpVtbl -> get_Size(This,size)

#define IITPlaylist_get_SongRepeat(This,repeatMode)	\
    (This)->lpVtbl -> get_SongRepeat(This,repeatMode)

#define IITPlaylist_put_SongRepeat(This,repeatMode)	\
    (This)->lpVtbl -> put_SongRepeat(This,repeatMode)

#define IITPlaylist_get_Time(This,time)	\
    (This)->lpVtbl -> get_Time(This,time)

#define IITPlaylist_get_Visible(This,isVisible)	\
    (This)->lpVtbl -> get_Visible(This,isVisible)

#define IITPlaylist_get_Tracks(This,iTrackCollection)	\
    (This)->lpVtbl -> get_Tracks(This,iTrackCollection)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring] */ HRESULT STDMETHODCALLTYPE IITPlaylist_Delete_Proxy( 
    IITPlaylist * This);


void __RPC_STUB IITPlaylist_Delete_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IITPlaylist_PlayFirstTrack_Proxy( 
    IITPlaylist * This);


void __RPC_STUB IITPlaylist_PlayFirstTrack_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IITPlaylist_Print_Proxy( 
    IITPlaylist * This,
    /* [in] */ VARIANT_BOOL showPrintDialog,
    /* [in] */ ITPlaylistPrintKind printKind,
    /* [in] */ BSTR theme);


void __RPC_STUB IITPlaylist_Print_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IITPlaylist_Search_Proxy( 
    IITPlaylist * This,
    /* [in] */ BSTR searchText,
    /* [in] */ ITPlaylistSearchField searchFields,
    /* [retval][out] */ IITTrackCollection **iTrackCollection);


void __RPC_STUB IITPlaylist_Search_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IITPlaylist_get_Kind_Proxy( 
    IITPlaylist * This,
    /* [retval][out] */ ITPlaylistKind *kind);


void __RPC_STUB IITPlaylist_get_Kind_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IITPlaylist_get_Source_Proxy( 
    IITPlaylist * This,
    /* [retval][out] */ IITSource **iSource);


void __RPC_STUB IITPlaylist_get_Source_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IITPlaylist_get_Duration_Proxy( 
    IITPlaylist * This,
    /* [retval][out] */ long *duration);


void __RPC_STUB IITPlaylist_get_Duration_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IITPlaylist_get_Shuffle_Proxy( 
    IITPlaylist * This,
    /* [retval][out] */ VARIANT_BOOL *isShuffle);


void __RPC_STUB IITPlaylist_get_Shuffle_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE IITPlaylist_put_Shuffle_Proxy( 
    IITPlaylist * This,
    /* [in] */ VARIANT_BOOL shouldShuffle);


void __RPC_STUB IITPlaylist_put_Shuffle_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IITPlaylist_get_Size_Proxy( 
    IITPlaylist * This,
    /* [retval][out] */ double *size);


void __RPC_STUB IITPlaylist_get_Size_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IITPlaylist_get_SongRepeat_Proxy( 
    IITPlaylist * This,
    /* [retval][out] */ ITPlaylistRepeatMode *repeatMode);


void __RPC_STUB IITPlaylist_get_SongRepeat_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE IITPlaylist_put_SongRepeat_Proxy( 
    IITPlaylist * This,
    /* [in] */ ITPlaylistRepeatMode repeatMode);


void __RPC_STUB IITPlaylist_put_SongRepeat_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IITPlaylist_get_Time_Proxy( 
    IITPlaylist * This,
    /* [retval][out] */ BSTR *time);


void __RPC_STUB IITPlaylist_get_Time_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IITPlaylist_get_Visible_Proxy( 
    IITPlaylist * This,
    /* [retval][out] */ VARIANT_BOOL *isVisible);


void __RPC_STUB IITPlaylist_get_Visible_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IITPlaylist_get_Tracks_Proxy( 
    IITPlaylist * This,
    /* [retval][out] */ IITTrackCollection **iTrackCollection);


void __RPC_STUB IITPlaylist_get_Tracks_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IITPlaylist_INTERFACE_DEFINED__ */


#ifndef __IITOperationStatus_INTERFACE_DEFINED__
#define __IITOperationStatus_INTERFACE_DEFINED__

/* interface IITOperationStatus */
/* [hidden][unique][helpstring][dual][uuid][object] */ 


EXTERN_C const IID IID_IITOperationStatus;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("206479C9-FE32-4f9b-A18A-475AC939B479")
    IITOperationStatus : public IDispatch
    {
    public:
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_InProgress( 
            /* [retval][out] */ VARIANT_BOOL *isInProgress) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_Tracks( 
            /* [retval][out] */ IITTrackCollection **iTrackCollection) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IITOperationStatusVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IITOperationStatus * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IITOperationStatus * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IITOperationStatus * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IITOperationStatus * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IITOperationStatus * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IITOperationStatus * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IITOperationStatus * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_InProgress )( 
            IITOperationStatus * This,
            /* [retval][out] */ VARIANT_BOOL *isInProgress);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Tracks )( 
            IITOperationStatus * This,
            /* [retval][out] */ IITTrackCollection **iTrackCollection);
        
        END_INTERFACE
    } IITOperationStatusVtbl;

    interface IITOperationStatus
    {
        CONST_VTBL struct IITOperationStatusVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IITOperationStatus_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IITOperationStatus_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IITOperationStatus_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IITOperationStatus_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IITOperationStatus_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IITOperationStatus_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IITOperationStatus_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IITOperationStatus_get_InProgress(This,isInProgress)	\
    (This)->lpVtbl -> get_InProgress(This,isInProgress)

#define IITOperationStatus_get_Tracks(This,iTrackCollection)	\
    (This)->lpVtbl -> get_Tracks(This,iTrackCollection)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IITOperationStatus_get_InProgress_Proxy( 
    IITOperationStatus * This,
    /* [retval][out] */ VARIANT_BOOL *isInProgress);


void __RPC_STUB IITOperationStatus_get_InProgress_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IITOperationStatus_get_Tracks_Proxy( 
    IITOperationStatus * This,
    /* [retval][out] */ IITTrackCollection **iTrackCollection);


void __RPC_STUB IITOperationStatus_get_Tracks_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IITOperationStatus_INTERFACE_DEFINED__ */


#ifndef __IITConvertOperationStatus_INTERFACE_DEFINED__
#define __IITConvertOperationStatus_INTERFACE_DEFINED__

/* interface IITConvertOperationStatus */
/* [hidden][unique][helpstring][dual][uuid][object] */ 


EXTERN_C const IID IID_IITConvertOperationStatus;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("7063AAF6-ABA0-493b-B4FC-920A9F105875")
    IITConvertOperationStatus : public IITOperationStatus
    {
    public:
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE GetConversionStatus( 
            /* [out] */ BSTR *trackName,
            /* [out] */ long *progressValue,
            /* [out] */ long *maxProgressValue) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE StopConversion( void) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_TrackName( 
            /* [retval][out] */ BSTR *trackName) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_ProgressValue( 
            /* [retval][out] */ long *progressValue) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_MaxProgressValue( 
            /* [retval][out] */ long *maxProgressValue) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IITConvertOperationStatusVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IITConvertOperationStatus * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IITConvertOperationStatus * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IITConvertOperationStatus * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IITConvertOperationStatus * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IITConvertOperationStatus * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IITConvertOperationStatus * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IITConvertOperationStatus * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_InProgress )( 
            IITConvertOperationStatus * This,
            /* [retval][out] */ VARIANT_BOOL *isInProgress);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Tracks )( 
            IITConvertOperationStatus * This,
            /* [retval][out] */ IITTrackCollection **iTrackCollection);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *GetConversionStatus )( 
            IITConvertOperationStatus * This,
            /* [out] */ BSTR *trackName,
            /* [out] */ long *progressValue,
            /* [out] */ long *maxProgressValue);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *StopConversion )( 
            IITConvertOperationStatus * This);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_TrackName )( 
            IITConvertOperationStatus * This,
            /* [retval][out] */ BSTR *trackName);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_ProgressValue )( 
            IITConvertOperationStatus * This,
            /* [retval][out] */ long *progressValue);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_MaxProgressValue )( 
            IITConvertOperationStatus * This,
            /* [retval][out] */ long *maxProgressValue);
        
        END_INTERFACE
    } IITConvertOperationStatusVtbl;

    interface IITConvertOperationStatus
    {
        CONST_VTBL struct IITConvertOperationStatusVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IITConvertOperationStatus_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IITConvertOperationStatus_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IITConvertOperationStatus_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IITConvertOperationStatus_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IITConvertOperationStatus_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IITConvertOperationStatus_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IITConvertOperationStatus_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IITConvertOperationStatus_get_InProgress(This,isInProgress)	\
    (This)->lpVtbl -> get_InProgress(This,isInProgress)

#define IITConvertOperationStatus_get_Tracks(This,iTrackCollection)	\
    (This)->lpVtbl -> get_Tracks(This,iTrackCollection)


#define IITConvertOperationStatus_GetConversionStatus(This,trackName,progressValue,maxProgressValue)	\
    (This)->lpVtbl -> GetConversionStatus(This,trackName,progressValue,maxProgressValue)

#define IITConvertOperationStatus_StopConversion(This)	\
    (This)->lpVtbl -> StopConversion(This)

#define IITConvertOperationStatus_get_TrackName(This,trackName)	\
    (This)->lpVtbl -> get_TrackName(This,trackName)

#define IITConvertOperationStatus_get_ProgressValue(This,progressValue)	\
    (This)->lpVtbl -> get_ProgressValue(This,progressValue)

#define IITConvertOperationStatus_get_MaxProgressValue(This,maxProgressValue)	\
    (This)->lpVtbl -> get_MaxProgressValue(This,maxProgressValue)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring] */ HRESULT STDMETHODCALLTYPE IITConvertOperationStatus_GetConversionStatus_Proxy( 
    IITConvertOperationStatus * This,
    /* [out] */ BSTR *trackName,
    /* [out] */ long *progressValue,
    /* [out] */ long *maxProgressValue);


void __RPC_STUB IITConvertOperationStatus_GetConversionStatus_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IITConvertOperationStatus_StopConversion_Proxy( 
    IITConvertOperationStatus * This);


void __RPC_STUB IITConvertOperationStatus_StopConversion_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IITConvertOperationStatus_get_TrackName_Proxy( 
    IITConvertOperationStatus * This,
    /* [retval][out] */ BSTR *trackName);


void __RPC_STUB IITConvertOperationStatus_get_TrackName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IITConvertOperationStatus_get_ProgressValue_Proxy( 
    IITConvertOperationStatus * This,
    /* [retval][out] */ long *progressValue);


void __RPC_STUB IITConvertOperationStatus_get_ProgressValue_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IITConvertOperationStatus_get_MaxProgressValue_Proxy( 
    IITConvertOperationStatus * This,
    /* [retval][out] */ long *maxProgressValue);


void __RPC_STUB IITConvertOperationStatus_get_MaxProgressValue_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IITConvertOperationStatus_INTERFACE_DEFINED__ */


#ifndef __IITLibraryPlaylist_INTERFACE_DEFINED__
#define __IITLibraryPlaylist_INTERFACE_DEFINED__

/* interface IITLibraryPlaylist */
/* [hidden][unique][helpstring][dual][uuid][object] */ 


EXTERN_C const IID IID_IITLibraryPlaylist;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("53AE1704-491C-4289-94A0-958815675A3D")
    IITLibraryPlaylist : public IITPlaylist
    {
    public:
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE AddFile( 
            /* [in] */ BSTR filePath,
            /* [retval][out] */ IITOperationStatus **iStatus) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE AddFiles( 
            /* [in] */ VARIANT *filePaths,
            /* [retval][out] */ IITOperationStatus **iStatus) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE AddURL( 
            /* [in] */ BSTR url,
            /* [retval][out] */ IITURLTrack **iURLTrack) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE AddTrack( 
            /* [in] */ VARIANT *iTrackToAdd,
            /* [retval][out] */ IITTrack **iAddedTrack) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IITLibraryPlaylistVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IITLibraryPlaylist * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IITLibraryPlaylist * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IITLibraryPlaylist * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IITLibraryPlaylist * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IITLibraryPlaylist * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IITLibraryPlaylist * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IITLibraryPlaylist * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *GetITObjectIDs )( 
            IITLibraryPlaylist * This,
            /* [out] */ long *sourceID,
            /* [out] */ long *playlistID,
            /* [out] */ long *trackID,
            /* [out] */ long *databaseID);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Name )( 
            IITLibraryPlaylist * This,
            /* [retval][out] */ BSTR *name);
        
        /* [helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Name )( 
            IITLibraryPlaylist * This,
            /* [in] */ BSTR name);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Index )( 
            IITLibraryPlaylist * This,
            /* [retval][out] */ long *index);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_SourceID )( 
            IITLibraryPlaylist * This,
            /* [retval][out] */ long *sourceID);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_PlaylistID )( 
            IITLibraryPlaylist * This,
            /* [retval][out] */ long *playlistID);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_TrackID )( 
            IITLibraryPlaylist * This,
            /* [retval][out] */ long *trackID);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_TrackDatabaseID )( 
            IITLibraryPlaylist * This,
            /* [retval][out] */ long *databaseID);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *Delete )( 
            IITLibraryPlaylist * This);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *PlayFirstTrack )( 
            IITLibraryPlaylist * This);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *Print )( 
            IITLibraryPlaylist * This,
            /* [in] */ VARIANT_BOOL showPrintDialog,
            /* [in] */ ITPlaylistPrintKind printKind,
            /* [in] */ BSTR theme);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *Search )( 
            IITLibraryPlaylist * This,
            /* [in] */ BSTR searchText,
            /* [in] */ ITPlaylistSearchField searchFields,
            /* [retval][out] */ IITTrackCollection **iTrackCollection);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Kind )( 
            IITLibraryPlaylist * This,
            /* [retval][out] */ ITPlaylistKind *kind);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Source )( 
            IITLibraryPlaylist * This,
            /* [retval][out] */ IITSource **iSource);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Duration )( 
            IITLibraryPlaylist * This,
            /* [retval][out] */ long *duration);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Shuffle )( 
            IITLibraryPlaylist * This,
            /* [retval][out] */ VARIANT_BOOL *isShuffle);
        
        /* [helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Shuffle )( 
            IITLibraryPlaylist * This,
            /* [in] */ VARIANT_BOOL shouldShuffle);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Size )( 
            IITLibraryPlaylist * This,
            /* [retval][out] */ double *size);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_SongRepeat )( 
            IITLibraryPlaylist * This,
            /* [retval][out] */ ITPlaylistRepeatMode *repeatMode);
        
        /* [helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE *put_SongRepeat )( 
            IITLibraryPlaylist * This,
            /* [in] */ ITPlaylistRepeatMode repeatMode);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Time )( 
            IITLibraryPlaylist * This,
            /* [retval][out] */ BSTR *time);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Visible )( 
            IITLibraryPlaylist * This,
            /* [retval][out] */ VARIANT_BOOL *isVisible);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Tracks )( 
            IITLibraryPlaylist * This,
            /* [retval][out] */ IITTrackCollection **iTrackCollection);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *AddFile )( 
            IITLibraryPlaylist * This,
            /* [in] */ BSTR filePath,
            /* [retval][out] */ IITOperationStatus **iStatus);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *AddFiles )( 
            IITLibraryPlaylist * This,
            /* [in] */ VARIANT *filePaths,
            /* [retval][out] */ IITOperationStatus **iStatus);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *AddURL )( 
            IITLibraryPlaylist * This,
            /* [in] */ BSTR url,
            /* [retval][out] */ IITURLTrack **iURLTrack);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *AddTrack )( 
            IITLibraryPlaylist * This,
            /* [in] */ VARIANT *iTrackToAdd,
            /* [retval][out] */ IITTrack **iAddedTrack);
        
        END_INTERFACE
    } IITLibraryPlaylistVtbl;

    interface IITLibraryPlaylist
    {
        CONST_VTBL struct IITLibraryPlaylistVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IITLibraryPlaylist_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IITLibraryPlaylist_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IITLibraryPlaylist_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IITLibraryPlaylist_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IITLibraryPlaylist_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IITLibraryPlaylist_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IITLibraryPlaylist_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IITLibraryPlaylist_GetITObjectIDs(This,sourceID,playlistID,trackID,databaseID)	\
    (This)->lpVtbl -> GetITObjectIDs(This,sourceID,playlistID,trackID,databaseID)

#define IITLibraryPlaylist_get_Name(This,name)	\
    (This)->lpVtbl -> get_Name(This,name)

#define IITLibraryPlaylist_put_Name(This,name)	\
    (This)->lpVtbl -> put_Name(This,name)

#define IITLibraryPlaylist_get_Index(This,index)	\
    (This)->lpVtbl -> get_Index(This,index)

#define IITLibraryPlaylist_get_SourceID(This,sourceID)	\
    (This)->lpVtbl -> get_SourceID(This,sourceID)

#define IITLibraryPlaylist_get_PlaylistID(This,playlistID)	\
    (This)->lpVtbl -> get_PlaylistID(This,playlistID)

#define IITLibraryPlaylist_get_TrackID(This,trackID)	\
    (This)->lpVtbl -> get_TrackID(This,trackID)

#define IITLibraryPlaylist_get_TrackDatabaseID(This,databaseID)	\
    (This)->lpVtbl -> get_TrackDatabaseID(This,databaseID)


#define IITLibraryPlaylist_Delete(This)	\
    (This)->lpVtbl -> Delete(This)

#define IITLibraryPlaylist_PlayFirstTrack(This)	\
    (This)->lpVtbl -> PlayFirstTrack(This)

#define IITLibraryPlaylist_Print(This,showPrintDialog,printKind,theme)	\
    (This)->lpVtbl -> Print(This,showPrintDialog,printKind,theme)

#define IITLibraryPlaylist_Search(This,searchText,searchFields,iTrackCollection)	\
    (This)->lpVtbl -> Search(This,searchText,searchFields,iTrackCollection)

#define IITLibraryPlaylist_get_Kind(This,kind)	\
    (This)->lpVtbl -> get_Kind(This,kind)

#define IITLibraryPlaylist_get_Source(This,iSource)	\
    (This)->lpVtbl -> get_Source(This,iSource)

#define IITLibraryPlaylist_get_Duration(This,duration)	\
    (This)->lpVtbl -> get_Duration(This,duration)

#define IITLibraryPlaylist_get_Shuffle(This,isShuffle)	\
    (This)->lpVtbl -> get_Shuffle(This,isShuffle)

#define IITLibraryPlaylist_put_Shuffle(This,shouldShuffle)	\
    (This)->lpVtbl -> put_Shuffle(This,shouldShuffle)

#define IITLibraryPlaylist_get_Size(This,size)	\
    (This)->lpVtbl -> get_Size(This,size)

#define IITLibraryPlaylist_get_SongRepeat(This,repeatMode)	\
    (This)->lpVtbl -> get_SongRepeat(This,repeatMode)

#define IITLibraryPlaylist_put_SongRepeat(This,repeatMode)	\
    (This)->lpVtbl -> put_SongRepeat(This,repeatMode)

#define IITLibraryPlaylist_get_Time(This,time)	\
    (This)->lpVtbl -> get_Time(This,time)

#define IITLibraryPlaylist_get_Visible(This,isVisible)	\
    (This)->lpVtbl -> get_Visible(This,isVisible)

#define IITLibraryPlaylist_get_Tracks(This,iTrackCollection)	\
    (This)->lpVtbl -> get_Tracks(This,iTrackCollection)


#define IITLibraryPlaylist_AddFile(This,filePath,iStatus)	\
    (This)->lpVtbl -> AddFile(This,filePath,iStatus)

#define IITLibraryPlaylist_AddFiles(This,filePaths,iStatus)	\
    (This)->lpVtbl -> AddFiles(This,filePaths,iStatus)

#define IITLibraryPlaylist_AddURL(This,url,iURLTrack)	\
    (This)->lpVtbl -> AddURL(This,url,iURLTrack)

#define IITLibraryPlaylist_AddTrack(This,iTrackToAdd,iAddedTrack)	\
    (This)->lpVtbl -> AddTrack(This,iTrackToAdd,iAddedTrack)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring] */ HRESULT STDMETHODCALLTYPE IITLibraryPlaylist_AddFile_Proxy( 
    IITLibraryPlaylist * This,
    /* [in] */ BSTR filePath,
    /* [retval][out] */ IITOperationStatus **iStatus);


void __RPC_STUB IITLibraryPlaylist_AddFile_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IITLibraryPlaylist_AddFiles_Proxy( 
    IITLibraryPlaylist * This,
    /* [in] */ VARIANT *filePaths,
    /* [retval][out] */ IITOperationStatus **iStatus);


void __RPC_STUB IITLibraryPlaylist_AddFiles_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IITLibraryPlaylist_AddURL_Proxy( 
    IITLibraryPlaylist * This,
    /* [in] */ BSTR url,
    /* [retval][out] */ IITURLTrack **iURLTrack);


void __RPC_STUB IITLibraryPlaylist_AddURL_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IITLibraryPlaylist_AddTrack_Proxy( 
    IITLibraryPlaylist * This,
    /* [in] */ VARIANT *iTrackToAdd,
    /* [retval][out] */ IITTrack **iAddedTrack);


void __RPC_STUB IITLibraryPlaylist_AddTrack_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IITLibraryPlaylist_INTERFACE_DEFINED__ */


#ifndef __IITUserPlaylist_INTERFACE_DEFINED__
#define __IITUserPlaylist_INTERFACE_DEFINED__

/* interface IITUserPlaylist */
/* [hidden][unique][helpstring][dual][uuid][object] */ 


EXTERN_C const IID IID_IITUserPlaylist;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("0A504DED-A0B5-465a-8A94-50E20D7DF692")
    IITUserPlaylist : public IITPlaylist
    {
    public:
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE AddFile( 
            /* [in] */ BSTR filePath,
            /* [retval][out] */ IITOperationStatus **iStatus) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE AddFiles( 
            /* [in] */ VARIANT *filePaths,
            /* [retval][out] */ IITOperationStatus **iStatus) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE AddURL( 
            /* [in] */ BSTR url,
            /* [retval][out] */ IITURLTrack **iURLTrack) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE AddTrack( 
            /* [in] */ VARIANT *iTrackToAdd,
            /* [retval][out] */ IITTrack **iAddedTrack) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_Shared( 
            /* [retval][out] */ VARIANT_BOOL *isShared) = 0;
        
        virtual /* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE put_Shared( 
            /* [in] */ VARIANT_BOOL shouldBeShared) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_Smart( 
            /* [retval][out] */ VARIANT_BOOL *isSmart) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_SpecialKind( 
            /* [retval][out] */ ITUserPlaylistSpecialKind *specialKind) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_Parent( 
            /* [retval][out] */ IITUserPlaylist **iParentPlayList) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE CreatePlaylist( 
            /* [in] */ BSTR playlistName,
            /* [retval][out] */ IITPlaylist **iPlaylist) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE CreateFolder( 
            /* [in] */ BSTR folderName,
            /* [retval][out] */ IITPlaylist **iFolder) = 0;
        
        virtual /* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE put_Parent( 
            /* [in] */ VARIANT *iParent) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE Reveal( void) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IITUserPlaylistVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IITUserPlaylist * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IITUserPlaylist * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IITUserPlaylist * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IITUserPlaylist * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IITUserPlaylist * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IITUserPlaylist * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IITUserPlaylist * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *GetITObjectIDs )( 
            IITUserPlaylist * This,
            /* [out] */ long *sourceID,
            /* [out] */ long *playlistID,
            /* [out] */ long *trackID,
            /* [out] */ long *databaseID);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Name )( 
            IITUserPlaylist * This,
            /* [retval][out] */ BSTR *name);
        
        /* [helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Name )( 
            IITUserPlaylist * This,
            /* [in] */ BSTR name);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Index )( 
            IITUserPlaylist * This,
            /* [retval][out] */ long *index);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_SourceID )( 
            IITUserPlaylist * This,
            /* [retval][out] */ long *sourceID);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_PlaylistID )( 
            IITUserPlaylist * This,
            /* [retval][out] */ long *playlistID);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_TrackID )( 
            IITUserPlaylist * This,
            /* [retval][out] */ long *trackID);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_TrackDatabaseID )( 
            IITUserPlaylist * This,
            /* [retval][out] */ long *databaseID);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *Delete )( 
            IITUserPlaylist * This);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *PlayFirstTrack )( 
            IITUserPlaylist * This);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *Print )( 
            IITUserPlaylist * This,
            /* [in] */ VARIANT_BOOL showPrintDialog,
            /* [in] */ ITPlaylistPrintKind printKind,
            /* [in] */ BSTR theme);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *Search )( 
            IITUserPlaylist * This,
            /* [in] */ BSTR searchText,
            /* [in] */ ITPlaylistSearchField searchFields,
            /* [retval][out] */ IITTrackCollection **iTrackCollection);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Kind )( 
            IITUserPlaylist * This,
            /* [retval][out] */ ITPlaylistKind *kind);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Source )( 
            IITUserPlaylist * This,
            /* [retval][out] */ IITSource **iSource);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Duration )( 
            IITUserPlaylist * This,
            /* [retval][out] */ long *duration);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Shuffle )( 
            IITUserPlaylist * This,
            /* [retval][out] */ VARIANT_BOOL *isShuffle);
        
        /* [helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Shuffle )( 
            IITUserPlaylist * This,
            /* [in] */ VARIANT_BOOL shouldShuffle);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Size )( 
            IITUserPlaylist * This,
            /* [retval][out] */ double *size);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_SongRepeat )( 
            IITUserPlaylist * This,
            /* [retval][out] */ ITPlaylistRepeatMode *repeatMode);
        
        /* [helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE *put_SongRepeat )( 
            IITUserPlaylist * This,
            /* [in] */ ITPlaylistRepeatMode repeatMode);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Time )( 
            IITUserPlaylist * This,
            /* [retval][out] */ BSTR *time);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Visible )( 
            IITUserPlaylist * This,
            /* [retval][out] */ VARIANT_BOOL *isVisible);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Tracks )( 
            IITUserPlaylist * This,
            /* [retval][out] */ IITTrackCollection **iTrackCollection);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *AddFile )( 
            IITUserPlaylist * This,
            /* [in] */ BSTR filePath,
            /* [retval][out] */ IITOperationStatus **iStatus);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *AddFiles )( 
            IITUserPlaylist * This,
            /* [in] */ VARIANT *filePaths,
            /* [retval][out] */ IITOperationStatus **iStatus);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *AddURL )( 
            IITUserPlaylist * This,
            /* [in] */ BSTR url,
            /* [retval][out] */ IITURLTrack **iURLTrack);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *AddTrack )( 
            IITUserPlaylist * This,
            /* [in] */ VARIANT *iTrackToAdd,
            /* [retval][out] */ IITTrack **iAddedTrack);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Shared )( 
            IITUserPlaylist * This,
            /* [retval][out] */ VARIANT_BOOL *isShared);
        
        /* [helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Shared )( 
            IITUserPlaylist * This,
            /* [in] */ VARIANT_BOOL shouldBeShared);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Smart )( 
            IITUserPlaylist * This,
            /* [retval][out] */ VARIANT_BOOL *isSmart);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_SpecialKind )( 
            IITUserPlaylist * This,
            /* [retval][out] */ ITUserPlaylistSpecialKind *specialKind);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Parent )( 
            IITUserPlaylist * This,
            /* [retval][out] */ IITUserPlaylist **iParentPlayList);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *CreatePlaylist )( 
            IITUserPlaylist * This,
            /* [in] */ BSTR playlistName,
            /* [retval][out] */ IITPlaylist **iPlaylist);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *CreateFolder )( 
            IITUserPlaylist * This,
            /* [in] */ BSTR folderName,
            /* [retval][out] */ IITPlaylist **iFolder);
        
        /* [helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Parent )( 
            IITUserPlaylist * This,
            /* [in] */ VARIANT *iParent);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *Reveal )( 
            IITUserPlaylist * This);
        
        END_INTERFACE
    } IITUserPlaylistVtbl;

    interface IITUserPlaylist
    {
        CONST_VTBL struct IITUserPlaylistVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IITUserPlaylist_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IITUserPlaylist_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IITUserPlaylist_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IITUserPlaylist_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IITUserPlaylist_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IITUserPlaylist_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IITUserPlaylist_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IITUserPlaylist_GetITObjectIDs(This,sourceID,playlistID,trackID,databaseID)	\
    (This)->lpVtbl -> GetITObjectIDs(This,sourceID,playlistID,trackID,databaseID)

#define IITUserPlaylist_get_Name(This,name)	\
    (This)->lpVtbl -> get_Name(This,name)

#define IITUserPlaylist_put_Name(This,name)	\
    (This)->lpVtbl -> put_Name(This,name)

#define IITUserPlaylist_get_Index(This,index)	\
    (This)->lpVtbl -> get_Index(This,index)

#define IITUserPlaylist_get_SourceID(This,sourceID)	\
    (This)->lpVtbl -> get_SourceID(This,sourceID)

#define IITUserPlaylist_get_PlaylistID(This,playlistID)	\
    (This)->lpVtbl -> get_PlaylistID(This,playlistID)

#define IITUserPlaylist_get_TrackID(This,trackID)	\
    (This)->lpVtbl -> get_TrackID(This,trackID)

#define IITUserPlaylist_get_TrackDatabaseID(This,databaseID)	\
    (This)->lpVtbl -> get_TrackDatabaseID(This,databaseID)


#define IITUserPlaylist_Delete(This)	\
    (This)->lpVtbl -> Delete(This)

#define IITUserPlaylist_PlayFirstTrack(This)	\
    (This)->lpVtbl -> PlayFirstTrack(This)

#define IITUserPlaylist_Print(This,showPrintDialog,printKind,theme)	\
    (This)->lpVtbl -> Print(This,showPrintDialog,printKind,theme)

#define IITUserPlaylist_Search(This,searchText,searchFields,iTrackCollection)	\
    (This)->lpVtbl -> Search(This,searchText,searchFields,iTrackCollection)

#define IITUserPlaylist_get_Kind(This,kind)	\
    (This)->lpVtbl -> get_Kind(This,kind)

#define IITUserPlaylist_get_Source(This,iSource)	\
    (This)->lpVtbl -> get_Source(This,iSource)

#define IITUserPlaylist_get_Duration(This,duration)	\
    (This)->lpVtbl -> get_Duration(This,duration)

#define IITUserPlaylist_get_Shuffle(This,isShuffle)	\
    (This)->lpVtbl -> get_Shuffle(This,isShuffle)

#define IITUserPlaylist_put_Shuffle(This,shouldShuffle)	\
    (This)->lpVtbl -> put_Shuffle(This,shouldShuffle)

#define IITUserPlaylist_get_Size(This,size)	\
    (This)->lpVtbl -> get_Size(This,size)

#define IITUserPlaylist_get_SongRepeat(This,repeatMode)	\
    (This)->lpVtbl -> get_SongRepeat(This,repeatMode)

#define IITUserPlaylist_put_SongRepeat(This,repeatMode)	\
    (This)->lpVtbl -> put_SongRepeat(This,repeatMode)

#define IITUserPlaylist_get_Time(This,time)	\
    (This)->lpVtbl -> get_Time(This,time)

#define IITUserPlaylist_get_Visible(This,isVisible)	\
    (This)->lpVtbl -> get_Visible(This,isVisible)

#define IITUserPlaylist_get_Tracks(This,iTrackCollection)	\
    (This)->lpVtbl -> get_Tracks(This,iTrackCollection)


#define IITUserPlaylist_AddFile(This,filePath,iStatus)	\
    (This)->lpVtbl -> AddFile(This,filePath,iStatus)

#define IITUserPlaylist_AddFiles(This,filePaths,iStatus)	\
    (This)->lpVtbl -> AddFiles(This,filePaths,iStatus)

#define IITUserPlaylist_AddURL(This,url,iURLTrack)	\
    (This)->lpVtbl -> AddURL(This,url,iURLTrack)

#define IITUserPlaylist_AddTrack(This,iTrackToAdd,iAddedTrack)	\
    (This)->lpVtbl -> AddTrack(This,iTrackToAdd,iAddedTrack)

#define IITUserPlaylist_get_Shared(This,isShared)	\
    (This)->lpVtbl -> get_Shared(This,isShared)

#define IITUserPlaylist_put_Shared(This,shouldBeShared)	\
    (This)->lpVtbl -> put_Shared(This,shouldBeShared)

#define IITUserPlaylist_get_Smart(This,isSmart)	\
    (This)->lpVtbl -> get_Smart(This,isSmart)

#define IITUserPlaylist_get_SpecialKind(This,specialKind)	\
    (This)->lpVtbl -> get_SpecialKind(This,specialKind)

#define IITUserPlaylist_get_Parent(This,iParentPlayList)	\
    (This)->lpVtbl -> get_Parent(This,iParentPlayList)

#define IITUserPlaylist_CreatePlaylist(This,playlistName,iPlaylist)	\
    (This)->lpVtbl -> CreatePlaylist(This,playlistName,iPlaylist)

#define IITUserPlaylist_CreateFolder(This,folderName,iFolder)	\
    (This)->lpVtbl -> CreateFolder(This,folderName,iFolder)

#define IITUserPlaylist_put_Parent(This,iParent)	\
    (This)->lpVtbl -> put_Parent(This,iParent)

#define IITUserPlaylist_Reveal(This)	\
    (This)->lpVtbl -> Reveal(This)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring] */ HRESULT STDMETHODCALLTYPE IITUserPlaylist_AddFile_Proxy( 
    IITUserPlaylist * This,
    /* [in] */ BSTR filePath,
    /* [retval][out] */ IITOperationStatus **iStatus);


void __RPC_STUB IITUserPlaylist_AddFile_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IITUserPlaylist_AddFiles_Proxy( 
    IITUserPlaylist * This,
    /* [in] */ VARIANT *filePaths,
    /* [retval][out] */ IITOperationStatus **iStatus);


void __RPC_STUB IITUserPlaylist_AddFiles_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IITUserPlaylist_AddURL_Proxy( 
    IITUserPlaylist * This,
    /* [in] */ BSTR url,
    /* [retval][out] */ IITURLTrack **iURLTrack);


void __RPC_STUB IITUserPlaylist_AddURL_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IITUserPlaylist_AddTrack_Proxy( 
    IITUserPlaylist * This,
    /* [in] */ VARIANT *iTrackToAdd,
    /* [retval][out] */ IITTrack **iAddedTrack);


void __RPC_STUB IITUserPlaylist_AddTrack_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IITUserPlaylist_get_Shared_Proxy( 
    IITUserPlaylist * This,
    /* [retval][out] */ VARIANT_BOOL *isShared);


void __RPC_STUB IITUserPlaylist_get_Shared_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE IITUserPlaylist_put_Shared_Proxy( 
    IITUserPlaylist * This,
    /* [in] */ VARIANT_BOOL shouldBeShared);


void __RPC_STUB IITUserPlaylist_put_Shared_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IITUserPlaylist_get_Smart_Proxy( 
    IITUserPlaylist * This,
    /* [retval][out] */ VARIANT_BOOL *isSmart);


void __RPC_STUB IITUserPlaylist_get_Smart_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IITUserPlaylist_get_SpecialKind_Proxy( 
    IITUserPlaylist * This,
    /* [retval][out] */ ITUserPlaylistSpecialKind *specialKind);


void __RPC_STUB IITUserPlaylist_get_SpecialKind_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IITUserPlaylist_get_Parent_Proxy( 
    IITUserPlaylist * This,
    /* [retval][out] */ IITUserPlaylist **iParentPlayList);


void __RPC_STUB IITUserPlaylist_get_Parent_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IITUserPlaylist_CreatePlaylist_Proxy( 
    IITUserPlaylist * This,
    /* [in] */ BSTR playlistName,
    /* [retval][out] */ IITPlaylist **iPlaylist);


void __RPC_STUB IITUserPlaylist_CreatePlaylist_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IITUserPlaylist_CreateFolder_Proxy( 
    IITUserPlaylist * This,
    /* [in] */ BSTR folderName,
    /* [retval][out] */ IITPlaylist **iFolder);


void __RPC_STUB IITUserPlaylist_CreateFolder_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE IITUserPlaylist_put_Parent_Proxy( 
    IITUserPlaylist * This,
    /* [in] */ VARIANT *iParent);


void __RPC_STUB IITUserPlaylist_put_Parent_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IITUserPlaylist_Reveal_Proxy( 
    IITUserPlaylist * This);


void __RPC_STUB IITUserPlaylist_Reveal_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IITUserPlaylist_INTERFACE_DEFINED__ */


#ifndef __IITTrack_INTERFACE_DEFINED__
#define __IITTrack_INTERFACE_DEFINED__

/* interface IITTrack */
/* [hidden][unique][helpstring][dual][uuid][object] */ 


EXTERN_C const IID IID_IITTrack;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("4CB0915D-1E54-4727-BAF3-CE6CC9A225A1")
    IITTrack : public IITObject
    {
    public:
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE Delete( void) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE Play( void) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE AddArtworkFromFile( 
            /* [in] */ BSTR filePath,
            /* [retval][out] */ IITArtwork **iArtwork) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_Kind( 
            /* [retval][out] */ ITTrackKind *kind) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_Playlist( 
            /* [retval][out] */ IITPlaylist **iPlaylist) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_Album( 
            /* [retval][out] */ BSTR *album) = 0;
        
        virtual /* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE put_Album( 
            /* [in] */ BSTR album) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_Artist( 
            /* [retval][out] */ BSTR *artist) = 0;
        
        virtual /* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE put_Artist( 
            /* [in] */ BSTR artist) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_BitRate( 
            /* [retval][out] */ long *bitrate) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_BPM( 
            /* [retval][out] */ long *beatsPerMinute) = 0;
        
        virtual /* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE put_BPM( 
            /* [in] */ long beatsPerMinute) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_Comment( 
            /* [retval][out] */ BSTR *comment) = 0;
        
        virtual /* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE put_Comment( 
            /* [in] */ BSTR comment) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_Compilation( 
            /* [retval][out] */ VARIANT_BOOL *isCompilation) = 0;
        
        virtual /* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE put_Compilation( 
            /* [in] */ VARIANT_BOOL shouldBeCompilation) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_Composer( 
            /* [retval][out] */ BSTR *composer) = 0;
        
        virtual /* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE put_Composer( 
            /* [in] */ BSTR composer) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_DateAdded( 
            /* [retval][out] */ DATE *dateAdded) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_DiscCount( 
            /* [retval][out] */ long *discCount) = 0;
        
        virtual /* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE put_DiscCount( 
            /* [in] */ long discCount) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_DiscNumber( 
            /* [retval][out] */ long *discNumber) = 0;
        
        virtual /* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE put_DiscNumber( 
            /* [in] */ long discNumber) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_Duration( 
            /* [retval][out] */ long *duration) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_Enabled( 
            /* [retval][out] */ VARIANT_BOOL *isEnabled) = 0;
        
        virtual /* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE put_Enabled( 
            /* [in] */ VARIANT_BOOL shouldBeEnabled) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_EQ( 
            /* [retval][out] */ BSTR *eq) = 0;
        
        virtual /* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE put_EQ( 
            /* [in] */ BSTR eq) = 0;
        
        virtual /* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE put_Finish( 
            /* [in] */ long finish) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_Finish( 
            /* [retval][out] */ long *finish) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_Genre( 
            /* [retval][out] */ BSTR *genre) = 0;
        
        virtual /* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE put_Genre( 
            /* [in] */ BSTR genre) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_Grouping( 
            /* [retval][out] */ BSTR *grouping) = 0;
        
        virtual /* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE put_Grouping( 
            /* [in] */ BSTR grouping) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_KindAsString( 
            /* [retval][out] */ BSTR *kind) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_ModificationDate( 
            /* [retval][out] */ DATE *dateModified) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_PlayedCount( 
            /* [retval][out] */ long *playedCount) = 0;
        
        virtual /* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE put_PlayedCount( 
            /* [in] */ long playedCount) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_PlayedDate( 
            /* [retval][out] */ DATE *playedDate) = 0;
        
        virtual /* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE put_PlayedDate( 
            /* [in] */ DATE playedDate) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_PlayOrderIndex( 
            /* [retval][out] */ long *index) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_Rating( 
            /* [retval][out] */ long *rating) = 0;
        
        virtual /* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE put_Rating( 
            /* [in] */ long rating) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_SampleRate( 
            /* [retval][out] */ long *sampleRate) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_Size( 
            /* [retval][out] */ long *size) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_Start( 
            /* [retval][out] */ long *start) = 0;
        
        virtual /* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE put_Start( 
            /* [in] */ long start) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_Time( 
            /* [retval][out] */ BSTR *time) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_TrackCount( 
            /* [retval][out] */ long *trackCount) = 0;
        
        virtual /* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE put_TrackCount( 
            /* [in] */ long trackCount) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_TrackNumber( 
            /* [retval][out] */ long *trackNumber) = 0;
        
        virtual /* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE put_TrackNumber( 
            /* [in] */ long trackNumber) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_VolumeAdjustment( 
            /* [retval][out] */ long *volumeAdjustment) = 0;
        
        virtual /* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE put_VolumeAdjustment( 
            /* [in] */ long volumeAdjustment) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_Year( 
            /* [retval][out] */ long *year) = 0;
        
        virtual /* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE put_Year( 
            /* [in] */ long year) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_Artwork( 
            /* [retval][out] */ IITArtworkCollection **iArtworkCollection) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IITTrackVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IITTrack * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IITTrack * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IITTrack * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IITTrack * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IITTrack * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IITTrack * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IITTrack * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *GetITObjectIDs )( 
            IITTrack * This,
            /* [out] */ long *sourceID,
            /* [out] */ long *playlistID,
            /* [out] */ long *trackID,
            /* [out] */ long *databaseID);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Name )( 
            IITTrack * This,
            /* [retval][out] */ BSTR *name);
        
        /* [helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Name )( 
            IITTrack * This,
            /* [in] */ BSTR name);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Index )( 
            IITTrack * This,
            /* [retval][out] */ long *index);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_SourceID )( 
            IITTrack * This,
            /* [retval][out] */ long *sourceID);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_PlaylistID )( 
            IITTrack * This,
            /* [retval][out] */ long *playlistID);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_TrackID )( 
            IITTrack * This,
            /* [retval][out] */ long *trackID);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_TrackDatabaseID )( 
            IITTrack * This,
            /* [retval][out] */ long *databaseID);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *Delete )( 
            IITTrack * This);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *Play )( 
            IITTrack * This);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *AddArtworkFromFile )( 
            IITTrack * This,
            /* [in] */ BSTR filePath,
            /* [retval][out] */ IITArtwork **iArtwork);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Kind )( 
            IITTrack * This,
            /* [retval][out] */ ITTrackKind *kind);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Playlist )( 
            IITTrack * This,
            /* [retval][out] */ IITPlaylist **iPlaylist);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Album )( 
            IITTrack * This,
            /* [retval][out] */ BSTR *album);
        
        /* [helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Album )( 
            IITTrack * This,
            /* [in] */ BSTR album);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Artist )( 
            IITTrack * This,
            /* [retval][out] */ BSTR *artist);
        
        /* [helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Artist )( 
            IITTrack * This,
            /* [in] */ BSTR artist);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_BitRate )( 
            IITTrack * This,
            /* [retval][out] */ long *bitrate);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_BPM )( 
            IITTrack * This,
            /* [retval][out] */ long *beatsPerMinute);
        
        /* [helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE *put_BPM )( 
            IITTrack * This,
            /* [in] */ long beatsPerMinute);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Comment )( 
            IITTrack * This,
            /* [retval][out] */ BSTR *comment);
        
        /* [helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Comment )( 
            IITTrack * This,
            /* [in] */ BSTR comment);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Compilation )( 
            IITTrack * This,
            /* [retval][out] */ VARIANT_BOOL *isCompilation);
        
        /* [helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Compilation )( 
            IITTrack * This,
            /* [in] */ VARIANT_BOOL shouldBeCompilation);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Composer )( 
            IITTrack * This,
            /* [retval][out] */ BSTR *composer);
        
        /* [helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Composer )( 
            IITTrack * This,
            /* [in] */ BSTR composer);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_DateAdded )( 
            IITTrack * This,
            /* [retval][out] */ DATE *dateAdded);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_DiscCount )( 
            IITTrack * This,
            /* [retval][out] */ long *discCount);
        
        /* [helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE *put_DiscCount )( 
            IITTrack * This,
            /* [in] */ long discCount);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_DiscNumber )( 
            IITTrack * This,
            /* [retval][out] */ long *discNumber);
        
        /* [helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE *put_DiscNumber )( 
            IITTrack * This,
            /* [in] */ long discNumber);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Duration )( 
            IITTrack * This,
            /* [retval][out] */ long *duration);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Enabled )( 
            IITTrack * This,
            /* [retval][out] */ VARIANT_BOOL *isEnabled);
        
        /* [helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Enabled )( 
            IITTrack * This,
            /* [in] */ VARIANT_BOOL shouldBeEnabled);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_EQ )( 
            IITTrack * This,
            /* [retval][out] */ BSTR *eq);
        
        /* [helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE *put_EQ )( 
            IITTrack * This,
            /* [in] */ BSTR eq);
        
        /* [helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Finish )( 
            IITTrack * This,
            /* [in] */ long finish);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Finish )( 
            IITTrack * This,
            /* [retval][out] */ long *finish);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Genre )( 
            IITTrack * This,
            /* [retval][out] */ BSTR *genre);
        
        /* [helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Genre )( 
            IITTrack * This,
            /* [in] */ BSTR genre);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Grouping )( 
            IITTrack * This,
            /* [retval][out] */ BSTR *grouping);
        
        /* [helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Grouping )( 
            IITTrack * This,
            /* [in] */ BSTR grouping);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_KindAsString )( 
            IITTrack * This,
            /* [retval][out] */ BSTR *kind);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_ModificationDate )( 
            IITTrack * This,
            /* [retval][out] */ DATE *dateModified);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_PlayedCount )( 
            IITTrack * This,
            /* [retval][out] */ long *playedCount);
        
        /* [helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE *put_PlayedCount )( 
            IITTrack * This,
            /* [in] */ long playedCount);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_PlayedDate )( 
            IITTrack * This,
            /* [retval][out] */ DATE *playedDate);
        
        /* [helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE *put_PlayedDate )( 
            IITTrack * This,
            /* [in] */ DATE playedDate);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_PlayOrderIndex )( 
            IITTrack * This,
            /* [retval][out] */ long *index);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Rating )( 
            IITTrack * This,
            /* [retval][out] */ long *rating);
        
        /* [helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Rating )( 
            IITTrack * This,
            /* [in] */ long rating);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_SampleRate )( 
            IITTrack * This,
            /* [retval][out] */ long *sampleRate);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Size )( 
            IITTrack * This,
            /* [retval][out] */ long *size);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Start )( 
            IITTrack * This,
            /* [retval][out] */ long *start);
        
        /* [helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Start )( 
            IITTrack * This,
            /* [in] */ long start);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Time )( 
            IITTrack * This,
            /* [retval][out] */ BSTR *time);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_TrackCount )( 
            IITTrack * This,
            /* [retval][out] */ long *trackCount);
        
        /* [helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE *put_TrackCount )( 
            IITTrack * This,
            /* [in] */ long trackCount);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_TrackNumber )( 
            IITTrack * This,
            /* [retval][out] */ long *trackNumber);
        
        /* [helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE *put_TrackNumber )( 
            IITTrack * This,
            /* [in] */ long trackNumber);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_VolumeAdjustment )( 
            IITTrack * This,
            /* [retval][out] */ long *volumeAdjustment);
        
        /* [helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE *put_VolumeAdjustment )( 
            IITTrack * This,
            /* [in] */ long volumeAdjustment);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Year )( 
            IITTrack * This,
            /* [retval][out] */ long *year);
        
        /* [helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Year )( 
            IITTrack * This,
            /* [in] */ long year);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Artwork )( 
            IITTrack * This,
            /* [retval][out] */ IITArtworkCollection **iArtworkCollection);
        
        END_INTERFACE
    } IITTrackVtbl;

    interface IITTrack
    {
        CONST_VTBL struct IITTrackVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IITTrack_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IITTrack_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IITTrack_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IITTrack_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IITTrack_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IITTrack_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IITTrack_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IITTrack_GetITObjectIDs(This,sourceID,playlistID,trackID,databaseID)	\
    (This)->lpVtbl -> GetITObjectIDs(This,sourceID,playlistID,trackID,databaseID)

#define IITTrack_get_Name(This,name)	\
    (This)->lpVtbl -> get_Name(This,name)

#define IITTrack_put_Name(This,name)	\
    (This)->lpVtbl -> put_Name(This,name)

#define IITTrack_get_Index(This,index)	\
    (This)->lpVtbl -> get_Index(This,index)

#define IITTrack_get_SourceID(This,sourceID)	\
    (This)->lpVtbl -> get_SourceID(This,sourceID)

#define IITTrack_get_PlaylistID(This,playlistID)	\
    (This)->lpVtbl -> get_PlaylistID(This,playlistID)

#define IITTrack_get_TrackID(This,trackID)	\
    (This)->lpVtbl -> get_TrackID(This,trackID)

#define IITTrack_get_TrackDatabaseID(This,databaseID)	\
    (This)->lpVtbl -> get_TrackDatabaseID(This,databaseID)


#define IITTrack_Delete(This)	\
    (This)->lpVtbl -> Delete(This)

#define IITTrack_Play(This)	\
    (This)->lpVtbl -> Play(This)

#define IITTrack_AddArtworkFromFile(This,filePath,iArtwork)	\
    (This)->lpVtbl -> AddArtworkFromFile(This,filePath,iArtwork)

#define IITTrack_get_Kind(This,kind)	\
    (This)->lpVtbl -> get_Kind(This,kind)

#define IITTrack_get_Playlist(This,iPlaylist)	\
    (This)->lpVtbl -> get_Playlist(This,iPlaylist)

#define IITTrack_get_Album(This,album)	\
    (This)->lpVtbl -> get_Album(This,album)

#define IITTrack_put_Album(This,album)	\
    (This)->lpVtbl -> put_Album(This,album)

#define IITTrack_get_Artist(This,artist)	\
    (This)->lpVtbl -> get_Artist(This,artist)

#define IITTrack_put_Artist(This,artist)	\
    (This)->lpVtbl -> put_Artist(This,artist)

#define IITTrack_get_BitRate(This,bitrate)	\
    (This)->lpVtbl -> get_BitRate(This,bitrate)

#define IITTrack_get_BPM(This,beatsPerMinute)	\
    (This)->lpVtbl -> get_BPM(This,beatsPerMinute)

#define IITTrack_put_BPM(This,beatsPerMinute)	\
    (This)->lpVtbl -> put_BPM(This,beatsPerMinute)

#define IITTrack_get_Comment(This,comment)	\
    (This)->lpVtbl -> get_Comment(This,comment)

#define IITTrack_put_Comment(This,comment)	\
    (This)->lpVtbl -> put_Comment(This,comment)

#define IITTrack_get_Compilation(This,isCompilation)	\
    (This)->lpVtbl -> get_Compilation(This,isCompilation)

#define IITTrack_put_Compilation(This,shouldBeCompilation)	\
    (This)->lpVtbl -> put_Compilation(This,shouldBeCompilation)

#define IITTrack_get_Composer(This,composer)	\
    (This)->lpVtbl -> get_Composer(This,composer)

#define IITTrack_put_Composer(This,composer)	\
    (This)->lpVtbl -> put_Composer(This,composer)

#define IITTrack_get_DateAdded(This,dateAdded)	\
    (This)->lpVtbl -> get_DateAdded(This,dateAdded)

#define IITTrack_get_DiscCount(This,discCount)	\
    (This)->lpVtbl -> get_DiscCount(This,discCount)

#define IITTrack_put_DiscCount(This,discCount)	\
    (This)->lpVtbl -> put_DiscCount(This,discCount)

#define IITTrack_get_DiscNumber(This,discNumber)	\
    (This)->lpVtbl -> get_DiscNumber(This,discNumber)

#define IITTrack_put_DiscNumber(This,discNumber)	\
    (This)->lpVtbl -> put_DiscNumber(This,discNumber)

#define IITTrack_get_Duration(This,duration)	\
    (This)->lpVtbl -> get_Duration(This,duration)

#define IITTrack_get_Enabled(This,isEnabled)	\
    (This)->lpVtbl -> get_Enabled(This,isEnabled)

#define IITTrack_put_Enabled(This,shouldBeEnabled)	\
    (This)->lpVtbl -> put_Enabled(This,shouldBeEnabled)

#define IITTrack_get_EQ(This,eq)	\
    (This)->lpVtbl -> get_EQ(This,eq)

#define IITTrack_put_EQ(This,eq)	\
    (This)->lpVtbl -> put_EQ(This,eq)

#define IITTrack_put_Finish(This,finish)	\
    (This)->lpVtbl -> put_Finish(This,finish)

#define IITTrack_get_Finish(This,finish)	\
    (This)->lpVtbl -> get_Finish(This,finish)

#define IITTrack_get_Genre(This,genre)	\
    (This)->lpVtbl -> get_Genre(This,genre)

#define IITTrack_put_Genre(This,genre)	\
    (This)->lpVtbl -> put_Genre(This,genre)

#define IITTrack_get_Grouping(This,grouping)	\
    (This)->lpVtbl -> get_Grouping(This,grouping)

#define IITTrack_put_Grouping(This,grouping)	\
    (This)->lpVtbl -> put_Grouping(This,grouping)

#define IITTrack_get_KindAsString(This,kind)	\
    (This)->lpVtbl -> get_KindAsString(This,kind)

#define IITTrack_get_ModificationDate(This,dateModified)	\
    (This)->lpVtbl -> get_ModificationDate(This,dateModified)

#define IITTrack_get_PlayedCount(This,playedCount)	\
    (This)->lpVtbl -> get_PlayedCount(This,playedCount)

#define IITTrack_put_PlayedCount(This,playedCount)	\
    (This)->lpVtbl -> put_PlayedCount(This,playedCount)

#define IITTrack_get_PlayedDate(This,playedDate)	\
    (This)->lpVtbl -> get_PlayedDate(This,playedDate)

#define IITTrack_put_PlayedDate(This,playedDate)	\
    (This)->lpVtbl -> put_PlayedDate(This,playedDate)

#define IITTrack_get_PlayOrderIndex(This,index)	\
    (This)->lpVtbl -> get_PlayOrderIndex(This,index)

#define IITTrack_get_Rating(This,rating)	\
    (This)->lpVtbl -> get_Rating(This,rating)

#define IITTrack_put_Rating(This,rating)	\
    (This)->lpVtbl -> put_Rating(This,rating)

#define IITTrack_get_SampleRate(This,sampleRate)	\
    (This)->lpVtbl -> get_SampleRate(This,sampleRate)

#define IITTrack_get_Size(This,size)	\
    (This)->lpVtbl -> get_Size(This,size)

#define IITTrack_get_Start(This,start)	\
    (This)->lpVtbl -> get_Start(This,start)

#define IITTrack_put_Start(This,start)	\
    (This)->lpVtbl -> put_Start(This,start)

#define IITTrack_get_Time(This,time)	\
    (This)->lpVtbl -> get_Time(This,time)

#define IITTrack_get_TrackCount(This,trackCount)	\
    (This)->lpVtbl -> get_TrackCount(This,trackCount)

#define IITTrack_put_TrackCount(This,trackCount)	\
    (This)->lpVtbl -> put_TrackCount(This,trackCount)

#define IITTrack_get_TrackNumber(This,trackNumber)	\
    (This)->lpVtbl -> get_TrackNumber(This,trackNumber)

#define IITTrack_put_TrackNumber(This,trackNumber)	\
    (This)->lpVtbl -> put_TrackNumber(This,trackNumber)

#define IITTrack_get_VolumeAdjustment(This,volumeAdjustment)	\
    (This)->lpVtbl -> get_VolumeAdjustment(This,volumeAdjustment)

#define IITTrack_put_VolumeAdjustment(This,volumeAdjustment)	\
    (This)->lpVtbl -> put_VolumeAdjustment(This,volumeAdjustment)

#define IITTrack_get_Year(This,year)	\
    (This)->lpVtbl -> get_Year(This,year)

#define IITTrack_put_Year(This,year)	\
    (This)->lpVtbl -> put_Year(This,year)

#define IITTrack_get_Artwork(This,iArtworkCollection)	\
    (This)->lpVtbl -> get_Artwork(This,iArtworkCollection)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring] */ HRESULT STDMETHODCALLTYPE IITTrack_Delete_Proxy( 
    IITTrack * This);


void __RPC_STUB IITTrack_Delete_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IITTrack_Play_Proxy( 
    IITTrack * This);


void __RPC_STUB IITTrack_Play_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IITTrack_AddArtworkFromFile_Proxy( 
    IITTrack * This,
    /* [in] */ BSTR filePath,
    /* [retval][out] */ IITArtwork **iArtwork);


void __RPC_STUB IITTrack_AddArtworkFromFile_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IITTrack_get_Kind_Proxy( 
    IITTrack * This,
    /* [retval][out] */ ITTrackKind *kind);


void __RPC_STUB IITTrack_get_Kind_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IITTrack_get_Playlist_Proxy( 
    IITTrack * This,
    /* [retval][out] */ IITPlaylist **iPlaylist);


void __RPC_STUB IITTrack_get_Playlist_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IITTrack_get_Album_Proxy( 
    IITTrack * This,
    /* [retval][out] */ BSTR *album);


void __RPC_STUB IITTrack_get_Album_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE IITTrack_put_Album_Proxy( 
    IITTrack * This,
    /* [in] */ BSTR album);


void __RPC_STUB IITTrack_put_Album_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IITTrack_get_Artist_Proxy( 
    IITTrack * This,
    /* [retval][out] */ BSTR *artist);


void __RPC_STUB IITTrack_get_Artist_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE IITTrack_put_Artist_Proxy( 
    IITTrack * This,
    /* [in] */ BSTR artist);


void __RPC_STUB IITTrack_put_Artist_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IITTrack_get_BitRate_Proxy( 
    IITTrack * This,
    /* [retval][out] */ long *bitrate);


void __RPC_STUB IITTrack_get_BitRate_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IITTrack_get_BPM_Proxy( 
    IITTrack * This,
    /* [retval][out] */ long *beatsPerMinute);


void __RPC_STUB IITTrack_get_BPM_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE IITTrack_put_BPM_Proxy( 
    IITTrack * This,
    /* [in] */ long beatsPerMinute);


void __RPC_STUB IITTrack_put_BPM_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IITTrack_get_Comment_Proxy( 
    IITTrack * This,
    /* [retval][out] */ BSTR *comment);


void __RPC_STUB IITTrack_get_Comment_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE IITTrack_put_Comment_Proxy( 
    IITTrack * This,
    /* [in] */ BSTR comment);


void __RPC_STUB IITTrack_put_Comment_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IITTrack_get_Compilation_Proxy( 
    IITTrack * This,
    /* [retval][out] */ VARIANT_BOOL *isCompilation);


void __RPC_STUB IITTrack_get_Compilation_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE IITTrack_put_Compilation_Proxy( 
    IITTrack * This,
    /* [in] */ VARIANT_BOOL shouldBeCompilation);


void __RPC_STUB IITTrack_put_Compilation_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IITTrack_get_Composer_Proxy( 
    IITTrack * This,
    /* [retval][out] */ BSTR *composer);


void __RPC_STUB IITTrack_get_Composer_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE IITTrack_put_Composer_Proxy( 
    IITTrack * This,
    /* [in] */ BSTR composer);


void __RPC_STUB IITTrack_put_Composer_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IITTrack_get_DateAdded_Proxy( 
    IITTrack * This,
    /* [retval][out] */ DATE *dateAdded);


void __RPC_STUB IITTrack_get_DateAdded_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IITTrack_get_DiscCount_Proxy( 
    IITTrack * This,
    /* [retval][out] */ long *discCount);


void __RPC_STUB IITTrack_get_DiscCount_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE IITTrack_put_DiscCount_Proxy( 
    IITTrack * This,
    /* [in] */ long discCount);


void __RPC_STUB IITTrack_put_DiscCount_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IITTrack_get_DiscNumber_Proxy( 
    IITTrack * This,
    /* [retval][out] */ long *discNumber);


void __RPC_STUB IITTrack_get_DiscNumber_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE IITTrack_put_DiscNumber_Proxy( 
    IITTrack * This,
    /* [in] */ long discNumber);


void __RPC_STUB IITTrack_put_DiscNumber_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IITTrack_get_Duration_Proxy( 
    IITTrack * This,
    /* [retval][out] */ long *duration);


void __RPC_STUB IITTrack_get_Duration_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IITTrack_get_Enabled_Proxy( 
    IITTrack * This,
    /* [retval][out] */ VARIANT_BOOL *isEnabled);


void __RPC_STUB IITTrack_get_Enabled_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE IITTrack_put_Enabled_Proxy( 
    IITTrack * This,
    /* [in] */ VARIANT_BOOL shouldBeEnabled);


void __RPC_STUB IITTrack_put_Enabled_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IITTrack_get_EQ_Proxy( 
    IITTrack * This,
    /* [retval][out] */ BSTR *eq);


void __RPC_STUB IITTrack_get_EQ_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE IITTrack_put_EQ_Proxy( 
    IITTrack * This,
    /* [in] */ BSTR eq);


void __RPC_STUB IITTrack_put_EQ_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE IITTrack_put_Finish_Proxy( 
    IITTrack * This,
    /* [in] */ long finish);


void __RPC_STUB IITTrack_put_Finish_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IITTrack_get_Finish_Proxy( 
    IITTrack * This,
    /* [retval][out] */ long *finish);


void __RPC_STUB IITTrack_get_Finish_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IITTrack_get_Genre_Proxy( 
    IITTrack * This,
    /* [retval][out] */ BSTR *genre);


void __RPC_STUB IITTrack_get_Genre_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE IITTrack_put_Genre_Proxy( 
    IITTrack * This,
    /* [in] */ BSTR genre);


void __RPC_STUB IITTrack_put_Genre_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IITTrack_get_Grouping_Proxy( 
    IITTrack * This,
    /* [retval][out] */ BSTR *grouping);


void __RPC_STUB IITTrack_get_Grouping_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE IITTrack_put_Grouping_Proxy( 
    IITTrack * This,
    /* [in] */ BSTR grouping);


void __RPC_STUB IITTrack_put_Grouping_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IITTrack_get_KindAsString_Proxy( 
    IITTrack * This,
    /* [retval][out] */ BSTR *kind);


void __RPC_STUB IITTrack_get_KindAsString_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IITTrack_get_ModificationDate_Proxy( 
    IITTrack * This,
    /* [retval][out] */ DATE *dateModified);


void __RPC_STUB IITTrack_get_ModificationDate_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IITTrack_get_PlayedCount_Proxy( 
    IITTrack * This,
    /* [retval][out] */ long *playedCount);


void __RPC_STUB IITTrack_get_PlayedCount_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE IITTrack_put_PlayedCount_Proxy( 
    IITTrack * This,
    /* [in] */ long playedCount);


void __RPC_STUB IITTrack_put_PlayedCount_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IITTrack_get_PlayedDate_Proxy( 
    IITTrack * This,
    /* [retval][out] */ DATE *playedDate);


void __RPC_STUB IITTrack_get_PlayedDate_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE IITTrack_put_PlayedDate_Proxy( 
    IITTrack * This,
    /* [in] */ DATE playedDate);


void __RPC_STUB IITTrack_put_PlayedDate_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IITTrack_get_PlayOrderIndex_Proxy( 
    IITTrack * This,
    /* [retval][out] */ long *index);


void __RPC_STUB IITTrack_get_PlayOrderIndex_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IITTrack_get_Rating_Proxy( 
    IITTrack * This,
    /* [retval][out] */ long *rating);


void __RPC_STUB IITTrack_get_Rating_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE IITTrack_put_Rating_Proxy( 
    IITTrack * This,
    /* [in] */ long rating);


void __RPC_STUB IITTrack_put_Rating_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IITTrack_get_SampleRate_Proxy( 
    IITTrack * This,
    /* [retval][out] */ long *sampleRate);


void __RPC_STUB IITTrack_get_SampleRate_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IITTrack_get_Size_Proxy( 
    IITTrack * This,
    /* [retval][out] */ long *size);


void __RPC_STUB IITTrack_get_Size_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IITTrack_get_Start_Proxy( 
    IITTrack * This,
    /* [retval][out] */ long *start);


void __RPC_STUB IITTrack_get_Start_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE IITTrack_put_Start_Proxy( 
    IITTrack * This,
    /* [in] */ long start);


void __RPC_STUB IITTrack_put_Start_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IITTrack_get_Time_Proxy( 
    IITTrack * This,
    /* [retval][out] */ BSTR *time);


void __RPC_STUB IITTrack_get_Time_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IITTrack_get_TrackCount_Proxy( 
    IITTrack * This,
    /* [retval][out] */ long *trackCount);


void __RPC_STUB IITTrack_get_TrackCount_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE IITTrack_put_TrackCount_Proxy( 
    IITTrack * This,
    /* [in] */ long trackCount);


void __RPC_STUB IITTrack_put_TrackCount_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IITTrack_get_TrackNumber_Proxy( 
    IITTrack * This,
    /* [retval][out] */ long *trackNumber);


void __RPC_STUB IITTrack_get_TrackNumber_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE IITTrack_put_TrackNumber_Proxy( 
    IITTrack * This,
    /* [in] */ long trackNumber);


void __RPC_STUB IITTrack_put_TrackNumber_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IITTrack_get_VolumeAdjustment_Proxy( 
    IITTrack * This,
    /* [retval][out] */ long *volumeAdjustment);


void __RPC_STUB IITTrack_get_VolumeAdjustment_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE IITTrack_put_VolumeAdjustment_Proxy( 
    IITTrack * This,
    /* [in] */ long volumeAdjustment);


void __RPC_STUB IITTrack_put_VolumeAdjustment_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IITTrack_get_Year_Proxy( 
    IITTrack * This,
    /* [retval][out] */ long *year);


void __RPC_STUB IITTrack_get_Year_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE IITTrack_put_Year_Proxy( 
    IITTrack * This,
    /* [in] */ long year);


void __RPC_STUB IITTrack_put_Year_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IITTrack_get_Artwork_Proxy( 
    IITTrack * This,
    /* [retval][out] */ IITArtworkCollection **iArtworkCollection);


void __RPC_STUB IITTrack_get_Artwork_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IITTrack_INTERFACE_DEFINED__ */


#ifndef __IITTrackCollection_INTERFACE_DEFINED__
#define __IITTrackCollection_INTERFACE_DEFINED__

/* interface IITTrackCollection */
/* [unique][helpstring][dual][uuid][object] */ 


EXTERN_C const IID IID_IITTrackCollection;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("755D76F1-6B85-4ce4-8F5F-F88D9743DCD8")
    IITTrackCollection : public IDispatch
    {
    public:
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_Count( 
            /* [retval][out] */ long *count) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Item( 
            /* [in] */ long index,
            /* [retval][out] */ IITTrack **iTrack) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_ItemByPlayOrder( 
            /* [in] */ long index,
            /* [retval][out] */ IITTrack **iTrack) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_ItemByName( 
            /* [in] */ BSTR name,
            /* [retval][out] */ IITTrack **iTrack) = 0;
        
        virtual /* [helpstring][restricted][id][propget] */ HRESULT STDMETHODCALLTYPE get__NewEnum( 
            /* [retval][out] */ IUnknown **iEnumerator) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_ItemByPersistentID( 
            /* [in] */ long highID,
            /* [in] */ long lowID,
            /* [retval][out] */ IITTrack **iTrack) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IITTrackCollectionVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IITTrackCollection * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IITTrackCollection * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IITTrackCollection * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IITTrackCollection * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IITTrackCollection * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IITTrackCollection * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IITTrackCollection * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Count )( 
            IITTrackCollection * This,
            /* [retval][out] */ long *count);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Item )( 
            IITTrackCollection * This,
            /* [in] */ long index,
            /* [retval][out] */ IITTrack **iTrack);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_ItemByPlayOrder )( 
            IITTrackCollection * This,
            /* [in] */ long index,
            /* [retval][out] */ IITTrack **iTrack);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_ItemByName )( 
            IITTrackCollection * This,
            /* [in] */ BSTR name,
            /* [retval][out] */ IITTrack **iTrack);
        
        /* [helpstring][restricted][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get__NewEnum )( 
            IITTrackCollection * This,
            /* [retval][out] */ IUnknown **iEnumerator);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_ItemByPersistentID )( 
            IITTrackCollection * This,
            /* [in] */ long highID,
            /* [in] */ long lowID,
            /* [retval][out] */ IITTrack **iTrack);
        
        END_INTERFACE
    } IITTrackCollectionVtbl;

    interface IITTrackCollection
    {
        CONST_VTBL struct IITTrackCollectionVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IITTrackCollection_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IITTrackCollection_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IITTrackCollection_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IITTrackCollection_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IITTrackCollection_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IITTrackCollection_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IITTrackCollection_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IITTrackCollection_get_Count(This,count)	\
    (This)->lpVtbl -> get_Count(This,count)

#define IITTrackCollection_get_Item(This,index,iTrack)	\
    (This)->lpVtbl -> get_Item(This,index,iTrack)

#define IITTrackCollection_get_ItemByPlayOrder(This,index,iTrack)	\
    (This)->lpVtbl -> get_ItemByPlayOrder(This,index,iTrack)

#define IITTrackCollection_get_ItemByName(This,name,iTrack)	\
    (This)->lpVtbl -> get_ItemByName(This,name,iTrack)

#define IITTrackCollection_get__NewEnum(This,iEnumerator)	\
    (This)->lpVtbl -> get__NewEnum(This,iEnumerator)

#define IITTrackCollection_get_ItemByPersistentID(This,highID,lowID,iTrack)	\
    (This)->lpVtbl -> get_ItemByPersistentID(This,highID,lowID,iTrack)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IITTrackCollection_get_Count_Proxy( 
    IITTrackCollection * This,
    /* [retval][out] */ long *count);


void __RPC_STUB IITTrackCollection_get_Count_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IITTrackCollection_get_Item_Proxy( 
    IITTrackCollection * This,
    /* [in] */ long index,
    /* [retval][out] */ IITTrack **iTrack);


void __RPC_STUB IITTrackCollection_get_Item_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IITTrackCollection_get_ItemByPlayOrder_Proxy( 
    IITTrackCollection * This,
    /* [in] */ long index,
    /* [retval][out] */ IITTrack **iTrack);


void __RPC_STUB IITTrackCollection_get_ItemByPlayOrder_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IITTrackCollection_get_ItemByName_Proxy( 
    IITTrackCollection * This,
    /* [in] */ BSTR name,
    /* [retval][out] */ IITTrack **iTrack);


void __RPC_STUB IITTrackCollection_get_ItemByName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][restricted][id][propget] */ HRESULT STDMETHODCALLTYPE IITTrackCollection_get__NewEnum_Proxy( 
    IITTrackCollection * This,
    /* [retval][out] */ IUnknown **iEnumerator);


void __RPC_STUB IITTrackCollection_get__NewEnum_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IITTrackCollection_get_ItemByPersistentID_Proxy( 
    IITTrackCollection * This,
    /* [in] */ long highID,
    /* [in] */ long lowID,
    /* [retval][out] */ IITTrack **iTrack);


void __RPC_STUB IITTrackCollection_get_ItemByPersistentID_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IITTrackCollection_INTERFACE_DEFINED__ */


#ifndef __IITVisual_INTERFACE_DEFINED__
#define __IITVisual_INTERFACE_DEFINED__

/* interface IITVisual */
/* [hidden][unique][helpstring][dual][uuid][object] */ 


EXTERN_C const IID IID_IITVisual;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("340F3315-ED72-4c09-9ACF-21EB4BDF9931")
    IITVisual : public IDispatch
    {
    public:
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_Name( 
            /* [retval][out] */ BSTR *name) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IITVisualVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IITVisual * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IITVisual * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IITVisual * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IITVisual * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IITVisual * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IITVisual * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IITVisual * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Name )( 
            IITVisual * This,
            /* [retval][out] */ BSTR *name);
        
        END_INTERFACE
    } IITVisualVtbl;

    interface IITVisual
    {
        CONST_VTBL struct IITVisualVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IITVisual_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IITVisual_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IITVisual_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IITVisual_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IITVisual_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IITVisual_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IITVisual_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IITVisual_get_Name(This,name)	\
    (This)->lpVtbl -> get_Name(This,name)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IITVisual_get_Name_Proxy( 
    IITVisual * This,
    /* [retval][out] */ BSTR *name);


void __RPC_STUB IITVisual_get_Name_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IITVisual_INTERFACE_DEFINED__ */


#ifndef __IITVisualCollection_INTERFACE_DEFINED__
#define __IITVisualCollection_INTERFACE_DEFINED__

/* interface IITVisualCollection */
/* [unique][helpstring][dual][uuid][object] */ 


EXTERN_C const IID IID_IITVisualCollection;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("88A4CCDD-114F-4043-B69B-84D4E6274957")
    IITVisualCollection : public IDispatch
    {
    public:
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_Count( 
            /* [retval][out] */ long *count) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Item( 
            /* [in] */ long index,
            /* [retval][out] */ IITVisual **iVisual) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_ItemByName( 
            /* [in] */ BSTR name,
            /* [retval][out] */ IITVisual **iVisual) = 0;
        
        virtual /* [helpstring][restricted][id][propget] */ HRESULT STDMETHODCALLTYPE get__NewEnum( 
            /* [retval][out] */ IUnknown **iEnumerator) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IITVisualCollectionVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IITVisualCollection * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IITVisualCollection * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IITVisualCollection * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IITVisualCollection * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IITVisualCollection * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IITVisualCollection * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IITVisualCollection * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Count )( 
            IITVisualCollection * This,
            /* [retval][out] */ long *count);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Item )( 
            IITVisualCollection * This,
            /* [in] */ long index,
            /* [retval][out] */ IITVisual **iVisual);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_ItemByName )( 
            IITVisualCollection * This,
            /* [in] */ BSTR name,
            /* [retval][out] */ IITVisual **iVisual);
        
        /* [helpstring][restricted][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get__NewEnum )( 
            IITVisualCollection * This,
            /* [retval][out] */ IUnknown **iEnumerator);
        
        END_INTERFACE
    } IITVisualCollectionVtbl;

    interface IITVisualCollection
    {
        CONST_VTBL struct IITVisualCollectionVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IITVisualCollection_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IITVisualCollection_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IITVisualCollection_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IITVisualCollection_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IITVisualCollection_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IITVisualCollection_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IITVisualCollection_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IITVisualCollection_get_Count(This,count)	\
    (This)->lpVtbl -> get_Count(This,count)

#define IITVisualCollection_get_Item(This,index,iVisual)	\
    (This)->lpVtbl -> get_Item(This,index,iVisual)

#define IITVisualCollection_get_ItemByName(This,name,iVisual)	\
    (This)->lpVtbl -> get_ItemByName(This,name,iVisual)

#define IITVisualCollection_get__NewEnum(This,iEnumerator)	\
    (This)->lpVtbl -> get__NewEnum(This,iEnumerator)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IITVisualCollection_get_Count_Proxy( 
    IITVisualCollection * This,
    /* [retval][out] */ long *count);


void __RPC_STUB IITVisualCollection_get_Count_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IITVisualCollection_get_Item_Proxy( 
    IITVisualCollection * This,
    /* [in] */ long index,
    /* [retval][out] */ IITVisual **iVisual);


void __RPC_STUB IITVisualCollection_get_Item_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IITVisualCollection_get_ItemByName_Proxy( 
    IITVisualCollection * This,
    /* [in] */ BSTR name,
    /* [retval][out] */ IITVisual **iVisual);


void __RPC_STUB IITVisualCollection_get_ItemByName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][restricted][id][propget] */ HRESULT STDMETHODCALLTYPE IITVisualCollection_get__NewEnum_Proxy( 
    IITVisualCollection * This,
    /* [retval][out] */ IUnknown **iEnumerator);


void __RPC_STUB IITVisualCollection_get__NewEnum_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IITVisualCollection_INTERFACE_DEFINED__ */


#ifndef __IITWindow_INTERFACE_DEFINED__
#define __IITWindow_INTERFACE_DEFINED__

/* interface IITWindow */
/* [hidden][unique][helpstring][dual][uuid][object] */ 


EXTERN_C const IID IID_IITWindow;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("370D7BE0-3A89-4a42-B902-C75FC138BE09")
    IITWindow : public IDispatch
    {
    public:
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_Name( 
            /* [retval][out] */ BSTR *name) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_Kind( 
            /* [retval][out] */ ITWindowKind *kind) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_Visible( 
            /* [retval][out] */ VARIANT_BOOL *isVisible) = 0;
        
        virtual /* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE put_Visible( 
            /* [in] */ VARIANT_BOOL shouldBeVisible) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_Resizable( 
            /* [retval][out] */ VARIANT_BOOL *isResizable) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_Minimized( 
            /* [retval][out] */ VARIANT_BOOL *isMinimized) = 0;
        
        virtual /* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE put_Minimized( 
            /* [in] */ VARIANT_BOOL shouldBeMinimized) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_Maximizable( 
            /* [retval][out] */ VARIANT_BOOL *isMaximizable) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_Maximized( 
            /* [retval][out] */ VARIANT_BOOL *isMaximized) = 0;
        
        virtual /* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE put_Maximized( 
            /* [in] */ VARIANT_BOOL shouldBeMaximized) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_Zoomable( 
            /* [retval][out] */ VARIANT_BOOL *isZoomable) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_Zoomed( 
            /* [retval][out] */ VARIANT_BOOL *isZoomed) = 0;
        
        virtual /* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE put_Zoomed( 
            /* [in] */ VARIANT_BOOL shouldBeZoomed) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_Top( 
            /* [retval][out] */ long *top) = 0;
        
        virtual /* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE put_Top( 
            /* [in] */ long top) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_Left( 
            /* [retval][out] */ long *left) = 0;
        
        virtual /* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE put_Left( 
            /* [in] */ long left) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_Bottom( 
            /* [retval][out] */ long *bottom) = 0;
        
        virtual /* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE put_Bottom( 
            /* [in] */ long bottom) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_Right( 
            /* [retval][out] */ long *right) = 0;
        
        virtual /* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE put_Right( 
            /* [in] */ long right) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_Width( 
            /* [retval][out] */ long *width) = 0;
        
        virtual /* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE put_Width( 
            /* [in] */ long width) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_Height( 
            /* [retval][out] */ long *height) = 0;
        
        virtual /* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE put_Height( 
            /* [in] */ long height) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IITWindowVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IITWindow * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IITWindow * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IITWindow * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IITWindow * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IITWindow * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IITWindow * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IITWindow * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Name )( 
            IITWindow * This,
            /* [retval][out] */ BSTR *name);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Kind )( 
            IITWindow * This,
            /* [retval][out] */ ITWindowKind *kind);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Visible )( 
            IITWindow * This,
            /* [retval][out] */ VARIANT_BOOL *isVisible);
        
        /* [helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Visible )( 
            IITWindow * This,
            /* [in] */ VARIANT_BOOL shouldBeVisible);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Resizable )( 
            IITWindow * This,
            /* [retval][out] */ VARIANT_BOOL *isResizable);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Minimized )( 
            IITWindow * This,
            /* [retval][out] */ VARIANT_BOOL *isMinimized);
        
        /* [helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Minimized )( 
            IITWindow * This,
            /* [in] */ VARIANT_BOOL shouldBeMinimized);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Maximizable )( 
            IITWindow * This,
            /* [retval][out] */ VARIANT_BOOL *isMaximizable);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Maximized )( 
            IITWindow * This,
            /* [retval][out] */ VARIANT_BOOL *isMaximized);
        
        /* [helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Maximized )( 
            IITWindow * This,
            /* [in] */ VARIANT_BOOL shouldBeMaximized);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Zoomable )( 
            IITWindow * This,
            /* [retval][out] */ VARIANT_BOOL *isZoomable);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Zoomed )( 
            IITWindow * This,
            /* [retval][out] */ VARIANT_BOOL *isZoomed);
        
        /* [helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Zoomed )( 
            IITWindow * This,
            /* [in] */ VARIANT_BOOL shouldBeZoomed);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Top )( 
            IITWindow * This,
            /* [retval][out] */ long *top);
        
        /* [helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Top )( 
            IITWindow * This,
            /* [in] */ long top);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Left )( 
            IITWindow * This,
            /* [retval][out] */ long *left);
        
        /* [helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Left )( 
            IITWindow * This,
            /* [in] */ long left);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Bottom )( 
            IITWindow * This,
            /* [retval][out] */ long *bottom);
        
        /* [helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Bottom )( 
            IITWindow * This,
            /* [in] */ long bottom);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Right )( 
            IITWindow * This,
            /* [retval][out] */ long *right);
        
        /* [helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Right )( 
            IITWindow * This,
            /* [in] */ long right);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Width )( 
            IITWindow * This,
            /* [retval][out] */ long *width);
        
        /* [helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Width )( 
            IITWindow * This,
            /* [in] */ long width);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Height )( 
            IITWindow * This,
            /* [retval][out] */ long *height);
        
        /* [helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Height )( 
            IITWindow * This,
            /* [in] */ long height);
        
        END_INTERFACE
    } IITWindowVtbl;

    interface IITWindow
    {
        CONST_VTBL struct IITWindowVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IITWindow_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IITWindow_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IITWindow_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IITWindow_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IITWindow_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IITWindow_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IITWindow_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IITWindow_get_Name(This,name)	\
    (This)->lpVtbl -> get_Name(This,name)

#define IITWindow_get_Kind(This,kind)	\
    (This)->lpVtbl -> get_Kind(This,kind)

#define IITWindow_get_Visible(This,isVisible)	\
    (This)->lpVtbl -> get_Visible(This,isVisible)

#define IITWindow_put_Visible(This,shouldBeVisible)	\
    (This)->lpVtbl -> put_Visible(This,shouldBeVisible)

#define IITWindow_get_Resizable(This,isResizable)	\
    (This)->lpVtbl -> get_Resizable(This,isResizable)

#define IITWindow_get_Minimized(This,isMinimized)	\
    (This)->lpVtbl -> get_Minimized(This,isMinimized)

#define IITWindow_put_Minimized(This,shouldBeMinimized)	\
    (This)->lpVtbl -> put_Minimized(This,shouldBeMinimized)

#define IITWindow_get_Maximizable(This,isMaximizable)	\
    (This)->lpVtbl -> get_Maximizable(This,isMaximizable)

#define IITWindow_get_Maximized(This,isMaximized)	\
    (This)->lpVtbl -> get_Maximized(This,isMaximized)

#define IITWindow_put_Maximized(This,shouldBeMaximized)	\
    (This)->lpVtbl -> put_Maximized(This,shouldBeMaximized)

#define IITWindow_get_Zoomable(This,isZoomable)	\
    (This)->lpVtbl -> get_Zoomable(This,isZoomable)

#define IITWindow_get_Zoomed(This,isZoomed)	\
    (This)->lpVtbl -> get_Zoomed(This,isZoomed)

#define IITWindow_put_Zoomed(This,shouldBeZoomed)	\
    (This)->lpVtbl -> put_Zoomed(This,shouldBeZoomed)

#define IITWindow_get_Top(This,top)	\
    (This)->lpVtbl -> get_Top(This,top)

#define IITWindow_put_Top(This,top)	\
    (This)->lpVtbl -> put_Top(This,top)

#define IITWindow_get_Left(This,left)	\
    (This)->lpVtbl -> get_Left(This,left)

#define IITWindow_put_Left(This,left)	\
    (This)->lpVtbl -> put_Left(This,left)

#define IITWindow_get_Bottom(This,bottom)	\
    (This)->lpVtbl -> get_Bottom(This,bottom)

#define IITWindow_put_Bottom(This,bottom)	\
    (This)->lpVtbl -> put_Bottom(This,bottom)

#define IITWindow_get_Right(This,right)	\
    (This)->lpVtbl -> get_Right(This,right)

#define IITWindow_put_Right(This,right)	\
    (This)->lpVtbl -> put_Right(This,right)

#define IITWindow_get_Width(This,width)	\
    (This)->lpVtbl -> get_Width(This,width)

#define IITWindow_put_Width(This,width)	\
    (This)->lpVtbl -> put_Width(This,width)

#define IITWindow_get_Height(This,height)	\
    (This)->lpVtbl -> get_Height(This,height)

#define IITWindow_put_Height(This,height)	\
    (This)->lpVtbl -> put_Height(This,height)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IITWindow_get_Name_Proxy( 
    IITWindow * This,
    /* [retval][out] */ BSTR *name);


void __RPC_STUB IITWindow_get_Name_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IITWindow_get_Kind_Proxy( 
    IITWindow * This,
    /* [retval][out] */ ITWindowKind *kind);


void __RPC_STUB IITWindow_get_Kind_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IITWindow_get_Visible_Proxy( 
    IITWindow * This,
    /* [retval][out] */ VARIANT_BOOL *isVisible);


void __RPC_STUB IITWindow_get_Visible_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE IITWindow_put_Visible_Proxy( 
    IITWindow * This,
    /* [in] */ VARIANT_BOOL shouldBeVisible);


void __RPC_STUB IITWindow_put_Visible_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IITWindow_get_Resizable_Proxy( 
    IITWindow * This,
    /* [retval][out] */ VARIANT_BOOL *isResizable);


void __RPC_STUB IITWindow_get_Resizable_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IITWindow_get_Minimized_Proxy( 
    IITWindow * This,
    /* [retval][out] */ VARIANT_BOOL *isMinimized);


void __RPC_STUB IITWindow_get_Minimized_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE IITWindow_put_Minimized_Proxy( 
    IITWindow * This,
    /* [in] */ VARIANT_BOOL shouldBeMinimized);


void __RPC_STUB IITWindow_put_Minimized_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IITWindow_get_Maximizable_Proxy( 
    IITWindow * This,
    /* [retval][out] */ VARIANT_BOOL *isMaximizable);


void __RPC_STUB IITWindow_get_Maximizable_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IITWindow_get_Maximized_Proxy( 
    IITWindow * This,
    /* [retval][out] */ VARIANT_BOOL *isMaximized);


void __RPC_STUB IITWindow_get_Maximized_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE IITWindow_put_Maximized_Proxy( 
    IITWindow * This,
    /* [in] */ VARIANT_BOOL shouldBeMaximized);


void __RPC_STUB IITWindow_put_Maximized_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IITWindow_get_Zoomable_Proxy( 
    IITWindow * This,
    /* [retval][out] */ VARIANT_BOOL *isZoomable);


void __RPC_STUB IITWindow_get_Zoomable_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IITWindow_get_Zoomed_Proxy( 
    IITWindow * This,
    /* [retval][out] */ VARIANT_BOOL *isZoomed);


void __RPC_STUB IITWindow_get_Zoomed_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE IITWindow_put_Zoomed_Proxy( 
    IITWindow * This,
    /* [in] */ VARIANT_BOOL shouldBeZoomed);


void __RPC_STUB IITWindow_put_Zoomed_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IITWindow_get_Top_Proxy( 
    IITWindow * This,
    /* [retval][out] */ long *top);


void __RPC_STUB IITWindow_get_Top_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE IITWindow_put_Top_Proxy( 
    IITWindow * This,
    /* [in] */ long top);


void __RPC_STUB IITWindow_put_Top_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IITWindow_get_Left_Proxy( 
    IITWindow * This,
    /* [retval][out] */ long *left);


void __RPC_STUB IITWindow_get_Left_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE IITWindow_put_Left_Proxy( 
    IITWindow * This,
    /* [in] */ long left);


void __RPC_STUB IITWindow_put_Left_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IITWindow_get_Bottom_Proxy( 
    IITWindow * This,
    /* [retval][out] */ long *bottom);


void __RPC_STUB IITWindow_get_Bottom_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE IITWindow_put_Bottom_Proxy( 
    IITWindow * This,
    /* [in] */ long bottom);


void __RPC_STUB IITWindow_put_Bottom_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IITWindow_get_Right_Proxy( 
    IITWindow * This,
    /* [retval][out] */ long *right);


void __RPC_STUB IITWindow_get_Right_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE IITWindow_put_Right_Proxy( 
    IITWindow * This,
    /* [in] */ long right);


void __RPC_STUB IITWindow_put_Right_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IITWindow_get_Width_Proxy( 
    IITWindow * This,
    /* [retval][out] */ long *width);


void __RPC_STUB IITWindow_get_Width_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE IITWindow_put_Width_Proxy( 
    IITWindow * This,
    /* [in] */ long width);


void __RPC_STUB IITWindow_put_Width_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IITWindow_get_Height_Proxy( 
    IITWindow * This,
    /* [retval][out] */ long *height);


void __RPC_STUB IITWindow_get_Height_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE IITWindow_put_Height_Proxy( 
    IITWindow * This,
    /* [in] */ long height);


void __RPC_STUB IITWindow_put_Height_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IITWindow_INTERFACE_DEFINED__ */


#ifndef __IITBrowserWindow_INTERFACE_DEFINED__
#define __IITBrowserWindow_INTERFACE_DEFINED__

/* interface IITBrowserWindow */
/* [hidden][unique][helpstring][dual][uuid][object] */ 


EXTERN_C const IID IID_IITBrowserWindow;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("C999F455-C4D5-4aa4-8277-F99753699974")
    IITBrowserWindow : public IITWindow
    {
    public:
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_MiniPlayer( 
            /* [retval][out] */ VARIANT_BOOL *isMiniPlayer) = 0;
        
        virtual /* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE put_MiniPlayer( 
            /* [in] */ VARIANT_BOOL shouldBeMiniPlayer) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_SelectedTracks( 
            /* [retval][out] */ IITTrackCollection **iTrackCollection) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_SelectedPlaylist( 
            /* [retval][out] */ IITPlaylist **iPlaylist) = 0;
        
        virtual /* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE put_SelectedPlaylist( 
            /* [in] */ VARIANT *iPlaylist) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IITBrowserWindowVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IITBrowserWindow * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IITBrowserWindow * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IITBrowserWindow * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IITBrowserWindow * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IITBrowserWindow * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IITBrowserWindow * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IITBrowserWindow * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Name )( 
            IITBrowserWindow * This,
            /* [retval][out] */ BSTR *name);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Kind )( 
            IITBrowserWindow * This,
            /* [retval][out] */ ITWindowKind *kind);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Visible )( 
            IITBrowserWindow * This,
            /* [retval][out] */ VARIANT_BOOL *isVisible);
        
        /* [helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Visible )( 
            IITBrowserWindow * This,
            /* [in] */ VARIANT_BOOL shouldBeVisible);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Resizable )( 
            IITBrowserWindow * This,
            /* [retval][out] */ VARIANT_BOOL *isResizable);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Minimized )( 
            IITBrowserWindow * This,
            /* [retval][out] */ VARIANT_BOOL *isMinimized);
        
        /* [helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Minimized )( 
            IITBrowserWindow * This,
            /* [in] */ VARIANT_BOOL shouldBeMinimized);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Maximizable )( 
            IITBrowserWindow * This,
            /* [retval][out] */ VARIANT_BOOL *isMaximizable);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Maximized )( 
            IITBrowserWindow * This,
            /* [retval][out] */ VARIANT_BOOL *isMaximized);
        
        /* [helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Maximized )( 
            IITBrowserWindow * This,
            /* [in] */ VARIANT_BOOL shouldBeMaximized);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Zoomable )( 
            IITBrowserWindow * This,
            /* [retval][out] */ VARIANT_BOOL *isZoomable);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Zoomed )( 
            IITBrowserWindow * This,
            /* [retval][out] */ VARIANT_BOOL *isZoomed);
        
        /* [helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Zoomed )( 
            IITBrowserWindow * This,
            /* [in] */ VARIANT_BOOL shouldBeZoomed);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Top )( 
            IITBrowserWindow * This,
            /* [retval][out] */ long *top);
        
        /* [helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Top )( 
            IITBrowserWindow * This,
            /* [in] */ long top);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Left )( 
            IITBrowserWindow * This,
            /* [retval][out] */ long *left);
        
        /* [helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Left )( 
            IITBrowserWindow * This,
            /* [in] */ long left);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Bottom )( 
            IITBrowserWindow * This,
            /* [retval][out] */ long *bottom);
        
        /* [helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Bottom )( 
            IITBrowserWindow * This,
            /* [in] */ long bottom);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Right )( 
            IITBrowserWindow * This,
            /* [retval][out] */ long *right);
        
        /* [helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Right )( 
            IITBrowserWindow * This,
            /* [in] */ long right);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Width )( 
            IITBrowserWindow * This,
            /* [retval][out] */ long *width);
        
        /* [helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Width )( 
            IITBrowserWindow * This,
            /* [in] */ long width);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Height )( 
            IITBrowserWindow * This,
            /* [retval][out] */ long *height);
        
        /* [helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Height )( 
            IITBrowserWindow * This,
            /* [in] */ long height);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_MiniPlayer )( 
            IITBrowserWindow * This,
            /* [retval][out] */ VARIANT_BOOL *isMiniPlayer);
        
        /* [helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE *put_MiniPlayer )( 
            IITBrowserWindow * This,
            /* [in] */ VARIANT_BOOL shouldBeMiniPlayer);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_SelectedTracks )( 
            IITBrowserWindow * This,
            /* [retval][out] */ IITTrackCollection **iTrackCollection);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_SelectedPlaylist )( 
            IITBrowserWindow * This,
            /* [retval][out] */ IITPlaylist **iPlaylist);
        
        /* [helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE *put_SelectedPlaylist )( 
            IITBrowserWindow * This,
            /* [in] */ VARIANT *iPlaylist);
        
        END_INTERFACE
    } IITBrowserWindowVtbl;

    interface IITBrowserWindow
    {
        CONST_VTBL struct IITBrowserWindowVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IITBrowserWindow_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IITBrowserWindow_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IITBrowserWindow_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IITBrowserWindow_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IITBrowserWindow_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IITBrowserWindow_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IITBrowserWindow_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IITBrowserWindow_get_Name(This,name)	\
    (This)->lpVtbl -> get_Name(This,name)

#define IITBrowserWindow_get_Kind(This,kind)	\
    (This)->lpVtbl -> get_Kind(This,kind)

#define IITBrowserWindow_get_Visible(This,isVisible)	\
    (This)->lpVtbl -> get_Visible(This,isVisible)

#define IITBrowserWindow_put_Visible(This,shouldBeVisible)	\
    (This)->lpVtbl -> put_Visible(This,shouldBeVisible)

#define IITBrowserWindow_get_Resizable(This,isResizable)	\
    (This)->lpVtbl -> get_Resizable(This,isResizable)

#define IITBrowserWindow_get_Minimized(This,isMinimized)	\
    (This)->lpVtbl -> get_Minimized(This,isMinimized)

#define IITBrowserWindow_put_Minimized(This,shouldBeMinimized)	\
    (This)->lpVtbl -> put_Minimized(This,shouldBeMinimized)

#define IITBrowserWindow_get_Maximizable(This,isMaximizable)	\
    (This)->lpVtbl -> get_Maximizable(This,isMaximizable)

#define IITBrowserWindow_get_Maximized(This,isMaximized)	\
    (This)->lpVtbl -> get_Maximized(This,isMaximized)

#define IITBrowserWindow_put_Maximized(This,shouldBeMaximized)	\
    (This)->lpVtbl -> put_Maximized(This,shouldBeMaximized)

#define IITBrowserWindow_get_Zoomable(This,isZoomable)	\
    (This)->lpVtbl -> get_Zoomable(This,isZoomable)

#define IITBrowserWindow_get_Zoomed(This,isZoomed)	\
    (This)->lpVtbl -> get_Zoomed(This,isZoomed)

#define IITBrowserWindow_put_Zoomed(This,shouldBeZoomed)	\
    (This)->lpVtbl -> put_Zoomed(This,shouldBeZoomed)

#define IITBrowserWindow_get_Top(This,top)	\
    (This)->lpVtbl -> get_Top(This,top)

#define IITBrowserWindow_put_Top(This,top)	\
    (This)->lpVtbl -> put_Top(This,top)

#define IITBrowserWindow_get_Left(This,left)	\
    (This)->lpVtbl -> get_Left(This,left)

#define IITBrowserWindow_put_Left(This,left)	\
    (This)->lpVtbl -> put_Left(This,left)

#define IITBrowserWindow_get_Bottom(This,bottom)	\
    (This)->lpVtbl -> get_Bottom(This,bottom)

#define IITBrowserWindow_put_Bottom(This,bottom)	\
    (This)->lpVtbl -> put_Bottom(This,bottom)

#define IITBrowserWindow_get_Right(This,right)	\
    (This)->lpVtbl -> get_Right(This,right)

#define IITBrowserWindow_put_Right(This,right)	\
    (This)->lpVtbl -> put_Right(This,right)

#define IITBrowserWindow_get_Width(This,width)	\
    (This)->lpVtbl -> get_Width(This,width)

#define IITBrowserWindow_put_Width(This,width)	\
    (This)->lpVtbl -> put_Width(This,width)

#define IITBrowserWindow_get_Height(This,height)	\
    (This)->lpVtbl -> get_Height(This,height)

#define IITBrowserWindow_put_Height(This,height)	\
    (This)->lpVtbl -> put_Height(This,height)


#define IITBrowserWindow_get_MiniPlayer(This,isMiniPlayer)	\
    (This)->lpVtbl -> get_MiniPlayer(This,isMiniPlayer)

#define IITBrowserWindow_put_MiniPlayer(This,shouldBeMiniPlayer)	\
    (This)->lpVtbl -> put_MiniPlayer(This,shouldBeMiniPlayer)

#define IITBrowserWindow_get_SelectedTracks(This,iTrackCollection)	\
    (This)->lpVtbl -> get_SelectedTracks(This,iTrackCollection)

#define IITBrowserWindow_get_SelectedPlaylist(This,iPlaylist)	\
    (This)->lpVtbl -> get_SelectedPlaylist(This,iPlaylist)

#define IITBrowserWindow_put_SelectedPlaylist(This,iPlaylist)	\
    (This)->lpVtbl -> put_SelectedPlaylist(This,iPlaylist)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IITBrowserWindow_get_MiniPlayer_Proxy( 
    IITBrowserWindow * This,
    /* [retval][out] */ VARIANT_BOOL *isMiniPlayer);


void __RPC_STUB IITBrowserWindow_get_MiniPlayer_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE IITBrowserWindow_put_MiniPlayer_Proxy( 
    IITBrowserWindow * This,
    /* [in] */ VARIANT_BOOL shouldBeMiniPlayer);


void __RPC_STUB IITBrowserWindow_put_MiniPlayer_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IITBrowserWindow_get_SelectedTracks_Proxy( 
    IITBrowserWindow * This,
    /* [retval][out] */ IITTrackCollection **iTrackCollection);


void __RPC_STUB IITBrowserWindow_get_SelectedTracks_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IITBrowserWindow_get_SelectedPlaylist_Proxy( 
    IITBrowserWindow * This,
    /* [retval][out] */ IITPlaylist **iPlaylist);


void __RPC_STUB IITBrowserWindow_get_SelectedPlaylist_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE IITBrowserWindow_put_SelectedPlaylist_Proxy( 
    IITBrowserWindow * This,
    /* [in] */ VARIANT *iPlaylist);


void __RPC_STUB IITBrowserWindow_put_SelectedPlaylist_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IITBrowserWindow_INTERFACE_DEFINED__ */


#ifndef __IITWindowCollection_INTERFACE_DEFINED__
#define __IITWindowCollection_INTERFACE_DEFINED__

/* interface IITWindowCollection */
/* [unique][helpstring][dual][uuid][object] */ 


EXTERN_C const IID IID_IITWindowCollection;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("3D8DE381-6C0E-481f-A865-E2385F59FA43")
    IITWindowCollection : public IDispatch
    {
    public:
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_Count( 
            /* [retval][out] */ long *count) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Item( 
            /* [in] */ long index,
            /* [retval][out] */ IITWindow **iWindow) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_ItemByName( 
            /* [in] */ BSTR name,
            /* [retval][out] */ IITWindow **iWindow) = 0;
        
        virtual /* [helpstring][restricted][id][propget] */ HRESULT STDMETHODCALLTYPE get__NewEnum( 
            /* [retval][out] */ IUnknown **iEnumerator) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IITWindowCollectionVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IITWindowCollection * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IITWindowCollection * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IITWindowCollection * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IITWindowCollection * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IITWindowCollection * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IITWindowCollection * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IITWindowCollection * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Count )( 
            IITWindowCollection * This,
            /* [retval][out] */ long *count);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Item )( 
            IITWindowCollection * This,
            /* [in] */ long index,
            /* [retval][out] */ IITWindow **iWindow);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_ItemByName )( 
            IITWindowCollection * This,
            /* [in] */ BSTR name,
            /* [retval][out] */ IITWindow **iWindow);
        
        /* [helpstring][restricted][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get__NewEnum )( 
            IITWindowCollection * This,
            /* [retval][out] */ IUnknown **iEnumerator);
        
        END_INTERFACE
    } IITWindowCollectionVtbl;

    interface IITWindowCollection
    {
        CONST_VTBL struct IITWindowCollectionVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IITWindowCollection_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IITWindowCollection_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IITWindowCollection_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IITWindowCollection_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IITWindowCollection_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IITWindowCollection_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IITWindowCollection_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IITWindowCollection_get_Count(This,count)	\
    (This)->lpVtbl -> get_Count(This,count)

#define IITWindowCollection_get_Item(This,index,iWindow)	\
    (This)->lpVtbl -> get_Item(This,index,iWindow)

#define IITWindowCollection_get_ItemByName(This,name,iWindow)	\
    (This)->lpVtbl -> get_ItemByName(This,name,iWindow)

#define IITWindowCollection_get__NewEnum(This,iEnumerator)	\
    (This)->lpVtbl -> get__NewEnum(This,iEnumerator)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IITWindowCollection_get_Count_Proxy( 
    IITWindowCollection * This,
    /* [retval][out] */ long *count);


void __RPC_STUB IITWindowCollection_get_Count_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IITWindowCollection_get_Item_Proxy( 
    IITWindowCollection * This,
    /* [in] */ long index,
    /* [retval][out] */ IITWindow **iWindow);


void __RPC_STUB IITWindowCollection_get_Item_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IITWindowCollection_get_ItemByName_Proxy( 
    IITWindowCollection * This,
    /* [in] */ BSTR name,
    /* [retval][out] */ IITWindow **iWindow);


void __RPC_STUB IITWindowCollection_get_ItemByName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][restricted][id][propget] */ HRESULT STDMETHODCALLTYPE IITWindowCollection_get__NewEnum_Proxy( 
    IITWindowCollection * This,
    /* [retval][out] */ IUnknown **iEnumerator);


void __RPC_STUB IITWindowCollection_get__NewEnum_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IITWindowCollection_INTERFACE_DEFINED__ */


#ifndef __IiTunes_INTERFACE_DEFINED__
#define __IiTunes_INTERFACE_DEFINED__

/* interface IiTunes */
/* [hidden][unique][helpstring][dual][uuid][object] */ 




EXTERN_C const IID IID_IiTunes;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("9DD6680B-3EDC-40db-A771-E6FE4832E34A")
    IiTunes : public IDispatch
    {
    public:
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE BackTrack( void) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE FastForward( void) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE NextTrack( void) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE Pause( void) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE Play( void) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE PlayFile( 
            /* [in] */ BSTR filePath) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE PlayPause( void) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE PreviousTrack( void) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE Resume( void) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE Rewind( void) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE Stop( void) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE ConvertFile( 
            /* [in] */ BSTR filePath,
            /* [retval][out] */ IITOperationStatus **iStatus) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE ConvertFiles( 
            /* [in] */ VARIANT *filePaths,
            /* [retval][out] */ IITOperationStatus **iStatus) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE ConvertTrack( 
            /* [in] */ VARIANT *iTrackToConvert,
            /* [retval][out] */ IITOperationStatus **iStatus) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE ConvertTracks( 
            /* [in] */ VARIANT *iTracksToConvert,
            /* [retval][out] */ IITOperationStatus **iStatus) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE CheckVersion( 
            /* [in] */ long majorVersion,
            /* [in] */ long minorVersion,
            /* [retval][out] */ VARIANT_BOOL *isCompatible) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE GetITObjectByID( 
            /* [in] */ long sourceID,
            /* [in] */ long playlistID,
            /* [in] */ long trackID,
            /* [in] */ long databaseID,
            /* [retval][out] */ IITObject **iObject) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE CreatePlaylist( 
            /* [in] */ BSTR playlistName,
            /* [retval][out] */ IITPlaylist **iPlaylist) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE OpenURL( 
            /* [in] */ BSTR url) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE GotoMusicStoreHomePage( void) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE UpdateIPod( void) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE Authorize( 
            /* [in] */ long numElems,
            /* [size_is][in] */ VARIANT data[  ],
            /* [size_is][in] */ BSTR names[  ]) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE Quit( void) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_Sources( 
            /* [retval][out] */ IITSourceCollection **iSourceCollection) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_Encoders( 
            /* [retval][out] */ IITEncoderCollection **iEncoderCollection) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_EQPresets( 
            /* [retval][out] */ IITEQPresetCollection **iEQPresetCollection) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_Visuals( 
            /* [retval][out] */ IITVisualCollection **iVisualCollection) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_Windows( 
            /* [retval][out] */ IITWindowCollection **iWindowCollection) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_SoundVolume( 
            /* [retval][out] */ long *volume) = 0;
        
        virtual /* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE put_SoundVolume( 
            /* [in] */ long volume) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_Mute( 
            /* [retval][out] */ VARIANT_BOOL *isMuted) = 0;
        
        virtual /* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE put_Mute( 
            /* [in] */ VARIANT_BOOL shouldMute) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_PlayerState( 
            /* [retval][out] */ ITPlayerState *playerState) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_PlayerPosition( 
            /* [retval][out] */ long *playerPos) = 0;
        
        virtual /* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE put_PlayerPosition( 
            /* [in] */ long playerPos) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_CurrentEncoder( 
            /* [retval][out] */ IITEncoder **iEncoder) = 0;
        
        virtual /* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE put_CurrentEncoder( 
            /* [in] */ IITEncoder *iEncoder) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_VisualsEnabled( 
            /* [retval][out] */ VARIANT_BOOL *isEnabled) = 0;
        
        virtual /* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE put_VisualsEnabled( 
            /* [in] */ VARIANT_BOOL shouldEnable) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_FullScreenVisuals( 
            /* [retval][out] */ VARIANT_BOOL *isFullScreen) = 0;
        
        virtual /* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE put_FullScreenVisuals( 
            /* [in] */ VARIANT_BOOL shouldUseFullScreen) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_VisualSize( 
            /* [retval][out] */ ITVisualSize *visualSize) = 0;
        
        virtual /* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE put_VisualSize( 
            /* [in] */ ITVisualSize visualSize) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_CurrentVisual( 
            /* [retval][out] */ IITVisual **iVisual) = 0;
        
        virtual /* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE put_CurrentVisual( 
            /* [in] */ IITVisual *iVisual) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_EQEnabled( 
            /* [retval][out] */ VARIANT_BOOL *isEnabled) = 0;
        
        virtual /* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE put_EQEnabled( 
            /* [in] */ VARIANT_BOOL shouldEnable) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_CurrentEQPreset( 
            /* [retval][out] */ IITEQPreset **iEQPreset) = 0;
        
        virtual /* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE put_CurrentEQPreset( 
            /* [in] */ IITEQPreset *iEQPreset) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_CurrentStreamTitle( 
            /* [retval][out] */ BSTR *streamTitle) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_CurrentStreamURL( 
            /* [retval][out] */ BSTR *streamURL) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_BrowserWindow( 
            /* [retval][out] */ IITBrowserWindow **iBrowserWindow) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_EQWindow( 
            /* [retval][out] */ IITWindow **iEQWindow) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_LibrarySource( 
            /* [retval][out] */ IITSource **iLibrarySource) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_LibraryPlaylist( 
            /* [retval][out] */ IITLibraryPlaylist **iLibraryPlaylist) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_CurrentTrack( 
            /* [retval][out] */ IITTrack **iTrack) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_CurrentPlaylist( 
            /* [retval][out] */ IITPlaylist **iPlaylist) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_SelectedTracks( 
            /* [retval][out] */ IITTrackCollection **iTrackCollection) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_Version( 
            /* [retval][out] */ BSTR *version) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE SetOptions( 
            /* [in] */ long options) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE ConvertFile2( 
            /* [in] */ BSTR filePath,
            /* [retval][out] */ IITConvertOperationStatus **iStatus) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE ConvertFiles2( 
            /* [in] */ VARIANT *filePaths,
            /* [retval][out] */ IITConvertOperationStatus **iStatus) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE ConvertTrack2( 
            /* [in] */ VARIANT *iTrackToConvert,
            /* [retval][out] */ IITConvertOperationStatus **iStatus) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE ConvertTracks2( 
            /* [in] */ VARIANT *iTracksToConvert,
            /* [retval][out] */ IITConvertOperationStatus **iStatus) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_AppCommandMessageProcessingEnabled( 
            /* [retval][out] */ VARIANT_BOOL *isEnabled) = 0;
        
        virtual /* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE put_AppCommandMessageProcessingEnabled( 
            /* [in] */ VARIANT_BOOL shouldEnable) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_ForceToForegroundOnDialog( 
            /* [retval][out] */ VARIANT_BOOL *forceToForegroundOnDialog) = 0;
        
        virtual /* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE put_ForceToForegroundOnDialog( 
            /* [in] */ VARIANT_BOOL forceToForegroundOnDialog) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE CreateEQPreset( 
            /* [in] */ BSTR eqPresetName,
            /* [retval][out] */ IITEQPreset **iEQPreset) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE CreatePlaylistInSource( 
            /* [in] */ BSTR playlistName,
            /* [in] */ VARIANT *iSource,
            /* [retval][out] */ IITPlaylist **iPlaylist) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE GetPlayerButtonsState( 
            /* [out] */ VARIANT_BOOL *previousEnabled,
            /* [out] */ ITPlayButtonState *playPauseStopState,
            /* [out] */ VARIANT_BOOL *nextEnabled) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE PlayerButtonClicked( 
            /* [in] */ ITPlayerButton playerButton,
            /* [in] */ long playerButtonModifierKeys) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_CanSetShuffle( 
            /* [in] */ VARIANT *iPlaylist,
            /* [retval][out] */ VARIANT_BOOL *canSetShuffle) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_CanSetSongRepeat( 
            /* [in] */ VARIANT *iPlaylist,
            /* [retval][out] */ VARIANT_BOOL *canSetSongRepeat) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_ConvertOperationStatus( 
            /* [retval][out] */ IITConvertOperationStatus **iStatus) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE SubscribeToPodcast( 
            /* [in] */ BSTR url) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE UpdatePodcastFeeds( void) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE CreateFolder( 
            /* [in] */ BSTR folderName,
            /* [retval][out] */ IITPlaylist **iFolder) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE CreateFolderInSource( 
            /* [in] */ BSTR folderName,
            /* [in] */ VARIANT *iSource,
            /* [retval][out] */ IITPlaylist **iFolder) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_SoundVolumeControlEnabled( 
            /* [retval][out] */ VARIANT_BOOL *isEnabled) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_LibraryXMLPath( 
            /* [retval][out] */ BSTR *filePath) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_ITObjectPersistentIDHigh( 
            /* [in] */ VARIANT *iObject,
            /* [retval][out] */ long *highID) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_ITObjectPersistentIDLow( 
            /* [in] */ VARIANT *iObject,
            /* [retval][out] */ long *lowID) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE GetITObjectPersistentIDs( 
            /* [in] */ VARIANT *iObject,
            /* [out] */ long *highID,
            /* [out] */ long *lowID) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IiTunesVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IiTunes * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IiTunes * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IiTunes * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IiTunes * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IiTunes * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IiTunes * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IiTunes * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *BackTrack )( 
            IiTunes * This);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *FastForward )( 
            IiTunes * This);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *NextTrack )( 
            IiTunes * This);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *Pause )( 
            IiTunes * This);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *Play )( 
            IiTunes * This);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *PlayFile )( 
            IiTunes * This,
            /* [in] */ BSTR filePath);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *PlayPause )( 
            IiTunes * This);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *PreviousTrack )( 
            IiTunes * This);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *Resume )( 
            IiTunes * This);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *Rewind )( 
            IiTunes * This);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *Stop )( 
            IiTunes * This);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *ConvertFile )( 
            IiTunes * This,
            /* [in] */ BSTR filePath,
            /* [retval][out] */ IITOperationStatus **iStatus);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *ConvertFiles )( 
            IiTunes * This,
            /* [in] */ VARIANT *filePaths,
            /* [retval][out] */ IITOperationStatus **iStatus);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *ConvertTrack )( 
            IiTunes * This,
            /* [in] */ VARIANT *iTrackToConvert,
            /* [retval][out] */ IITOperationStatus **iStatus);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *ConvertTracks )( 
            IiTunes * This,
            /* [in] */ VARIANT *iTracksToConvert,
            /* [retval][out] */ IITOperationStatus **iStatus);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *CheckVersion )( 
            IiTunes * This,
            /* [in] */ long majorVersion,
            /* [in] */ long minorVersion,
            /* [retval][out] */ VARIANT_BOOL *isCompatible);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *GetITObjectByID )( 
            IiTunes * This,
            /* [in] */ long sourceID,
            /* [in] */ long playlistID,
            /* [in] */ long trackID,
            /* [in] */ long databaseID,
            /* [retval][out] */ IITObject **iObject);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *CreatePlaylist )( 
            IiTunes * This,
            /* [in] */ BSTR playlistName,
            /* [retval][out] */ IITPlaylist **iPlaylist);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *OpenURL )( 
            IiTunes * This,
            /* [in] */ BSTR url);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *GotoMusicStoreHomePage )( 
            IiTunes * This);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *UpdateIPod )( 
            IiTunes * This);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *Authorize )( 
            IiTunes * This,
            /* [in] */ long numElems,
            /* [size_is][in] */ VARIANT data[  ],
            /* [size_is][in] */ BSTR names[  ]);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *Quit )( 
            IiTunes * This);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Sources )( 
            IiTunes * This,
            /* [retval][out] */ IITSourceCollection **iSourceCollection);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Encoders )( 
            IiTunes * This,
            /* [retval][out] */ IITEncoderCollection **iEncoderCollection);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_EQPresets )( 
            IiTunes * This,
            /* [retval][out] */ IITEQPresetCollection **iEQPresetCollection);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Visuals )( 
            IiTunes * This,
            /* [retval][out] */ IITVisualCollection **iVisualCollection);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Windows )( 
            IiTunes * This,
            /* [retval][out] */ IITWindowCollection **iWindowCollection);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_SoundVolume )( 
            IiTunes * This,
            /* [retval][out] */ long *volume);
        
        /* [helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE *put_SoundVolume )( 
            IiTunes * This,
            /* [in] */ long volume);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Mute )( 
            IiTunes * This,
            /* [retval][out] */ VARIANT_BOOL *isMuted);
        
        /* [helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Mute )( 
            IiTunes * This,
            /* [in] */ VARIANT_BOOL shouldMute);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_PlayerState )( 
            IiTunes * This,
            /* [retval][out] */ ITPlayerState *playerState);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_PlayerPosition )( 
            IiTunes * This,
            /* [retval][out] */ long *playerPos);
        
        /* [helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE *put_PlayerPosition )( 
            IiTunes * This,
            /* [in] */ long playerPos);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_CurrentEncoder )( 
            IiTunes * This,
            /* [retval][out] */ IITEncoder **iEncoder);
        
        /* [helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE *put_CurrentEncoder )( 
            IiTunes * This,
            /* [in] */ IITEncoder *iEncoder);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_VisualsEnabled )( 
            IiTunes * This,
            /* [retval][out] */ VARIANT_BOOL *isEnabled);
        
        /* [helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE *put_VisualsEnabled )( 
            IiTunes * This,
            /* [in] */ VARIANT_BOOL shouldEnable);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_FullScreenVisuals )( 
            IiTunes * This,
            /* [retval][out] */ VARIANT_BOOL *isFullScreen);
        
        /* [helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE *put_FullScreenVisuals )( 
            IiTunes * This,
            /* [in] */ VARIANT_BOOL shouldUseFullScreen);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_VisualSize )( 
            IiTunes * This,
            /* [retval][out] */ ITVisualSize *visualSize);
        
        /* [helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE *put_VisualSize )( 
            IiTunes * This,
            /* [in] */ ITVisualSize visualSize);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_CurrentVisual )( 
            IiTunes * This,
            /* [retval][out] */ IITVisual **iVisual);
        
        /* [helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE *put_CurrentVisual )( 
            IiTunes * This,
            /* [in] */ IITVisual *iVisual);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_EQEnabled )( 
            IiTunes * This,
            /* [retval][out] */ VARIANT_BOOL *isEnabled);
        
        /* [helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE *put_EQEnabled )( 
            IiTunes * This,
            /* [in] */ VARIANT_BOOL shouldEnable);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_CurrentEQPreset )( 
            IiTunes * This,
            /* [retval][out] */ IITEQPreset **iEQPreset);
        
        /* [helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE *put_CurrentEQPreset )( 
            IiTunes * This,
            /* [in] */ IITEQPreset *iEQPreset);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_CurrentStreamTitle )( 
            IiTunes * This,
            /* [retval][out] */ BSTR *streamTitle);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_CurrentStreamURL )( 
            IiTunes * This,
            /* [retval][out] */ BSTR *streamURL);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_BrowserWindow )( 
            IiTunes * This,
            /* [retval][out] */ IITBrowserWindow **iBrowserWindow);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_EQWindow )( 
            IiTunes * This,
            /* [retval][out] */ IITWindow **iEQWindow);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_LibrarySource )( 
            IiTunes * This,
            /* [retval][out] */ IITSource **iLibrarySource);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_LibraryPlaylist )( 
            IiTunes * This,
            /* [retval][out] */ IITLibraryPlaylist **iLibraryPlaylist);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_CurrentTrack )( 
            IiTunes * This,
            /* [retval][out] */ IITTrack **iTrack);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_CurrentPlaylist )( 
            IiTunes * This,
            /* [retval][out] */ IITPlaylist **iPlaylist);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_SelectedTracks )( 
            IiTunes * This,
            /* [retval][out] */ IITTrackCollection **iTrackCollection);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Version )( 
            IiTunes * This,
            /* [retval][out] */ BSTR *version);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *SetOptions )( 
            IiTunes * This,
            /* [in] */ long options);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *ConvertFile2 )( 
            IiTunes * This,
            /* [in] */ BSTR filePath,
            /* [retval][out] */ IITConvertOperationStatus **iStatus);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *ConvertFiles2 )( 
            IiTunes * This,
            /* [in] */ VARIANT *filePaths,
            /* [retval][out] */ IITConvertOperationStatus **iStatus);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *ConvertTrack2 )( 
            IiTunes * This,
            /* [in] */ VARIANT *iTrackToConvert,
            /* [retval][out] */ IITConvertOperationStatus **iStatus);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *ConvertTracks2 )( 
            IiTunes * This,
            /* [in] */ VARIANT *iTracksToConvert,
            /* [retval][out] */ IITConvertOperationStatus **iStatus);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_AppCommandMessageProcessingEnabled )( 
            IiTunes * This,
            /* [retval][out] */ VARIANT_BOOL *isEnabled);
        
        /* [helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE *put_AppCommandMessageProcessingEnabled )( 
            IiTunes * This,
            /* [in] */ VARIANT_BOOL shouldEnable);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_ForceToForegroundOnDialog )( 
            IiTunes * This,
            /* [retval][out] */ VARIANT_BOOL *forceToForegroundOnDialog);
        
        /* [helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE *put_ForceToForegroundOnDialog )( 
            IiTunes * This,
            /* [in] */ VARIANT_BOOL forceToForegroundOnDialog);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *CreateEQPreset )( 
            IiTunes * This,
            /* [in] */ BSTR eqPresetName,
            /* [retval][out] */ IITEQPreset **iEQPreset);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *CreatePlaylistInSource )( 
            IiTunes * This,
            /* [in] */ BSTR playlistName,
            /* [in] */ VARIANT *iSource,
            /* [retval][out] */ IITPlaylist **iPlaylist);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *GetPlayerButtonsState )( 
            IiTunes * This,
            /* [out] */ VARIANT_BOOL *previousEnabled,
            /* [out] */ ITPlayButtonState *playPauseStopState,
            /* [out] */ VARIANT_BOOL *nextEnabled);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *PlayerButtonClicked )( 
            IiTunes * This,
            /* [in] */ ITPlayerButton playerButton,
            /* [in] */ long playerButtonModifierKeys);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_CanSetShuffle )( 
            IiTunes * This,
            /* [in] */ VARIANT *iPlaylist,
            /* [retval][out] */ VARIANT_BOOL *canSetShuffle);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_CanSetSongRepeat )( 
            IiTunes * This,
            /* [in] */ VARIANT *iPlaylist,
            /* [retval][out] */ VARIANT_BOOL *canSetSongRepeat);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_ConvertOperationStatus )( 
            IiTunes * This,
            /* [retval][out] */ IITConvertOperationStatus **iStatus);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *SubscribeToPodcast )( 
            IiTunes * This,
            /* [in] */ BSTR url);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *UpdatePodcastFeeds )( 
            IiTunes * This);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *CreateFolder )( 
            IiTunes * This,
            /* [in] */ BSTR folderName,
            /* [retval][out] */ IITPlaylist **iFolder);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *CreateFolderInSource )( 
            IiTunes * This,
            /* [in] */ BSTR folderName,
            /* [in] */ VARIANT *iSource,
            /* [retval][out] */ IITPlaylist **iFolder);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_SoundVolumeControlEnabled )( 
            IiTunes * This,
            /* [retval][out] */ VARIANT_BOOL *isEnabled);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_LibraryXMLPath )( 
            IiTunes * This,
            /* [retval][out] */ BSTR *filePath);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_ITObjectPersistentIDHigh )( 
            IiTunes * This,
            /* [in] */ VARIANT *iObject,
            /* [retval][out] */ long *highID);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_ITObjectPersistentIDLow )( 
            IiTunes * This,
            /* [in] */ VARIANT *iObject,
            /* [retval][out] */ long *lowID);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *GetITObjectPersistentIDs )( 
            IiTunes * This,
            /* [in] */ VARIANT *iObject,
            /* [out] */ long *highID,
            /* [out] */ long *lowID);
        
        END_INTERFACE
    } IiTunesVtbl;

    interface IiTunes
    {
        CONST_VTBL struct IiTunesVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IiTunes_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IiTunes_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IiTunes_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IiTunes_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IiTunes_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IiTunes_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IiTunes_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IiTunes_BackTrack(This)	\
    (This)->lpVtbl -> BackTrack(This)

#define IiTunes_FastForward(This)	\
    (This)->lpVtbl -> FastForward(This)

#define IiTunes_NextTrack(This)	\
    (This)->lpVtbl -> NextTrack(This)

#define IiTunes_Pause(This)	\
    (This)->lpVtbl -> Pause(This)

#define IiTunes_Play(This)	\
    (This)->lpVtbl -> Play(This)

#define IiTunes_PlayFile(This,filePath)	\
    (This)->lpVtbl -> PlayFile(This,filePath)

#define IiTunes_PlayPause(This)	\
    (This)->lpVtbl -> PlayPause(This)

#define IiTunes_PreviousTrack(This)	\
    (This)->lpVtbl -> PreviousTrack(This)

#define IiTunes_Resume(This)	\
    (This)->lpVtbl -> Resume(This)

#define IiTunes_Rewind(This)	\
    (This)->lpVtbl -> Rewind(This)

#define IiTunes_Stop(This)	\
    (This)->lpVtbl -> Stop(This)

#define IiTunes_ConvertFile(This,filePath,iStatus)	\
    (This)->lpVtbl -> ConvertFile(This,filePath,iStatus)

#define IiTunes_ConvertFiles(This,filePaths,iStatus)	\
    (This)->lpVtbl -> ConvertFiles(This,filePaths,iStatus)

#define IiTunes_ConvertTrack(This,iTrackToConvert,iStatus)	\
    (This)->lpVtbl -> ConvertTrack(This,iTrackToConvert,iStatus)

#define IiTunes_ConvertTracks(This,iTracksToConvert,iStatus)	\
    (This)->lpVtbl -> ConvertTracks(This,iTracksToConvert,iStatus)

#define IiTunes_CheckVersion(This,majorVersion,minorVersion,isCompatible)	\
    (This)->lpVtbl -> CheckVersion(This,majorVersion,minorVersion,isCompatible)

#define IiTunes_GetITObjectByID(This,sourceID,playlistID,trackID,databaseID,iObject)	\
    (This)->lpVtbl -> GetITObjectByID(This,sourceID,playlistID,trackID,databaseID,iObject)

#define IiTunes_CreatePlaylist(This,playlistName,iPlaylist)	\
    (This)->lpVtbl -> CreatePlaylist(This,playlistName,iPlaylist)

#define IiTunes_OpenURL(This,url)	\
    (This)->lpVtbl -> OpenURL(This,url)

#define IiTunes_GotoMusicStoreHomePage(This)	\
    (This)->lpVtbl -> GotoMusicStoreHomePage(This)

#define IiTunes_UpdateIPod(This)	\
    (This)->lpVtbl -> UpdateIPod(This)

#define IiTunes_Authorize(This,numElems,data,names)	\
    (This)->lpVtbl -> Authorize(This,numElems,data,names)

#define IiTunes_Quit(This)	\
    (This)->lpVtbl -> Quit(This)

#define IiTunes_get_Sources(This,iSourceCollection)	\
    (This)->lpVtbl -> get_Sources(This,iSourceCollection)

#define IiTunes_get_Encoders(This,iEncoderCollection)	\
    (This)->lpVtbl -> get_Encoders(This,iEncoderCollection)

#define IiTunes_get_EQPresets(This,iEQPresetCollection)	\
    (This)->lpVtbl -> get_EQPresets(This,iEQPresetCollection)

#define IiTunes_get_Visuals(This,iVisualCollection)	\
    (This)->lpVtbl -> get_Visuals(This,iVisualCollection)

#define IiTunes_get_Windows(This,iWindowCollection)	\
    (This)->lpVtbl -> get_Windows(This,iWindowCollection)

#define IiTunes_get_SoundVolume(This,volume)	\
    (This)->lpVtbl -> get_SoundVolume(This,volume)

#define IiTunes_put_SoundVolume(This,volume)	\
    (This)->lpVtbl -> put_SoundVolume(This,volume)

#define IiTunes_get_Mute(This,isMuted)	\
    (This)->lpVtbl -> get_Mute(This,isMuted)

#define IiTunes_put_Mute(This,shouldMute)	\
    (This)->lpVtbl -> put_Mute(This,shouldMute)

#define IiTunes_get_PlayerState(This,playerState)	\
    (This)->lpVtbl -> get_PlayerState(This,playerState)

#define IiTunes_get_PlayerPosition(This,playerPos)	\
    (This)->lpVtbl -> get_PlayerPosition(This,playerPos)

#define IiTunes_put_PlayerPosition(This,playerPos)	\
    (This)->lpVtbl -> put_PlayerPosition(This,playerPos)

#define IiTunes_get_CurrentEncoder(This,iEncoder)	\
    (This)->lpVtbl -> get_CurrentEncoder(This,iEncoder)

#define IiTunes_put_CurrentEncoder(This,iEncoder)	\
    (This)->lpVtbl -> put_CurrentEncoder(This,iEncoder)

#define IiTunes_get_VisualsEnabled(This,isEnabled)	\
    (This)->lpVtbl -> get_VisualsEnabled(This,isEnabled)

#define IiTunes_put_VisualsEnabled(This,shouldEnable)	\
    (This)->lpVtbl -> put_VisualsEnabled(This,shouldEnable)

#define IiTunes_get_FullScreenVisuals(This,isFullScreen)	\
    (This)->lpVtbl -> get_FullScreenVisuals(This,isFullScreen)

#define IiTunes_put_FullScreenVisuals(This,shouldUseFullScreen)	\
    (This)->lpVtbl -> put_FullScreenVisuals(This,shouldUseFullScreen)

#define IiTunes_get_VisualSize(This,visualSize)	\
    (This)->lpVtbl -> get_VisualSize(This,visualSize)

#define IiTunes_put_VisualSize(This,visualSize)	\
    (This)->lpVtbl -> put_VisualSize(This,visualSize)

#define IiTunes_get_CurrentVisual(This,iVisual)	\
    (This)->lpVtbl -> get_CurrentVisual(This,iVisual)

#define IiTunes_put_CurrentVisual(This,iVisual)	\
    (This)->lpVtbl -> put_CurrentVisual(This,iVisual)

#define IiTunes_get_EQEnabled(This,isEnabled)	\
    (This)->lpVtbl -> get_EQEnabled(This,isEnabled)

#define IiTunes_put_EQEnabled(This,shouldEnable)	\
    (This)->lpVtbl -> put_EQEnabled(This,shouldEnable)

#define IiTunes_get_CurrentEQPreset(This,iEQPreset)	\
    (This)->lpVtbl -> get_CurrentEQPreset(This,iEQPreset)

#define IiTunes_put_CurrentEQPreset(This,iEQPreset)	\
    (This)->lpVtbl -> put_CurrentEQPreset(This,iEQPreset)

#define IiTunes_get_CurrentStreamTitle(This,streamTitle)	\
    (This)->lpVtbl -> get_CurrentStreamTitle(This,streamTitle)

#define IiTunes_get_CurrentStreamURL(This,streamURL)	\
    (This)->lpVtbl -> get_CurrentStreamURL(This,streamURL)

#define IiTunes_get_BrowserWindow(This,iBrowserWindow)	\
    (This)->lpVtbl -> get_BrowserWindow(This,iBrowserWindow)

#define IiTunes_get_EQWindow(This,iEQWindow)	\
    (This)->lpVtbl -> get_EQWindow(This,iEQWindow)

#define IiTunes_get_LibrarySource(This,iLibrarySource)	\
    (This)->lpVtbl -> get_LibrarySource(This,iLibrarySource)

#define IiTunes_get_LibraryPlaylist(This,iLibraryPlaylist)	\
    (This)->lpVtbl -> get_LibraryPlaylist(This,iLibraryPlaylist)

#define IiTunes_get_CurrentTrack(This,iTrack)	\
    (This)->lpVtbl -> get_CurrentTrack(This,iTrack)

#define IiTunes_get_CurrentPlaylist(This,iPlaylist)	\
    (This)->lpVtbl -> get_CurrentPlaylist(This,iPlaylist)

#define IiTunes_get_SelectedTracks(This,iTrackCollection)	\
    (This)->lpVtbl -> get_SelectedTracks(This,iTrackCollection)

#define IiTunes_get_Version(This,version)	\
    (This)->lpVtbl -> get_Version(This,version)

#define IiTunes_SetOptions(This,options)	\
    (This)->lpVtbl -> SetOptions(This,options)

#define IiTunes_ConvertFile2(This,filePath,iStatus)	\
    (This)->lpVtbl -> ConvertFile2(This,filePath,iStatus)

#define IiTunes_ConvertFiles2(This,filePaths,iStatus)	\
    (This)->lpVtbl -> ConvertFiles2(This,filePaths,iStatus)

#define IiTunes_ConvertTrack2(This,iTrackToConvert,iStatus)	\
    (This)->lpVtbl -> ConvertTrack2(This,iTrackToConvert,iStatus)

#define IiTunes_ConvertTracks2(This,iTracksToConvert,iStatus)	\
    (This)->lpVtbl -> ConvertTracks2(This,iTracksToConvert,iStatus)

#define IiTunes_get_AppCommandMessageProcessingEnabled(This,isEnabled)	\
    (This)->lpVtbl -> get_AppCommandMessageProcessingEnabled(This,isEnabled)

#define IiTunes_put_AppCommandMessageProcessingEnabled(This,shouldEnable)	\
    (This)->lpVtbl -> put_AppCommandMessageProcessingEnabled(This,shouldEnable)

#define IiTunes_get_ForceToForegroundOnDialog(This,forceToForegroundOnDialog)	\
    (This)->lpVtbl -> get_ForceToForegroundOnDialog(This,forceToForegroundOnDialog)

#define IiTunes_put_ForceToForegroundOnDialog(This,forceToForegroundOnDialog)	\
    (This)->lpVtbl -> put_ForceToForegroundOnDialog(This,forceToForegroundOnDialog)

#define IiTunes_CreateEQPreset(This,eqPresetName,iEQPreset)	\
    (This)->lpVtbl -> CreateEQPreset(This,eqPresetName,iEQPreset)

#define IiTunes_CreatePlaylistInSource(This,playlistName,iSource,iPlaylist)	\
    (This)->lpVtbl -> CreatePlaylistInSource(This,playlistName,iSource,iPlaylist)

#define IiTunes_GetPlayerButtonsState(This,previousEnabled,playPauseStopState,nextEnabled)	\
    (This)->lpVtbl -> GetPlayerButtonsState(This,previousEnabled,playPauseStopState,nextEnabled)

#define IiTunes_PlayerButtonClicked(This,playerButton,playerButtonModifierKeys)	\
    (This)->lpVtbl -> PlayerButtonClicked(This,playerButton,playerButtonModifierKeys)

#define IiTunes_get_CanSetShuffle(This,iPlaylist,canSetShuffle)	\
    (This)->lpVtbl -> get_CanSetShuffle(This,iPlaylist,canSetShuffle)

#define IiTunes_get_CanSetSongRepeat(This,iPlaylist,canSetSongRepeat)	\
    (This)->lpVtbl -> get_CanSetSongRepeat(This,iPlaylist,canSetSongRepeat)

#define IiTunes_get_ConvertOperationStatus(This,iStatus)	\
    (This)->lpVtbl -> get_ConvertOperationStatus(This,iStatus)

#define IiTunes_SubscribeToPodcast(This,url)	\
    (This)->lpVtbl -> SubscribeToPodcast(This,url)

#define IiTunes_UpdatePodcastFeeds(This)	\
    (This)->lpVtbl -> UpdatePodcastFeeds(This)

#define IiTunes_CreateFolder(This,folderName,iFolder)	\
    (This)->lpVtbl -> CreateFolder(This,folderName,iFolder)

#define IiTunes_CreateFolderInSource(This,folderName,iSource,iFolder)	\
    (This)->lpVtbl -> CreateFolderInSource(This,folderName,iSource,iFolder)

#define IiTunes_get_SoundVolumeControlEnabled(This,isEnabled)	\
    (This)->lpVtbl -> get_SoundVolumeControlEnabled(This,isEnabled)

#define IiTunes_get_LibraryXMLPath(This,filePath)	\
    (This)->lpVtbl -> get_LibraryXMLPath(This,filePath)

#define IiTunes_get_ITObjectPersistentIDHigh(This,iObject,highID)	\
    (This)->lpVtbl -> get_ITObjectPersistentIDHigh(This,iObject,highID)

#define IiTunes_get_ITObjectPersistentIDLow(This,iObject,lowID)	\
    (This)->lpVtbl -> get_ITObjectPersistentIDLow(This,iObject,lowID)

#define IiTunes_GetITObjectPersistentIDs(This,iObject,highID,lowID)	\
    (This)->lpVtbl -> GetITObjectPersistentIDs(This,iObject,highID,lowID)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring] */ HRESULT STDMETHODCALLTYPE IiTunes_BackTrack_Proxy( 
    IiTunes * This);


void __RPC_STUB IiTunes_BackTrack_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IiTunes_FastForward_Proxy( 
    IiTunes * This);


void __RPC_STUB IiTunes_FastForward_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IiTunes_NextTrack_Proxy( 
    IiTunes * This);


void __RPC_STUB IiTunes_NextTrack_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IiTunes_Pause_Proxy( 
    IiTunes * This);


void __RPC_STUB IiTunes_Pause_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IiTunes_Play_Proxy( 
    IiTunes * This);


void __RPC_STUB IiTunes_Play_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IiTunes_PlayFile_Proxy( 
    IiTunes * This,
    /* [in] */ BSTR filePath);


void __RPC_STUB IiTunes_PlayFile_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IiTunes_PlayPause_Proxy( 
    IiTunes * This);


void __RPC_STUB IiTunes_PlayPause_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IiTunes_PreviousTrack_Proxy( 
    IiTunes * This);


void __RPC_STUB IiTunes_PreviousTrack_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IiTunes_Resume_Proxy( 
    IiTunes * This);


void __RPC_STUB IiTunes_Resume_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IiTunes_Rewind_Proxy( 
    IiTunes * This);


void __RPC_STUB IiTunes_Rewind_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IiTunes_Stop_Proxy( 
    IiTunes * This);


void __RPC_STUB IiTunes_Stop_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IiTunes_ConvertFile_Proxy( 
    IiTunes * This,
    /* [in] */ BSTR filePath,
    /* [retval][out] */ IITOperationStatus **iStatus);


void __RPC_STUB IiTunes_ConvertFile_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IiTunes_ConvertFiles_Proxy( 
    IiTunes * This,
    /* [in] */ VARIANT *filePaths,
    /* [retval][out] */ IITOperationStatus **iStatus);


void __RPC_STUB IiTunes_ConvertFiles_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IiTunes_ConvertTrack_Proxy( 
    IiTunes * This,
    /* [in] */ VARIANT *iTrackToConvert,
    /* [retval][out] */ IITOperationStatus **iStatus);


void __RPC_STUB IiTunes_ConvertTrack_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IiTunes_ConvertTracks_Proxy( 
    IiTunes * This,
    /* [in] */ VARIANT *iTracksToConvert,
    /* [retval][out] */ IITOperationStatus **iStatus);


void __RPC_STUB IiTunes_ConvertTracks_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IiTunes_CheckVersion_Proxy( 
    IiTunes * This,
    /* [in] */ long majorVersion,
    /* [in] */ long minorVersion,
    /* [retval][out] */ VARIANT_BOOL *isCompatible);


void __RPC_STUB IiTunes_CheckVersion_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IiTunes_GetITObjectByID_Proxy( 
    IiTunes * This,
    /* [in] */ long sourceID,
    /* [in] */ long playlistID,
    /* [in] */ long trackID,
    /* [in] */ long databaseID,
    /* [retval][out] */ IITObject **iObject);


void __RPC_STUB IiTunes_GetITObjectByID_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IiTunes_CreatePlaylist_Proxy( 
    IiTunes * This,
    /* [in] */ BSTR playlistName,
    /* [retval][out] */ IITPlaylist **iPlaylist);


void __RPC_STUB IiTunes_CreatePlaylist_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IiTunes_OpenURL_Proxy( 
    IiTunes * This,
    /* [in] */ BSTR url);


void __RPC_STUB IiTunes_OpenURL_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IiTunes_GotoMusicStoreHomePage_Proxy( 
    IiTunes * This);


void __RPC_STUB IiTunes_GotoMusicStoreHomePage_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IiTunes_UpdateIPod_Proxy( 
    IiTunes * This);


void __RPC_STUB IiTunes_UpdateIPod_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IiTunes_Authorize_Proxy( 
    IiTunes * This,
    /* [in] */ long numElems,
    /* [size_is][in] */ VARIANT data[  ],
    /* [size_is][in] */ BSTR names[  ]);


void __RPC_STUB IiTunes_Authorize_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IiTunes_Quit_Proxy( 
    IiTunes * This);


void __RPC_STUB IiTunes_Quit_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IiTunes_get_Sources_Proxy( 
    IiTunes * This,
    /* [retval][out] */ IITSourceCollection **iSourceCollection);


void __RPC_STUB IiTunes_get_Sources_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IiTunes_get_Encoders_Proxy( 
    IiTunes * This,
    /* [retval][out] */ IITEncoderCollection **iEncoderCollection);


void __RPC_STUB IiTunes_get_Encoders_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IiTunes_get_EQPresets_Proxy( 
    IiTunes * This,
    /* [retval][out] */ IITEQPresetCollection **iEQPresetCollection);


void __RPC_STUB IiTunes_get_EQPresets_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IiTunes_get_Visuals_Proxy( 
    IiTunes * This,
    /* [retval][out] */ IITVisualCollection **iVisualCollection);


void __RPC_STUB IiTunes_get_Visuals_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IiTunes_get_Windows_Proxy( 
    IiTunes * This,
    /* [retval][out] */ IITWindowCollection **iWindowCollection);


void __RPC_STUB IiTunes_get_Windows_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IiTunes_get_SoundVolume_Proxy( 
    IiTunes * This,
    /* [retval][out] */ long *volume);


void __RPC_STUB IiTunes_get_SoundVolume_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE IiTunes_put_SoundVolume_Proxy( 
    IiTunes * This,
    /* [in] */ long volume);


void __RPC_STUB IiTunes_put_SoundVolume_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IiTunes_get_Mute_Proxy( 
    IiTunes * This,
    /* [retval][out] */ VARIANT_BOOL *isMuted);


void __RPC_STUB IiTunes_get_Mute_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE IiTunes_put_Mute_Proxy( 
    IiTunes * This,
    /* [in] */ VARIANT_BOOL shouldMute);


void __RPC_STUB IiTunes_put_Mute_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IiTunes_get_PlayerState_Proxy( 
    IiTunes * This,
    /* [retval][out] */ ITPlayerState *playerState);


void __RPC_STUB IiTunes_get_PlayerState_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IiTunes_get_PlayerPosition_Proxy( 
    IiTunes * This,
    /* [retval][out] */ long *playerPos);


void __RPC_STUB IiTunes_get_PlayerPosition_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE IiTunes_put_PlayerPosition_Proxy( 
    IiTunes * This,
    /* [in] */ long playerPos);


void __RPC_STUB IiTunes_put_PlayerPosition_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IiTunes_get_CurrentEncoder_Proxy( 
    IiTunes * This,
    /* [retval][out] */ IITEncoder **iEncoder);


void __RPC_STUB IiTunes_get_CurrentEncoder_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE IiTunes_put_CurrentEncoder_Proxy( 
    IiTunes * This,
    /* [in] */ IITEncoder *iEncoder);


void __RPC_STUB IiTunes_put_CurrentEncoder_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IiTunes_get_VisualsEnabled_Proxy( 
    IiTunes * This,
    /* [retval][out] */ VARIANT_BOOL *isEnabled);


void __RPC_STUB IiTunes_get_VisualsEnabled_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE IiTunes_put_VisualsEnabled_Proxy( 
    IiTunes * This,
    /* [in] */ VARIANT_BOOL shouldEnable);


void __RPC_STUB IiTunes_put_VisualsEnabled_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IiTunes_get_FullScreenVisuals_Proxy( 
    IiTunes * This,
    /* [retval][out] */ VARIANT_BOOL *isFullScreen);


void __RPC_STUB IiTunes_get_FullScreenVisuals_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE IiTunes_put_FullScreenVisuals_Proxy( 
    IiTunes * This,
    /* [in] */ VARIANT_BOOL shouldUseFullScreen);


void __RPC_STUB IiTunes_put_FullScreenVisuals_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IiTunes_get_VisualSize_Proxy( 
    IiTunes * This,
    /* [retval][out] */ ITVisualSize *visualSize);


void __RPC_STUB IiTunes_get_VisualSize_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE IiTunes_put_VisualSize_Proxy( 
    IiTunes * This,
    /* [in] */ ITVisualSize visualSize);


void __RPC_STUB IiTunes_put_VisualSize_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IiTunes_get_CurrentVisual_Proxy( 
    IiTunes * This,
    /* [retval][out] */ IITVisual **iVisual);


void __RPC_STUB IiTunes_get_CurrentVisual_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE IiTunes_put_CurrentVisual_Proxy( 
    IiTunes * This,
    /* [in] */ IITVisual *iVisual);


void __RPC_STUB IiTunes_put_CurrentVisual_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IiTunes_get_EQEnabled_Proxy( 
    IiTunes * This,
    /* [retval][out] */ VARIANT_BOOL *isEnabled);


void __RPC_STUB IiTunes_get_EQEnabled_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE IiTunes_put_EQEnabled_Proxy( 
    IiTunes * This,
    /* [in] */ VARIANT_BOOL shouldEnable);


void __RPC_STUB IiTunes_put_EQEnabled_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IiTunes_get_CurrentEQPreset_Proxy( 
    IiTunes * This,
    /* [retval][out] */ IITEQPreset **iEQPreset);


void __RPC_STUB IiTunes_get_CurrentEQPreset_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE IiTunes_put_CurrentEQPreset_Proxy( 
    IiTunes * This,
    /* [in] */ IITEQPreset *iEQPreset);


void __RPC_STUB IiTunes_put_CurrentEQPreset_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IiTunes_get_CurrentStreamTitle_Proxy( 
    IiTunes * This,
    /* [retval][out] */ BSTR *streamTitle);


void __RPC_STUB IiTunes_get_CurrentStreamTitle_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IiTunes_get_CurrentStreamURL_Proxy( 
    IiTunes * This,
    /* [retval][out] */ BSTR *streamURL);


void __RPC_STUB IiTunes_get_CurrentStreamURL_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IiTunes_get_BrowserWindow_Proxy( 
    IiTunes * This,
    /* [retval][out] */ IITBrowserWindow **iBrowserWindow);


void __RPC_STUB IiTunes_get_BrowserWindow_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IiTunes_get_EQWindow_Proxy( 
    IiTunes * This,
    /* [retval][out] */ IITWindow **iEQWindow);


void __RPC_STUB IiTunes_get_EQWindow_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IiTunes_get_LibrarySource_Proxy( 
    IiTunes * This,
    /* [retval][out] */ IITSource **iLibrarySource);


void __RPC_STUB IiTunes_get_LibrarySource_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IiTunes_get_LibraryPlaylist_Proxy( 
    IiTunes * This,
    /* [retval][out] */ IITLibraryPlaylist **iLibraryPlaylist);


void __RPC_STUB IiTunes_get_LibraryPlaylist_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IiTunes_get_CurrentTrack_Proxy( 
    IiTunes * This,
    /* [retval][out] */ IITTrack **iTrack);


void __RPC_STUB IiTunes_get_CurrentTrack_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IiTunes_get_CurrentPlaylist_Proxy( 
    IiTunes * This,
    /* [retval][out] */ IITPlaylist **iPlaylist);


void __RPC_STUB IiTunes_get_CurrentPlaylist_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IiTunes_get_SelectedTracks_Proxy( 
    IiTunes * This,
    /* [retval][out] */ IITTrackCollection **iTrackCollection);


void __RPC_STUB IiTunes_get_SelectedTracks_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IiTunes_get_Version_Proxy( 
    IiTunes * This,
    /* [retval][out] */ BSTR *version);


void __RPC_STUB IiTunes_get_Version_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IiTunes_SetOptions_Proxy( 
    IiTunes * This,
    /* [in] */ long options);


void __RPC_STUB IiTunes_SetOptions_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IiTunes_ConvertFile2_Proxy( 
    IiTunes * This,
    /* [in] */ BSTR filePath,
    /* [retval][out] */ IITConvertOperationStatus **iStatus);


void __RPC_STUB IiTunes_ConvertFile2_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IiTunes_ConvertFiles2_Proxy( 
    IiTunes * This,
    /* [in] */ VARIANT *filePaths,
    /* [retval][out] */ IITConvertOperationStatus **iStatus);


void __RPC_STUB IiTunes_ConvertFiles2_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IiTunes_ConvertTrack2_Proxy( 
    IiTunes * This,
    /* [in] */ VARIANT *iTrackToConvert,
    /* [retval][out] */ IITConvertOperationStatus **iStatus);


void __RPC_STUB IiTunes_ConvertTrack2_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IiTunes_ConvertTracks2_Proxy( 
    IiTunes * This,
    /* [in] */ VARIANT *iTracksToConvert,
    /* [retval][out] */ IITConvertOperationStatus **iStatus);


void __RPC_STUB IiTunes_ConvertTracks2_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IiTunes_get_AppCommandMessageProcessingEnabled_Proxy( 
    IiTunes * This,
    /* [retval][out] */ VARIANT_BOOL *isEnabled);


void __RPC_STUB IiTunes_get_AppCommandMessageProcessingEnabled_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE IiTunes_put_AppCommandMessageProcessingEnabled_Proxy( 
    IiTunes * This,
    /* [in] */ VARIANT_BOOL shouldEnable);


void __RPC_STUB IiTunes_put_AppCommandMessageProcessingEnabled_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IiTunes_get_ForceToForegroundOnDialog_Proxy( 
    IiTunes * This,
    /* [retval][out] */ VARIANT_BOOL *forceToForegroundOnDialog);


void __RPC_STUB IiTunes_get_ForceToForegroundOnDialog_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE IiTunes_put_ForceToForegroundOnDialog_Proxy( 
    IiTunes * This,
    /* [in] */ VARIANT_BOOL forceToForegroundOnDialog);


void __RPC_STUB IiTunes_put_ForceToForegroundOnDialog_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IiTunes_CreateEQPreset_Proxy( 
    IiTunes * This,
    /* [in] */ BSTR eqPresetName,
    /* [retval][out] */ IITEQPreset **iEQPreset);


void __RPC_STUB IiTunes_CreateEQPreset_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IiTunes_CreatePlaylistInSource_Proxy( 
    IiTunes * This,
    /* [in] */ BSTR playlistName,
    /* [in] */ VARIANT *iSource,
    /* [retval][out] */ IITPlaylist **iPlaylist);


void __RPC_STUB IiTunes_CreatePlaylistInSource_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IiTunes_GetPlayerButtonsState_Proxy( 
    IiTunes * This,
    /* [out] */ VARIANT_BOOL *previousEnabled,
    /* [out] */ ITPlayButtonState *playPauseStopState,
    /* [out] */ VARIANT_BOOL *nextEnabled);


void __RPC_STUB IiTunes_GetPlayerButtonsState_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IiTunes_PlayerButtonClicked_Proxy( 
    IiTunes * This,
    /* [in] */ ITPlayerButton playerButton,
    /* [in] */ long playerButtonModifierKeys);


void __RPC_STUB IiTunes_PlayerButtonClicked_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IiTunes_get_CanSetShuffle_Proxy( 
    IiTunes * This,
    /* [in] */ VARIANT *iPlaylist,
    /* [retval][out] */ VARIANT_BOOL *canSetShuffle);


void __RPC_STUB IiTunes_get_CanSetShuffle_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IiTunes_get_CanSetSongRepeat_Proxy( 
    IiTunes * This,
    /* [in] */ VARIANT *iPlaylist,
    /* [retval][out] */ VARIANT_BOOL *canSetSongRepeat);


void __RPC_STUB IiTunes_get_CanSetSongRepeat_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IiTunes_get_ConvertOperationStatus_Proxy( 
    IiTunes * This,
    /* [retval][out] */ IITConvertOperationStatus **iStatus);


void __RPC_STUB IiTunes_get_ConvertOperationStatus_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IiTunes_SubscribeToPodcast_Proxy( 
    IiTunes * This,
    /* [in] */ BSTR url);


void __RPC_STUB IiTunes_SubscribeToPodcast_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IiTunes_UpdatePodcastFeeds_Proxy( 
    IiTunes * This);


void __RPC_STUB IiTunes_UpdatePodcastFeeds_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IiTunes_CreateFolder_Proxy( 
    IiTunes * This,
    /* [in] */ BSTR folderName,
    /* [retval][out] */ IITPlaylist **iFolder);


void __RPC_STUB IiTunes_CreateFolder_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IiTunes_CreateFolderInSource_Proxy( 
    IiTunes * This,
    /* [in] */ BSTR folderName,
    /* [in] */ VARIANT *iSource,
    /* [retval][out] */ IITPlaylist **iFolder);


void __RPC_STUB IiTunes_CreateFolderInSource_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IiTunes_get_SoundVolumeControlEnabled_Proxy( 
    IiTunes * This,
    /* [retval][out] */ VARIANT_BOOL *isEnabled);


void __RPC_STUB IiTunes_get_SoundVolumeControlEnabled_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IiTunes_get_LibraryXMLPath_Proxy( 
    IiTunes * This,
    /* [retval][out] */ BSTR *filePath);


void __RPC_STUB IiTunes_get_LibraryXMLPath_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IiTunes_get_ITObjectPersistentIDHigh_Proxy( 
    IiTunes * This,
    /* [in] */ VARIANT *iObject,
    /* [retval][out] */ long *highID);


void __RPC_STUB IiTunes_get_ITObjectPersistentIDHigh_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IiTunes_get_ITObjectPersistentIDLow_Proxy( 
    IiTunes * This,
    /* [in] */ VARIANT *iObject,
    /* [retval][out] */ long *lowID);


void __RPC_STUB IiTunes_get_ITObjectPersistentIDLow_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IiTunes_GetITObjectPersistentIDs_Proxy( 
    IiTunes * This,
    /* [in] */ VARIANT *iObject,
    /* [out] */ long *highID,
    /* [out] */ long *lowID);


void __RPC_STUB IiTunes_GetITObjectPersistentIDs_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IiTunes_INTERFACE_DEFINED__ */


#ifndef ___IiTunesEvents_DISPINTERFACE_DEFINED__
#define ___IiTunesEvents_DISPINTERFACE_DEFINED__

/* dispinterface _IiTunesEvents */
/* [helpstring][uuid] */ 


EXTERN_C const IID DIID__IiTunesEvents;

#if defined(__cplusplus) && !defined(CINTERFACE)

    MIDL_INTERFACE("5846EB78-317E-4b6f-B0C3-11EE8C8FEEF2")
    _IiTunesEvents : public IDispatch
    {
    };
    
#else 	/* C style interface */

    typedef struct _IiTunesEventsVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            _IiTunesEvents * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            _IiTunesEvents * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            _IiTunesEvents * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            _IiTunesEvents * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            _IiTunesEvents * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            _IiTunesEvents * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            _IiTunesEvents * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        END_INTERFACE
    } _IiTunesEventsVtbl;

    interface _IiTunesEvents
    {
        CONST_VTBL struct _IiTunesEventsVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define _IiTunesEvents_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define _IiTunesEvents_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define _IiTunesEvents_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define _IiTunesEvents_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define _IiTunesEvents_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define _IiTunesEvents_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define _IiTunesEvents_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)

#endif /* COBJMACROS */


#endif 	/* C style interface */


#endif 	/* ___IiTunesEvents_DISPINTERFACE_DEFINED__ */


#ifndef ___IITConvertOperationStatusEvents_DISPINTERFACE_DEFINED__
#define ___IITConvertOperationStatusEvents_DISPINTERFACE_DEFINED__

/* dispinterface _IITConvertOperationStatusEvents */
/* [helpstring][uuid] */ 


EXTERN_C const IID DIID__IITConvertOperationStatusEvents;

#if defined(__cplusplus) && !defined(CINTERFACE)

    MIDL_INTERFACE("5C47A705-8E8A-45a1-9EED-71C993F0BF60")
    _IITConvertOperationStatusEvents : public IDispatch
    {
    };
    
#else 	/* C style interface */

    typedef struct _IITConvertOperationStatusEventsVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            _IITConvertOperationStatusEvents * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            _IITConvertOperationStatusEvents * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            _IITConvertOperationStatusEvents * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            _IITConvertOperationStatusEvents * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            _IITConvertOperationStatusEvents * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            _IITConvertOperationStatusEvents * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            _IITConvertOperationStatusEvents * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        END_INTERFACE
    } _IITConvertOperationStatusEventsVtbl;

    interface _IITConvertOperationStatusEvents
    {
        CONST_VTBL struct _IITConvertOperationStatusEventsVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define _IITConvertOperationStatusEvents_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define _IITConvertOperationStatusEvents_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define _IITConvertOperationStatusEvents_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define _IITConvertOperationStatusEvents_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define _IITConvertOperationStatusEvents_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define _IITConvertOperationStatusEvents_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define _IITConvertOperationStatusEvents_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)

#endif /* COBJMACROS */


#endif 	/* C style interface */


#endif 	/* ___IITConvertOperationStatusEvents_DISPINTERFACE_DEFINED__ */


EXTERN_C const CLSID CLSID_iTunesApp;

#ifdef __cplusplus

class DECLSPEC_UUID("DC0C2640-1415-4644-875C-6F4D769839BA")
iTunesApp;
#endif

EXTERN_C const CLSID CLSID_iTunesConvertOperationStatus;

#ifdef __cplusplus

class DECLSPEC_UUID("D06596AD-C900-41b2-BC68-1B486450FC56")
iTunesConvertOperationStatus;
#endif

#ifndef __IITArtwork_INTERFACE_DEFINED__
#define __IITArtwork_INTERFACE_DEFINED__

/* interface IITArtwork */
/* [hidden][unique][helpstring][dual][uuid][object] */ 


EXTERN_C const IID IID_IITArtwork;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("D0A6C1F8-BF3D-4cd8-AC47-FE32BDD17257")
    IITArtwork : public IDispatch
    {
    public:
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE Delete( void) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE SetArtworkFromFile( 
            /* [in] */ BSTR filePath) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE SaveArtworkToFile( 
            /* [in] */ BSTR filePath) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_Format( 
            /* [retval][out] */ ITArtworkFormat *format) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_IsDownloadedArtwork( 
            /* [retval][out] */ VARIANT_BOOL *isDownloadedArtwork) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_Description( 
            /* [retval][out] */ BSTR *description) = 0;
        
        virtual /* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE put_Description( 
            /* [in] */ BSTR description) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IITArtworkVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IITArtwork * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IITArtwork * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IITArtwork * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IITArtwork * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IITArtwork * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IITArtwork * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IITArtwork * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *Delete )( 
            IITArtwork * This);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *SetArtworkFromFile )( 
            IITArtwork * This,
            /* [in] */ BSTR filePath);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *SaveArtworkToFile )( 
            IITArtwork * This,
            /* [in] */ BSTR filePath);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Format )( 
            IITArtwork * This,
            /* [retval][out] */ ITArtworkFormat *format);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_IsDownloadedArtwork )( 
            IITArtwork * This,
            /* [retval][out] */ VARIANT_BOOL *isDownloadedArtwork);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Description )( 
            IITArtwork * This,
            /* [retval][out] */ BSTR *description);
        
        /* [helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Description )( 
            IITArtwork * This,
            /* [in] */ BSTR description);
        
        END_INTERFACE
    } IITArtworkVtbl;

    interface IITArtwork
    {
        CONST_VTBL struct IITArtworkVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IITArtwork_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IITArtwork_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IITArtwork_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IITArtwork_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IITArtwork_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IITArtwork_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IITArtwork_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IITArtwork_Delete(This)	\
    (This)->lpVtbl -> Delete(This)

#define IITArtwork_SetArtworkFromFile(This,filePath)	\
    (This)->lpVtbl -> SetArtworkFromFile(This,filePath)

#define IITArtwork_SaveArtworkToFile(This,filePath)	\
    (This)->lpVtbl -> SaveArtworkToFile(This,filePath)

#define IITArtwork_get_Format(This,format)	\
    (This)->lpVtbl -> get_Format(This,format)

#define IITArtwork_get_IsDownloadedArtwork(This,isDownloadedArtwork)	\
    (This)->lpVtbl -> get_IsDownloadedArtwork(This,isDownloadedArtwork)

#define IITArtwork_get_Description(This,description)	\
    (This)->lpVtbl -> get_Description(This,description)

#define IITArtwork_put_Description(This,description)	\
    (This)->lpVtbl -> put_Description(This,description)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring] */ HRESULT STDMETHODCALLTYPE IITArtwork_Delete_Proxy( 
    IITArtwork * This);


void __RPC_STUB IITArtwork_Delete_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IITArtwork_SetArtworkFromFile_Proxy( 
    IITArtwork * This,
    /* [in] */ BSTR filePath);


void __RPC_STUB IITArtwork_SetArtworkFromFile_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IITArtwork_SaveArtworkToFile_Proxy( 
    IITArtwork * This,
    /* [in] */ BSTR filePath);


void __RPC_STUB IITArtwork_SaveArtworkToFile_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IITArtwork_get_Format_Proxy( 
    IITArtwork * This,
    /* [retval][out] */ ITArtworkFormat *format);


void __RPC_STUB IITArtwork_get_Format_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IITArtwork_get_IsDownloadedArtwork_Proxy( 
    IITArtwork * This,
    /* [retval][out] */ VARIANT_BOOL *isDownloadedArtwork);


void __RPC_STUB IITArtwork_get_IsDownloadedArtwork_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IITArtwork_get_Description_Proxy( 
    IITArtwork * This,
    /* [retval][out] */ BSTR *description);


void __RPC_STUB IITArtwork_get_Description_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE IITArtwork_put_Description_Proxy( 
    IITArtwork * This,
    /* [in] */ BSTR description);


void __RPC_STUB IITArtwork_put_Description_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IITArtwork_INTERFACE_DEFINED__ */


#ifndef __IITArtworkCollection_INTERFACE_DEFINED__
#define __IITArtworkCollection_INTERFACE_DEFINED__

/* interface IITArtworkCollection */
/* [unique][helpstring][dual][uuid][object] */ 


EXTERN_C const IID IID_IITArtworkCollection;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("BF2742D7-418C-4858-9AF9-2981B062D23E")
    IITArtworkCollection : public IDispatch
    {
    public:
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_Count( 
            /* [retval][out] */ long *count) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Item( 
            /* [in] */ long index,
            /* [retval][out] */ IITArtwork **iArtwork) = 0;
        
        virtual /* [helpstring][restricted][id][propget] */ HRESULT STDMETHODCALLTYPE get__NewEnum( 
            /* [retval][out] */ IUnknown **iEnumerator) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IITArtworkCollectionVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IITArtworkCollection * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IITArtworkCollection * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IITArtworkCollection * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IITArtworkCollection * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IITArtworkCollection * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IITArtworkCollection * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IITArtworkCollection * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Count )( 
            IITArtworkCollection * This,
            /* [retval][out] */ long *count);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Item )( 
            IITArtworkCollection * This,
            /* [in] */ long index,
            /* [retval][out] */ IITArtwork **iArtwork);
        
        /* [helpstring][restricted][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get__NewEnum )( 
            IITArtworkCollection * This,
            /* [retval][out] */ IUnknown **iEnumerator);
        
        END_INTERFACE
    } IITArtworkCollectionVtbl;

    interface IITArtworkCollection
    {
        CONST_VTBL struct IITArtworkCollectionVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IITArtworkCollection_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IITArtworkCollection_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IITArtworkCollection_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IITArtworkCollection_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IITArtworkCollection_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IITArtworkCollection_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IITArtworkCollection_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IITArtworkCollection_get_Count(This,count)	\
    (This)->lpVtbl -> get_Count(This,count)

#define IITArtworkCollection_get_Item(This,index,iArtwork)	\
    (This)->lpVtbl -> get_Item(This,index,iArtwork)

#define IITArtworkCollection_get__NewEnum(This,iEnumerator)	\
    (This)->lpVtbl -> get__NewEnum(This,iEnumerator)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IITArtworkCollection_get_Count_Proxy( 
    IITArtworkCollection * This,
    /* [retval][out] */ long *count);


void __RPC_STUB IITArtworkCollection_get_Count_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IITArtworkCollection_get_Item_Proxy( 
    IITArtworkCollection * This,
    /* [in] */ long index,
    /* [retval][out] */ IITArtwork **iArtwork);


void __RPC_STUB IITArtworkCollection_get_Item_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][restricted][id][propget] */ HRESULT STDMETHODCALLTYPE IITArtworkCollection_get__NewEnum_Proxy( 
    IITArtworkCollection * This,
    /* [retval][out] */ IUnknown **iEnumerator);


void __RPC_STUB IITArtworkCollection_get__NewEnum_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IITArtworkCollection_INTERFACE_DEFINED__ */


#ifndef __IITURLTrack_INTERFACE_DEFINED__
#define __IITURLTrack_INTERFACE_DEFINED__

/* interface IITURLTrack */
/* [hidden][unique][helpstring][dual][uuid][object] */ 


EXTERN_C const IID IID_IITURLTrack;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("1116E3B5-29FD-4393-A7BD-454E5E327900")
    IITURLTrack : public IITTrack
    {
    public:
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_URL( 
            /* [retval][out] */ BSTR *url) = 0;
        
        virtual /* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE put_URL( 
            /* [in] */ BSTR url) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_Podcast( 
            /* [retval][out] */ VARIANT_BOOL *isPodcast) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE UpdatePodcastFeed( void) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE DownloadPodcastEpisode( void) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_Category( 
            /* [retval][out] */ BSTR *category) = 0;
        
        virtual /* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE put_Category( 
            /* [in] */ BSTR category) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_Description( 
            /* [retval][out] */ BSTR *description) = 0;
        
        virtual /* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE put_Description( 
            /* [in] */ BSTR description) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_LongDescription( 
            /* [retval][out] */ BSTR *longDescription) = 0;
        
        virtual /* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE put_LongDescription( 
            /* [in] */ BSTR longDescription) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE Reveal( void) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_AlbumRating( 
            /* [retval][out] */ long *rating) = 0;
        
        virtual /* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE put_AlbumRating( 
            /* [in] */ long rating) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_AlbumRatingKind( 
            /* [retval][out] */ ITRatingKind *ratingKind) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_RatingKind( 
            /* [retval][out] */ ITRatingKind *ratingKind) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_Playlists( 
            /* [retval][out] */ IITPlaylistCollection **iPlaylistCollection) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IITURLTrackVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IITURLTrack * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IITURLTrack * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IITURLTrack * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IITURLTrack * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IITURLTrack * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IITURLTrack * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IITURLTrack * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *GetITObjectIDs )( 
            IITURLTrack * This,
            /* [out] */ long *sourceID,
            /* [out] */ long *playlistID,
            /* [out] */ long *trackID,
            /* [out] */ long *databaseID);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Name )( 
            IITURLTrack * This,
            /* [retval][out] */ BSTR *name);
        
        /* [helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Name )( 
            IITURLTrack * This,
            /* [in] */ BSTR name);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Index )( 
            IITURLTrack * This,
            /* [retval][out] */ long *index);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_SourceID )( 
            IITURLTrack * This,
            /* [retval][out] */ long *sourceID);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_PlaylistID )( 
            IITURLTrack * This,
            /* [retval][out] */ long *playlistID);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_TrackID )( 
            IITURLTrack * This,
            /* [retval][out] */ long *trackID);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_TrackDatabaseID )( 
            IITURLTrack * This,
            /* [retval][out] */ long *databaseID);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *Delete )( 
            IITURLTrack * This);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *Play )( 
            IITURLTrack * This);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *AddArtworkFromFile )( 
            IITURLTrack * This,
            /* [in] */ BSTR filePath,
            /* [retval][out] */ IITArtwork **iArtwork);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Kind )( 
            IITURLTrack * This,
            /* [retval][out] */ ITTrackKind *kind);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Playlist )( 
            IITURLTrack * This,
            /* [retval][out] */ IITPlaylist **iPlaylist);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Album )( 
            IITURLTrack * This,
            /* [retval][out] */ BSTR *album);
        
        /* [helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Album )( 
            IITURLTrack * This,
            /* [in] */ BSTR album);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Artist )( 
            IITURLTrack * This,
            /* [retval][out] */ BSTR *artist);
        
        /* [helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Artist )( 
            IITURLTrack * This,
            /* [in] */ BSTR artist);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_BitRate )( 
            IITURLTrack * This,
            /* [retval][out] */ long *bitrate);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_BPM )( 
            IITURLTrack * This,
            /* [retval][out] */ long *beatsPerMinute);
        
        /* [helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE *put_BPM )( 
            IITURLTrack * This,
            /* [in] */ long beatsPerMinute);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Comment )( 
            IITURLTrack * This,
            /* [retval][out] */ BSTR *comment);
        
        /* [helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Comment )( 
            IITURLTrack * This,
            /* [in] */ BSTR comment);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Compilation )( 
            IITURLTrack * This,
            /* [retval][out] */ VARIANT_BOOL *isCompilation);
        
        /* [helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Compilation )( 
            IITURLTrack * This,
            /* [in] */ VARIANT_BOOL shouldBeCompilation);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Composer )( 
            IITURLTrack * This,
            /* [retval][out] */ BSTR *composer);
        
        /* [helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Composer )( 
            IITURLTrack * This,
            /* [in] */ BSTR composer);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_DateAdded )( 
            IITURLTrack * This,
            /* [retval][out] */ DATE *dateAdded);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_DiscCount )( 
            IITURLTrack * This,
            /* [retval][out] */ long *discCount);
        
        /* [helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE *put_DiscCount )( 
            IITURLTrack * This,
            /* [in] */ long discCount);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_DiscNumber )( 
            IITURLTrack * This,
            /* [retval][out] */ long *discNumber);
        
        /* [helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE *put_DiscNumber )( 
            IITURLTrack * This,
            /* [in] */ long discNumber);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Duration )( 
            IITURLTrack * This,
            /* [retval][out] */ long *duration);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Enabled )( 
            IITURLTrack * This,
            /* [retval][out] */ VARIANT_BOOL *isEnabled);
        
        /* [helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Enabled )( 
            IITURLTrack * This,
            /* [in] */ VARIANT_BOOL shouldBeEnabled);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_EQ )( 
            IITURLTrack * This,
            /* [retval][out] */ BSTR *eq);
        
        /* [helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE *put_EQ )( 
            IITURLTrack * This,
            /* [in] */ BSTR eq);
        
        /* [helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Finish )( 
            IITURLTrack * This,
            /* [in] */ long finish);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Finish )( 
            IITURLTrack * This,
            /* [retval][out] */ long *finish);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Genre )( 
            IITURLTrack * This,
            /* [retval][out] */ BSTR *genre);
        
        /* [helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Genre )( 
            IITURLTrack * This,
            /* [in] */ BSTR genre);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Grouping )( 
            IITURLTrack * This,
            /* [retval][out] */ BSTR *grouping);
        
        /* [helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Grouping )( 
            IITURLTrack * This,
            /* [in] */ BSTR grouping);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_KindAsString )( 
            IITURLTrack * This,
            /* [retval][out] */ BSTR *kind);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_ModificationDate )( 
            IITURLTrack * This,
            /* [retval][out] */ DATE *dateModified);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_PlayedCount )( 
            IITURLTrack * This,
            /* [retval][out] */ long *playedCount);
        
        /* [helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE *put_PlayedCount )( 
            IITURLTrack * This,
            /* [in] */ long playedCount);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_PlayedDate )( 
            IITURLTrack * This,
            /* [retval][out] */ DATE *playedDate);
        
        /* [helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE *put_PlayedDate )( 
            IITURLTrack * This,
            /* [in] */ DATE playedDate);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_PlayOrderIndex )( 
            IITURLTrack * This,
            /* [retval][out] */ long *index);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Rating )( 
            IITURLTrack * This,
            /* [retval][out] */ long *rating);
        
        /* [helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Rating )( 
            IITURLTrack * This,
            /* [in] */ long rating);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_SampleRate )( 
            IITURLTrack * This,
            /* [retval][out] */ long *sampleRate);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Size )( 
            IITURLTrack * This,
            /* [retval][out] */ long *size);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Start )( 
            IITURLTrack * This,
            /* [retval][out] */ long *start);
        
        /* [helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Start )( 
            IITURLTrack * This,
            /* [in] */ long start);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Time )( 
            IITURLTrack * This,
            /* [retval][out] */ BSTR *time);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_TrackCount )( 
            IITURLTrack * This,
            /* [retval][out] */ long *trackCount);
        
        /* [helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE *put_TrackCount )( 
            IITURLTrack * This,
            /* [in] */ long trackCount);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_TrackNumber )( 
            IITURLTrack * This,
            /* [retval][out] */ long *trackNumber);
        
        /* [helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE *put_TrackNumber )( 
            IITURLTrack * This,
            /* [in] */ long trackNumber);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_VolumeAdjustment )( 
            IITURLTrack * This,
            /* [retval][out] */ long *volumeAdjustment);
        
        /* [helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE *put_VolumeAdjustment )( 
            IITURLTrack * This,
            /* [in] */ long volumeAdjustment);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Year )( 
            IITURLTrack * This,
            /* [retval][out] */ long *year);
        
        /* [helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Year )( 
            IITURLTrack * This,
            /* [in] */ long year);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Artwork )( 
            IITURLTrack * This,
            /* [retval][out] */ IITArtworkCollection **iArtworkCollection);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_URL )( 
            IITURLTrack * This,
            /* [retval][out] */ BSTR *url);
        
        /* [helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE *put_URL )( 
            IITURLTrack * This,
            /* [in] */ BSTR url);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Podcast )( 
            IITURLTrack * This,
            /* [retval][out] */ VARIANT_BOOL *isPodcast);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *UpdatePodcastFeed )( 
            IITURLTrack * This);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *DownloadPodcastEpisode )( 
            IITURLTrack * This);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Category )( 
            IITURLTrack * This,
            /* [retval][out] */ BSTR *category);
        
        /* [helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Category )( 
            IITURLTrack * This,
            /* [in] */ BSTR category);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Description )( 
            IITURLTrack * This,
            /* [retval][out] */ BSTR *description);
        
        /* [helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Description )( 
            IITURLTrack * This,
            /* [in] */ BSTR description);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_LongDescription )( 
            IITURLTrack * This,
            /* [retval][out] */ BSTR *longDescription);
        
        /* [helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE *put_LongDescription )( 
            IITURLTrack * This,
            /* [in] */ BSTR longDescription);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *Reveal )( 
            IITURLTrack * This);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_AlbumRating )( 
            IITURLTrack * This,
            /* [retval][out] */ long *rating);
        
        /* [helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE *put_AlbumRating )( 
            IITURLTrack * This,
            /* [in] */ long rating);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_AlbumRatingKind )( 
            IITURLTrack * This,
            /* [retval][out] */ ITRatingKind *ratingKind);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_RatingKind )( 
            IITURLTrack * This,
            /* [retval][out] */ ITRatingKind *ratingKind);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Playlists )( 
            IITURLTrack * This,
            /* [retval][out] */ IITPlaylistCollection **iPlaylistCollection);
        
        END_INTERFACE
    } IITURLTrackVtbl;

    interface IITURLTrack
    {
        CONST_VTBL struct IITURLTrackVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IITURLTrack_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IITURLTrack_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IITURLTrack_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IITURLTrack_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IITURLTrack_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IITURLTrack_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IITURLTrack_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IITURLTrack_GetITObjectIDs(This,sourceID,playlistID,trackID,databaseID)	\
    (This)->lpVtbl -> GetITObjectIDs(This,sourceID,playlistID,trackID,databaseID)

#define IITURLTrack_get_Name(This,name)	\
    (This)->lpVtbl -> get_Name(This,name)

#define IITURLTrack_put_Name(This,name)	\
    (This)->lpVtbl -> put_Name(This,name)

#define IITURLTrack_get_Index(This,index)	\
    (This)->lpVtbl -> get_Index(This,index)

#define IITURLTrack_get_SourceID(This,sourceID)	\
    (This)->lpVtbl -> get_SourceID(This,sourceID)

#define IITURLTrack_get_PlaylistID(This,playlistID)	\
    (This)->lpVtbl -> get_PlaylistID(This,playlistID)

#define IITURLTrack_get_TrackID(This,trackID)	\
    (This)->lpVtbl -> get_TrackID(This,trackID)

#define IITURLTrack_get_TrackDatabaseID(This,databaseID)	\
    (This)->lpVtbl -> get_TrackDatabaseID(This,databaseID)


#define IITURLTrack_Delete(This)	\
    (This)->lpVtbl -> Delete(This)

#define IITURLTrack_Play(This)	\
    (This)->lpVtbl -> Play(This)

#define IITURLTrack_AddArtworkFromFile(This,filePath,iArtwork)	\
    (This)->lpVtbl -> AddArtworkFromFile(This,filePath,iArtwork)

#define IITURLTrack_get_Kind(This,kind)	\
    (This)->lpVtbl -> get_Kind(This,kind)

#define IITURLTrack_get_Playlist(This,iPlaylist)	\
    (This)->lpVtbl -> get_Playlist(This,iPlaylist)

#define IITURLTrack_get_Album(This,album)	\
    (This)->lpVtbl -> get_Album(This,album)

#define IITURLTrack_put_Album(This,album)	\
    (This)->lpVtbl -> put_Album(This,album)

#define IITURLTrack_get_Artist(This,artist)	\
    (This)->lpVtbl -> get_Artist(This,artist)

#define IITURLTrack_put_Artist(This,artist)	\
    (This)->lpVtbl -> put_Artist(This,artist)

#define IITURLTrack_get_BitRate(This,bitrate)	\
    (This)->lpVtbl -> get_BitRate(This,bitrate)

#define IITURLTrack_get_BPM(This,beatsPerMinute)	\
    (This)->lpVtbl -> get_BPM(This,beatsPerMinute)

#define IITURLTrack_put_BPM(This,beatsPerMinute)	\
    (This)->lpVtbl -> put_BPM(This,beatsPerMinute)

#define IITURLTrack_get_Comment(This,comment)	\
    (This)->lpVtbl -> get_Comment(This,comment)

#define IITURLTrack_put_Comment(This,comment)	\
    (This)->lpVtbl -> put_Comment(This,comment)

#define IITURLTrack_get_Compilation(This,isCompilation)	\
    (This)->lpVtbl -> get_Compilation(This,isCompilation)

#define IITURLTrack_put_Compilation(This,shouldBeCompilation)	\
    (This)->lpVtbl -> put_Compilation(This,shouldBeCompilation)

#define IITURLTrack_get_Composer(This,composer)	\
    (This)->lpVtbl -> get_Composer(This,composer)

#define IITURLTrack_put_Composer(This,composer)	\
    (This)->lpVtbl -> put_Composer(This,composer)

#define IITURLTrack_get_DateAdded(This,dateAdded)	\
    (This)->lpVtbl -> get_DateAdded(This,dateAdded)

#define IITURLTrack_get_DiscCount(This,discCount)	\
    (This)->lpVtbl -> get_DiscCount(This,discCount)

#define IITURLTrack_put_DiscCount(This,discCount)	\
    (This)->lpVtbl -> put_DiscCount(This,discCount)

#define IITURLTrack_get_DiscNumber(This,discNumber)	\
    (This)->lpVtbl -> get_DiscNumber(This,discNumber)

#define IITURLTrack_put_DiscNumber(This,discNumber)	\
    (This)->lpVtbl -> put_DiscNumber(This,discNumber)

#define IITURLTrack_get_Duration(This,duration)	\
    (This)->lpVtbl -> get_Duration(This,duration)

#define IITURLTrack_get_Enabled(This,isEnabled)	\
    (This)->lpVtbl -> get_Enabled(This,isEnabled)

#define IITURLTrack_put_Enabled(This,shouldBeEnabled)	\
    (This)->lpVtbl -> put_Enabled(This,shouldBeEnabled)

#define IITURLTrack_get_EQ(This,eq)	\
    (This)->lpVtbl -> get_EQ(This,eq)

#define IITURLTrack_put_EQ(This,eq)	\
    (This)->lpVtbl -> put_EQ(This,eq)

#define IITURLTrack_put_Finish(This,finish)	\
    (This)->lpVtbl -> put_Finish(This,finish)

#define IITURLTrack_get_Finish(This,finish)	\
    (This)->lpVtbl -> get_Finish(This,finish)

#define IITURLTrack_get_Genre(This,genre)	\
    (This)->lpVtbl -> get_Genre(This,genre)

#define IITURLTrack_put_Genre(This,genre)	\
    (This)->lpVtbl -> put_Genre(This,genre)

#define IITURLTrack_get_Grouping(This,grouping)	\
    (This)->lpVtbl -> get_Grouping(This,grouping)

#define IITURLTrack_put_Grouping(This,grouping)	\
    (This)->lpVtbl -> put_Grouping(This,grouping)

#define IITURLTrack_get_KindAsString(This,kind)	\
    (This)->lpVtbl -> get_KindAsString(This,kind)

#define IITURLTrack_get_ModificationDate(This,dateModified)	\
    (This)->lpVtbl -> get_ModificationDate(This,dateModified)

#define IITURLTrack_get_PlayedCount(This,playedCount)	\
    (This)->lpVtbl -> get_PlayedCount(This,playedCount)

#define IITURLTrack_put_PlayedCount(This,playedCount)	\
    (This)->lpVtbl -> put_PlayedCount(This,playedCount)

#define IITURLTrack_get_PlayedDate(This,playedDate)	\
    (This)->lpVtbl -> get_PlayedDate(This,playedDate)

#define IITURLTrack_put_PlayedDate(This,playedDate)	\
    (This)->lpVtbl -> put_PlayedDate(This,playedDate)

#define IITURLTrack_get_PlayOrderIndex(This,index)	\
    (This)->lpVtbl -> get_PlayOrderIndex(This,index)

#define IITURLTrack_get_Rating(This,rating)	\
    (This)->lpVtbl -> get_Rating(This,rating)

#define IITURLTrack_put_Rating(This,rating)	\
    (This)->lpVtbl -> put_Rating(This,rating)

#define IITURLTrack_get_SampleRate(This,sampleRate)	\
    (This)->lpVtbl -> get_SampleRate(This,sampleRate)

#define IITURLTrack_get_Size(This,size)	\
    (This)->lpVtbl -> get_Size(This,size)

#define IITURLTrack_get_Start(This,start)	\
    (This)->lpVtbl -> get_Start(This,start)

#define IITURLTrack_put_Start(This,start)	\
    (This)->lpVtbl -> put_Start(This,start)

#define IITURLTrack_get_Time(This,time)	\
    (This)->lpVtbl -> get_Time(This,time)

#define IITURLTrack_get_TrackCount(This,trackCount)	\
    (This)->lpVtbl -> get_TrackCount(This,trackCount)

#define IITURLTrack_put_TrackCount(This,trackCount)	\
    (This)->lpVtbl -> put_TrackCount(This,trackCount)

#define IITURLTrack_get_TrackNumber(This,trackNumber)	\
    (This)->lpVtbl -> get_TrackNumber(This,trackNumber)

#define IITURLTrack_put_TrackNumber(This,trackNumber)	\
    (This)->lpVtbl -> put_TrackNumber(This,trackNumber)

#define IITURLTrack_get_VolumeAdjustment(This,volumeAdjustment)	\
    (This)->lpVtbl -> get_VolumeAdjustment(This,volumeAdjustment)

#define IITURLTrack_put_VolumeAdjustment(This,volumeAdjustment)	\
    (This)->lpVtbl -> put_VolumeAdjustment(This,volumeAdjustment)

#define IITURLTrack_get_Year(This,year)	\
    (This)->lpVtbl -> get_Year(This,year)

#define IITURLTrack_put_Year(This,year)	\
    (This)->lpVtbl -> put_Year(This,year)

#define IITURLTrack_get_Artwork(This,iArtworkCollection)	\
    (This)->lpVtbl -> get_Artwork(This,iArtworkCollection)


#define IITURLTrack_get_URL(This,url)	\
    (This)->lpVtbl -> get_URL(This,url)

#define IITURLTrack_put_URL(This,url)	\
    (This)->lpVtbl -> put_URL(This,url)

#define IITURLTrack_get_Podcast(This,isPodcast)	\
    (This)->lpVtbl -> get_Podcast(This,isPodcast)

#define IITURLTrack_UpdatePodcastFeed(This)	\
    (This)->lpVtbl -> UpdatePodcastFeed(This)

#define IITURLTrack_DownloadPodcastEpisode(This)	\
    (This)->lpVtbl -> DownloadPodcastEpisode(This)

#define IITURLTrack_get_Category(This,category)	\
    (This)->lpVtbl -> get_Category(This,category)

#define IITURLTrack_put_Category(This,category)	\
    (This)->lpVtbl -> put_Category(This,category)

#define IITURLTrack_get_Description(This,description)	\
    (This)->lpVtbl -> get_Description(This,description)

#define IITURLTrack_put_Description(This,description)	\
    (This)->lpVtbl -> put_Description(This,description)

#define IITURLTrack_get_LongDescription(This,longDescription)	\
    (This)->lpVtbl -> get_LongDescription(This,longDescription)

#define IITURLTrack_put_LongDescription(This,longDescription)	\
    (This)->lpVtbl -> put_LongDescription(This,longDescription)

#define IITURLTrack_Reveal(This)	\
    (This)->lpVtbl -> Reveal(This)

#define IITURLTrack_get_AlbumRating(This,rating)	\
    (This)->lpVtbl -> get_AlbumRating(This,rating)

#define IITURLTrack_put_AlbumRating(This,rating)	\
    (This)->lpVtbl -> put_AlbumRating(This,rating)

#define IITURLTrack_get_AlbumRatingKind(This,ratingKind)	\
    (This)->lpVtbl -> get_AlbumRatingKind(This,ratingKind)

#define IITURLTrack_get_RatingKind(This,ratingKind)	\
    (This)->lpVtbl -> get_RatingKind(This,ratingKind)

#define IITURLTrack_get_Playlists(This,iPlaylistCollection)	\
    (This)->lpVtbl -> get_Playlists(This,iPlaylistCollection)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IITURLTrack_get_URL_Proxy( 
    IITURLTrack * This,
    /* [retval][out] */ BSTR *url);


void __RPC_STUB IITURLTrack_get_URL_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE IITURLTrack_put_URL_Proxy( 
    IITURLTrack * This,
    /* [in] */ BSTR url);


void __RPC_STUB IITURLTrack_put_URL_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IITURLTrack_get_Podcast_Proxy( 
    IITURLTrack * This,
    /* [retval][out] */ VARIANT_BOOL *isPodcast);


void __RPC_STUB IITURLTrack_get_Podcast_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IITURLTrack_UpdatePodcastFeed_Proxy( 
    IITURLTrack * This);


void __RPC_STUB IITURLTrack_UpdatePodcastFeed_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IITURLTrack_DownloadPodcastEpisode_Proxy( 
    IITURLTrack * This);


void __RPC_STUB IITURLTrack_DownloadPodcastEpisode_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IITURLTrack_get_Category_Proxy( 
    IITURLTrack * This,
    /* [retval][out] */ BSTR *category);


void __RPC_STUB IITURLTrack_get_Category_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE IITURLTrack_put_Category_Proxy( 
    IITURLTrack * This,
    /* [in] */ BSTR category);


void __RPC_STUB IITURLTrack_put_Category_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IITURLTrack_get_Description_Proxy( 
    IITURLTrack * This,
    /* [retval][out] */ BSTR *description);


void __RPC_STUB IITURLTrack_get_Description_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE IITURLTrack_put_Description_Proxy( 
    IITURLTrack * This,
    /* [in] */ BSTR description);


void __RPC_STUB IITURLTrack_put_Description_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IITURLTrack_get_LongDescription_Proxy( 
    IITURLTrack * This,
    /* [retval][out] */ BSTR *longDescription);


void __RPC_STUB IITURLTrack_get_LongDescription_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE IITURLTrack_put_LongDescription_Proxy( 
    IITURLTrack * This,
    /* [in] */ BSTR longDescription);


void __RPC_STUB IITURLTrack_put_LongDescription_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IITURLTrack_Reveal_Proxy( 
    IITURLTrack * This);


void __RPC_STUB IITURLTrack_Reveal_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IITURLTrack_get_AlbumRating_Proxy( 
    IITURLTrack * This,
    /* [retval][out] */ long *rating);


void __RPC_STUB IITURLTrack_get_AlbumRating_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE IITURLTrack_put_AlbumRating_Proxy( 
    IITURLTrack * This,
    /* [in] */ long rating);


void __RPC_STUB IITURLTrack_put_AlbumRating_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IITURLTrack_get_AlbumRatingKind_Proxy( 
    IITURLTrack * This,
    /* [retval][out] */ ITRatingKind *ratingKind);


void __RPC_STUB IITURLTrack_get_AlbumRatingKind_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IITURLTrack_get_RatingKind_Proxy( 
    IITURLTrack * This,
    /* [retval][out] */ ITRatingKind *ratingKind);


void __RPC_STUB IITURLTrack_get_RatingKind_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IITURLTrack_get_Playlists_Proxy( 
    IITURLTrack * This,
    /* [retval][out] */ IITPlaylistCollection **iPlaylistCollection);


void __RPC_STUB IITURLTrack_get_Playlists_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IITURLTrack_INTERFACE_DEFINED__ */


#ifndef __IITAudioCDPlaylist_INTERFACE_DEFINED__
#define __IITAudioCDPlaylist_INTERFACE_DEFINED__

/* interface IITAudioCDPlaylist */
/* [hidden][unique][helpstring][dual][uuid][object] */ 


EXTERN_C const IID IID_IITAudioCDPlaylist;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("CF496DF3-0FED-4d7d-9BD8-529B6E8A082E")
    IITAudioCDPlaylist : public IITPlaylist
    {
    public:
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_Artist( 
            /* [retval][out] */ BSTR *artist) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_Compilation( 
            /* [retval][out] */ VARIANT_BOOL *isCompiliation) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_Composer( 
            /* [retval][out] */ BSTR *composer) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_DiscCount( 
            /* [retval][out] */ long *discCount) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_DiscNumber( 
            /* [retval][out] */ long *discNumber) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_Genre( 
            /* [retval][out] */ BSTR *genre) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_Year( 
            /* [retval][out] */ long *year) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE Reveal( void) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IITAudioCDPlaylistVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IITAudioCDPlaylist * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IITAudioCDPlaylist * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IITAudioCDPlaylist * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IITAudioCDPlaylist * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IITAudioCDPlaylist * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IITAudioCDPlaylist * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IITAudioCDPlaylist * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *GetITObjectIDs )( 
            IITAudioCDPlaylist * This,
            /* [out] */ long *sourceID,
            /* [out] */ long *playlistID,
            /* [out] */ long *trackID,
            /* [out] */ long *databaseID);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Name )( 
            IITAudioCDPlaylist * This,
            /* [retval][out] */ BSTR *name);
        
        /* [helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Name )( 
            IITAudioCDPlaylist * This,
            /* [in] */ BSTR name);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Index )( 
            IITAudioCDPlaylist * This,
            /* [retval][out] */ long *index);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_SourceID )( 
            IITAudioCDPlaylist * This,
            /* [retval][out] */ long *sourceID);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_PlaylistID )( 
            IITAudioCDPlaylist * This,
            /* [retval][out] */ long *playlistID);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_TrackID )( 
            IITAudioCDPlaylist * This,
            /* [retval][out] */ long *trackID);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_TrackDatabaseID )( 
            IITAudioCDPlaylist * This,
            /* [retval][out] */ long *databaseID);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *Delete )( 
            IITAudioCDPlaylist * This);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *PlayFirstTrack )( 
            IITAudioCDPlaylist * This);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *Print )( 
            IITAudioCDPlaylist * This,
            /* [in] */ VARIANT_BOOL showPrintDialog,
            /* [in] */ ITPlaylistPrintKind printKind,
            /* [in] */ BSTR theme);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *Search )( 
            IITAudioCDPlaylist * This,
            /* [in] */ BSTR searchText,
            /* [in] */ ITPlaylistSearchField searchFields,
            /* [retval][out] */ IITTrackCollection **iTrackCollection);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Kind )( 
            IITAudioCDPlaylist * This,
            /* [retval][out] */ ITPlaylistKind *kind);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Source )( 
            IITAudioCDPlaylist * This,
            /* [retval][out] */ IITSource **iSource);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Duration )( 
            IITAudioCDPlaylist * This,
            /* [retval][out] */ long *duration);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Shuffle )( 
            IITAudioCDPlaylist * This,
            /* [retval][out] */ VARIANT_BOOL *isShuffle);
        
        /* [helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Shuffle )( 
            IITAudioCDPlaylist * This,
            /* [in] */ VARIANT_BOOL shouldShuffle);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Size )( 
            IITAudioCDPlaylist * This,
            /* [retval][out] */ double *size);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_SongRepeat )( 
            IITAudioCDPlaylist * This,
            /* [retval][out] */ ITPlaylistRepeatMode *repeatMode);
        
        /* [helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE *put_SongRepeat )( 
            IITAudioCDPlaylist * This,
            /* [in] */ ITPlaylistRepeatMode repeatMode);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Time )( 
            IITAudioCDPlaylist * This,
            /* [retval][out] */ BSTR *time);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Visible )( 
            IITAudioCDPlaylist * This,
            /* [retval][out] */ VARIANT_BOOL *isVisible);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Tracks )( 
            IITAudioCDPlaylist * This,
            /* [retval][out] */ IITTrackCollection **iTrackCollection);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Artist )( 
            IITAudioCDPlaylist * This,
            /* [retval][out] */ BSTR *artist);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Compilation )( 
            IITAudioCDPlaylist * This,
            /* [retval][out] */ VARIANT_BOOL *isCompiliation);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Composer )( 
            IITAudioCDPlaylist * This,
            /* [retval][out] */ BSTR *composer);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_DiscCount )( 
            IITAudioCDPlaylist * This,
            /* [retval][out] */ long *discCount);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_DiscNumber )( 
            IITAudioCDPlaylist * This,
            /* [retval][out] */ long *discNumber);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Genre )( 
            IITAudioCDPlaylist * This,
            /* [retval][out] */ BSTR *genre);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Year )( 
            IITAudioCDPlaylist * This,
            /* [retval][out] */ long *year);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *Reveal )( 
            IITAudioCDPlaylist * This);
        
        END_INTERFACE
    } IITAudioCDPlaylistVtbl;

    interface IITAudioCDPlaylist
    {
        CONST_VTBL struct IITAudioCDPlaylistVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IITAudioCDPlaylist_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IITAudioCDPlaylist_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IITAudioCDPlaylist_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IITAudioCDPlaylist_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IITAudioCDPlaylist_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IITAudioCDPlaylist_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IITAudioCDPlaylist_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IITAudioCDPlaylist_GetITObjectIDs(This,sourceID,playlistID,trackID,databaseID)	\
    (This)->lpVtbl -> GetITObjectIDs(This,sourceID,playlistID,trackID,databaseID)

#define IITAudioCDPlaylist_get_Name(This,name)	\
    (This)->lpVtbl -> get_Name(This,name)

#define IITAudioCDPlaylist_put_Name(This,name)	\
    (This)->lpVtbl -> put_Name(This,name)

#define IITAudioCDPlaylist_get_Index(This,index)	\
    (This)->lpVtbl -> get_Index(This,index)

#define IITAudioCDPlaylist_get_SourceID(This,sourceID)	\
    (This)->lpVtbl -> get_SourceID(This,sourceID)

#define IITAudioCDPlaylist_get_PlaylistID(This,playlistID)	\
    (This)->lpVtbl -> get_PlaylistID(This,playlistID)

#define IITAudioCDPlaylist_get_TrackID(This,trackID)	\
    (This)->lpVtbl -> get_TrackID(This,trackID)

#define IITAudioCDPlaylist_get_TrackDatabaseID(This,databaseID)	\
    (This)->lpVtbl -> get_TrackDatabaseID(This,databaseID)


#define IITAudioCDPlaylist_Delete(This)	\
    (This)->lpVtbl -> Delete(This)

#define IITAudioCDPlaylist_PlayFirstTrack(This)	\
    (This)->lpVtbl -> PlayFirstTrack(This)

#define IITAudioCDPlaylist_Print(This,showPrintDialog,printKind,theme)	\
    (This)->lpVtbl -> Print(This,showPrintDialog,printKind,theme)

#define IITAudioCDPlaylist_Search(This,searchText,searchFields,iTrackCollection)	\
    (This)->lpVtbl -> Search(This,searchText,searchFields,iTrackCollection)

#define IITAudioCDPlaylist_get_Kind(This,kind)	\
    (This)->lpVtbl -> get_Kind(This,kind)

#define IITAudioCDPlaylist_get_Source(This,iSource)	\
    (This)->lpVtbl -> get_Source(This,iSource)

#define IITAudioCDPlaylist_get_Duration(This,duration)	\
    (This)->lpVtbl -> get_Duration(This,duration)

#define IITAudioCDPlaylist_get_Shuffle(This,isShuffle)	\
    (This)->lpVtbl -> get_Shuffle(This,isShuffle)

#define IITAudioCDPlaylist_put_Shuffle(This,shouldShuffle)	\
    (This)->lpVtbl -> put_Shuffle(This,shouldShuffle)

#define IITAudioCDPlaylist_get_Size(This,size)	\
    (This)->lpVtbl -> get_Size(This,size)

#define IITAudioCDPlaylist_get_SongRepeat(This,repeatMode)	\
    (This)->lpVtbl -> get_SongRepeat(This,repeatMode)

#define IITAudioCDPlaylist_put_SongRepeat(This,repeatMode)	\
    (This)->lpVtbl -> put_SongRepeat(This,repeatMode)

#define IITAudioCDPlaylist_get_Time(This,time)	\
    (This)->lpVtbl -> get_Time(This,time)

#define IITAudioCDPlaylist_get_Visible(This,isVisible)	\
    (This)->lpVtbl -> get_Visible(This,isVisible)

#define IITAudioCDPlaylist_get_Tracks(This,iTrackCollection)	\
    (This)->lpVtbl -> get_Tracks(This,iTrackCollection)


#define IITAudioCDPlaylist_get_Artist(This,artist)	\
    (This)->lpVtbl -> get_Artist(This,artist)

#define IITAudioCDPlaylist_get_Compilation(This,isCompiliation)	\
    (This)->lpVtbl -> get_Compilation(This,isCompiliation)

#define IITAudioCDPlaylist_get_Composer(This,composer)	\
    (This)->lpVtbl -> get_Composer(This,composer)

#define IITAudioCDPlaylist_get_DiscCount(This,discCount)	\
    (This)->lpVtbl -> get_DiscCount(This,discCount)

#define IITAudioCDPlaylist_get_DiscNumber(This,discNumber)	\
    (This)->lpVtbl -> get_DiscNumber(This,discNumber)

#define IITAudioCDPlaylist_get_Genre(This,genre)	\
    (This)->lpVtbl -> get_Genre(This,genre)

#define IITAudioCDPlaylist_get_Year(This,year)	\
    (This)->lpVtbl -> get_Year(This,year)

#define IITAudioCDPlaylist_Reveal(This)	\
    (This)->lpVtbl -> Reveal(This)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IITAudioCDPlaylist_get_Artist_Proxy( 
    IITAudioCDPlaylist * This,
    /* [retval][out] */ BSTR *artist);


void __RPC_STUB IITAudioCDPlaylist_get_Artist_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IITAudioCDPlaylist_get_Compilation_Proxy( 
    IITAudioCDPlaylist * This,
    /* [retval][out] */ VARIANT_BOOL *isCompiliation);


void __RPC_STUB IITAudioCDPlaylist_get_Compilation_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IITAudioCDPlaylist_get_Composer_Proxy( 
    IITAudioCDPlaylist * This,
    /* [retval][out] */ BSTR *composer);


void __RPC_STUB IITAudioCDPlaylist_get_Composer_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IITAudioCDPlaylist_get_DiscCount_Proxy( 
    IITAudioCDPlaylist * This,
    /* [retval][out] */ long *discCount);


void __RPC_STUB IITAudioCDPlaylist_get_DiscCount_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IITAudioCDPlaylist_get_DiscNumber_Proxy( 
    IITAudioCDPlaylist * This,
    /* [retval][out] */ long *discNumber);


void __RPC_STUB IITAudioCDPlaylist_get_DiscNumber_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IITAudioCDPlaylist_get_Genre_Proxy( 
    IITAudioCDPlaylist * This,
    /* [retval][out] */ BSTR *genre);


void __RPC_STUB IITAudioCDPlaylist_get_Genre_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IITAudioCDPlaylist_get_Year_Proxy( 
    IITAudioCDPlaylist * This,
    /* [retval][out] */ long *year);


void __RPC_STUB IITAudioCDPlaylist_get_Year_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IITAudioCDPlaylist_Reveal_Proxy( 
    IITAudioCDPlaylist * This);


void __RPC_STUB IITAudioCDPlaylist_Reveal_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IITAudioCDPlaylist_INTERFACE_DEFINED__ */


#ifndef __IITPlaylistCollection_INTERFACE_DEFINED__
#define __IITPlaylistCollection_INTERFACE_DEFINED__

/* interface IITPlaylistCollection */
/* [unique][helpstring][dual][uuid][object] */ 


EXTERN_C const IID IID_IITPlaylistCollection;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("FF194254-909D-4437-9C50-3AAC2AE6305C")
    IITPlaylistCollection : public IDispatch
    {
    public:
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_Count( 
            /* [retval][out] */ long *count) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Item( 
            /* [in] */ long index,
            /* [retval][out] */ IITPlaylist **iPlaylist) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_ItemByName( 
            /* [in] */ BSTR name,
            /* [retval][out] */ IITPlaylist **iPlaylist) = 0;
        
        virtual /* [helpstring][restricted][id][propget] */ HRESULT STDMETHODCALLTYPE get__NewEnum( 
            /* [retval][out] */ IUnknown **iEnumerator) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_ItemByPersistentID( 
            /* [in] */ long highID,
            /* [in] */ long lowID,
            /* [retval][out] */ IITPlaylist **iPlaylist) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IITPlaylistCollectionVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IITPlaylistCollection * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IITPlaylistCollection * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IITPlaylistCollection * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IITPlaylistCollection * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IITPlaylistCollection * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IITPlaylistCollection * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IITPlaylistCollection * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Count )( 
            IITPlaylistCollection * This,
            /* [retval][out] */ long *count);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Item )( 
            IITPlaylistCollection * This,
            /* [in] */ long index,
            /* [retval][out] */ IITPlaylist **iPlaylist);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_ItemByName )( 
            IITPlaylistCollection * This,
            /* [in] */ BSTR name,
            /* [retval][out] */ IITPlaylist **iPlaylist);
        
        /* [helpstring][restricted][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get__NewEnum )( 
            IITPlaylistCollection * This,
            /* [retval][out] */ IUnknown **iEnumerator);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_ItemByPersistentID )( 
            IITPlaylistCollection * This,
            /* [in] */ long highID,
            /* [in] */ long lowID,
            /* [retval][out] */ IITPlaylist **iPlaylist);
        
        END_INTERFACE
    } IITPlaylistCollectionVtbl;

    interface IITPlaylistCollection
    {
        CONST_VTBL struct IITPlaylistCollectionVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IITPlaylistCollection_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IITPlaylistCollection_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IITPlaylistCollection_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IITPlaylistCollection_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IITPlaylistCollection_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IITPlaylistCollection_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IITPlaylistCollection_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IITPlaylistCollection_get_Count(This,count)	\
    (This)->lpVtbl -> get_Count(This,count)

#define IITPlaylistCollection_get_Item(This,index,iPlaylist)	\
    (This)->lpVtbl -> get_Item(This,index,iPlaylist)

#define IITPlaylistCollection_get_ItemByName(This,name,iPlaylist)	\
    (This)->lpVtbl -> get_ItemByName(This,name,iPlaylist)

#define IITPlaylistCollection_get__NewEnum(This,iEnumerator)	\
    (This)->lpVtbl -> get__NewEnum(This,iEnumerator)

#define IITPlaylistCollection_get_ItemByPersistentID(This,highID,lowID,iPlaylist)	\
    (This)->lpVtbl -> get_ItemByPersistentID(This,highID,lowID,iPlaylist)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IITPlaylistCollection_get_Count_Proxy( 
    IITPlaylistCollection * This,
    /* [retval][out] */ long *count);


void __RPC_STUB IITPlaylistCollection_get_Count_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IITPlaylistCollection_get_Item_Proxy( 
    IITPlaylistCollection * This,
    /* [in] */ long index,
    /* [retval][out] */ IITPlaylist **iPlaylist);


void __RPC_STUB IITPlaylistCollection_get_Item_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IITPlaylistCollection_get_ItemByName_Proxy( 
    IITPlaylistCollection * This,
    /* [in] */ BSTR name,
    /* [retval][out] */ IITPlaylist **iPlaylist);


void __RPC_STUB IITPlaylistCollection_get_ItemByName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][restricted][id][propget] */ HRESULT STDMETHODCALLTYPE IITPlaylistCollection_get__NewEnum_Proxy( 
    IITPlaylistCollection * This,
    /* [retval][out] */ IUnknown **iEnumerator);


void __RPC_STUB IITPlaylistCollection_get__NewEnum_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IITPlaylistCollection_get_ItemByPersistentID_Proxy( 
    IITPlaylistCollection * This,
    /* [in] */ long highID,
    /* [in] */ long lowID,
    /* [retval][out] */ IITPlaylist **iPlaylist);


void __RPC_STUB IITPlaylistCollection_get_ItemByPersistentID_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IITPlaylistCollection_INTERFACE_DEFINED__ */


#ifndef __IITIPodSource_INTERFACE_DEFINED__
#define __IITIPodSource_INTERFACE_DEFINED__

/* interface IITIPodSource */
/* [hidden][unique][helpstring][dual][uuid][object] */ 


EXTERN_C const IID IID_IITIPodSource;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("CF4D8ACE-1720-4fb9-B0AE-9877249E89B0")
    IITIPodSource : public IITSource
    {
    public:
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE UpdateIPod( void) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE EjectIPod( void) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_SoftwareVersion( 
            /* [retval][out] */ BSTR *softwareVersion) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IITIPodSourceVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IITIPodSource * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IITIPodSource * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IITIPodSource * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IITIPodSource * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IITIPodSource * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IITIPodSource * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IITIPodSource * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *GetITObjectIDs )( 
            IITIPodSource * This,
            /* [out] */ long *sourceID,
            /* [out] */ long *playlistID,
            /* [out] */ long *trackID,
            /* [out] */ long *databaseID);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Name )( 
            IITIPodSource * This,
            /* [retval][out] */ BSTR *name);
        
        /* [helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Name )( 
            IITIPodSource * This,
            /* [in] */ BSTR name);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Index )( 
            IITIPodSource * This,
            /* [retval][out] */ long *index);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_SourceID )( 
            IITIPodSource * This,
            /* [retval][out] */ long *sourceID);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_PlaylistID )( 
            IITIPodSource * This,
            /* [retval][out] */ long *playlistID);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_TrackID )( 
            IITIPodSource * This,
            /* [retval][out] */ long *trackID);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_TrackDatabaseID )( 
            IITIPodSource * This,
            /* [retval][out] */ long *databaseID);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Kind )( 
            IITIPodSource * This,
            /* [retval][out] */ ITSourceKind *kind);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Capacity )( 
            IITIPodSource * This,
            /* [retval][out] */ double *capacity);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_FreeSpace )( 
            IITIPodSource * This,
            /* [retval][out] */ double *freespace);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Playlists )( 
            IITIPodSource * This,
            /* [retval][out] */ IITPlaylistCollection **iPlaylistCollection);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *UpdateIPod )( 
            IITIPodSource * This);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *EjectIPod )( 
            IITIPodSource * This);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_SoftwareVersion )( 
            IITIPodSource * This,
            /* [retval][out] */ BSTR *softwareVersion);
        
        END_INTERFACE
    } IITIPodSourceVtbl;

    interface IITIPodSource
    {
        CONST_VTBL struct IITIPodSourceVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IITIPodSource_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IITIPodSource_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IITIPodSource_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IITIPodSource_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IITIPodSource_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IITIPodSource_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IITIPodSource_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IITIPodSource_GetITObjectIDs(This,sourceID,playlistID,trackID,databaseID)	\
    (This)->lpVtbl -> GetITObjectIDs(This,sourceID,playlistID,trackID,databaseID)

#define IITIPodSource_get_Name(This,name)	\
    (This)->lpVtbl -> get_Name(This,name)

#define IITIPodSource_put_Name(This,name)	\
    (This)->lpVtbl -> put_Name(This,name)

#define IITIPodSource_get_Index(This,index)	\
    (This)->lpVtbl -> get_Index(This,index)

#define IITIPodSource_get_SourceID(This,sourceID)	\
    (This)->lpVtbl -> get_SourceID(This,sourceID)

#define IITIPodSource_get_PlaylistID(This,playlistID)	\
    (This)->lpVtbl -> get_PlaylistID(This,playlistID)

#define IITIPodSource_get_TrackID(This,trackID)	\
    (This)->lpVtbl -> get_TrackID(This,trackID)

#define IITIPodSource_get_TrackDatabaseID(This,databaseID)	\
    (This)->lpVtbl -> get_TrackDatabaseID(This,databaseID)


#define IITIPodSource_get_Kind(This,kind)	\
    (This)->lpVtbl -> get_Kind(This,kind)

#define IITIPodSource_get_Capacity(This,capacity)	\
    (This)->lpVtbl -> get_Capacity(This,capacity)

#define IITIPodSource_get_FreeSpace(This,freespace)	\
    (This)->lpVtbl -> get_FreeSpace(This,freespace)

#define IITIPodSource_get_Playlists(This,iPlaylistCollection)	\
    (This)->lpVtbl -> get_Playlists(This,iPlaylistCollection)


#define IITIPodSource_UpdateIPod(This)	\
    (This)->lpVtbl -> UpdateIPod(This)

#define IITIPodSource_EjectIPod(This)	\
    (This)->lpVtbl -> EjectIPod(This)

#define IITIPodSource_get_SoftwareVersion(This,softwareVersion)	\
    (This)->lpVtbl -> get_SoftwareVersion(This,softwareVersion)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring] */ HRESULT STDMETHODCALLTYPE IITIPodSource_UpdateIPod_Proxy( 
    IITIPodSource * This);


void __RPC_STUB IITIPodSource_UpdateIPod_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IITIPodSource_EjectIPod_Proxy( 
    IITIPodSource * This);


void __RPC_STUB IITIPodSource_EjectIPod_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IITIPodSource_get_SoftwareVersion_Proxy( 
    IITIPodSource * This,
    /* [retval][out] */ BSTR *softwareVersion);


void __RPC_STUB IITIPodSource_get_SoftwareVersion_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IITIPodSource_INTERFACE_DEFINED__ */


#ifndef __IITFileOrCDTrack_INTERFACE_DEFINED__
#define __IITFileOrCDTrack_INTERFACE_DEFINED__

/* interface IITFileOrCDTrack */
/* [hidden][unique][helpstring][dual][uuid][object] */ 


EXTERN_C const IID IID_IITFileOrCDTrack;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("00D7FE99-7868-4cc7-AD9E-ACFD70D09566")
    IITFileOrCDTrack : public IITTrack
    {
    public:
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_Location( 
            /* [retval][out] */ BSTR *location) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE UpdateInfoFromFile( void) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_Podcast( 
            /* [retval][out] */ VARIANT_BOOL *isPodcast) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE UpdatePodcastFeed( void) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_RememberBookmark( 
            /* [retval][out] */ VARIANT_BOOL *rememberBookmark) = 0;
        
        virtual /* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE put_RememberBookmark( 
            /* [in] */ VARIANT_BOOL shouldRememberBookmark) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_ExcludeFromShuffle( 
            /* [retval][out] */ VARIANT_BOOL *excludeFromShuffle) = 0;
        
        virtual /* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE put_ExcludeFromShuffle( 
            /* [in] */ VARIANT_BOOL shouldExcludeFromShuffle) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_Lyrics( 
            /* [retval][out] */ BSTR *lyrics) = 0;
        
        virtual /* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE put_Lyrics( 
            /* [in] */ BSTR lyrics) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_Category( 
            /* [retval][out] */ BSTR *category) = 0;
        
        virtual /* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE put_Category( 
            /* [in] */ BSTR category) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_Description( 
            /* [retval][out] */ BSTR *description) = 0;
        
        virtual /* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE put_Description( 
            /* [in] */ BSTR description) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_LongDescription( 
            /* [retval][out] */ BSTR *longDescription) = 0;
        
        virtual /* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE put_LongDescription( 
            /* [in] */ BSTR longDescription) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_BookmarkTime( 
            /* [retval][out] */ long *bookmarkTime) = 0;
        
        virtual /* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE put_BookmarkTime( 
            /* [in] */ long bookmarkTime) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_VideoKind( 
            /* [retval][out] */ ITVideoKind *videoKind) = 0;
        
        virtual /* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE put_VideoKind( 
            /* [in] */ ITVideoKind videoKind) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_SkippedCount( 
            /* [retval][out] */ long *skippedCount) = 0;
        
        virtual /* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE put_SkippedCount( 
            /* [in] */ long skippedCount) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_SkippedDate( 
            /* [retval][out] */ DATE *skippedDate) = 0;
        
        virtual /* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE put_SkippedDate( 
            /* [in] */ DATE skippedDate) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_PartOfGaplessAlbum( 
            /* [retval][out] */ VARIANT_BOOL *partOfGaplessAlbum) = 0;
        
        virtual /* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE put_PartOfGaplessAlbum( 
            /* [in] */ VARIANT_BOOL shouldBePartOfGaplessAlbum) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_AlbumArtist( 
            /* [retval][out] */ BSTR *albumArtist) = 0;
        
        virtual /* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE put_AlbumArtist( 
            /* [in] */ BSTR albumArtist) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_Show( 
            /* [retval][out] */ BSTR *showName) = 0;
        
        virtual /* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE put_Show( 
            /* [in] */ BSTR showName) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_SeasonNumber( 
            /* [retval][out] */ long *seasonNumber) = 0;
        
        virtual /* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE put_SeasonNumber( 
            /* [in] */ long seasonNumber) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_EpisodeID( 
            /* [retval][out] */ BSTR *episodeID) = 0;
        
        virtual /* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE put_EpisodeID( 
            /* [in] */ BSTR episodeID) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_EpisodeNumber( 
            /* [retval][out] */ long *episodeNumber) = 0;
        
        virtual /* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE put_EpisodeNumber( 
            /* [in] */ long episodeNumber) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_Size64High( 
            /* [retval][out] */ long *sizeHigh) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_Size64Low( 
            /* [retval][out] */ long *sizeLow) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_Unplayed( 
            /* [retval][out] */ VARIANT_BOOL *isUnplayed) = 0;
        
        virtual /* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE put_Unplayed( 
            /* [in] */ VARIANT_BOOL shouldBeUnplayed) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_SortAlbum( 
            /* [retval][out] */ BSTR *album) = 0;
        
        virtual /* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE put_SortAlbum( 
            /* [in] */ BSTR album) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_SortAlbumArtist( 
            /* [retval][out] */ BSTR *albumArtist) = 0;
        
        virtual /* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE put_SortAlbumArtist( 
            /* [in] */ BSTR albumArtist) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_SortArtist( 
            /* [retval][out] */ BSTR *artist) = 0;
        
        virtual /* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE put_SortArtist( 
            /* [in] */ BSTR artist) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_SortComposer( 
            /* [retval][out] */ BSTR *composer) = 0;
        
        virtual /* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE put_SortComposer( 
            /* [in] */ BSTR composer) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_SortName( 
            /* [retval][out] */ BSTR *name) = 0;
        
        virtual /* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE put_SortName( 
            /* [in] */ BSTR name) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_SortShow( 
            /* [retval][out] */ BSTR *showName) = 0;
        
        virtual /* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE put_SortShow( 
            /* [in] */ BSTR showName) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE Reveal( void) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_AlbumRating( 
            /* [retval][out] */ long *rating) = 0;
        
        virtual /* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE put_AlbumRating( 
            /* [in] */ long rating) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_AlbumRatingKind( 
            /* [retval][out] */ ITRatingKind *ratingKind) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_RatingKind( 
            /* [retval][out] */ ITRatingKind *ratingKind) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_Playlists( 
            /* [retval][out] */ IITPlaylistCollection **iPlaylistCollection) = 0;
        
        virtual /* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE put_Location( 
            /* [in] */ BSTR location) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_ReleaseDate( 
            /* [retval][out] */ DATE *releaseDate) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IITFileOrCDTrackVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IITFileOrCDTrack * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IITFileOrCDTrack * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IITFileOrCDTrack * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IITFileOrCDTrack * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IITFileOrCDTrack * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IITFileOrCDTrack * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IITFileOrCDTrack * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *GetITObjectIDs )( 
            IITFileOrCDTrack * This,
            /* [out] */ long *sourceID,
            /* [out] */ long *playlistID,
            /* [out] */ long *trackID,
            /* [out] */ long *databaseID);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Name )( 
            IITFileOrCDTrack * This,
            /* [retval][out] */ BSTR *name);
        
        /* [helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Name )( 
            IITFileOrCDTrack * This,
            /* [in] */ BSTR name);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Index )( 
            IITFileOrCDTrack * This,
            /* [retval][out] */ long *index);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_SourceID )( 
            IITFileOrCDTrack * This,
            /* [retval][out] */ long *sourceID);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_PlaylistID )( 
            IITFileOrCDTrack * This,
            /* [retval][out] */ long *playlistID);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_TrackID )( 
            IITFileOrCDTrack * This,
            /* [retval][out] */ long *trackID);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_TrackDatabaseID )( 
            IITFileOrCDTrack * This,
            /* [retval][out] */ long *databaseID);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *Delete )( 
            IITFileOrCDTrack * This);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *Play )( 
            IITFileOrCDTrack * This);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *AddArtworkFromFile )( 
            IITFileOrCDTrack * This,
            /* [in] */ BSTR filePath,
            /* [retval][out] */ IITArtwork **iArtwork);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Kind )( 
            IITFileOrCDTrack * This,
            /* [retval][out] */ ITTrackKind *kind);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Playlist )( 
            IITFileOrCDTrack * This,
            /* [retval][out] */ IITPlaylist **iPlaylist);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Album )( 
            IITFileOrCDTrack * This,
            /* [retval][out] */ BSTR *album);
        
        /* [helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Album )( 
            IITFileOrCDTrack * This,
            /* [in] */ BSTR album);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Artist )( 
            IITFileOrCDTrack * This,
            /* [retval][out] */ BSTR *artist);
        
        /* [helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Artist )( 
            IITFileOrCDTrack * This,
            /* [in] */ BSTR artist);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_BitRate )( 
            IITFileOrCDTrack * This,
            /* [retval][out] */ long *bitrate);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_BPM )( 
            IITFileOrCDTrack * This,
            /* [retval][out] */ long *beatsPerMinute);
        
        /* [helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE *put_BPM )( 
            IITFileOrCDTrack * This,
            /* [in] */ long beatsPerMinute);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Comment )( 
            IITFileOrCDTrack * This,
            /* [retval][out] */ BSTR *comment);
        
        /* [helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Comment )( 
            IITFileOrCDTrack * This,
            /* [in] */ BSTR comment);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Compilation )( 
            IITFileOrCDTrack * This,
            /* [retval][out] */ VARIANT_BOOL *isCompilation);
        
        /* [helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Compilation )( 
            IITFileOrCDTrack * This,
            /* [in] */ VARIANT_BOOL shouldBeCompilation);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Composer )( 
            IITFileOrCDTrack * This,
            /* [retval][out] */ BSTR *composer);
        
        /* [helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Composer )( 
            IITFileOrCDTrack * This,
            /* [in] */ BSTR composer);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_DateAdded )( 
            IITFileOrCDTrack * This,
            /* [retval][out] */ DATE *dateAdded);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_DiscCount )( 
            IITFileOrCDTrack * This,
            /* [retval][out] */ long *discCount);
        
        /* [helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE *put_DiscCount )( 
            IITFileOrCDTrack * This,
            /* [in] */ long discCount);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_DiscNumber )( 
            IITFileOrCDTrack * This,
            /* [retval][out] */ long *discNumber);
        
        /* [helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE *put_DiscNumber )( 
            IITFileOrCDTrack * This,
            /* [in] */ long discNumber);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Duration )( 
            IITFileOrCDTrack * This,
            /* [retval][out] */ long *duration);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Enabled )( 
            IITFileOrCDTrack * This,
            /* [retval][out] */ VARIANT_BOOL *isEnabled);
        
        /* [helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Enabled )( 
            IITFileOrCDTrack * This,
            /* [in] */ VARIANT_BOOL shouldBeEnabled);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_EQ )( 
            IITFileOrCDTrack * This,
            /* [retval][out] */ BSTR *eq);
        
        /* [helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE *put_EQ )( 
            IITFileOrCDTrack * This,
            /* [in] */ BSTR eq);
        
        /* [helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Finish )( 
            IITFileOrCDTrack * This,
            /* [in] */ long finish);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Finish )( 
            IITFileOrCDTrack * This,
            /* [retval][out] */ long *finish);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Genre )( 
            IITFileOrCDTrack * This,
            /* [retval][out] */ BSTR *genre);
        
        /* [helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Genre )( 
            IITFileOrCDTrack * This,
            /* [in] */ BSTR genre);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Grouping )( 
            IITFileOrCDTrack * This,
            /* [retval][out] */ BSTR *grouping);
        
        /* [helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Grouping )( 
            IITFileOrCDTrack * This,
            /* [in] */ BSTR grouping);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_KindAsString )( 
            IITFileOrCDTrack * This,
            /* [retval][out] */ BSTR *kind);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_ModificationDate )( 
            IITFileOrCDTrack * This,
            /* [retval][out] */ DATE *dateModified);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_PlayedCount )( 
            IITFileOrCDTrack * This,
            /* [retval][out] */ long *playedCount);
        
        /* [helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE *put_PlayedCount )( 
            IITFileOrCDTrack * This,
            /* [in] */ long playedCount);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_PlayedDate )( 
            IITFileOrCDTrack * This,
            /* [retval][out] */ DATE *playedDate);
        
        /* [helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE *put_PlayedDate )( 
            IITFileOrCDTrack * This,
            /* [in] */ DATE playedDate);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_PlayOrderIndex )( 
            IITFileOrCDTrack * This,
            /* [retval][out] */ long *index);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Rating )( 
            IITFileOrCDTrack * This,
            /* [retval][out] */ long *rating);
        
        /* [helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Rating )( 
            IITFileOrCDTrack * This,
            /* [in] */ long rating);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_SampleRate )( 
            IITFileOrCDTrack * This,
            /* [retval][out] */ long *sampleRate);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Size )( 
            IITFileOrCDTrack * This,
            /* [retval][out] */ long *size);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Start )( 
            IITFileOrCDTrack * This,
            /* [retval][out] */ long *start);
        
        /* [helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Start )( 
            IITFileOrCDTrack * This,
            /* [in] */ long start);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Time )( 
            IITFileOrCDTrack * This,
            /* [retval][out] */ BSTR *time);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_TrackCount )( 
            IITFileOrCDTrack * This,
            /* [retval][out] */ long *trackCount);
        
        /* [helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE *put_TrackCount )( 
            IITFileOrCDTrack * This,
            /* [in] */ long trackCount);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_TrackNumber )( 
            IITFileOrCDTrack * This,
            /* [retval][out] */ long *trackNumber);
        
        /* [helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE *put_TrackNumber )( 
            IITFileOrCDTrack * This,
            /* [in] */ long trackNumber);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_VolumeAdjustment )( 
            IITFileOrCDTrack * This,
            /* [retval][out] */ long *volumeAdjustment);
        
        /* [helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE *put_VolumeAdjustment )( 
            IITFileOrCDTrack * This,
            /* [in] */ long volumeAdjustment);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Year )( 
            IITFileOrCDTrack * This,
            /* [retval][out] */ long *year);
        
        /* [helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Year )( 
            IITFileOrCDTrack * This,
            /* [in] */ long year);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Artwork )( 
            IITFileOrCDTrack * This,
            /* [retval][out] */ IITArtworkCollection **iArtworkCollection);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Location )( 
            IITFileOrCDTrack * This,
            /* [retval][out] */ BSTR *location);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *UpdateInfoFromFile )( 
            IITFileOrCDTrack * This);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Podcast )( 
            IITFileOrCDTrack * This,
            /* [retval][out] */ VARIANT_BOOL *isPodcast);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *UpdatePodcastFeed )( 
            IITFileOrCDTrack * This);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_RememberBookmark )( 
            IITFileOrCDTrack * This,
            /* [retval][out] */ VARIANT_BOOL *rememberBookmark);
        
        /* [helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE *put_RememberBookmark )( 
            IITFileOrCDTrack * This,
            /* [in] */ VARIANT_BOOL shouldRememberBookmark);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_ExcludeFromShuffle )( 
            IITFileOrCDTrack * This,
            /* [retval][out] */ VARIANT_BOOL *excludeFromShuffle);
        
        /* [helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE *put_ExcludeFromShuffle )( 
            IITFileOrCDTrack * This,
            /* [in] */ VARIANT_BOOL shouldExcludeFromShuffle);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Lyrics )( 
            IITFileOrCDTrack * This,
            /* [retval][out] */ BSTR *lyrics);
        
        /* [helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Lyrics )( 
            IITFileOrCDTrack * This,
            /* [in] */ BSTR lyrics);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Category )( 
            IITFileOrCDTrack * This,
            /* [retval][out] */ BSTR *category);
        
        /* [helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Category )( 
            IITFileOrCDTrack * This,
            /* [in] */ BSTR category);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Description )( 
            IITFileOrCDTrack * This,
            /* [retval][out] */ BSTR *description);
        
        /* [helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Description )( 
            IITFileOrCDTrack * This,
            /* [in] */ BSTR description);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_LongDescription )( 
            IITFileOrCDTrack * This,
            /* [retval][out] */ BSTR *longDescription);
        
        /* [helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE *put_LongDescription )( 
            IITFileOrCDTrack * This,
            /* [in] */ BSTR longDescription);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_BookmarkTime )( 
            IITFileOrCDTrack * This,
            /* [retval][out] */ long *bookmarkTime);
        
        /* [helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE *put_BookmarkTime )( 
            IITFileOrCDTrack * This,
            /* [in] */ long bookmarkTime);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_VideoKind )( 
            IITFileOrCDTrack * This,
            /* [retval][out] */ ITVideoKind *videoKind);
        
        /* [helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE *put_VideoKind )( 
            IITFileOrCDTrack * This,
            /* [in] */ ITVideoKind videoKind);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_SkippedCount )( 
            IITFileOrCDTrack * This,
            /* [retval][out] */ long *skippedCount);
        
        /* [helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE *put_SkippedCount )( 
            IITFileOrCDTrack * This,
            /* [in] */ long skippedCount);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_SkippedDate )( 
            IITFileOrCDTrack * This,
            /* [retval][out] */ DATE *skippedDate);
        
        /* [helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE *put_SkippedDate )( 
            IITFileOrCDTrack * This,
            /* [in] */ DATE skippedDate);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_PartOfGaplessAlbum )( 
            IITFileOrCDTrack * This,
            /* [retval][out] */ VARIANT_BOOL *partOfGaplessAlbum);
        
        /* [helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE *put_PartOfGaplessAlbum )( 
            IITFileOrCDTrack * This,
            /* [in] */ VARIANT_BOOL shouldBePartOfGaplessAlbum);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_AlbumArtist )( 
            IITFileOrCDTrack * This,
            /* [retval][out] */ BSTR *albumArtist);
        
        /* [helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE *put_AlbumArtist )( 
            IITFileOrCDTrack * This,
            /* [in] */ BSTR albumArtist);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Show )( 
            IITFileOrCDTrack * This,
            /* [retval][out] */ BSTR *showName);
        
        /* [helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Show )( 
            IITFileOrCDTrack * This,
            /* [in] */ BSTR showName);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_SeasonNumber )( 
            IITFileOrCDTrack * This,
            /* [retval][out] */ long *seasonNumber);
        
        /* [helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE *put_SeasonNumber )( 
            IITFileOrCDTrack * This,
            /* [in] */ long seasonNumber);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_EpisodeID )( 
            IITFileOrCDTrack * This,
            /* [retval][out] */ BSTR *episodeID);
        
        /* [helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE *put_EpisodeID )( 
            IITFileOrCDTrack * This,
            /* [in] */ BSTR episodeID);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_EpisodeNumber )( 
            IITFileOrCDTrack * This,
            /* [retval][out] */ long *episodeNumber);
        
        /* [helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE *put_EpisodeNumber )( 
            IITFileOrCDTrack * This,
            /* [in] */ long episodeNumber);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Size64High )( 
            IITFileOrCDTrack * This,
            /* [retval][out] */ long *sizeHigh);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Size64Low )( 
            IITFileOrCDTrack * This,
            /* [retval][out] */ long *sizeLow);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Unplayed )( 
            IITFileOrCDTrack * This,
            /* [retval][out] */ VARIANT_BOOL *isUnplayed);
        
        /* [helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Unplayed )( 
            IITFileOrCDTrack * This,
            /* [in] */ VARIANT_BOOL shouldBeUnplayed);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_SortAlbum )( 
            IITFileOrCDTrack * This,
            /* [retval][out] */ BSTR *album);
        
        /* [helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE *put_SortAlbum )( 
            IITFileOrCDTrack * This,
            /* [in] */ BSTR album);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_SortAlbumArtist )( 
            IITFileOrCDTrack * This,
            /* [retval][out] */ BSTR *albumArtist);
        
        /* [helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE *put_SortAlbumArtist )( 
            IITFileOrCDTrack * This,
            /* [in] */ BSTR albumArtist);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_SortArtist )( 
            IITFileOrCDTrack * This,
            /* [retval][out] */ BSTR *artist);
        
        /* [helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE *put_SortArtist )( 
            IITFileOrCDTrack * This,
            /* [in] */ BSTR artist);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_SortComposer )( 
            IITFileOrCDTrack * This,
            /* [retval][out] */ BSTR *composer);
        
        /* [helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE *put_SortComposer )( 
            IITFileOrCDTrack * This,
            /* [in] */ BSTR composer);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_SortName )( 
            IITFileOrCDTrack * This,
            /* [retval][out] */ BSTR *name);
        
        /* [helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE *put_SortName )( 
            IITFileOrCDTrack * This,
            /* [in] */ BSTR name);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_SortShow )( 
            IITFileOrCDTrack * This,
            /* [retval][out] */ BSTR *showName);
        
        /* [helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE *put_SortShow )( 
            IITFileOrCDTrack * This,
            /* [in] */ BSTR showName);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *Reveal )( 
            IITFileOrCDTrack * This);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_AlbumRating )( 
            IITFileOrCDTrack * This,
            /* [retval][out] */ long *rating);
        
        /* [helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE *put_AlbumRating )( 
            IITFileOrCDTrack * This,
            /* [in] */ long rating);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_AlbumRatingKind )( 
            IITFileOrCDTrack * This,
            /* [retval][out] */ ITRatingKind *ratingKind);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_RatingKind )( 
            IITFileOrCDTrack * This,
            /* [retval][out] */ ITRatingKind *ratingKind);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Playlists )( 
            IITFileOrCDTrack * This,
            /* [retval][out] */ IITPlaylistCollection **iPlaylistCollection);
        
        /* [helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Location )( 
            IITFileOrCDTrack * This,
            /* [in] */ BSTR location);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_ReleaseDate )( 
            IITFileOrCDTrack * This,
            /* [retval][out] */ DATE *releaseDate);
        
        END_INTERFACE
    } IITFileOrCDTrackVtbl;

    interface IITFileOrCDTrack
    {
        CONST_VTBL struct IITFileOrCDTrackVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IITFileOrCDTrack_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IITFileOrCDTrack_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IITFileOrCDTrack_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IITFileOrCDTrack_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IITFileOrCDTrack_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IITFileOrCDTrack_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IITFileOrCDTrack_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IITFileOrCDTrack_GetITObjectIDs(This,sourceID,playlistID,trackID,databaseID)	\
    (This)->lpVtbl -> GetITObjectIDs(This,sourceID,playlistID,trackID,databaseID)

#define IITFileOrCDTrack_get_Name(This,name)	\
    (This)->lpVtbl -> get_Name(This,name)

#define IITFileOrCDTrack_put_Name(This,name)	\
    (This)->lpVtbl -> put_Name(This,name)

#define IITFileOrCDTrack_get_Index(This,index)	\
    (This)->lpVtbl -> get_Index(This,index)

#define IITFileOrCDTrack_get_SourceID(This,sourceID)	\
    (This)->lpVtbl -> get_SourceID(This,sourceID)

#define IITFileOrCDTrack_get_PlaylistID(This,playlistID)	\
    (This)->lpVtbl -> get_PlaylistID(This,playlistID)

#define IITFileOrCDTrack_get_TrackID(This,trackID)	\
    (This)->lpVtbl -> get_TrackID(This,trackID)

#define IITFileOrCDTrack_get_TrackDatabaseID(This,databaseID)	\
    (This)->lpVtbl -> get_TrackDatabaseID(This,databaseID)


#define IITFileOrCDTrack_Delete(This)	\
    (This)->lpVtbl -> Delete(This)

#define IITFileOrCDTrack_Play(This)	\
    (This)->lpVtbl -> Play(This)

#define IITFileOrCDTrack_AddArtworkFromFile(This,filePath,iArtwork)	\
    (This)->lpVtbl -> AddArtworkFromFile(This,filePath,iArtwork)

#define IITFileOrCDTrack_get_Kind(This,kind)	\
    (This)->lpVtbl -> get_Kind(This,kind)

#define IITFileOrCDTrack_get_Playlist(This,iPlaylist)	\
    (This)->lpVtbl -> get_Playlist(This,iPlaylist)

#define IITFileOrCDTrack_get_Album(This,album)	\
    (This)->lpVtbl -> get_Album(This,album)

#define IITFileOrCDTrack_put_Album(This,album)	\
    (This)->lpVtbl -> put_Album(This,album)

#define IITFileOrCDTrack_get_Artist(This,artist)	\
    (This)->lpVtbl -> get_Artist(This,artist)

#define IITFileOrCDTrack_put_Artist(This,artist)	\
    (This)->lpVtbl -> put_Artist(This,artist)

#define IITFileOrCDTrack_get_BitRate(This,bitrate)	\
    (This)->lpVtbl -> get_BitRate(This,bitrate)

#define IITFileOrCDTrack_get_BPM(This,beatsPerMinute)	\
    (This)->lpVtbl -> get_BPM(This,beatsPerMinute)

#define IITFileOrCDTrack_put_BPM(This,beatsPerMinute)	\
    (This)->lpVtbl -> put_BPM(This,beatsPerMinute)

#define IITFileOrCDTrack_get_Comment(This,comment)	\
    (This)->lpVtbl -> get_Comment(This,comment)

#define IITFileOrCDTrack_put_Comment(This,comment)	\
    (This)->lpVtbl -> put_Comment(This,comment)

#define IITFileOrCDTrack_get_Compilation(This,isCompilation)	\
    (This)->lpVtbl -> get_Compilation(This,isCompilation)

#define IITFileOrCDTrack_put_Compilation(This,shouldBeCompilation)	\
    (This)->lpVtbl -> put_Compilation(This,shouldBeCompilation)

#define IITFileOrCDTrack_get_Composer(This,composer)	\
    (This)->lpVtbl -> get_Composer(This,composer)

#define IITFileOrCDTrack_put_Composer(This,composer)	\
    (This)->lpVtbl -> put_Composer(This,composer)

#define IITFileOrCDTrack_get_DateAdded(This,dateAdded)	\
    (This)->lpVtbl -> get_DateAdded(This,dateAdded)

#define IITFileOrCDTrack_get_DiscCount(This,discCount)	\
    (This)->lpVtbl -> get_DiscCount(This,discCount)

#define IITFileOrCDTrack_put_DiscCount(This,discCount)	\
    (This)->lpVtbl -> put_DiscCount(This,discCount)

#define IITFileOrCDTrack_get_DiscNumber(This,discNumber)	\
    (This)->lpVtbl -> get_DiscNumber(This,discNumber)

#define IITFileOrCDTrack_put_DiscNumber(This,discNumber)	\
    (This)->lpVtbl -> put_DiscNumber(This,discNumber)

#define IITFileOrCDTrack_get_Duration(This,duration)	\
    (This)->lpVtbl -> get_Duration(This,duration)

#define IITFileOrCDTrack_get_Enabled(This,isEnabled)	\
    (This)->lpVtbl -> get_Enabled(This,isEnabled)

#define IITFileOrCDTrack_put_Enabled(This,shouldBeEnabled)	\
    (This)->lpVtbl -> put_Enabled(This,shouldBeEnabled)

#define IITFileOrCDTrack_get_EQ(This,eq)	\
    (This)->lpVtbl -> get_EQ(This,eq)

#define IITFileOrCDTrack_put_EQ(This,eq)	\
    (This)->lpVtbl -> put_EQ(This,eq)

#define IITFileOrCDTrack_put_Finish(This,finish)	\
    (This)->lpVtbl -> put_Finish(This,finish)

#define IITFileOrCDTrack_get_Finish(This,finish)	\
    (This)->lpVtbl -> get_Finish(This,finish)

#define IITFileOrCDTrack_get_Genre(This,genre)	\
    (This)->lpVtbl -> get_Genre(This,genre)

#define IITFileOrCDTrack_put_Genre(This,genre)	\
    (This)->lpVtbl -> put_Genre(This,genre)

#define IITFileOrCDTrack_get_Grouping(This,grouping)	\
    (This)->lpVtbl -> get_Grouping(This,grouping)

#define IITFileOrCDTrack_put_Grouping(This,grouping)	\
    (This)->lpVtbl -> put_Grouping(This,grouping)

#define IITFileOrCDTrack_get_KindAsString(This,kind)	\
    (This)->lpVtbl -> get_KindAsString(This,kind)

#define IITFileOrCDTrack_get_ModificationDate(This,dateModified)	\
    (This)->lpVtbl -> get_ModificationDate(This,dateModified)

#define IITFileOrCDTrack_get_PlayedCount(This,playedCount)	\
    (This)->lpVtbl -> get_PlayedCount(This,playedCount)

#define IITFileOrCDTrack_put_PlayedCount(This,playedCount)	\
    (This)->lpVtbl -> put_PlayedCount(This,playedCount)

#define IITFileOrCDTrack_get_PlayedDate(This,playedDate)	\
    (This)->lpVtbl -> get_PlayedDate(This,playedDate)

#define IITFileOrCDTrack_put_PlayedDate(This,playedDate)	\
    (This)->lpVtbl -> put_PlayedDate(This,playedDate)

#define IITFileOrCDTrack_get_PlayOrderIndex(This,index)	\
    (This)->lpVtbl -> get_PlayOrderIndex(This,index)

#define IITFileOrCDTrack_get_Rating(This,rating)	\
    (This)->lpVtbl -> get_Rating(This,rating)

#define IITFileOrCDTrack_put_Rating(This,rating)	\
    (This)->lpVtbl -> put_Rating(This,rating)

#define IITFileOrCDTrack_get_SampleRate(This,sampleRate)	\
    (This)->lpVtbl -> get_SampleRate(This,sampleRate)

#define IITFileOrCDTrack_get_Size(This,size)	\
    (This)->lpVtbl -> get_Size(This,size)

#define IITFileOrCDTrack_get_Start(This,start)	\
    (This)->lpVtbl -> get_Start(This,start)

#define IITFileOrCDTrack_put_Start(This,start)	\
    (This)->lpVtbl -> put_Start(This,start)

#define IITFileOrCDTrack_get_Time(This,time)	\
    (This)->lpVtbl -> get_Time(This,time)

#define IITFileOrCDTrack_get_TrackCount(This,trackCount)	\
    (This)->lpVtbl -> get_TrackCount(This,trackCount)

#define IITFileOrCDTrack_put_TrackCount(This,trackCount)	\
    (This)->lpVtbl -> put_TrackCount(This,trackCount)

#define IITFileOrCDTrack_get_TrackNumber(This,trackNumber)	\
    (This)->lpVtbl -> get_TrackNumber(This,trackNumber)

#define IITFileOrCDTrack_put_TrackNumber(This,trackNumber)	\
    (This)->lpVtbl -> put_TrackNumber(This,trackNumber)

#define IITFileOrCDTrack_get_VolumeAdjustment(This,volumeAdjustment)	\
    (This)->lpVtbl -> get_VolumeAdjustment(This,volumeAdjustment)

#define IITFileOrCDTrack_put_VolumeAdjustment(This,volumeAdjustment)	\
    (This)->lpVtbl -> put_VolumeAdjustment(This,volumeAdjustment)

#define IITFileOrCDTrack_get_Year(This,year)	\
    (This)->lpVtbl -> get_Year(This,year)

#define IITFileOrCDTrack_put_Year(This,year)	\
    (This)->lpVtbl -> put_Year(This,year)

#define IITFileOrCDTrack_get_Artwork(This,iArtworkCollection)	\
    (This)->lpVtbl -> get_Artwork(This,iArtworkCollection)


#define IITFileOrCDTrack_get_Location(This,location)	\
    (This)->lpVtbl -> get_Location(This,location)

#define IITFileOrCDTrack_UpdateInfoFromFile(This)	\
    (This)->lpVtbl -> UpdateInfoFromFile(This)

#define IITFileOrCDTrack_get_Podcast(This,isPodcast)	\
    (This)->lpVtbl -> get_Podcast(This,isPodcast)

#define IITFileOrCDTrack_UpdatePodcastFeed(This)	\
    (This)->lpVtbl -> UpdatePodcastFeed(This)

#define IITFileOrCDTrack_get_RememberBookmark(This,rememberBookmark)	\
    (This)->lpVtbl -> get_RememberBookmark(This,rememberBookmark)

#define IITFileOrCDTrack_put_RememberBookmark(This,shouldRememberBookmark)	\
    (This)->lpVtbl -> put_RememberBookmark(This,shouldRememberBookmark)

#define IITFileOrCDTrack_get_ExcludeFromShuffle(This,excludeFromShuffle)	\
    (This)->lpVtbl -> get_ExcludeFromShuffle(This,excludeFromShuffle)

#define IITFileOrCDTrack_put_ExcludeFromShuffle(This,shouldExcludeFromShuffle)	\
    (This)->lpVtbl -> put_ExcludeFromShuffle(This,shouldExcludeFromShuffle)

#define IITFileOrCDTrack_get_Lyrics(This,lyrics)	\
    (This)->lpVtbl -> get_Lyrics(This,lyrics)

#define IITFileOrCDTrack_put_Lyrics(This,lyrics)	\
    (This)->lpVtbl -> put_Lyrics(This,lyrics)

#define IITFileOrCDTrack_get_Category(This,category)	\
    (This)->lpVtbl -> get_Category(This,category)

#define IITFileOrCDTrack_put_Category(This,category)	\
    (This)->lpVtbl -> put_Category(This,category)

#define IITFileOrCDTrack_get_Description(This,description)	\
    (This)->lpVtbl -> get_Description(This,description)

#define IITFileOrCDTrack_put_Description(This,description)	\
    (This)->lpVtbl -> put_Description(This,description)

#define IITFileOrCDTrack_get_LongDescription(This,longDescription)	\
    (This)->lpVtbl -> get_LongDescription(This,longDescription)

#define IITFileOrCDTrack_put_LongDescription(This,longDescription)	\
    (This)->lpVtbl -> put_LongDescription(This,longDescription)

#define IITFileOrCDTrack_get_BookmarkTime(This,bookmarkTime)	\
    (This)->lpVtbl -> get_BookmarkTime(This,bookmarkTime)

#define IITFileOrCDTrack_put_BookmarkTime(This,bookmarkTime)	\
    (This)->lpVtbl -> put_BookmarkTime(This,bookmarkTime)

#define IITFileOrCDTrack_get_VideoKind(This,videoKind)	\
    (This)->lpVtbl -> get_VideoKind(This,videoKind)

#define IITFileOrCDTrack_put_VideoKind(This,videoKind)	\
    (This)->lpVtbl -> put_VideoKind(This,videoKind)

#define IITFileOrCDTrack_get_SkippedCount(This,skippedCount)	\
    (This)->lpVtbl -> get_SkippedCount(This,skippedCount)

#define IITFileOrCDTrack_put_SkippedCount(This,skippedCount)	\
    (This)->lpVtbl -> put_SkippedCount(This,skippedCount)

#define IITFileOrCDTrack_get_SkippedDate(This,skippedDate)	\
    (This)->lpVtbl -> get_SkippedDate(This,skippedDate)

#define IITFileOrCDTrack_put_SkippedDate(This,skippedDate)	\
    (This)->lpVtbl -> put_SkippedDate(This,skippedDate)

#define IITFileOrCDTrack_get_PartOfGaplessAlbum(This,partOfGaplessAlbum)	\
    (This)->lpVtbl -> get_PartOfGaplessAlbum(This,partOfGaplessAlbum)

#define IITFileOrCDTrack_put_PartOfGaplessAlbum(This,shouldBePartOfGaplessAlbum)	\
    (This)->lpVtbl -> put_PartOfGaplessAlbum(This,shouldBePartOfGaplessAlbum)

#define IITFileOrCDTrack_get_AlbumArtist(This,albumArtist)	\
    (This)->lpVtbl -> get_AlbumArtist(This,albumArtist)

#define IITFileOrCDTrack_put_AlbumArtist(This,albumArtist)	\
    (This)->lpVtbl -> put_AlbumArtist(This,albumArtist)

#define IITFileOrCDTrack_get_Show(This,showName)	\
    (This)->lpVtbl -> get_Show(This,showName)

#define IITFileOrCDTrack_put_Show(This,showName)	\
    (This)->lpVtbl -> put_Show(This,showName)

#define IITFileOrCDTrack_get_SeasonNumber(This,seasonNumber)	\
    (This)->lpVtbl -> get_SeasonNumber(This,seasonNumber)

#define IITFileOrCDTrack_put_SeasonNumber(This,seasonNumber)	\
    (This)->lpVtbl -> put_SeasonNumber(This,seasonNumber)

#define IITFileOrCDTrack_get_EpisodeID(This,episodeID)	\
    (This)->lpVtbl -> get_EpisodeID(This,episodeID)

#define IITFileOrCDTrack_put_EpisodeID(This,episodeID)	\
    (This)->lpVtbl -> put_EpisodeID(This,episodeID)

#define IITFileOrCDTrack_get_EpisodeNumber(This,episodeNumber)	\
    (This)->lpVtbl -> get_EpisodeNumber(This,episodeNumber)

#define IITFileOrCDTrack_put_EpisodeNumber(This,episodeNumber)	\
    (This)->lpVtbl -> put_EpisodeNumber(This,episodeNumber)

#define IITFileOrCDTrack_get_Size64High(This,sizeHigh)	\
    (This)->lpVtbl -> get_Size64High(This,sizeHigh)

#define IITFileOrCDTrack_get_Size64Low(This,sizeLow)	\
    (This)->lpVtbl -> get_Size64Low(This,sizeLow)

#define IITFileOrCDTrack_get_Unplayed(This,isUnplayed)	\
    (This)->lpVtbl -> get_Unplayed(This,isUnplayed)

#define IITFileOrCDTrack_put_Unplayed(This,shouldBeUnplayed)	\
    (This)->lpVtbl -> put_Unplayed(This,shouldBeUnplayed)

#define IITFileOrCDTrack_get_SortAlbum(This,album)	\
    (This)->lpVtbl -> get_SortAlbum(This,album)

#define IITFileOrCDTrack_put_SortAlbum(This,album)	\
    (This)->lpVtbl -> put_SortAlbum(This,album)

#define IITFileOrCDTrack_get_SortAlbumArtist(This,albumArtist)	\
    (This)->lpVtbl -> get_SortAlbumArtist(This,albumArtist)

#define IITFileOrCDTrack_put_SortAlbumArtist(This,albumArtist)	\
    (This)->lpVtbl -> put_SortAlbumArtist(This,albumArtist)

#define IITFileOrCDTrack_get_SortArtist(This,artist)	\
    (This)->lpVtbl -> get_SortArtist(This,artist)

#define IITFileOrCDTrack_put_SortArtist(This,artist)	\
    (This)->lpVtbl -> put_SortArtist(This,artist)

#define IITFileOrCDTrack_get_SortComposer(This,composer)	\
    (This)->lpVtbl -> get_SortComposer(This,composer)

#define IITFileOrCDTrack_put_SortComposer(This,composer)	\
    (This)->lpVtbl -> put_SortComposer(This,composer)

#define IITFileOrCDTrack_get_SortName(This,name)	\
    (This)->lpVtbl -> get_SortName(This,name)

#define IITFileOrCDTrack_put_SortName(This,name)	\
    (This)->lpVtbl -> put_SortName(This,name)

#define IITFileOrCDTrack_get_SortShow(This,showName)	\
    (This)->lpVtbl -> get_SortShow(This,showName)

#define IITFileOrCDTrack_put_SortShow(This,showName)	\
    (This)->lpVtbl -> put_SortShow(This,showName)

#define IITFileOrCDTrack_Reveal(This)	\
    (This)->lpVtbl -> Reveal(This)

#define IITFileOrCDTrack_get_AlbumRating(This,rating)	\
    (This)->lpVtbl -> get_AlbumRating(This,rating)

#define IITFileOrCDTrack_put_AlbumRating(This,rating)	\
    (This)->lpVtbl -> put_AlbumRating(This,rating)

#define IITFileOrCDTrack_get_AlbumRatingKind(This,ratingKind)	\
    (This)->lpVtbl -> get_AlbumRatingKind(This,ratingKind)

#define IITFileOrCDTrack_get_RatingKind(This,ratingKind)	\
    (This)->lpVtbl -> get_RatingKind(This,ratingKind)

#define IITFileOrCDTrack_get_Playlists(This,iPlaylistCollection)	\
    (This)->lpVtbl -> get_Playlists(This,iPlaylistCollection)

#define IITFileOrCDTrack_put_Location(This,location)	\
    (This)->lpVtbl -> put_Location(This,location)

#define IITFileOrCDTrack_get_ReleaseDate(This,releaseDate)	\
    (This)->lpVtbl -> get_ReleaseDate(This,releaseDate)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IITFileOrCDTrack_get_Location_Proxy( 
    IITFileOrCDTrack * This,
    /* [retval][out] */ BSTR *location);


void __RPC_STUB IITFileOrCDTrack_get_Location_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IITFileOrCDTrack_UpdateInfoFromFile_Proxy( 
    IITFileOrCDTrack * This);


void __RPC_STUB IITFileOrCDTrack_UpdateInfoFromFile_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IITFileOrCDTrack_get_Podcast_Proxy( 
    IITFileOrCDTrack * This,
    /* [retval][out] */ VARIANT_BOOL *isPodcast);


void __RPC_STUB IITFileOrCDTrack_get_Podcast_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IITFileOrCDTrack_UpdatePodcastFeed_Proxy( 
    IITFileOrCDTrack * This);


void __RPC_STUB IITFileOrCDTrack_UpdatePodcastFeed_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IITFileOrCDTrack_get_RememberBookmark_Proxy( 
    IITFileOrCDTrack * This,
    /* [retval][out] */ VARIANT_BOOL *rememberBookmark);


void __RPC_STUB IITFileOrCDTrack_get_RememberBookmark_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE IITFileOrCDTrack_put_RememberBookmark_Proxy( 
    IITFileOrCDTrack * This,
    /* [in] */ VARIANT_BOOL shouldRememberBookmark);


void __RPC_STUB IITFileOrCDTrack_put_RememberBookmark_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IITFileOrCDTrack_get_ExcludeFromShuffle_Proxy( 
    IITFileOrCDTrack * This,
    /* [retval][out] */ VARIANT_BOOL *excludeFromShuffle);


void __RPC_STUB IITFileOrCDTrack_get_ExcludeFromShuffle_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE IITFileOrCDTrack_put_ExcludeFromShuffle_Proxy( 
    IITFileOrCDTrack * This,
    /* [in] */ VARIANT_BOOL shouldExcludeFromShuffle);


void __RPC_STUB IITFileOrCDTrack_put_ExcludeFromShuffle_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IITFileOrCDTrack_get_Lyrics_Proxy( 
    IITFileOrCDTrack * This,
    /* [retval][out] */ BSTR *lyrics);


void __RPC_STUB IITFileOrCDTrack_get_Lyrics_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE IITFileOrCDTrack_put_Lyrics_Proxy( 
    IITFileOrCDTrack * This,
    /* [in] */ BSTR lyrics);


void __RPC_STUB IITFileOrCDTrack_put_Lyrics_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IITFileOrCDTrack_get_Category_Proxy( 
    IITFileOrCDTrack * This,
    /* [retval][out] */ BSTR *category);


void __RPC_STUB IITFileOrCDTrack_get_Category_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE IITFileOrCDTrack_put_Category_Proxy( 
    IITFileOrCDTrack * This,
    /* [in] */ BSTR category);


void __RPC_STUB IITFileOrCDTrack_put_Category_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IITFileOrCDTrack_get_Description_Proxy( 
    IITFileOrCDTrack * This,
    /* [retval][out] */ BSTR *description);


void __RPC_STUB IITFileOrCDTrack_get_Description_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE IITFileOrCDTrack_put_Description_Proxy( 
    IITFileOrCDTrack * This,
    /* [in] */ BSTR description);


void __RPC_STUB IITFileOrCDTrack_put_Description_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IITFileOrCDTrack_get_LongDescription_Proxy( 
    IITFileOrCDTrack * This,
    /* [retval][out] */ BSTR *longDescription);


void __RPC_STUB IITFileOrCDTrack_get_LongDescription_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE IITFileOrCDTrack_put_LongDescription_Proxy( 
    IITFileOrCDTrack * This,
    /* [in] */ BSTR longDescription);


void __RPC_STUB IITFileOrCDTrack_put_LongDescription_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IITFileOrCDTrack_get_BookmarkTime_Proxy( 
    IITFileOrCDTrack * This,
    /* [retval][out] */ long *bookmarkTime);


void __RPC_STUB IITFileOrCDTrack_get_BookmarkTime_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE IITFileOrCDTrack_put_BookmarkTime_Proxy( 
    IITFileOrCDTrack * This,
    /* [in] */ long bookmarkTime);


void __RPC_STUB IITFileOrCDTrack_put_BookmarkTime_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IITFileOrCDTrack_get_VideoKind_Proxy( 
    IITFileOrCDTrack * This,
    /* [retval][out] */ ITVideoKind *videoKind);


void __RPC_STUB IITFileOrCDTrack_get_VideoKind_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE IITFileOrCDTrack_put_VideoKind_Proxy( 
    IITFileOrCDTrack * This,
    /* [in] */ ITVideoKind videoKind);


void __RPC_STUB IITFileOrCDTrack_put_VideoKind_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IITFileOrCDTrack_get_SkippedCount_Proxy( 
    IITFileOrCDTrack * This,
    /* [retval][out] */ long *skippedCount);


void __RPC_STUB IITFileOrCDTrack_get_SkippedCount_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE IITFileOrCDTrack_put_SkippedCount_Proxy( 
    IITFileOrCDTrack * This,
    /* [in] */ long skippedCount);


void __RPC_STUB IITFileOrCDTrack_put_SkippedCount_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IITFileOrCDTrack_get_SkippedDate_Proxy( 
    IITFileOrCDTrack * This,
    /* [retval][out] */ DATE *skippedDate);


void __RPC_STUB IITFileOrCDTrack_get_SkippedDate_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE IITFileOrCDTrack_put_SkippedDate_Proxy( 
    IITFileOrCDTrack * This,
    /* [in] */ DATE skippedDate);


void __RPC_STUB IITFileOrCDTrack_put_SkippedDate_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IITFileOrCDTrack_get_PartOfGaplessAlbum_Proxy( 
    IITFileOrCDTrack * This,
    /* [retval][out] */ VARIANT_BOOL *partOfGaplessAlbum);


void __RPC_STUB IITFileOrCDTrack_get_PartOfGaplessAlbum_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE IITFileOrCDTrack_put_PartOfGaplessAlbum_Proxy( 
    IITFileOrCDTrack * This,
    /* [in] */ VARIANT_BOOL shouldBePartOfGaplessAlbum);


void __RPC_STUB IITFileOrCDTrack_put_PartOfGaplessAlbum_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IITFileOrCDTrack_get_AlbumArtist_Proxy( 
    IITFileOrCDTrack * This,
    /* [retval][out] */ BSTR *albumArtist);


void __RPC_STUB IITFileOrCDTrack_get_AlbumArtist_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE IITFileOrCDTrack_put_AlbumArtist_Proxy( 
    IITFileOrCDTrack * This,
    /* [in] */ BSTR albumArtist);


void __RPC_STUB IITFileOrCDTrack_put_AlbumArtist_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IITFileOrCDTrack_get_Show_Proxy( 
    IITFileOrCDTrack * This,
    /* [retval][out] */ BSTR *showName);


void __RPC_STUB IITFileOrCDTrack_get_Show_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE IITFileOrCDTrack_put_Show_Proxy( 
    IITFileOrCDTrack * This,
    /* [in] */ BSTR showName);


void __RPC_STUB IITFileOrCDTrack_put_Show_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IITFileOrCDTrack_get_SeasonNumber_Proxy( 
    IITFileOrCDTrack * This,
    /* [retval][out] */ long *seasonNumber);


void __RPC_STUB IITFileOrCDTrack_get_SeasonNumber_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE IITFileOrCDTrack_put_SeasonNumber_Proxy( 
    IITFileOrCDTrack * This,
    /* [in] */ long seasonNumber);


void __RPC_STUB IITFileOrCDTrack_put_SeasonNumber_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IITFileOrCDTrack_get_EpisodeID_Proxy( 
    IITFileOrCDTrack * This,
    /* [retval][out] */ BSTR *episodeID);


void __RPC_STUB IITFileOrCDTrack_get_EpisodeID_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE IITFileOrCDTrack_put_EpisodeID_Proxy( 
    IITFileOrCDTrack * This,
    /* [in] */ BSTR episodeID);


void __RPC_STUB IITFileOrCDTrack_put_EpisodeID_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IITFileOrCDTrack_get_EpisodeNumber_Proxy( 
    IITFileOrCDTrack * This,
    /* [retval][out] */ long *episodeNumber);


void __RPC_STUB IITFileOrCDTrack_get_EpisodeNumber_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE IITFileOrCDTrack_put_EpisodeNumber_Proxy( 
    IITFileOrCDTrack * This,
    /* [in] */ long episodeNumber);


void __RPC_STUB IITFileOrCDTrack_put_EpisodeNumber_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IITFileOrCDTrack_get_Size64High_Proxy( 
    IITFileOrCDTrack * This,
    /* [retval][out] */ long *sizeHigh);


void __RPC_STUB IITFileOrCDTrack_get_Size64High_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IITFileOrCDTrack_get_Size64Low_Proxy( 
    IITFileOrCDTrack * This,
    /* [retval][out] */ long *sizeLow);


void __RPC_STUB IITFileOrCDTrack_get_Size64Low_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IITFileOrCDTrack_get_Unplayed_Proxy( 
    IITFileOrCDTrack * This,
    /* [retval][out] */ VARIANT_BOOL *isUnplayed);


void __RPC_STUB IITFileOrCDTrack_get_Unplayed_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE IITFileOrCDTrack_put_Unplayed_Proxy( 
    IITFileOrCDTrack * This,
    /* [in] */ VARIANT_BOOL shouldBeUnplayed);


void __RPC_STUB IITFileOrCDTrack_put_Unplayed_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IITFileOrCDTrack_get_SortAlbum_Proxy( 
    IITFileOrCDTrack * This,
    /* [retval][out] */ BSTR *album);


void __RPC_STUB IITFileOrCDTrack_get_SortAlbum_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE IITFileOrCDTrack_put_SortAlbum_Proxy( 
    IITFileOrCDTrack * This,
    /* [in] */ BSTR album);


void __RPC_STUB IITFileOrCDTrack_put_SortAlbum_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IITFileOrCDTrack_get_SortAlbumArtist_Proxy( 
    IITFileOrCDTrack * This,
    /* [retval][out] */ BSTR *albumArtist);


void __RPC_STUB IITFileOrCDTrack_get_SortAlbumArtist_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE IITFileOrCDTrack_put_SortAlbumArtist_Proxy( 
    IITFileOrCDTrack * This,
    /* [in] */ BSTR albumArtist);


void __RPC_STUB IITFileOrCDTrack_put_SortAlbumArtist_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IITFileOrCDTrack_get_SortArtist_Proxy( 
    IITFileOrCDTrack * This,
    /* [retval][out] */ BSTR *artist);


void __RPC_STUB IITFileOrCDTrack_get_SortArtist_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE IITFileOrCDTrack_put_SortArtist_Proxy( 
    IITFileOrCDTrack * This,
    /* [in] */ BSTR artist);


void __RPC_STUB IITFileOrCDTrack_put_SortArtist_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IITFileOrCDTrack_get_SortComposer_Proxy( 
    IITFileOrCDTrack * This,
    /* [retval][out] */ BSTR *composer);


void __RPC_STUB IITFileOrCDTrack_get_SortComposer_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE IITFileOrCDTrack_put_SortComposer_Proxy( 
    IITFileOrCDTrack * This,
    /* [in] */ BSTR composer);


void __RPC_STUB IITFileOrCDTrack_put_SortComposer_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IITFileOrCDTrack_get_SortName_Proxy( 
    IITFileOrCDTrack * This,
    /* [retval][out] */ BSTR *name);


void __RPC_STUB IITFileOrCDTrack_get_SortName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE IITFileOrCDTrack_put_SortName_Proxy( 
    IITFileOrCDTrack * This,
    /* [in] */ BSTR name);


void __RPC_STUB IITFileOrCDTrack_put_SortName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IITFileOrCDTrack_get_SortShow_Proxy( 
    IITFileOrCDTrack * This,
    /* [retval][out] */ BSTR *showName);


void __RPC_STUB IITFileOrCDTrack_get_SortShow_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE IITFileOrCDTrack_put_SortShow_Proxy( 
    IITFileOrCDTrack * This,
    /* [in] */ BSTR showName);


void __RPC_STUB IITFileOrCDTrack_put_SortShow_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IITFileOrCDTrack_Reveal_Proxy( 
    IITFileOrCDTrack * This);


void __RPC_STUB IITFileOrCDTrack_Reveal_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IITFileOrCDTrack_get_AlbumRating_Proxy( 
    IITFileOrCDTrack * This,
    /* [retval][out] */ long *rating);


void __RPC_STUB IITFileOrCDTrack_get_AlbumRating_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE IITFileOrCDTrack_put_AlbumRating_Proxy( 
    IITFileOrCDTrack * This,
    /* [in] */ long rating);


void __RPC_STUB IITFileOrCDTrack_put_AlbumRating_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IITFileOrCDTrack_get_AlbumRatingKind_Proxy( 
    IITFileOrCDTrack * This,
    /* [retval][out] */ ITRatingKind *ratingKind);


void __RPC_STUB IITFileOrCDTrack_get_AlbumRatingKind_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IITFileOrCDTrack_get_RatingKind_Proxy( 
    IITFileOrCDTrack * This,
    /* [retval][out] */ ITRatingKind *ratingKind);


void __RPC_STUB IITFileOrCDTrack_get_RatingKind_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IITFileOrCDTrack_get_Playlists_Proxy( 
    IITFileOrCDTrack * This,
    /* [retval][out] */ IITPlaylistCollection **iPlaylistCollection);


void __RPC_STUB IITFileOrCDTrack_get_Playlists_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE IITFileOrCDTrack_put_Location_Proxy( 
    IITFileOrCDTrack * This,
    /* [in] */ BSTR location);


void __RPC_STUB IITFileOrCDTrack_put_Location_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IITFileOrCDTrack_get_ReleaseDate_Proxy( 
    IITFileOrCDTrack * This,
    /* [retval][out] */ DATE *releaseDate);


void __RPC_STUB IITFileOrCDTrack_get_ReleaseDate_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IITFileOrCDTrack_INTERFACE_DEFINED__ */


#ifndef __IITPlaylistWindow_INTERFACE_DEFINED__
#define __IITPlaylistWindow_INTERFACE_DEFINED__

/* interface IITPlaylistWindow */
/* [hidden][unique][helpstring][dual][uuid][object] */ 


EXTERN_C const IID IID_IITPlaylistWindow;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("349CBB45-2E5A-4822-8E4A-A75555A186F7")
    IITPlaylistWindow : public IITWindow
    {
    public:
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_SelectedTracks( 
            /* [retval][out] */ IITTrackCollection **iTrackCollection) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_Playlist( 
            /* [retval][out] */ IITPlaylist **iPlaylist) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IITPlaylistWindowVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IITPlaylistWindow * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IITPlaylistWindow * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IITPlaylistWindow * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IITPlaylistWindow * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IITPlaylistWindow * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IITPlaylistWindow * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IITPlaylistWindow * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Name )( 
            IITPlaylistWindow * This,
            /* [retval][out] */ BSTR *name);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Kind )( 
            IITPlaylistWindow * This,
            /* [retval][out] */ ITWindowKind *kind);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Visible )( 
            IITPlaylistWindow * This,
            /* [retval][out] */ VARIANT_BOOL *isVisible);
        
        /* [helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Visible )( 
            IITPlaylistWindow * This,
            /* [in] */ VARIANT_BOOL shouldBeVisible);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Resizable )( 
            IITPlaylistWindow * This,
            /* [retval][out] */ VARIANT_BOOL *isResizable);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Minimized )( 
            IITPlaylistWindow * This,
            /* [retval][out] */ VARIANT_BOOL *isMinimized);
        
        /* [helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Minimized )( 
            IITPlaylistWindow * This,
            /* [in] */ VARIANT_BOOL shouldBeMinimized);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Maximizable )( 
            IITPlaylistWindow * This,
            /* [retval][out] */ VARIANT_BOOL *isMaximizable);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Maximized )( 
            IITPlaylistWindow * This,
            /* [retval][out] */ VARIANT_BOOL *isMaximized);
        
        /* [helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Maximized )( 
            IITPlaylistWindow * This,
            /* [in] */ VARIANT_BOOL shouldBeMaximized);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Zoomable )( 
            IITPlaylistWindow * This,
            /* [retval][out] */ VARIANT_BOOL *isZoomable);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Zoomed )( 
            IITPlaylistWindow * This,
            /* [retval][out] */ VARIANT_BOOL *isZoomed);
        
        /* [helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Zoomed )( 
            IITPlaylistWindow * This,
            /* [in] */ VARIANT_BOOL shouldBeZoomed);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Top )( 
            IITPlaylistWindow * This,
            /* [retval][out] */ long *top);
        
        /* [helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Top )( 
            IITPlaylistWindow * This,
            /* [in] */ long top);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Left )( 
            IITPlaylistWindow * This,
            /* [retval][out] */ long *left);
        
        /* [helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Left )( 
            IITPlaylistWindow * This,
            /* [in] */ long left);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Bottom )( 
            IITPlaylistWindow * This,
            /* [retval][out] */ long *bottom);
        
        /* [helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Bottom )( 
            IITPlaylistWindow * This,
            /* [in] */ long bottom);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Right )( 
            IITPlaylistWindow * This,
            /* [retval][out] */ long *right);
        
        /* [helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Right )( 
            IITPlaylistWindow * This,
            /* [in] */ long right);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Width )( 
            IITPlaylistWindow * This,
            /* [retval][out] */ long *width);
        
        /* [helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Width )( 
            IITPlaylistWindow * This,
            /* [in] */ long width);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Height )( 
            IITPlaylistWindow * This,
            /* [retval][out] */ long *height);
        
        /* [helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Height )( 
            IITPlaylistWindow * This,
            /* [in] */ long height);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_SelectedTracks )( 
            IITPlaylistWindow * This,
            /* [retval][out] */ IITTrackCollection **iTrackCollection);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Playlist )( 
            IITPlaylistWindow * This,
            /* [retval][out] */ IITPlaylist **iPlaylist);
        
        END_INTERFACE
    } IITPlaylistWindowVtbl;

    interface IITPlaylistWindow
    {
        CONST_VTBL struct IITPlaylistWindowVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IITPlaylistWindow_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IITPlaylistWindow_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IITPlaylistWindow_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IITPlaylistWindow_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IITPlaylistWindow_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IITPlaylistWindow_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IITPlaylistWindow_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IITPlaylistWindow_get_Name(This,name)	\
    (This)->lpVtbl -> get_Name(This,name)

#define IITPlaylistWindow_get_Kind(This,kind)	\
    (This)->lpVtbl -> get_Kind(This,kind)

#define IITPlaylistWindow_get_Visible(This,isVisible)	\
    (This)->lpVtbl -> get_Visible(This,isVisible)

#define IITPlaylistWindow_put_Visible(This,shouldBeVisible)	\
    (This)->lpVtbl -> put_Visible(This,shouldBeVisible)

#define IITPlaylistWindow_get_Resizable(This,isResizable)	\
    (This)->lpVtbl -> get_Resizable(This,isResizable)

#define IITPlaylistWindow_get_Minimized(This,isMinimized)	\
    (This)->lpVtbl -> get_Minimized(This,isMinimized)

#define IITPlaylistWindow_put_Minimized(This,shouldBeMinimized)	\
    (This)->lpVtbl -> put_Minimized(This,shouldBeMinimized)

#define IITPlaylistWindow_get_Maximizable(This,isMaximizable)	\
    (This)->lpVtbl -> get_Maximizable(This,isMaximizable)

#define IITPlaylistWindow_get_Maximized(This,isMaximized)	\
    (This)->lpVtbl -> get_Maximized(This,isMaximized)

#define IITPlaylistWindow_put_Maximized(This,shouldBeMaximized)	\
    (This)->lpVtbl -> put_Maximized(This,shouldBeMaximized)

#define IITPlaylistWindow_get_Zoomable(This,isZoomable)	\
    (This)->lpVtbl -> get_Zoomable(This,isZoomable)

#define IITPlaylistWindow_get_Zoomed(This,isZoomed)	\
    (This)->lpVtbl -> get_Zoomed(This,isZoomed)

#define IITPlaylistWindow_put_Zoomed(This,shouldBeZoomed)	\
    (This)->lpVtbl -> put_Zoomed(This,shouldBeZoomed)

#define IITPlaylistWindow_get_Top(This,top)	\
    (This)->lpVtbl -> get_Top(This,top)

#define IITPlaylistWindow_put_Top(This,top)	\
    (This)->lpVtbl -> put_Top(This,top)

#define IITPlaylistWindow_get_Left(This,left)	\
    (This)->lpVtbl -> get_Left(This,left)

#define IITPlaylistWindow_put_Left(This,left)	\
    (This)->lpVtbl -> put_Left(This,left)

#define IITPlaylistWindow_get_Bottom(This,bottom)	\
    (This)->lpVtbl -> get_Bottom(This,bottom)

#define IITPlaylistWindow_put_Bottom(This,bottom)	\
    (This)->lpVtbl -> put_Bottom(This,bottom)

#define IITPlaylistWindow_get_Right(This,right)	\
    (This)->lpVtbl -> get_Right(This,right)

#define IITPlaylistWindow_put_Right(This,right)	\
    (This)->lpVtbl -> put_Right(This,right)

#define IITPlaylistWindow_get_Width(This,width)	\
    (This)->lpVtbl -> get_Width(This,width)

#define IITPlaylistWindow_put_Width(This,width)	\
    (This)->lpVtbl -> put_Width(This,width)

#define IITPlaylistWindow_get_Height(This,height)	\
    (This)->lpVtbl -> get_Height(This,height)

#define IITPlaylistWindow_put_Height(This,height)	\
    (This)->lpVtbl -> put_Height(This,height)


#define IITPlaylistWindow_get_SelectedTracks(This,iTrackCollection)	\
    (This)->lpVtbl -> get_SelectedTracks(This,iTrackCollection)

#define IITPlaylistWindow_get_Playlist(This,iPlaylist)	\
    (This)->lpVtbl -> get_Playlist(This,iPlaylist)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IITPlaylistWindow_get_SelectedTracks_Proxy( 
    IITPlaylistWindow * This,
    /* [retval][out] */ IITTrackCollection **iTrackCollection);


void __RPC_STUB IITPlaylistWindow_get_SelectedTracks_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IITPlaylistWindow_get_Playlist_Proxy( 
    IITPlaylistWindow * This,
    /* [retval][out] */ IITPlaylist **iPlaylist);


void __RPC_STUB IITPlaylistWindow_get_Playlist_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IITPlaylistWindow_INTERFACE_DEFINED__ */

#endif /* __iTunesLib_LIBRARY_DEFINED__ */

/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


