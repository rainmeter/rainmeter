/*
** Copyright (C) 2006 Nullsoft, Inc.
**
** This software is provided 'as-is', without any express or implied warranty. In no event will the authors be held 
** liable for any damages arising from the use of this software. 
**
** Permission is granted to anyone to use this software for any purpose, including commercial applications, and to 
** alter it and redistribute it freely, subject to the following restrictions:
**
**   1. The origin of this software must not be misrepresented; you must not claim that you wrote the original software. 
**      If you use this software in a product, an acknowledgment in the product documentation would be appreciated but is not required.
**
**   2. Altered source versions must be plainly marked as such, and must not be misrepresented as being the original software.
**
**   3. This notice may not be removed or altered from any source distribution.
**
*/

#ifndef _WA_IPC_H_
#define _WA_IPC_H_

#include <windows.h>
#include <stddef.h>
#if (_MSC_VER <= 1200)
typedef int intptr_t;
#endif
/*
** This is the modern replacement for the classic 'frontend.h'. Most of these 
** updates are designed for in-process use, i.e. from a plugin.
**
*/

/* message used to sent many messages to winamp's main window. 
** most all of the IPC_* messages involve sending the message in the form of:
**   result = SendMessage(hwnd_winamp,WM_WA_IPC,(parameter),IPC_*);
**
** When you use SendMessage(hwnd_winamp,WM_WA_IPC,(parameter),IPC_*) and specify a IPC_*
** which is not currently implemented/supported by the Winamp version being used then it
** will return 1 for 'result'. This is a good way of helping to check if an api being
** used which returns a function pointer, etc is even going to be valid.
*/
#define WM_WA_IPC WM_USER
/* but some of them use WM_COPYDATA. be afraid.
*/

#define WINAMP_VERSION_MAJOR(winampVersion) ((winampVersion & 0x0000FF00) >> 12)
#define WINAMP_VERSION_MINOR(winampVersion) (winampVersion & 0x000000FF)  // returns, i.e. 0x12 for 5.12 and 0x10 for 5.1... 

#define IPC_GETVERSION 0
/* int version = SendMessage(hwnd_winamp,WM_WA_IPC,0,IPC_GETVERSION);
**
** The version returned will be 0x20yx for Winamp 2.yx.
** Versions previous to Winamp 2.0 typically (but not always) use 0x1zyx for 1.zx.
** Just a bit weird but that's the way it goes.
**
** For Winamp 5.x it uses the format 0x50yx for Winamp 5.yx
** e.g.   5.01 -> 0x5001
**        5.09 -> 0x5009
**        5.1  -> 0x5010
**
** Notes: For 5.02 this api will return the same value as for a 5.01 build.
**        For 5.07 this api will return the same value as for a 5.06 build.
*/
#define IPC_GETVERSIONSTRING 1

#define IPC_GETREGISTEREDVERSION 770


typedef struct {
  const char *filename;
  const char *title;
  int length;
} enqueueFileWithMetaStruct; // send this to a IPC_PLAYFILE in a non WM_COPYDATA, 
// and you get the nice desired result. if title is NULL, it is treated as a "thing",
// otherwise it's assumed to be a file (for speed)

typedef struct {
  const wchar_t *filename;
  const wchar_t *title;
  int length;
} enqueueFileWithMetaStructW;

#define IPC_PLAYFILE 100  // dont be fooled, this is really the same as enqueufile
#define IPC_ENQUEUEFILE 100 
#define IPC_PLAYFILEW 1100
/* This is sent as a WM_COPYDATA with IPC_PLAYFILE as the dwData member and the string
** of the file / playlist to be enqueued into the playlist editor as the lpData member.
** This will just enqueue the file or files since you can use this to enqueue a playlist.
** It will not clear the current playlist or change the playback state.
**
** COPYDATASTRUCT cds = {0};
**   cds.dwData = IPC_ENQUEUEFILE;
**   cds.lpData = (void*)"c:\\test\\folder\\test.mp3";
**   cds.cbData = lstrlen((char*)cds.lpData)+1;  // include space for null char
**   SendMessage(hwnd_winamp,WM_COPYDATA,0,(LPARAM)&cds);
**
**
** With 2.9+ and all of the 5.x versions you can send this as a normal WM_WA_IPC
** (non WM_COPYDATA) with an enqueueFileWithMetaStruct as the param.
** If the title member is null then it is treated as a "thing" otherwise it will be
** assumed to be a file (for speed).
**
** enqueueFileWithMetaStruct eFWMS = {0};
**   eFWMS.filename = "c:\\test\\folder\\test.mp3";
**   eFWMS.title = "Whipping Good";
**   eFWMS.length = 300;  // this is the number of seconds for the track
**   SendMessage(hwnd_winamp,WM_WA_IPC,(WPARAM)&eFWMS,IPC_ENQUEUEFILE);
*/


#define IPC_DELETE 101
#define IPC_DELETE_INT 1101 
/* SendMessage(hwnd_winamp,WM_WA_IPC,0,IPC_DELETE);
** Use this api to clear Winamp's internal playlist.
** You should not need to use IPC_DELETE_INT since it is used internally by Winamp when
** it is dealing with some lame Windows Explorer issues (hard to believe that!).
*/


#define IPC_STARTPLAY 102   
#define IPC_STARTPLAY_INT 1102 
/* SendMessage(hwnd_winamp,WM_WA_IPC,0,IPC_STARTPLAY);
** Sending this will start playback and is almost the same as hitting the play button.
** The IPC_STARTPLAY_INT version is used internally and you should not need to use it
** since it won't be any fun.
*/

#define IPC_CHDIR 103
/* This is sent as a WM_COPYDATA type message with IPC_CHDIR as the dwData value and the
** directory you want to change to as the lpData member.
**
** COPYDATASTRUCT cds = {0};
**   cds.dwData = IPC_CHDIR;
**   cds.lpData = (void*)"c:\\download";
**   cds.cbData = lstrlen((char*)cds.lpData)+1; // include space for null char
**   SendMessage(hwnd_winamp,WM_COPYDATA,0,(LPARAM)&cds);
**
** The above example will make Winamp change to the directory 'C:\download'.
*/


#define IPC_ISPLAYING 104
/* int res = SendMessage(hwnd_winamp,WM_WA_IPC,0,IPC_ISPLAYING);
** This is sent to retrieve the current playback state of Winamp.
** If it returns 1, Winamp is playing.
** If it returns 3, Winamp is paused.
** If it returns 0, Winamp is not playing.
*/


#define IPC_GETOUTPUTTIME 105
/* int res = SendMessage(hwnd_winamp,WM_WA_IPC,mode,IPC_GETOUTPUTTIME);
** This api can return two different sets of information about current playback status.
**
** If mode = 0 then it will return the position (in ms) of the currently playing track.
** Will return -1 if Winamp is not playing.
**
** If mode = 1 then it will return the current track length (in seconds).
** Will return -1 if there are no tracks (or possibly if Winamp cannot get the length).
*/



#define IPC_JUMPTOTIME 106
/* (requires Winamp 1.60+)
** SendMessage(hwnd_winamp,WM_WA_IPC,ms,IPC_JUMPTOTIME);
** This api sets the current position (in milliseconds) for the currently playing song.
** The resulting playback position may only be an approximate time since some playback
** formats do not provide exact seeking e.g. mp3
** This returns -1 if Winamp is not playing, 1 on end of file, or 0 if it was successful.
*/

#define IPC_GETMODULENAME 109
#define IPC_EX_ISRIGHTEXE 666
/* usually shouldnt bother using these, but here goes:
** send a WM_COPYDATA with IPC_GETMODULENAME, and an internal
** flag gets set, which if you send a normal WM_WA_IPC message with
** IPC_EX_ISRIGHTEXE, it returns whether or not that filename
** matches. lame, I know.
*/

#define IPC_WRITEPLAYLIST 120
/* (requires Winamp 1.666+)
** int cur = SendMessage(hwnd_winamp,WM_WA_IPC,0,IPC_WRITEPLAYLIST);
**
** IPC_WRITEPLAYLIST will write the current playlist to '<winampdir>\\Winamp.m3u' and
** will also return the current playlist position (see IPC_GETLISTPOS).
**
** This is kinda obsoleted by some of the newer 2.x api items but it still is good for
** use with a front-end program (instead of a plug-in) and you want to see what is in the
** current playlist.
**
** This api will only save out extended file information in the #EXTINF entry if Winamp
** has already read the data such as if the file was played of scrolled into view. If
** Winamp has not read the data then you will only find the file with its filepath entry
** (as is the base requirements for a m3u playlist).
*/


