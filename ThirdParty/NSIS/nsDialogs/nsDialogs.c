// Unicode support by Jim Park -- 08/10/2007

#include <windows.h>

#include <nsis/pluginapi.h> // nsis plugin

#include "defs.h"
#include "input.h"
#include "rtl.h"

#ifndef ODS_NOACCEL
#define ODS_NOACCEL 0x0100
#define ODS_NOFOCUSRECT 0x0200
#endif
#ifndef DT_HIDEPREFIX
#define DT_HIDEPREFIX 0x00100000
#endif

HINSTANCE g_hInstance;
struct nsDialog g_dialog;
extra_parameters* g_pluginParms;
LPTSTR g_callbackRetVar;

static COLORREF GetLinkColor()
{
  COLORREF clr = GetSysColor(COLOR_HOTLIGHT);
#ifndef _WIN64
  // COLOR_HOTLIGHT is Win98/2000+. GetSysColorBrush is the correct way to
  // detect valid colors but here we just assume nobody uses black.
  if (!clr) clr = RGB(0,0,255);
#endif
  return clr;
}

struct nsControl* NSDFUNC GetControl(HWND hwCtl)
{
  unsigned id = (unsigned)(UINT_PTR) GetProp(hwCtl, NSCONTROL_ID_PROP);

  if (id == 0 || id > g_dialog.controlCount)
  {
    return NULL;
  }

  return &g_dialog.controls[id - 1];
}

INT_PTR CALLBACK ParentProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
  INT_PTR res;

  if (message == WM_NOTIFY_OUTER_NEXT)
  {
    if (wParam == (WPARAM)-1)
    {
      if (g_pluginParms->ExecuteCodeSegment(g_dialog.callbacks.onBack - 1, 0))
      {
        return 0;
      }
    }
  }

  res = CallWindowProc(g_dialog.parentOriginalWndproc, hwnd, message, wParam, lParam);

  if (message == WM_NOTIFY_OUTER_NEXT && !res)
  {
    DestroyWindow(g_dialog.hwDialog);
    HeapFree(GetProcessHeap(), 0, g_dialog.controls);
    g_dialog.hwDialog = NULL;
    g_dialog.controls = NULL;
  }

  return res;
}

LRESULT CALLBACK LinkWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
  struct nsControl* ctl = GetControl(hwnd);

  if(ctl == NULL)
    return 0;

  if(message == WM_SETCURSOR)
  {
    SetCursor(LoadCursor(NULL, IDC_HAND));
    return TRUE;
  }

  return CallWindowProc(ctl->oldWndProc, hwnd, message, wParam, lParam);
}

