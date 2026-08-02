#ifndef WA_HOTKEYS
#define WA_HOTKEYS

//#define IPC_GEN_HOTKEYS_ADD xxxx //pass a genHotkeysAddStruct * struct in data
//
//To get the IPC_GEN_HOTKEYS_ADD IPC number, do this:
//
//  genhotkeys_add_ipc=SendMessage(winampWindow,WM_WA_IPC,(WPARAM)&"GenHotkeysAdd",IPC_REGISTER_WINAMP_IPCMESSAGE);
//
//Then you can use:
//
//  PostMessage(winampWindow,WM_WA_IPC,(WPARAM)&myGenHotkeysAddStruct,genhotkeys_add_ipc);
//

//flags for the genHotkeysAddStruct struct
#define HKF_BRING_TO_FRONT 0x1  // calls SetForegroundWindow before sending the message
#define HKF_HWND_WPARAM 0x2     // sets wParam with Winamp's window handle
#define HKF_COPY 0x4            // copies returned text to the clipboard
#define HKF_PLPOS_WPARAM 0x8    // sets wParam with current pledit position
#define HKF_ISPLAYING_WL 0x10   // sets wParam to genHotkeysAddStruct's wParam if playing, lParam if not
                                // uses IPC_ISPLAYING to check if playing
#define HKF_SHOWHIDE 0x20       // brings Winamp to front or minimizes Winamp if already at front
#define HKF_NOSENDMSG 0x40      // don't send any message to the winamp window

#define HKF_DISABLED 0x80000000

typedef struct {
  char *name;     //name that will appear in the Global Hotkeys preferences panel
  DWORD flags;    //one or more flags from above
  UINT uMsg;      //message that will be sent to winamp's main window (must always be !=NULL)
  WPARAM wParam;  //wParam that will be sent to winamp's main window
  LPARAM lParam;  //lParam that will be sent to winamp's main window
  char *id;       //unique string to identify this command - case insensitive
  HWND wnd;       //set the HWND to send message (or 0 for main winamp window)
  
  int extended[6]; //for future extension - always set to zero!
} genHotkeysAddStruct;

#endif