#define IPC_SETPLAYLISTPOS 121
/* (requires Winamp 2.0+)
** SendMessage(hwnd_winamp,WM_WA_IPC,position,IPC_SETPLAYLISTPOS)
** IPC_SETPLAYLISTPOS sets the playlist position to the specified 'position'.
** It will not change playback status or anything else. It will just set the current
** position in the playlist and will update the playlist view if necessary.
**
** If you use SendMessage(hwnd_winamp,WM_COMMAND,MAKEWPARAM(WINAMP_BUTTON2,0),0);
** after using IPC_SETPLAYLISTPOS then Winamp will start playing the file at 'position'.
*/


#define IPC_SETVOLUME 122
/* (requires Winamp 2.0+)
** SendMessage(hwnd_winamp,WM_WA_IPC,volume,IPC_SETVOLUME);
** IPC_SETVOLUME sets the volume of Winamp (between the range of 0 to 255).
**
** If you pass 'volume' as -666 then the message will return the current volume.
** int curvol = SendMessage(hwnd_winamp,WM_WA_IPC,-666,IPC_SETVOLUME);
*/


#define IPC_SETPANNING 123
/* (requires Winamp 2.0+)
** SendMessage(hwnd_winamp,WM_WA_IPC,panning,IPC_SETPANNING);
** IPC_SETPANNING sets the panning of Winamp from 0 (left) to 255 (right).
**
** At least in 5.x+ this works from -127 (left) to 127 (right).
**
** If you pass 'panning' as -666 to this api then it will return the current panning.
** int curpan = SendMessage(hwnd_winamp,WM_WA_IPC,-666,IPC_SETPANNING);
*/


#define IPC_GETLISTLENGTH 124
/* (requires Winamp 2.0+)
** int length = SendMessage(hwnd_winamp,WM_WA_IPC,0,IPC_GETLISTLENGTH);
** IPC_GETLISTLENGTH returns the length of the current playlist as the number of tracks.
*/


#define IPC_GETLISTPOS 125
/* (requires Winamp 2.05+)
** int pos=SendMessage(hwnd_winamp,WM_WA_IPC,0,IPC_GETLISTPOS);
** IPC_GETLISTPOS returns the current playlist position (which is shown in the playlist
** editor as a differently coloured text entry e.g is yellow for the classic skin).
**
** This api is a lot like IPC_WRITEPLAYLIST but a lot faster since it does not have to
** write out the whole of the current playlist first.
*/


#define IPC_GETINFO 126
/* (requires Winamp 2.05+)
** int inf=SendMessage(hwnd_winamp,WM_WA_IPC,mode,IPC_GETINFO);
** IPC_GETINFO returns info about the current playing song. The value
** it returns depends on the value of 'mode'.
** Mode      Meaning
** ------------------
** 0         Samplerate, in kilohertz (i.e. 44)
** 1         Bitrate  (i.e. 128)
** 2         Channels (i.e. 2)
** 3 (5+)    Video LOWORD=w HIWORD=h
** 4 (5+)    > 65536, string (video description)
** 5 (5.25+) Samplerate, in hertz (i.e. 44100)
*/


#define IPC_GETEQDATA 127
/* (requires Winamp 2.05+)
** int data=SendMessage(hwnd_winamp,WM_WA_IPC,pos,IPC_GETEQDATA);
** IPC_GETEQDATA queries the status of the EQ. 
** The value returned depends on what 'pos' is set to:
** Value      Meaning
** ------------------
** 0-9        The 10 bands of EQ data. 0-63 (+20db - -20db)
** 10         The preamp value. 0-63 (+20db - -20db)
** 11         Enabled. zero if disabled, nonzero if enabled.
** 12         Autoload. zero if disabled, nonzero if enabled.
*/


#define IPC_SETEQDATA 128
/* (requires Winamp 2.05+)
** SendMessage(hwnd_winamp,WM_WA_IPC,pos,IPC_GETEQDATA);
** SendMessage(hwnd_winamp,WM_WA_IPC,value,IPC_SETEQDATA);
** IPC_SETEQDATA sets the value of the last position retrieved
** by IPC_GETEQDATA. This is pretty lame, and we should provide
** an extended version that lets you do a MAKELPARAM(pos,value).
** someday...

  new (2.92+): 
    if the high byte is set to 0xDB, then the third byte specifies
    which band, and the bottom word specifies the value.
*/

#define IPC_ADDBOOKMARK 129
#define IPC_ADDBOOKMARKW 131
/* (requires Winamp 2.4+)
** This is sent as a WM_COPYDATA using IPC_ADDBOOKMARK as the dwData value and the
** directory you want to change to as the lpData member. This will add the specified
** file / url to the Winamp bookmark list.
**
** COPYDATASTRUCT cds = {0};
**   cds.dwData = IPC_ADDBOOKMARK;
**   cds.lpData = (void*)"http://www.blah.com/listen.pls";
**   cds.cbData = lstrlen((char*)cds.lpData)+1; // include space for null char
**   SendMessage(hwnd_winamp,WM_COPYDATA,0,(LPARAM)&cds);
**
**
** In Winamp 5.0+ we use this as a normal WM_WA_IPC and the string is null separated as
** the filename and then the title of the entry.
**
** SendMessage(hwnd_winamp,WM_WA_IPC,(WPARAM)(char*)"filename\0title\0",IPC_ADDBOOKMARK);
**
** This will notify the library / bookmark editor that a bookmark was added.
** Note that using this message in this context does not actually add the bookmark.
** Do not use, it is essentially just a notification type message :)
*/


#define IPC_INSTALLPLUGIN 130
/* This is not implemented (and is very unlikely to be done due to safety concerns).
** If it was then you could do a WM_COPYDATA with a path to a .wpz and it would then
** install the plugin for you.
**
** COPYDATASTRUCT cds = {0};
**   cds.dwData = IPC_INSTALLPLUGIN;
**   cds.lpData = (void*)"c:\\path\\to\\file.wpz";
**   cds.cbData = lstrlen((char*)cds.lpData)+1; // include space for null char
**   SendMessage(hwnd_winamp,WM_COPYDATA,0,(LPARAM)&cds);
*/


#define IPC_RESTARTWINAMP 135
/* (requires Winamp 2.2+)
** SendMessage(hwnd_winamp,WM_WA_IPC,0,IPC_RESTARTWINAMP);
** IPC_RESTARTWINAMP will restart Winamp (isn't that obvious ? :) )
** If this fails to make Winamp start after closing then there is a good chance one (or
** more) of the currently installed plugins caused Winamp to crash on exit (either as a
** silent crash or a full crash log report before it could call itself start again.
*/


#define IPC_ISFULLSTOP 400
/* (requires winamp 2.7+ I think)
** int ret=SendMessage(hwnd_winamp,WM_WA_IPC,0,IPC_ISFULLSTOP);
** This is useful for when you're an output plugin and you want to see if the stop/close
** happening is a full stop or if you are just between tracks. This returns non zero if
** it is a full stop or zero if it is just a new track.
*/


#define IPC_INETAVAILABLE 242
/* (requires Winamp 2.05+)
** int val=SendMessage(hwnd_winamp,WM_WA_IPC,0,IPC_INETAVAILABLE);
** IPC_INETAVAILABLE will return 1 if an Internet connection is available for Winamp and
** relates to the internet connection type setting on the main general preferences page
** in the Winamp preferences.
*/


#define IPC_UPDTITLE 243
/* (requires Winamp 2.2+)
** SendMessage(hwnd_winamp,WM_WA_IPC,0,IPC_UPDTITLE);
** IPC_UPDTITLE will ask Winamp to update the information about the current title and
** causes GetFileInfo(..) in the input plugin associated with the current playlist entry
** to be called. This can be called such as when an input plugin is buffering a file so
** that it can cause the buffer percentage to appear in the playlist.
*/


#define IPC_REFRESHPLCACHE 247
/* (requires Winamp 2.2+)
** SendMessage(hwnd_winamp,WM_WA_IPC,0,IPC_REFRESHPLCACHE);
** IPC_REFRESHPLCACHE will flush the playlist cache buffer and you send this if you want
** Winamp to go refetch the titles for all of the entries in the current playlist.

5.3+: pass a wchar_t * string in wParam, and it'll do a strnicmp() before clearing the cache
*/


#define IPC_GET_SHUFFLE 250
/* (requires Winamp 2.4+)
** int val=SendMessage(hwnd_winamp,WM_WA_IPC,0,IPC_GET_SHUFFLE);
** IPC_GET_SHUFFLE returns the status of the shuffle option.
** If set then it will return 1 and if not set then it will return 0.
*/


