#ifndef __CADSDK_H__
#define __CADSDK_H__

enum IPCMESSAGE
{
	/*
		Most of these messages are sent to the player. Messages that the player
		should send back to CAD are noted by '[Sent by player]'

		Based on information available in cad-sdk-2.1.pdf and in foo_cdartdisplay source.
	*/

	// uMsg: WM_USER, wParam: 0, lParam: 100
	IPC_PLAY						= 100,
	
	// uMsg: WM_USER, wParam: 0, lParam: 101
	IPC_PLAYPAUSE,
	
	// uMsg: WM_USER, wParam: 0, lParam: 102
	IPC_FORCEPAUSE,
	
	// uMsg: WM_USER, wParam: 0, lParam: 103
	IPC_STOP,

	// uMsg: WM_USER, wParam: 0, lParam: 104
	IPC_NEXT,

	// uMsg: WM_USER, wParam: 0, lParam: 105
	IPC_PREVIOUS,

	// uMsg: WM_USER, wParam: volume (0 - 100), lParam: 108
	IPC_SET_VOLUME					= 108,

	// uMsg: WM_USER, wParam: 0, lParam: 109, result: 0 to 100
	IPC_GET_VOLUME,

	// uMsg: WM_USER, wParam: 0, lParam: 110
	// When the player recieves this message, it should send
	// the IPC_CURRENT_TRACK message back to CAD
	IPC_GET_CURRENT_TRACK,

	// uMsg: WM_USER, wParam: 0, lParam: 113
	IPC_GET_DURATION				= 113,

	// uMsg: WM_USER, wParam: position (seconds), lParam: 114
	IPC_SET_POSITION,

	// uMsg: WM_USER, wParam: 0, lParam: 115, result: 1 if playing
	IPC_IS_PLAYING,

	// uMsg: WM_USER, wParam: 0, lParam: 116, result: 1 if paused
	IPC_IS_PAUSED,

	// uMsg: WM_USER, wParam: 0, lParam: 117
	IPC_GET_LIST_LENGTH,

	// uMsg: WM_USER, wParam: position, lParam: 118
	IPC_SET_LIST_POS,

	// uMsg: WM_USER, wParam: 0, lParam: 119
	IPC_GET_LIST_ITEM,
	
	// uMsg: WM_USER, wParam: hwnd, lParam: 120
	// CAD sends this mesage to the player on startup. The player
	// should then send all future WM_COPYDATA messages to this window.
	IPC_SET_CALLBACK_HWND,

	// uMsg: WM_USER, wParam: 0, lParam: 121
	IPC_GET_LIST_POS,

	// uMsg: WM_USER, wParam: 0, lParam: 122, result: position (in seconds)
	IPC_GET_POSITION,

	// uMsg: WM_USER, wParam: 0, lParam: 123 [Sent by player]
	// The player should send this message when the track changes.
	IPC_TRACK_CHANGED_NOTIFICATION,

	// uMsg: WM_USER, wParam: 0, lParam: 124
	IPC_SHOW_PLAYER_WINDOW,

	// uMsg: WM_USER, wParam: 0, lParam: 125, result: 0 (stopped), 1 (playing), or 2 (paused)
	IPC_GET_PLAYER_STATE,

	// uMsg: WM_USER, wParam: 0, lParam: 126 [Sent by player]
	// The player should send this notification when the play state changes.
	IPC_PLAYER_STATE_CHANGED_NOTIFICATION,

	// uMsg: WM_USER, wParam: 0, lParam: 127 [Not implemented in CAD yet]
	IPC_AUTOENQUEUE_OPTIONS,

	// uMsg: WM_USER, wParam: 0 or 1, lParam: 128
	IPC_SET_REPEAT,

	// uMsg: WM_USER, wParam: 0 or 1, lParam: 129 [Sent by/to player]
	// The player should send this message when it quits.
	// CAD will also send this message on exit. Upon receival, the player should
	// disconnect the communication interface and get ready for a IPC_SET_CALLBACK_HWND message.
	IPC_SHUTDOWN_NOTIFICATION,

	// uMsg: WM_USER, wParam: 0, lParam: 130, result: 1 if shuffle is active
	IPC_GET_REPEAT,

	// uMsg: WM_USER, wParam: 0, lParam: 131
	// The player should quit completely upon receival.
	IPC_CLOSE_PLAYER,

	// uMsg: WM_USER, wParam: 0, lParam: 140, result: 1 if shuffle is active
	IPC_GET_SHUFFLE					= 140,

	// uMsg: WM_USER, wParam: 0 or 1, lParam: 141
	IPC_SET_SHUFFLE,

	// uMsg: WM_USER, wParam: rating (0 - 10), lParam: 141 [Sent by/to player]
	// CAD sends this message to the player with the new rating (0 - 10). The
	// player sends this message when the rating changes.
	IPC_RATING_CHANGED_NOTIFICATION = 639,

	// uMsg: WM_COPYDATA, wParam: 700, lParam: COPYDATASTRUCT [Sent by player]
	// The player sends this message on start up. Refer to the SDK manual for detailed info.
	IPC_REGISTER_PLAYER				= 700,	// Sent by player to CAD on startup.

	// uMsg: WM_COPYDATA, wParam: 701, lParam: COPYDATASTRUCT [Sent by player]
	// The player sends this when it receives the IPC_GET_CURRENT_TRACK message.
	IPC_CURRENT_TRACK_INFO,

	// uMsg: WM_COPYDATA, wParam: 702, lParam: COPYDATASTRUCT [Sent by player]
	// The player sends this when it receives the IPC_GET_CURRENT_LYRICS message.
	IPC_CURRENT_LYRICS,

	// uMsg: WM_COPYDATA, wParam: 703, lParam: COPYDATASTRUCT [Sent by player]
	// The player sends this when, for example, new lyrics have been retrieved.
	IPC_NEW_LYRICS,

	// uMsg: WM_COPYDATA, wParam: 800, lParam: COPYDATASTRUCT [Sent by player]
	// The player sends this when a new cover is available for playing track.
	IPC_NEW_COVER					= 800,

	// uMsg: WM_USER, wParam: 0, lParam: 801
	IPC_GET_CURRENT_LYRICS,
};

#endif