INT_PTR CALLBACK DialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  switch (uMsg)
  {
    // handle notifications
    case WM_COMMAND:
    {
      HWND hwCtl = GetDlgItem(hwndDlg, LOWORD(wParam));
      struct nsControl* ctl = GetControl(hwCtl);

      if (ctl == NULL)
        break;

      if (HIWORD(wParam) == BN_CLICKED && (ctl->type == NSCTL_BUTTON || ctl->type == NSCTL_LINK))
      {
        if (ctl->callbacks.onClick)
        {
          pushintptr((INT_PTR) hwCtl);
          g_pluginParms->ExecuteCodeSegment(ctl->callbacks.onClick - 1, 0);
        }
      }
      else if (HIWORD(wParam) == EN_CHANGE && ctl->type == NSCTL_EDIT)
      {
        if (ctl->callbacks.onChange)
        {
          pushintptr((INT_PTR) hwCtl);
          g_pluginParms->ExecuteCodeSegment(ctl->callbacks.onChange - 1, 0);
        }
      }
      else if (HIWORD(wParam) == LBN_SELCHANGE && ctl->type == NSCTL_LISTBOX)
      {
        if (ctl->callbacks.onChange)
        {
          pushintptr((INT_PTR) hwCtl);
          g_pluginParms->ExecuteCodeSegment(ctl->callbacks.onChange - 1, 0);
        }
      }
      else if ((HIWORD(wParam) == CBN_EDITUPDATE || HIWORD(wParam) == CBN_SELCHANGE)
                && ctl->type == NSCTL_COMBOBOX)
      {
        if (ctl->callbacks.onChange)
        {
          pushintptr((INT_PTR) hwCtl);
          g_pluginParms->ExecuteCodeSegment(ctl->callbacks.onChange - 1, 0);
        }
      }
      else if (HIWORD(wParam) == STN_CLICKED && ctl->type == NSCTL_STATIC)
      {
        if (ctl->callbacks.onClick)
        {
          pushintptr((INT_PTR) hwCtl);
          g_pluginParms->ExecuteCodeSegment(ctl->callbacks.onClick - 1, 0);
        }
      }

      break;
    }

    case WM_NOTIFY:
    {
      LPNMHDR nmhdr = (LPNMHDR) lParam;
      struct nsControl* ctl = GetControl(nmhdr->hwndFrom);
      int *pFlag = &g_pluginParms->exec_flags->silent, orgFlag, ret; // The silent flag can only be changed in .onInit and custom pages will not be displayed in silent mode so we can use this flag in the callback

      if (ctl == NULL)
        break;

      if (!ctl->callbacks.onNotify)
        break;

      orgFlag = *pFlag, *pFlag = 0;
      pushintptr((INT_PTR) nmhdr);
      pushintptr(nmhdr->code);
      pushintptr((INT_PTR) nmhdr->hwndFrom);
      g_pluginParms->ExecuteCodeSegment(ctl->callbacks.onNotify - 1, 0);
      ret = *pFlag, *pFlag = orgFlag;
      if (ret)
        return DlgRet(hwndDlg, StrToIntPtr(g_callbackRetVar));
    }
    break;

    // handle links
    case WM_DRAWITEM:
    {
      DRAWITEMSTRUCT* lpdis = (DRAWITEMSTRUCT*)lParam;
      RECT rc;
      TCHAR text[1024];

      // http://blogs.msdn.com/oldnewthing/archive/2005/05/03/414317.aspx#414357
      // says we should call SystemParametersInfo(SPI_GETKEYBOARDCUES,...) to make
      // sure, does not seem to be required, might be a win2k bug, or it might
      // only apply to menus
      BOOL hideFocus = (lpdis->itemState & ODS_NOFOCUSRECT);
      BOOL hideAccel = (lpdis->itemState & ODS_NOACCEL);

      struct nsControl* ctl = GetControl(lpdis->hwndItem);
      if (ctl == NULL)
        break;

      // We need lpdis->rcItem later
      rc = lpdis->rcItem;

      // Get button's text
      text[0] = _T('\0');
      GetWindowText(lpdis->hwndItem, text, 1024);

      // Calculate needed size of the control
      DrawText(lpdis->hDC, text, -1, &rc, DT_VCENTER | DT_WORDBREAK | DT_CALCRECT);

      // Make some more room so the focus rect won't cut letters off
      rc.right = min(rc.right + 2, lpdis->rcItem.right);

      // Move rect to right if in RTL mode
      if (g_dialog.rtl)
      {
        rc.left += lpdis->rcItem.right - rc.right;
        rc.right += lpdis->rcItem.right - rc.right;
      }

      if (lpdis->itemAction & ODA_DRAWENTIRE)
      {
        DWORD xtraDrawStyle = (g_dialog.rtl ? DT_RTLREADING : 0);
        if (hideAccel)
          xtraDrawStyle |= DT_HIDEPREFIX;

        // Use the system color unless the user has set another using SetCtlColors
        if (!GetWindowLongPtr(lpdis->hwndItem, GWLP_USERDATA))
          SetTextColor(lpdis->hDC, GetLinkColor());

        // Draw the text
        DrawText(lpdis->hDC, text, -1, &rc, xtraDrawStyle | DT_CENTER | DT_VCENTER | DT_WORDBREAK);
      }

      // Draw the focus rect if needed
      if (((lpdis->itemState & ODS_FOCUS) && (lpdis->itemAction & ODA_DRAWENTIRE)) || (lpdis->itemAction & ODA_FOCUS))
      {
        // NB: when not in DRAWENTIRE mode, this will actually toggle the focus
        // rectangle since it's drawn in a XOR way
        if (!hideFocus)
          DrawFocusRect(lpdis->hDC, &rc);
      }

      return TRUE;
    }

    // handle colors
    case WM_CTLCOLORSTATIC:
    case WM_CTLCOLOREDIT:
    case WM_CTLCOLORDLG:
    case WM_CTLCOLORBTN:
    case WM_CTLCOLORLISTBOX:
      // let the NSIS window handle colors, it knows best
      return SendMessage(g_dialog.hwParent, uMsg, wParam, lParam);

    // bye bye
    case WM_DESTROY:
    {
      unsigned i;
      for (i = 0; i < g_dialog.controlCount; i++)
      {
        RemoveProp(g_dialog.controls[i].window, NSCONTROL_ID_PROP);
      }
      break;
    }
  }

  return FALSE;
}