#define IPC_GET_REPEAT 251
/* (requires Winamp 2.4+)
** int val=SendMessage(hwnd_winamp,WM_WA_IPC,0,IPC_GET_REPEAT);
** IPC_GET_REPEAT returns the status of the repeat option.
** If set then it will return 1 and if not set then it will return 0.
*/


#define IPC_SET_SHUFFLE 252
/* (requires Winamp 2.4+)
** SendMessage(hwnd_winamp,WM_WA_IPC,value,IPC_SET_SHUFFLE);
** IPC_SET_SHUFFLE sets the status of the shuffle option.
** If 'value' is 1 then shuffle is turned on.
** If 'value' is 0 then shuffle is turned off.
*/


#define IPC_SET_REPEAT 253
/* (requires Winamp 2.4+)
** SendMessage(hwnd_winamp,WM_WA_IPC,value,IPC_SET_REPEAT);
** IPC_SET_REPEAT sets the status of the repeat option.
** If 'value' is 1 then shuffle is turned on.
** If 'value' is 0 then shuffle is turned off.
*/


#define IPC_ENABLEDISABLE_ALL_WINDOWS 259 // 0xdeadbeef to disable
/* (requires Winamp 2.9+)
** SendMessage(hwnd_winamp,WM_WA_IPC,(enable?0:0xdeadbeef),IPC_ENABLEDISABLE_ALL_WINDOWS);
** Sending this message with 0xdeadbeef as the param will disable all winamp windows and
** any other values will enable all of the Winamp windows again. When disabled you won't
** get any response on clicking or trying to do anything to the Winamp windows. If the
** taskbar icon is shown then you may still have control ;)
*/


#define IPC_GETWND 260
/* (requires Winamp 2.9+)
** HWND h=SendMessage(hwnd_winamp,WM_WA_IPC,IPC_GETWND_xxx,IPC_GETWND);
** returns the HWND of the window specified.
*/
  #define IPC_GETWND_EQ 0 // use one of these for the param
  #define IPC_GETWND_PE 1
  #define IPC_GETWND_MB 2
  #define IPC_GETWND_VIDEO 3
#define IPC_ISWNDVISIBLE 261 // same param as IPC_GETWND




/************************************************************************
***************** in-process only (WE LOVE PLUGINS)
************************************************************************/

#define IPC_SETSKINW 199
#define IPC_SETSKIN 200
/* (requires Winamp 2.04+, only usable from plug-ins (not external apps))
** SendMessage(hwnd_winamp,WM_WA_IPC,(WPARAM)"skinname",IPC_SETSKIN);
** IPC_SETSKIN sets the current skin to "skinname". Note that skinname 
** can be the name of a skin, a skin .zip file, with or without path. 
** If path isn't specified, the default search path is the winamp skins 
** directory.
*/


#define IPC_GETSKIN 201
/* (requires Winamp 2.04+, only usable from plug-ins (not external apps))
** SendMessage(hwnd_winamp,WM_WA_IPC,(WPARAM)skinname_buffer,IPC_GETSKIN);
** IPC_GETSKIN puts the directory where skin bitmaps can be found 
** into  skinname_buffer.
** skinname_buffer must be MAX_PATH characters in length.
** When using a .zip'd skin file, it'll return a temporary directory
** where the ZIP was decompressed.
*/


#define IPC_EXECPLUG 202
/* (requires Winamp 2.04+, only usable from plug-ins (not external apps))
** SendMessage(hwnd_winamp,WM_WA_IPC,(WPARAM)"vis_file.dll",IPC_EXECPLUG);
** IPC_EXECPLUG executes a visualization plug-in pointed to by WPARAM.
** the format of this string can be:
** "vis_whatever.dll"
** "vis_whatever.dll,0" // (first mod, file in winamp plug-in dir)
** "C:\\dir\\vis_whatever.dll,1" 
*/


#define IPC_GETPLAYLISTFILE 211
#define IPC_GETPLAYLISTFILEW 214
/* (requires Winamp 2.04+, only usable from plug-ins (not external apps))
** char *name=SendMessage(hwnd_winamp,WM_WA_IPC,index,IPC_GETPLAYLISTFILE);
** IPC_GETPLAYLISTFILE gets the filename of the playlist entry [index].
** returns a pointer to it. returns NULL on error.
*/


#define IPC_GETPLAYLISTTITLE 212
#define IPC_GETPLAYLISTTITLEW 213
/* (requires Winamp 2.04+, only usable from plug-ins (not external apps))
** char *name=SendMessage(hwnd_winamp,WM_WA_IPC,index,IPC_GETPLAYLISTTITLE);
**
** IPC_GETPLAYLISTTITLE gets the title of the playlist entry [index].
** returns a pointer to it. returns NULL on error.
*/


#define IPC_GETHTTPGETTER 240
/* retrieves a function pointer to a HTTP retrieval function.
** if this is unsupported, returns 1 or 0.
** the function should be:
** int (*httpRetrieveFile)(HWND hwnd, char *url, char *file, char *dlgtitle);
** if you call this function, with a parent window, a URL, an output file, and a dialog title,
** it will return 0 on successful download, 1 on error.
*/


#define IPC_MBOPEN 241
/* (requires Winamp 2.05+)
** SendMessage(hwnd_winamp,WM_WA_IPC,0,IPC_MBOPEN);
** SendMessage(hwnd_winamp,WM_WA_IPC,(WPARAM)url,IPC_MBOPEN);
** IPC_MBOPEN will open a new URL in the minibrowser. if url is NULL, it will open the Minibrowser window.
*/



#define IPC_CHANGECURRENTFILE 245
/* (requires Winamp 2.05+)
** SendMessage(hwnd_winamp,WM_WA_IPC,(WPARAM)file,IPC_CHANGECURRENTFILE);
** IPC_CHANGECURRENTFILE will set the current playlist item.
*/

#define IPC_CHANGECURRENTFILEW 1245
/* (requires Winamp 2.05+)
** SendMessage(hwnd_winamp,WM_WA_IPC,(WPARAM)file,IPC_CHANGECURRENTFILEW);
** IPC_CHANGECURRENTFILEW will set the current playlist item.
*/


#define IPC_GETMBURL 246
/* (requires Winamp 2.2+)
** char buffer[4096]; // Urls can be VERY long
** SendMessage(hwnd_winamp,WM_WA_IPC,(WPARAM)buffer,IPC_GETMBURL);
** IPC_GETMBURL will retrieve the current Minibrowser URL into buffer.
** buffer must be at least 4096 bytes long.
*/


#define IPC_MBBLOCK 248
/* (requires Winamp 2.4+)
** SendMessage(hwnd_winamp,WM_WA_IPC,value,IPC_MBBLOCK);
**
** IPC_MBBLOCK will block the Minibrowser from updates if value is set to 1
*/

#define IPC_MBOPENREAL 249
/* (requires Winamp 2.4+)
** SendMessage(hwnd_winamp,WM_WA_IPC,(WPARAM)url,IPC_MBOPENREAL);
**
** IPC_MBOPENREAL works the same as IPC_MBOPEN except that it will works even if 
** IPC_MBBLOCK has been set to 1
*/

#define IPC_ADJUST_OPTIONSMENUPOS 280
/* (requires Winamp 2.9+)
** int newpos=SendMessage(hwnd_winamp,WM_WA_IPC,(WPARAM)adjust_offset,IPC_ADJUST_OPTIONSMENUPOS);
** moves where winamp expects the Options menu in the main menu. Useful if you wish to insert a
** menu item above the options/skins/vis menus.
*/

#define IPC_GET_HMENU 281
/* (requires Winamp 2.9+)
** HMENU hMenu=SendMessage(hwnd_winamp,WM_WA_IPC,(WPARAM)0,IPC_GET_HMENU);
** values for data:
** 0 : main popup menu 
** 1 : main menubar file menu
** 2 : main menubar options menu
** 3 : main menubar windows menu
** 4 : main menubar help menu
** other values will return NULL.
*/

#define IPC_GET_EXTENDED_FILE_INFO 290 //pass a pointer to the following struct in wParam
#define IPC_GET_EXTENDED_FILE_INFO_HOOKABLE 296
/* (requires Winamp 2.9+)
** to use, create an extendedFileInfoStruct, point the values filename and metadata to the
** filename and metadata field you wish to query, and ret to a buffer, with retlen to the
** length of that buffer, and then SendMessage(hwnd_winamp,WM_WA_IPC,&struct,IPC_GET_EXTENDED_FILE_INFO);
** the results should be in the buffer pointed to by ret.
** returns 1 if the decoder supports a getExtendedFileInfo method
*/
typedef struct {
  const char *filename;
  const char *metadata;
  char *ret;
  int retlen;
} extendedFileInfoStruct;


