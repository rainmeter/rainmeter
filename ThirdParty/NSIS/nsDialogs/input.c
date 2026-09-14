// Unicode support by Jim Park -- 08/24/2007

#include <windows.h>

#include <nsis/pluginapi.h> // nsis plugin

#include "input.h"
#include "defs.h"
#include "rtl.h"

extern struct nsDialog g_dialog;

static int NSDFUNC ConvertPlacement(TCHAR *str, int total, int height)
{
  TCHAR unit = *CharPrev(str, str + lstrlen(str));
  int x = myatoi(str);

  if (unit == _T('%'))
  {
    if (x < 0)
    {
      return MulDiv(total, 100 + x, 100);
    }

    return MulDiv(total, x, 100);
  }
  else if (unit == _T('u'))
  {
    RECT r;

    r.left = r.top = x;
    r.right = r.bottom = 0;

    MapDialogRect(g_dialog.hwParent, &r);

    if (height)
      return x >= 0 ? r.top : total + r.top;
    else
      return x >= 0 ? r.left : total + r.left;
  }

  if (x < 0)
  {
    return total + x;
  }

  return x;
}

int NSDFUNC PopPlacement(int *x, int *y, int *width, int *height)
{
  RECT dialogRect;
  int  dialogWidth;
  int  dialogHeight;
  TCHAR buf[1024];

  GetClientRect(g_dialog.hwDialog, &dialogRect);
  dialogWidth = dialogRect.right;
  dialogHeight = dialogRect.bottom;

  if (popstringn(buf, 1024))
    return 1;

  *x = ConvertPlacement(buf, dialogWidth, 0);

  if (popstringn(buf, 1024))
    return 1;

  *y = ConvertPlacement(buf, dialogHeight, 1);

  if (popstringn(buf, 1024))
    return 1;

  *width = ConvertPlacement(buf, dialogWidth, 0);

  if (popstringn(buf, 1024))
    return 1;

  *height = ConvertPlacement(buf, dialogHeight, 1);

  ConvertPosToRTL(x, *width, dialogWidth);

  return 0;
}