static UINT_PTR PluginCallback(enum NSPIM msg)
{
  return 0;
}

void __declspec(dllexport) Create(HWND hwndParent, int string_size, TCHAR *variables, stack_t **stacktop, extra_parameters *extra)
{
  HWND hwPlacementRect;
  RECT rcPlacement;

  EXDLL_INIT();

  extra->RegisterPluginCallback(g_hInstance, PluginCallback);

  g_dialog.hwParent = hwndParent;
  g_pluginParms = extra;
  g_callbackRetVar = GetVar(variables, string_size, 31); // The undocumented $_OUTDIR variable

  hwPlacementRect = GetDlgItem(hwndParent, popint());
  GetWindowRect(hwPlacementRect, &rcPlacement);
  MapWindowPoints(NULL, hwndParent, (LPPOINT) &rcPlacement, 2);

  g_dialog.hwDialog = CreateDialog(g_hInstance, MAKEINTRESOURCE(1), hwndParent, DialogProc);

  if (g_dialog.hwDialog == NULL)
  {
    pushstring(_T("error"));
    return;
  }

  SetWindowPos(
    g_dialog.hwDialog,
    0,
    rcPlacement.left,
    rcPlacement.top,
    rcPlacement.right - rcPlacement.left,
    rcPlacement.bottom - rcPlacement.top,
    SWP_NOZORDER | SWP_NOACTIVATE
  );

  g_dialog.parentOriginalWndproc = (WNDPROC) SetWindowLongPtr(hwndParent, DWLP_DLGPROC, (LONG_PTR) ParentProc);

  g_dialog.rtl = FALSE;

  g_dialog.controlCount = 0;
  g_dialog.controls = (struct nsControl*) HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, 0);

  g_dialog.callbacks.onBack = 0;

  pushintptr((INT_PTR) g_dialog.hwDialog);
}