#define IPC_GET_BASIC_FILE_INFO 291 //pass a pointer to the following struct in wParam
typedef struct {
  const char *filename;

  int quickCheck; // set to 0 to always get, 1 for quick, 2 for default (if 2, quickCheck will be set to 0 if quick wasnot used)

  // filled in by winamp
  int length;
  char *title;
  int titlelen;
} basicFileInfoStruct;

#define IPC_GET_BASIC_FILE_INFOW 1291 //pass a pointer to the following struct in wParam
typedef struct {
  const wchar_t *filename;

  int quickCheck; // set to 0 to always get, 1 for quick, 2 for default (if 2, quickCheck will be set to 0 if quick wasnot used)

  // filled in by winamp
  int length;
  wchar_t *title;
  int titlelen;
} basicFileInfoStructW;


#define IPC_GET_EXTLIST 292 //returns doublenull delimited. GlobalFree() it when done. if data is 0, returns raw extlist, if 1, returns something suitable for getopenfilename
#define IPC_GET_EXTLISTW 1292 // wide char version of above

#define IPC_INFOBOX 293
typedef struct {
  HWND parent;
  char *filename;
} infoBoxParam;

#define IPC_INFOBOXW 1293
typedef struct {
  HWND parent;
  const wchar_t *filename;
} infoBoxParamW;

#define IPC_SET_EXTENDED_FILE_INFO 294 //pass a pointer to the a extendedFileInfoStruct in wParam
/* (requires Winamp 2.9+)
** to use, create an extendedFileInfoStruct, point the values filename and metadata to the
** filename and metadata field you wish to write in ret. (retlen is not used). and then 
** SendMessage(hwnd_winamp,WM_WA_IPC,&struct,IPC_SET_EXTENDED_FILE_INFO);
** returns 1 if the metadata is supported
** Call IPC_WRITE_EXTENDED_FILE_INFO once you're done setting all the metadata you want to update
*/

#define IPC_WRITE_EXTENDED_FILE_INFO 295 
/* (requires Winamp 2.9+)
** writes all the metadata set thru IPC_SET_EXTENDED_FILE_INFO to the file
** returns 1 if the file has been successfully updated, 0 if error
*/

#define IPC_FORMAT_TITLE 297
typedef struct 
{
  char *spec; // NULL=default winamp spec
  void *p;

  char *out;
  int out_len;

  char * (*TAGFUNC)(const char * tag, void * p); //return 0 if not found
  void (*TAGFREEFUNC)(char * tag,void * p);
} waFormatTitle;

#define IPC_FORMAT_TITLE_EXTENDED 298 // similiar to IPC_FORMAT_TITLE, but falls back to Winamp's %tags% if your passed tag function doesn't handle it
typedef struct 
{
	const wchar_t *filename;
	int useExtendedInfo; // set to 1 if you want the Title Formatter to query the input plugins for any tags that your tag function fails on
  const wchar_t *spec; // NULL=default winamp spec
  void *p;

  wchar_t *out;
  int out_len;

  wchar_t * (*TAGFUNC)(const wchar_t * tag, void * p); //return 0 if not found, -1 for empty tag
  void (*TAGFREEFUNC)(wchar_t *tag, void *p);
} waFormatTitleExtended;

#define IPC_GETUNCOMPRESSINTERFACE 331
/* returns a function pointer to uncompress().
** int (*uncompress)(unsigned char *dest, unsigned long *destLen, const unsigned char *source, unsigned long sourceLen);
** right out of zlib, useful for decompressing zlibbed data.
** if you pass the parm of 0x10100000, it will return a wa_inflate_struct * to an inflate API.
*/

typedef struct {
  int (*inflateReset)(void *strm);
  int (*inflateInit_)(void *strm,const char *version, int stream_size);
  int (*inflate)(void *strm, int flush);
  int (*inflateEnd)(void *strm);
  unsigned long (*crc32)(unsigned long crc, const unsigned  char *buf, unsigned int len);
} wa_inflate_struct;


#define IPC_ADD_PREFS_DLG 332
#define IPC_REMOVE_PREFS_DLG 333
/* (requires Winamp 2.9+)
** to use, allocate a prefsDlgRec structure (either on the heap or some global
** data, but NOT on the stack), initialze the members:
** hInst to the DLL instance where the resource is located
** dlgID to the ID of the dialog,
** proc to the window procedure for the dialog
** name to the name of the prefs page in the prefs.
** where to 0 (eventually we may add more options)
** then, SendMessage(hwnd_winamp,WM_WA_IPC,&prefsRec,IPC_ADD_PREFS_DLG);
**
** you can also IPC_REMOVE_PREFS_DLG with the address of the same prefsRec,
** but you shouldn't really ever have to.
**
*/
#define IPC_OPENPREFSTOPAGE 380 // pass an id of a builtin page, or a &prefsDlgRec of prefs page to open

typedef struct _prefsDlgRec {
  HINSTANCE hInst;
  int dlgID;
  void *proc;

  char *name;
  intptr_t where; // 0 for options, 1 for plugins, 2 for skins, 3 for bookmarks, 4 for prefs

  intptr_t _id;
  struct _prefsDlgRec *next;
} prefsDlgRec;


#define IPC_GETINIFILE 334 // returns a pointer to winamp.ini
#define IPC_GETINIDIRECTORY 335 // returns a pointer to the directory to put config files in (if you dont want to use winamp.ini)
#define IPC_GETPLUGINDIRECTORY 336
#define IPC_GETM3UDIRECTORY 337 // returns a char pointer to the directory where winamp.m3u is stored in.
#define IPC_GETM3UDIRECTORYW 338 // returns a wchar_t pointer to the directory where winamp.m3u is stored in.

#define IPC_SPAWNBUTTONPOPUP 361 // param =
// 0 = eject
// 1 = previous
// 2 = next
// 3 = pause
// 4 = play
// 5 = stop

#define IPC_OPENURLBOX 360 // pass a HWND to a parent, returns a HGLOBAL that needs to be freed with GlobalFree(), if successful
#define IPC_OPENFILEBOX 362 // pass a HWND to a parent
#define IPC_OPENDIRBOX 363 // pass a HWND to a parent

// pass an HWND to a parent. call this if you take over the whole UI so that the dialogs are not appearing on the
// bottom right of the screen since the main winamp window is at 3000x3000, call again with NULL to reset
#define IPC_SETDIALOGBOXPARENT 364



#define IPC_DRO_MIN 401 // reserved for DrO
#define IPC_SET_JTF_COMPARATOR 409
/* pass me an int (__cdecl *)(const char *, const char *) in wParam */
#define IPC_SET_JTF_COMPARATOR_W 410
/* pass me an int (__cdecl *)(const wchar_t *, const wchar_t *) in wParam ... maybe someday :) */
#define IPC_SET_JTF_DRAWTEXT 416

#define IPC_DRO_MAX 499


// pass 0 for a copy of the skin HBITMAP
// pass 1 for name of font to use for playlist editor likeness
// pass 2 for font charset
// pass 3 for font size
#define IPC_GET_GENSKINBITMAP 503


#define IPC_GET_EMBEDIF 505 // pass an embedWindowState
// returns an HWND embedWindow(embedWindowState *); if the data is NULL, otherwise returns the HWND directly
typedef struct
{
  HWND me; //hwnd of the window

  int flags;

  RECT r;

  void *user_ptr; // for application use

  intptr_t extra_data[64]; // for internal winamp use
} embedWindowState;

#define EMBED_FLAGS_NORESIZE 0x1 // set this bit in embedWindowState.flags to keep window from being resizable
#define EMBED_FLAGS_NOTRANSPARENCY 0x2 // set this bit in embedWindowState.flags to make gen_ff turn transparency off for this wnd
#define EMBED_FLAGS_NOWINDOWMENU 0x4 // set this bit to prevent gen_ff from automatically adding your window to the right-click menu
#define EMBED_FLAGS_GUID 0x8 // call SET_EMBED_GUID(yourEmbedWindowStateStruct, GUID) to define a GUID for this window 

#define SET_EMBED_GUID(windowState, windowGUID) { windowState->flags |= EMBED_FLAGS_GUID; *((GUID *)&windowState->extra_data[4])=windowGUID; }
#define GET_EMBED_GUID(windowState) (*((GUID *)&windowState->extra_data[4]))

