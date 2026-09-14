// Unicode support by Jim Park -- 08/24/2007

#ifndef __NS_DIALOGS__DEFS_H__
#define __NS_DIALOGS__DEFS_H__

#include <windows.h>

#define NSDFUNC WINAPI

#define GetVar(vars, varlen, varid) ( (vars) + ((varid) * (varlen)) )
#define DlgRet(hDlg, val) ( SetWindowLongPtr((hDlg), DWLP_MSGRESULT, (val)) | TRUE )
#define StrToIntPtr nsishelper_str_to_ptr


typedef int nsFunction;

enum nsControlType
{
  NSCTL_UNKNOWN,
  NSCTL_BUTTON,
  NSCTL_EDIT,
  NSCTL_COMBOBOX,
  NSCTL_LISTBOX,
  NSCTL_RICHEDIT,
  NSCTL_RICHEDIT2,
  NSCTL_STATIC,
  NSCTL_LINK,
  NSCTL_TREE
};

struct nsDialogCallbacks
{
  nsFunction onBack;
};

#define DLG_CALLBACK_IDX(x) (FIELD_OFFSET(struct nsDialogCallbacks, x)/sizeof(nsFunction))

struct nsControlCallbacks
{
  nsFunction onClick;
  nsFunction onChange;
  nsFunction onNotify;
};

#define CTL_CALLBACK_IDX(x) (FIELD_OFFSET(struct nsControlCallbacks, x)/sizeof(nsFunction))

#define USERDATA_SIZE 1024

struct nsControl
{
  HWND window;
  enum nsControlType type;
  TCHAR userData[USERDATA_SIZE];
  struct nsControlCallbacks callbacks;
  WNDPROC oldWndProc;
};

struct nsDialog
{
  HWND hwDialog;
  HWND hwParent;

  WNDPROC parentOriginalWndproc;

  BOOL rtl;

  struct nsDialogCallbacks callbacks;

  unsigned controlCount;

  struct nsControl* controls;
};

#define NSCONTROL_ID_PROP _T("NSIS: nsControl pointer property")

#endif//__NS_DIALOGS__DEFS_H__