void __declspec(dllexport) CreateControl(HWND hwndParent, int string_size, TCHAR *variables, stack_t **stacktop, extra_parameters *extra)
{
  TCHAR *className;
  TCHAR *text;

  HWND hwItem;
  int x, y, width, height;
  DWORD style, exStyle;
  size_t id;

  // get info from stack

  className = (TCHAR *) HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, (g_stringsize * 2)*sizeof(TCHAR));
  text = &className[g_stringsize];

  if (!className)
  {
    pushstring(_T("error"));
    return;
  }

  if (popstringn(className, 0))
  {
    pushstring(_T("error"));
    HeapFree(GetProcessHeap(), 0, className);
    return;
  }

  style = (DWORD) popint_or();
  exStyle = (DWORD) popint_or();

  PopPlacement(&x, &y, &width, &height);

  if (popstringn(text, 0))
  {
    pushstring(_T("error"));
    HeapFree(GetProcessHeap(), 0, className);
    return;
  }

  // create item descriptor

  id = g_dialog.controlCount;
  g_dialog.controlCount++;
  g_dialog.controls = (struct nsControl*) HeapReAlloc(
    GetProcessHeap(),
    HEAP_ZERO_MEMORY,
    g_dialog.controls,
    g_dialog.controlCount * sizeof(struct nsControl));

  if (!lstrcmpi(className, _T("BUTTON")))
    g_dialog.controls[id].type = NSCTL_BUTTON;
  else if (!lstrcmpi(className, _T("EDIT")))
    g_dialog.controls[id].type = NSCTL_EDIT;
  else if (!lstrcmpi(className, _T("COMBOBOX")))
    g_dialog.controls[id].type = NSCTL_COMBOBOX;
  else if (!lstrcmpi(className, _T("LISTBOX")))
    g_dialog.controls[id].type = NSCTL_LISTBOX;
  else if (!lstrcmpi(className, _T("RichEdit")))
    g_dialog.controls[id].type = NSCTL_RICHEDIT;
  else if (!lstrcmpi(className, _T("RICHEDIT_CLASS")))
    g_dialog.controls[id].type = NSCTL_RICHEDIT2;
  else if (!lstrcmpi(className, _T("STATIC")))
    g_dialog.controls[id].type = NSCTL_STATIC;
  else if (!lstrcmpi(className, _T("LINK")))
    g_dialog.controls[id].type = NSCTL_LINK;
  else
    g_dialog.controls[id].type = NSCTL_UNKNOWN;

  // apply rtl to style

  ConvertStyleToRTL(g_dialog.controls[id].type, &style, &exStyle);

  // create item's window

  hwItem = CreateWindowEx(
    exStyle,
    lstrcmpi(className, _T("LINK")) ? className : _T("BUTTON"),
    text,
    style,
    x, y, width, height,
    g_dialog.hwDialog,
    (HMENU) (1200 + id),
    g_hInstance,
    NULL);

  g_dialog.controls[id].window = hwItem;

  // remember id

  SetProp(hwItem, NSCONTROL_ID_PROP, (HANDLE) (id + 1));

  // set font

  SendMessage(hwItem, WM_SETFONT, SendMessage(g_dialog.hwParent, WM_GETFONT, 0, 0), TRUE);

  // set the WndProc for the link control

  if(g_dialog.controls[id].type == NSCTL_LINK)
    g_dialog.controls[id].oldWndProc = (WNDPROC) SetWindowLongPtr(hwItem, GWLP_WNDPROC, (LONG_PTR) LinkWndProc);

  // push back result

  pushintptr((INT_PTR) hwItem);

  // done

  HeapFree(GetProcessHeap(), 0, className);
}

// for backward compatibility (2.29 had CreateItem)
void __declspec(dllexport) CreateItem(HWND hwndParent, int string_size, TCHAR *variables, stack_t **stacktop, extra_parameters *extra)
{
  CreateControl(hwndParent, string_size, variables, stacktop, extra);
}

void __declspec(dllexport) SetUserData(HWND hwndParent, int string_size, TCHAR *variables, stack_t **stacktop, extra_parameters *extra)
{
  HWND hwCtl;
  struct nsControl* ctl;

  // get info from stack

  hwCtl = (HWND) popintptr();

  if (!IsWindow(hwCtl))
  {
    popintptr(); // remove user data from stack
    return;
  }

  // get descriptor

  ctl = GetControl(hwCtl);

  if (ctl == NULL)
    return;

  // set user data

  popstringn(ctl->userData, USERDATA_SIZE);
}

void __declspec(dllexport) GetUserData(HWND hwndParent, int string_size, TCHAR *variables, stack_t **stacktop, extra_parameters *extra)
{
  HWND hwCtl;
  struct nsControl* ctl;

  // get info from stack

  hwCtl = (HWND) popintptr();

  if (!IsWindow(hwCtl))
  {
    pushstring(_T(""));
    return;
  }

  // get descriptor

  ctl = GetControl(hwCtl);

  if (ctl == NULL)
  {
    pushstring(_T(""));
    return;
  }

  // return user data

  pushstring(ctl->userData);
}