#define IPC_EMBED_ENUM 532
typedef struct embedEnumStruct
{
  int (*enumProc)(embedWindowState *ws, struct embedEnumStruct *param); // return 1 to abort
  int user_data; // or more :)
} embedEnumStruct;
  // pass 

#define IPC_EMBED_ISVALID 533

#define IPC_CONVERTFILE 506
/* (requires Winamp 2.92+)
** Converts a given file to a different format (PCM, MP3, etc...)
** To use, pass a pointer to a waFileConvertStruct struct
** This struct can be either on the heap or some global
** data, but NOT on the stack. At least, until the conversion is done.
**
** eg: SendMessage(hwnd_winamp,WM_WA_IPC,&myConvertStruct,IPC_CONVERTFILE);
**
** Return value:
** 0: Can't start the conversion. Look at myConvertStruct->error for details.
** 1: Conversion started. Status messages will be sent to the specified callbackhwnd.
**    Be sure to call IPC_CONVERTFILE_END when your callback window receives the
**    IPC_CB_CONVERT_DONE message.
*/
typedef struct 
{
  char *sourcefile;  // "c:\\source.mp3"
  char *destfile;    // "c:\\dest.pcm"
  int destformat[8]; // like 'PCM ',srate,nch,bps.
  //hack alert! you can set destformat[6]=mmioFOURCC('I','N','I',' '); and destformat[7]=(int)my_ini_file; (where my_ini_file is a char*)
  HWND callbackhwnd; // window that will receive the IPC_CB_CONVERT notification messages
  
  //filled in by winamp.exe
  char *error;        //if IPC_CONVERTFILE returns 0, the reason will be here

  int bytes_done;     //you can look at both of these values for speed statistics
  int bytes_total;
  int bytes_out;

  int killswitch;     // don't set it manually, use IPC_CONVERTFILE_END
  intptr_t extra_data[64]; // for internal winamp use
} convertFileStruct;

#define IPC_CONVERTFILE_END 507
/* (requires Winamp 2.92+)
** Stop/ends a convert process started from IPC_CONVERTFILE
** You need to call this when you receive the IPC_CB_CONVERTDONE message or when you
** want to abort a conversion process
**
** eg: SendMessage(hwnd_winamp,WM_WA_IPC,&myConvertStruct,IPC_CONVERTFILE_END);
**
** No return value
*/

typedef struct {
  HWND hwndParent;
  int format;

  //filled in by winamp.exe
  HWND hwndConfig;
  int extra_data[8];
  //hack alert! you can set extra_data[6]=mmioFOURCC('I','N','I',' '); and extra_data[7]=(int)my_ini_file; (where my_ini_file is a char*)
} convertConfigStruct;
#define IPC_CONVERT_CONFIG 508
#define IPC_CONVERT_CONFIG_END 509

typedef struct
{
  void (*enumProc)(intptr_t user_data, const char *desc, int fourcc);
  intptr_t user_data;
} converterEnumFmtStruct;
#define IPC_CONVERT_CONFIG_ENUMFMTS 510
/* (requires Winamp 2.92+)
*/

typedef struct
{
  char cdletter;
  char *playlist_file;
  HWND callback_hwnd;

  //filled in by winamp.exe
  char *error;
} burnCDStruct;
#define IPC_BURN_CD 511
/* (requires Winamp 5.0+)
*/

typedef struct
{
  convertFileStruct *cfs;
  int priority;
} convertSetPriority;
#define IPC_CONVERT_SET_PRIORITY 512

typedef struct
{
  unsigned int format; //fourcc value
  char *item; // config item, eg "bitrate"
  char *data; // buffer to recieve, or buffer that contains the data
  int len; // length of the data buffer (only used when getting a config item)
  char *configfile; // config file to read from
} convertConfigItem;

#define IPC_CONVERT_CONFIG_SET_ITEM 513 // returns TRUE if successful
#define IPC_CONVERT_CONFIG_GET_ITEM 514 // returns TRUE if successful

typedef struct
{
  const char *filename;
  char *title; // 2048 bytes
  int length;
  int force_useformatting; // can set this to 1 if you want to force a url to use title formatting shit
} waHookTitleStruct;
// return TRUE if you hook this
#define IPC_HOOK_TITLES 850

typedef struct
{
  const wchar_t *filename;
  wchar_t *title; // 2048 bytes
  int length;
  int force_useformatting; // can set this to 1 if you want to force a url to use title formatting shit
} waHookTitleStructW;
// return TRUE if you hook this
#define IPC_HOOK_TITLESW 851

#define IPC_GETSADATAFUNC 800 
// 0: returns a char *export_sa_get() that returns 150 bytes of data
// 1: returns a export_sa_setreq(int want);

#define IPC_GETVUDATAFUNC 801 
// 0: returns a int export_vu_get(int channel) that returns 0-255 (or -1 for bad channel)

#define IPC_ISMAINWNDVISIBLE 900


#define IPC_SETPLEDITCOLORS 920
typedef struct
{
  int numElems;
  int *elems;
  HBITMAP bm; // set if you want to override
} waSetPlColorsStruct;


// the following IPC use waSpawnMenuParms as parameter
#define IPC_SPAWNEQPRESETMENU 933
#define IPC_SPAWNFILEMENU 934 //menubar
#define IPC_SPAWNOPTIONSMENU 935 //menubar
#define IPC_SPAWNWINDOWSMENU 936 //menubar
#define IPC_SPAWNHELPMENU 937 //menubar
#define IPC_SPAWNPLAYMENU 938 //menubar
#define IPC_SPAWNPEFILEMENU 939 //menubar
#define IPC_SPAWNPEPLAYLISTMENU 940 //menubar
#define IPC_SPAWNPESORTMENU 941 //menubar
#define IPC_SPAWNPEHELPMENU 942 //menubar
#define IPC_SPAWNMLFILEMENU 943 //menubar
#define IPC_SPAWNMLVIEWMENU 944 //menubar
#define IPC_SPAWNMLHELPMENU 945 //menubar
#define IPC_SPAWNPELISTOFPLAYLISTS 946

typedef struct
{
  HWND wnd;
  int xpos; // in screen coordinates
  int ypos;
} waSpawnMenuParms;

// waSpawnMenuParms2 is used by the menubar submenus
typedef struct
{
  HWND wnd;
  int xpos; // in screen coordinates
  int ypos;
  int width;
  int height;
} waSpawnMenuParms2;


// system tray sends this (you might want to simulate it)
#define WM_WA_SYSTRAY WM_USER+1

// input plugins send this when they are done playing back
#define WM_WA_MPEG_EOF WM_USER+2



//// video stuff

#define IPC_IS_PLAYING_VIDEO 501 // returns >1 if playing, 0 if not, 1 if old version (so who knows):)
#define IPC_GET_IVIDEOOUTPUT 500 // see below for IVideoOutput interface
#define VIDEO_MAKETYPE(A,B,C,D) ((A) | ((B)<<8) | ((C)<<16) | ((D)<<24))
#define VIDUSER_SET_INFOSTRING 0x1000
#define VIDUSER_GET_VIDEOHWND  0x1001
#define VIDUSER_SET_VFLIP      0x1002
#define VIDUSER_SET_TRACKSELINTERFACE 0x1003 // give your ITrackSelector interface as param2
#define VIDUSER_OPENVIDEORENDERER 0x1004
#define VIDUSER_CLOSEVIDEORENDERER 0x1005
#define VIDUSER_GETPOPUPMENU 0x1006

typedef struct
{
	int w;
	int h;
	int vflip;
	double aspectratio;
	unsigned int fmt;
} VideoOpenStruct;

#ifndef NO_IVIDEO_DECLARE
#ifdef __cplusplus

class VideoOutput;
class SubsItem;

#ifndef _NSV_DEC_IF_H_
typedef	struct {
	unsigned char*	baseAddr;
	long			rowBytes;
} YV12_PLANE;

typedef	struct {
	YV12_PLANE	y;
	YV12_PLANE	u;
	YV12_PLANE	v;
} YV12_PLANES;
#endif

class IVideoOutput
{
  public:
    virtual ~IVideoOutput() { }
    virtual int open(int w, int h, int vflip, double aspectratio, unsigned int fmt)=0;
    virtual void setcallback(LRESULT (*msgcallback)(void *token, HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam), void *token)		{ (void)token; (void)msgcallback; /* to eliminate warning C4100 */ }
    virtual void close()=0;
    virtual void draw(void *frame)=0;
    virtual void drawSubtitle(SubsItem *item) { }
    virtual void showStatusMsg(const char *text) { }
    virtual int get_latency() { return 0; }
    virtual void notifyBufferState(int bufferstate) { } /* 0-255*/

