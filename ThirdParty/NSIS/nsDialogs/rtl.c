#include <windows.h>
#include <commctrl.h>

#include <nsis/pluginapi.h> // nsis plugin

#include "defs.h"

#ifndef WS_EX_RIGHT
#  define WS_EX_RIGHT 0x1000
#endif

#ifndef WS_EX_RTLREADING
#  define WS_EX_RTLREADING 0x2000
#endif

#ifndef WS_EX_LEFTSCROLLBAR
#  define WS_EX_LEFTSCROLLBAR 0x4000
#endif

#ifndef TVS_RTLREADING
#  define TVS_RTLREADING 64
#endif

extern struct nsDialog g_dialog;

void __declspec(dllexport) SetRTL(HWND hwndParent, int string_size, TCHAR *variables, stack_t **stacktop, extra_parameters *extra)
{
  g_dialog.rtl = (BOOL) popint();
}

void NSDFUNC ConvertStyleToRTL(enum nsControlType type, LPDWORD style, LPDWORD exStyle)
{
  if (!g_dialog.rtl)
    return;

  switch (type)
  {
  case NSCTL_LINK:
  case NSCTL_BUTTON:
    *style ^= BS_LEFTTEXT | BS_RIGHT | BS_LEFT;

    if ((*style & (BS_LEFT|BS_RIGHT)) == (BS_LEFT|BS_RIGHT))
    {
      *style ^= BS_LEFT | BS_RIGHT;
      if (*style & (BS_RADIOBUTTON | BS_CHECKBOX | BS_USERBUTTON))
      {
        *style |= BS_RIGHT;
      }
    }
    break;

  case NSCTL_EDIT:
    if ((*style & ES_CENTER) == 0)
    {
      *style ^= ES_RIGHT;
    }
    break;

  case NSCTL_STATIC:
    if ((*style & SS_TYPEMASK) == SS_LEFT || (*style & SS_TYPEMASK) == SS_LEFTNOWORDWRAP)
    {
      *style &= ~SS_TYPEMASK;
      *style |= SS_RIGHT;
    }
    else if ((*style & SS_TYPEMASK) == SS_ICON) {
      *style |= SS_CENTERIMAGE;
    }
    break;

  case NSCTL_RICHEDIT:
  case NSCTL_RICHEDIT2:
    if ((*style & ES_CENTER) == 0)
    {
      *style ^= ES_RIGHT;
    }
    break;

  case NSCTL_TREE:
    *style |= TVS_RTLREADING;
    *exStyle |= WS_EX_RIGHT | WS_EX_LAYOUTRTL;
    break;

  default:
    *exStyle |= WS_EX_RIGHT;
    break;
  }

  *exStyle |= WS_EX_RTLREADING | WS_EX_LEFTSCROLLBAR;
}

void NSDFUNC ConvertPosToRTL(int *x, int width, int dialogWidth)
{
  if (!g_dialog.rtl)
    return;

  *x = dialogWidth - width - *x;
}