void CALLBACK TimerProc(HWND hwnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime)
{
  // we use a timer proc instead of WM_TIMER to make sure no one messes with the ids but us
  g_pluginParms->ExecuteCodeSegment((int)(idEvent - 1), 0);
}

void __declspec(dllexport) CreateTimer(HWND hwndParent, int string_size, TCHAR *variables, stack_t **stacktop, extra_parameters *extra)
{
  UINT callback;
  UINT interval;

  // get info from stack

  callback = popint();
  interval = popint();

  if (!callback || !interval)
    return;

  // create timer

  SetTimer(
    g_dialog.hwDialog,
    callback,
    interval,
    TimerProc);
}

void nsdKillTimer(HWND hwndParent, int string_size, TCHAR *variables, stack_t **stacktop, extra_parameters *extra)
{
  UINT id;

  // get timer id from stack

  id = popint();

  // kill timer

  KillTimer(g_dialog.hwDialog, id);
}

void NSDFUNC SetControlCallback(size_t callbackIdx)
{
  HWND hwCtl;
  nsFunction  callback;
  nsFunction* callbacks;
  struct nsControl* ctl;

  // get info from stack

  hwCtl = (HWND) popintptr();
  callback = (nsFunction) popint();

  if (!IsWindow(hwCtl))
    return;

  // get descriptor

  ctl = GetControl(hwCtl);

  if (ctl == NULL)
    return;

  // set callback

  callbacks = (nsFunction*) &ctl->callbacks;
  callbacks[callbackIdx] = callback;
}

void NSDFUNC SetDialogCallback(size_t callbackIdx)
{
  nsFunction  callback;
  nsFunction* callbacks;

  // get info from stack

  callback = (nsFunction) popint();

  // set callback

  callbacks = (nsFunction*) &g_dialog.callbacks;
  callbacks[callbackIdx] = callback;
}


void __declspec(dllexport) OnClick(HWND hwndParent, int string_size, TCHAR *variables, stack_t **stacktop, extra_parameters *extra)
{
  SetControlCallback(CTL_CALLBACK_IDX(onClick));
}

void __declspec(dllexport) OnChange(HWND hwndParent, int string_size, TCHAR *variables, stack_t **stacktop, extra_parameters *extra)
{
  SetControlCallback(CTL_CALLBACK_IDX(onChange));
}

void __declspec(dllexport) OnNotify(HWND hwndParent, int string_size, TCHAR *variables, stack_t **stacktop, extra_parameters *extra)
{
  SetControlCallback(CTL_CALLBACK_IDX(onNotify));
}

void __declspec(dllexport) OnBack(HWND hwndParent, int string_size, TCHAR *variables, stack_t **stacktop, extra_parameters *extra)
{
  SetDialogCallback(DLG_CALLBACK_IDX(onBack));
}

void __declspec(dllexport) Show(HWND hwndParent, int string_size, TCHAR *variables, stack_t **stacktop, extra_parameters *extra)
{
  // tell NSIS to remove old inner dialog and pass handle of the new inner dialog

  SendMessage(hwndParent, WM_NOTIFY_CUSTOM_READY, (WPARAM) g_dialog.hwDialog, 0);
  ShowWindow(g_dialog.hwDialog, SW_SHOWNA);

  // message loop

  while (g_dialog.hwDialog)
  {
    MSG msg;
    GetMessage(&msg, NULL, 0, 0);
    if (!IsDialogMessage(g_dialog.hwDialog, &msg) && !IsDialogMessage(g_dialog.hwParent, &msg))
    {
      TranslateMessage(&msg);
      DispatchMessage(&msg);
    }
  }

  // reset wndproc

  SetWindowLongPtr(hwndParent, DWLP_DLGPROC, (LONG_PTR) g_dialog.parentOriginalWndproc);
}

BOOL WINAPI DllMain(HINSTANCE hInst, ULONG ul_reason_for_call, LPVOID lpReserved)
{
  g_hInstance = hInst;
  return TRUE;
}