    virtual INT_PTR extended(INT_PTR param1, INT_PTR param2, INT_PTR param3) { return 0; } // Dispatchable, eat this!
};

class ITrackSelector 
{
  public:
    virtual int getNumAudioTracks()=0;
    virtual void enumAudioTrackName(int n, const char *buf, int size)=0;
    virtual int getCurAudioTrack()=0;
    virtual int getNumVideoTracks()=0;
    virtual void enumVideoTrackName(int n, const char *buf, int size)=0;
    virtual int getCurVideoTrack()=0;

    virtual void setAudioTrack(int n)=0;
    virtual void setVideoTrack(int n)=0;
};

#endif //cplusplus
#endif//NO_IVIDEO_DECLARE

// these messages are callbacks that you can grab by subclassing the winamp window

// wParam = 
#define IPC_CB_WND_EQ 0 // use one of these for the param
#define IPC_CB_WND_PE 1
#define IPC_CB_WND_MB 2
#define IPC_CB_WND_VIDEO 3
#define IPC_CB_WND_MAIN 4

#define IPC_CB_ONSHOWWND 600 
#define IPC_CB_ONHIDEWND 601 

#define IPC_CB_GETTOOLTIP 602

#define IPC_CB_MISC 603
    #define IPC_CB_MISC_TITLE 0
    #define IPC_CB_MISC_VOLUME 1 // volume/pan
    #define IPC_CB_MISC_STATUS 2
    #define IPC_CB_MISC_EQ 3
    #define IPC_CB_MISC_INFO 4
    #define IPC_CB_MISC_VIDEOINFO 5

#define IPC_CB_CONVERT_STATUS 604 // param value goes from 0 to 100 (percent)
#define IPC_CB_CONVERT_DONE   605

#define IPC_ADJUST_FFWINDOWSMENUPOS 606
/* (requires Winamp 2.9+)
** int newpos=SendMessage(hwnd_winamp,WM_WA_IPC,(WPARAM)adjust_offset,IPC_ADJUST_FFWINDOWSMENUPOS);
** moves where winamp expects the freeform windows in the menubar windows main menu. Useful if you wish to insert a
** menu item above extra freeform windows.
*/

#define IPC_ISDOUBLESIZE 608

#define IPC_ADJUST_FFOPTIONSMENUPOS 609
/* (requires Winamp 2.9+)
** int newpos=SendMessage(hwnd_winamp,WM_WA_IPC,(WPARAM)adjust_offset,IPC_ADJUST_FFOPTIONSMENUPOS);
** moves where winamp expects the freeform preferences item in the menubar windows main menu. Useful if you wish to insert a
** menu item above preferences item.
*/

#define IPC_GETTIMEDISPLAYMODE 610 // returns 0 if displaying elapsed time or 1 if displaying remaining time

#define IPC_SETVISWND 611 // param is hwnd, setting this allows you to receive ID_VIS_NEXT/PREVOUS/RANDOM/FS wm_commands
#define ID_VIS_NEXT                     40382
#define ID_VIS_PREV                     40383
#define ID_VIS_RANDOM                   40384
#define ID_VIS_FS                       40389
#define ID_VIS_CFG                      40390
#define ID_VIS_MENU                     40391

#define IPC_GETVISWND 612 // returns the vis cmd handler hwnd
#define IPC_ISVISRUNNING 613
#define IPC_CB_VISRANDOM 628 // param is status of random

#define IPC_SETIDEALVIDEOSIZE 614 // sent by winamp to winamp, trap it if you need it. width=HIWORD(param), height=LOWORD(param)

#define IPC_GETSTOPONVIDEOCLOSE 615
#define IPC_SETSTOPONVIDEOCLOSE 616

typedef struct {
  HWND hwnd;
  int uMsg;
  WPARAM wParam;
  LPARAM lParam;
} transAccelStruct;

#define IPC_TRANSLATEACCELERATOR 617

typedef struct {
  int cmd;
  int x;
  int y;
  int align;
} windowCommand; // send this as param to an IPC_PLCMD, IPC_MBCMD, IPC_VIDCMD

#define IPC_CB_ONTOGGLEAOT 618 

#define IPC_GETPREFSWND 619

#define IPC_SET_PE_WIDTHHEIGHT 620 // data is a pointer to a POINT structure that holds width & height

#define IPC_GETLANGUAGEPACKINSTANCE 621

#define IPC_CB_PEINFOTEXT 622 // data is a string, ie: "04:21/45:02"

#define IPC_CB_OUTPUTCHANGED 623 // output plugin was changed in config

#define IPC_GETOUTPUTPLUGIN 625

#define IPC_SETDRAWBORDERS 626
#define IPC_DISABLESKINCURSORS 627
#define IPC_CB_RESETFONT 629

#define IPC_IS_FULLSCREEN 630 // returns 1 if video or vis is in fullscreen mode
#define IPC_SET_VIS_FS_FLAG 631 // a vis should send this message with 1/as param to notify winamp that it has gone to or has come back from fullscreen mode

#define IPC_SHOW_NOTIFICATION 632

#define IPC_GETSKININFO 633

#define IPC_GET_MANUALPLADVANCE 634
/* (requires Winamp 5.03+)
** val=SendMessage(hwnd_winamp,WM_WA_IPC,0,IPC_GET_MANUALPLADVANCE);
**
** IPC_GET_MANUALPLADVANCE returns the status of the Manual Playlist Advance (1 if set)
*/

#define IPC_SET_MANUALPLADVANCE 635
/* (requires Winamp 5.03+)
** SendMessage(hwnd_winamp,WM_WA_IPC,value,IPC_SET_MANUALPLADVANCE);
**
** IPC_SET_MANUALPLADVANCE sets the status of the Manual Playlist Advance option (1 to turn it on)
*/

#define IPC_GET_NEXT_PLITEM 636
/* (requires Winamp 5.04+)
** SendMessage(hwnd_winamp,WM_WA_IPC,0,IPC_EOF_GET_NEXT_PLITEM);
**
** Sent to Winamp's main window when an item has just finished playback or the next button has been pressed and 
** requesting the new playlist item number to go to.
** Mainly used by gen_jumpex. Subclass this message in your application to return the new item number. 
** -1 for normal winamp operation (default) or the new item number in the playlist to play.
*/

#define IPC_GET_PREVIOUS_PLITEM 637
/* (requires Winamp 5.04+)
** SendMessage(hwnd_winamp,WM_WA_IPC,0,IPC_EOF_GET_PREVIOUS_PLITEM);
**
** Sent to Winamp's main window when the previous button has been pressed and Winamp is requesting the new playlist item number to go to.
** Mainly used by gen_jumpex. Subclass this message in your application to return the new item number. 
** -1 for normal winamp operation (default) or the new item number in the playlist to play.
*/

#define IPC_IS_WNDSHADE 638 
/* (requires Winamp 5.04+)
** SendMessage(hwnd_winamp,WM_WA_IPC,wnd,IPC_IS_WNDSHADE);
**
** 'wnd' is window id as defined for IPC_GETWND, or -1 for main window
** Returns 1 if wnd is set to winshade mode, or 0 if it is not
*/

#define IPC_SETRATING 639 
/* (requires Winamp 5.04+ with ML)
** SendMessage(hwnd_winamp,WM_WA_IPC,rating,IPC_SETRATING);
** 'rating' is an int value from 0 (no rating) to 5 
*/

#define IPC_GETRATING 640 
/* (requires Winamp 5.04+ with ML)
** SendMessage(hwnd_winamp,WM_WA_IPC,0,IPC_GETRATING);
** returns the current item's rating
*/

#define IPC_GETNUMAUDIOTRACKS 641
/* (requires Winamp 5.04+)
** int n = SendMessage(hwnd_winamp,WM_WA_IPC,0,IPC_GETNUMAUDIOTRACKS);
** returns the number of audio tracks for the currently playing item
*/

#define IPC_GETNUMVIDEOTRACKS 642
/* (requires Winamp 5.04+)
** int n = SendMessage(hwnd_winamp,WM_WA_IPC,0,IPC_GETNUMVIDEOTRACKS);
** returns the number of video tracks for the currently playing item
*/

#define IPC_GETAUDIOTRACK 643
/* (requires Winamp 5.04+)
** int cur = SendMessage(hwnd_winamp,WM_WA_IPC,0,IPC_GETAUDIOTRACK);
** returns the id of the current audio track for the currently playing item
*/

#define IPC_GETVIDEOTRACK 644
/* (requires Winamp 5.04+)
** int cur = SendMessage(hwnd_winamp,WM_WA_IPC,0,IPC_GETVIDEOTRACK);
** returns the id of the current video track for the currently playing item
*/

#define IPC_SETAUDIOTRACK 645
/* (requires Winamp 5.04+)
** SendMessage(hwnd_winamp,WM_WA_IPC,track,IPC_SETAUDIOTRACK);
** switch the currently playing item to a new audio track 
*/

#define IPC_SETVIDEOTRACK 646
/* (requires Winamp 5.04+)
** SendMessage(hwnd_winamp,WM_WA_IPC,track,IPC_SETVIDEOTRACK);
** switch the currently playing item to a new video track 
*/

#define IPC_PUSH_DISABLE_EXIT 647
/* (requires Winamp 5.04+)
** SendMessage(hwnd_winamp,WM_WA_IPC,0,IPC_PUSH_DISABLE_EXIT );
** lets you disable or re-enable the UI exit functions (close button, 
** context menu, alt-f4).
** call IPC_POP_DISABLE_EXIT when you are done doing whatever required 
** preventing exit
*/

#define IPC_POP_DISABLE_EXIT  648
/* (requires Winamp 5.04+)
** SendMessage(hwnd_winamp,WM_WA_IPC,0,IPC_POP_DISABLE_EXIT );
** see IPC_PUSH_DISABLE_EXIT
*/

#define IPC_IS_EXIT_ENABLED 649
/* (requires Winamp 5.04+)
** SendMessage(hwnd_winamp,WM_WA_IPC,0,IPC_IS_EXIT_ENABLED);
** returns 0 if exit is disabled, 1 otherwise
*/

#define IPC_IS_AOT 650
/* (requires Winamp 5.04+)
** SendMessage(hwnd_winamp,WM_WA_IPC,0,IPC_IS_AOT);
** returns status of always on top flag. note: this may not match the actual
** TOPMOST window flag while another fullscreen application is focused
*/

#define IPC_USES_RECYCLEBIN 651
/*
** SendMessage(hwnd_winamp,WM_WA_IPC,0,IPC_USES_RECYCLEBIN);
** returns 1 if deleted files should be sent to the recycle bin.
** returns 0 if deleted files should be deleted permanently.
** 
** You should check for this option if your plugin deletes files
** so that your setting matches the winamp setting
*/

#define IPC_FLUSHAUDITS 652
/*
** SendMessage(hwnd_winamp,WM_WA_IPC,0,IPC_FLUSHAUDITS);
** 
** Will flush any pending audits in the global audits que
**
*/

#define IPC_GETPLAYITEM_START 653
#define IPC_GETPLAYITEM_END   654


#define IPC_GETVIDEORESIZE 655
#define IPC_SETVIDEORESIZE 656

// >>>>>>>>>>> Next is 657

#define IPC_PLCMD  1000 

#define PLCMD_ADD  0
#define PLCMD_REM  1
#define PLCMD_SEL  2
#define PLCMD_MISC 3
#define PLCMD_LIST 4

//#define IPC_MBCMD  1001 

#define MBCMD_BACK    0
#define MBCMD_FORWARD 1
#define MBCMD_STOP    2
#define MBCMD_RELOAD  3
#define MBCMD_MISC  4

#define IPC_VIDCMD 1002 

#define VIDCMD_FULLSCREEN 0
#define VIDCMD_1X         1
#define VIDCMD_2X         2
#define VIDCMD_LIB        3
#define VIDPOPUP_MISC     4

//#define IPC_MBURL       1003 //sets the URL
//#define IPC_MBGETCURURL 1004 //copies the current URL into wParam (have a 4096 buffer ready)
//#define IPC_MBGETDESC   1005 //copies the current URL description into wParam (have a 4096 buffer ready)
//#define IPC_MBCHECKLOCFILE 1006 //checks that the link file is up to date (otherwise updates it). wParam=parent HWND
//#define IPC_MBREFRESH   1007 //refreshes the "now playing" view in the library
//#define IPC_MBGETDEFURL 1008 //copies the default URL into wParam (have a 4096 buffer ready)

#define IPC_STATS_LIBRARY_ITEMCNT 1300 // updates library count status

// IPC 2000-3000 reserved for freeform messages, see gen_ff/ff_ipc.h
#define IPC_FF_FIRST 2000
#define IPC_FF_LAST  3000


/*
** General IPC messages in Winamp
**
** All notification messages appear in the lParam of the main window message proceedure.
*/


#define IPC_GETDROPTARGET 3001
/* (requires Winamp 5.0+)
** IDropTarget* IDrop = (IDropTarget*)SendMessage(hwnd_winamp,WM_WA_IPC,0,IPC_GETDROPTARGET);
**
** You call this to retrieve a copy of the IDropTarget interface which Winamp created for
** handling external drag and drop operations on to it's Windows. This is only really
** useful if you're providing an alternate interface and want your Windows to provide the
** same drag and drop support as Winamp normally provides the user. Check out MSDN or
** your prefered search facility for more information about the IDropTarget interface and
** what's needed to handle it in your own instance.
*/

#define IPC_PLAYLIST_MODIFIED 3002 
/* (requires Winamp 5.0+)
** This is a notification message sent to the main Winamp window whenever the playlist is
** modified in any way e.g. the addition/removal of a playlist entry.
**
** It is not a good idea to do large amounts of processing in this notification since it
** will slow down Winamp as playlist entries are modified (especially when you're adding
** in a large playlist).
**
** if(uMsg == WM_WA_IPC && lParam == IPC_PLAYLIST_MODIFIED){
**   // do what you need to do here
** }
*/

#define IPC_PLAYING_FILE 3003 
/* (requires Winamp 5.0+)
** This is a notification message sent to the main Winamp window when playback begins for
** a file. This passes the full filepath in the wParam of the message received.
**
** if(uMsg == WM_WA_IPC && lParam == IPC_PLAYING_FILE){
**   // do what you need to do here, e.g.
**   process_file((char*)wParam);
** }
*/

#define IPC_PLAYING_FILEW 13003 
/* (requires Winamp 5.0+)
** This is a notification message sent to the main Winamp window when playback begins for
** a file. This passes the full filepath in the wParam of the message received.
**
** if(uMsg == WM_WA_IPC && lParam == IPC_PLAYING_FILEW){
**   // do what you need to do here, e.g.
**   process_file((wchar_t*)wParam);
** }
*/

#define IPC_FILE_TAG_MAY_HAVE_UPDATED 3004 // sent to main wnd with the file as parm whenever a file tag might be updated
/* (requires Winamp 5.0+)
** This is a notification message sent to the main Winamp window when a file's tag
** (e.g. id3) may have been updated. This appears to be sent when the InfoBox(..) function
** of the associated input plugin returns a 1 (which is the file information dialog/editor
** call normally).
**
** if(uMsg == WM_WA_IPC && lParam == IPC_FILE_TAG_MAY_HAVE_UPDATED){
**   // do what you need to do here, e.g.
**   update_info_on_file_update((char*)wParam);
** }
*/

#define IPC_ALLOW_PLAYTRACKING 3007
/* (requires Winamp 5.0+)
** SendMessage(hwnd_winamp,WM_WA_IPC,allow,IPC_ALLOW_PLAYTRACKING);
** Send allow as nonzero to allow play tracking and zero to disable the mode.
*/

#define IPC_HOOK_OKTOQUIT 3010 
/* (requires Winamp 5.0+)
** This is a notification message sent to the main Winamp window asking if it's okay to
** close or not. Return zero to prevent Winamp from closing or return anything non-zero
** to allow Winamp to close.
**
** The best implementation of this option is to let the message pass through to the
** original window proceedure since another plugin may want to have a say in the matter
** with regards to Winamp closing.
**
** if(uMsg == WM_WA_IPC && lParam == IPC_HOOK_OKTOQUIT){
**   // do what you need to do here, e.g.
**   if(no_shut_down()){
**     return 1;
**   }
** }
*/

#define IPC_WRITECONFIG 3011
/* (requires Winamp 5.0+)
** SendMessage(hwnd_winamp,WM_WA_IPC,write_type,IPC_WRITECONFIG);
**
** Send write_type as 2 to write all config settings and the current playlist.
**
** Send write_type as 1 to write the playlist and common settings.
** This won't save the following ini settings::
**
**   defext, titlefmt, proxy, visplugin_name, dspplugin_name, check_ft_startup,
**   visplugin_num, pe_fontsize, visplugin_priority, visplugin_autoexec, dspplugin_num,
**   sticon, splash, taskbar, dropaotfs, ascb_new, ttips, riol, minst, whichicon,
**   whichicon2, addtolist, snap, snaplen, parent, hilite, disvis, rofiob, shownumsinpl,
**   keeponscreen, eqdsize, usecursors, fixtitles, priority, shuffle_morph_rate,
**   useexttitles, bifont, inet_mode, ospb, embedwnd_freesize, no_visseh
** (the above was valid for 5.1)
**
** Send write_type as 0 to write the common and less common settings and no playlist.
*/

#define IPC_UPDATE_URL 3012	// pass the URL (char *) in lparam, will be free()'d on done. 

// pass a string to be the name to register, and returns a value > 65536, which is a unique value you can use
// for custom WM_WA_IPC messages. 

#define IPC_GET_RANDFUNC 3015 // returns a function to get a random number
/* (requires Winamp 5.1+)
** int (*randfunc)(void) = (int(*)(void))SendMessage(plugin.hwndParent,WM_WA_IPC,0,IPC_GET_RANDFUNC);
** if(randfunc && randfunc != 1){
**   randfunc();
** }
**
** This will return a positive 32-bit number (essentially 31-bit).
** The check for a returned value of 1 is because that's the default return value from
** SendMessage(..) when it is not handled so is good to check for it in this situation.
*/

#define IPC_METADATA_CHANGED 3017 
/* (requires Winamp 5.1+)
** int changed=SendMessage(hwnd_winamp,WM_WA_IPC,(WPARAM)(char*)field,IPC_METADATA_CHANGED);
** a plugin can SendMessage this to winamp if internal metadata has changes.
** wParam should be a char * of what field changed
**
** Currently used for internal actions (and very few at that) the intent of this api is
** to allow a plugin to call it when metadata has changed in the current playlist entry
** e.g.a new id3v2 tag was found in a stream
**
** The wparam value can either be NULL or a pointer to an ansi string for the metadata
** which has changed. This can be thought of as an advanced version of IPC_UPDTITLE and
** could be used to allow for update of the current title when a specific tag changed.
**
** Not recommended to be used since there is not the complete handling implemented in
** Winamp for it at the moment.
*/

#define IPC_SKIN_CHANGED 3018 
/* (requires Winamp 5.1+)
** This is a notification message sent to the main Winamp window by itself whenever
** the skin in use is changed. There is no information sent by the wParam for this.
**
** if(message == WM_WA_IPC && lparam == IPC_SKIN_CHANGED){
**   // do what you need to do to handle skin changes, e.g. call WADlg_init(hwnd_winamp);
** }
*/

#define IPC_REGISTER_LOWORD_COMMAND 3019 
/* (requires Winamp 5.1+)
** WORD id = SendMessage(hwnd_winamp,WM_WA_IPC,0,IPC_REGISTER_LOWORD_COMMAND);
** This will assign you a unique id for making your own commands such as for extra menu
** entries. The starting value returned by this message will potentially change as and
** when the main resource file of Winamp is updated with extra data so assumptions cannot
** be made on what will be returned and plugin loading order will affect things as well.
*/

#define IPC_GET_DISPATCH_OBJECT 3020  // gets winamp main IDispatch * (for embedded webpages)
#define IPC_GET_UNIQUE_DISPATCH_ID 3021 // gives you a unique dispatch ID that won't conflict with anything in winamp's IDispatch *

#define IPC_ADD_DISPATCH_OBJECT 3022 // add your own dispatch object into winamp's.  This lets embedded webpages access your functions
// pass a pointer to DispatchInfo (see below).  Winamp makes a copy of all this data so you can safely delete it later
typedef struct 
{
	wchar_t *name; // filled in by plugin, make sure it's a unicode string!! (i.e. L"myObject" instead of "myObject).
	struct IDispatch *dispatch; // filled in by plugin 
	DWORD id; // filled in by winamp on return
} DispatchInfo;

#define IPC_GET_PROXY_STRING 3023
/* (requires Winamp 5.11+)
** char* proxy_string=(char*)SendMessage(hwnd_winamp,WM_WA_IPC,0,IPC_GET_PROXY_STRING);
** This will return the same string as is shown on the General Preferences page.
*/

#define IPC_USE_REGISTRY 3024 
/* (requires Winamp 5.11+)
** int reg_enabled=SendMessage(hwnd_winamp,WM_WA_IPC,0,IPC_USE_REGISTRY);
** This will return 0 if you should leave your grubby hands off the registry (i.e. for
** lockdown mode). This is useful if Winamp is run from a USB drive and you can't alter
** system settings, etc.
*/

#define IPC_GET_API_SERVICE 3025 
/* (requires Winamp 5.12+)
** api_service* api_service = (api_service)SendMessage(hwnd_winamp,WM_WA_IPC,0,IPC_GET_API_SERVICE);
** This api will return Winamp's api_service pointer (which is what Winamp3 used, heh).
** If this api is not supported in the Winamp version that is being used at the time then
** the returned value from this api will be 1 which is a good version check.
**
** As of 5.12 there is support for .w5s plugins which reside in %WinampDir%\System and
** are intended for common code that can be shared amongst other plugins e.g. jnetlib.w5s
** which contains jnetlib in one instance instead of being duplicated multiple times in
** all of the plugins which need internet access.
**
** Details on the .w5s specifications will come at some stage (possibly).
*/



typedef struct {
  const wchar_t *filename;
  const wchar_t *metadata;
  wchar_t *ret;
  int retlen;
} extendedFileInfoStructW;

#define IPC_GET_EXTENDED_FILE_INFOW 3026 
/* (requires Winamp 5.13+)
** Pass a pointer to the above struct in wParam
*/
#define IPC_GET_EXTENDED_FILE_INFOW_HOOKABLE 3027
#define IPC_SET_EXTENDED_FILE_INFOW 3028 
/* (requires Winamp 5.13+)
** Pass a pointer to the above struct in wParam
*/

#define IPC_PLAYLIST_GET_NEXT_SELECTED 3029
/* (requires 5.2+)
** int pl_item = SendMessage(hwnd_winamp,WM_WA_IPC,start,IPC_PLAYLIST_GET_NEXT_SELECTED);
**
** This works just like the ListView_GetNextItem(..) macro for ListView controls.
** 'start' is the index of the playlist item that you want to begin the search after or
** set this as -1 for the search to begin with the first item of the current playlist.
**
** This will return the index of the selected playlist according to the 'start' value or
** it returns -1 if there is no selection or no more selected items according to 'start'.
**
** Quick example:
**
** int sel = -1;
** // keep incrementing the start of the search so we get all of the selected entries
** while((sel = SendMessage(hwnd_winamp,WM_WA_IPC,sel,IPC_PLAYLIST_GET_NEXT_SELECTED))!=-1){
**   // show the playlist file entry of the selected item(s) if there were any
**   MessageBox(hwnd_winamp,(char*)SendMessage(hwnd_winamp,WM_WA_IPC,sel,IPC_GETPLAYLISTFILE),0,0);
** }
*/

#define IPC_PLAYLIST_GET_SELECTED_COUNT 3030
/* (requires 5.2+)
** int selcnt = SendMessage(hwnd_winamp,WM_WA_IPC,0,IPC_PLAYLIST_GET_SELECTED_COUNT);
** This will return the number of selected playlist entries.
*/

#define IPC_GET_PLAYING_FILENAME 3031 // returns wchar_t * of the currently playing filename

#define IPC_OPEN_URL 3032 // send either ANSI or Unicode string (it'll figure it out, but it MUST start with "h"!, so don't send ftp:// or anything funny)

#define IPC_REGISTER_WINAMP_IPCMESSAGE 65536 
/* (requires Winamp 5.0+)
** DWORD id = SendMessage(hwnd_winamp,WM_WA_IPC,(WPARAM)name,IPC_REGISTER_WINAMP_IPCMESSAGE);
** The value 'id' returned is > 65536 and is incremented on subsequent calls for unique
** 'name' values passed to it. By using the same 'name' in different plugins will allow a
** common runtime api to be provided for the currently running instance of Winamp
** e.g.
**   PostMessage(hwnd_winamp,WM_WA_IPC,(WPARAM)my_param,registered_ipc);
** Have a look at wa_hotkeys.h for an example on how this api is used in practice for a
** custom WM_WA_IPC message.
**
** if(uMsg == WM_WA_IPC && lParam == id_from_register_winamp_ipcmessage){
**   // do things
** }
*/

#endif//_WA_IPC_H_