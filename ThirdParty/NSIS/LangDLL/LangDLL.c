#include <windows.h>
#include "resource.h"
#include <nsis/nsis_tchar.h>

// JF> updated usage
// call like this:
// LangDLL:LangDialog "Window Title" "Window subtext" <number of languages|"A">[C][F] language_text language_id [codepage] ... [font_size font_face]
// ex:
//  LangDLL:LangDialog "Language Selection" "Choose a language" "2" French 1036 English 1033
// or (the F after the 2 means we're supplying font information)
//  LangDLL:LangDialog "Language Selection" "Choose a language" "2F" French 1036 English 1033 12 Garamond
//
// Unicode support added by Jim Park -- 07/27/2007


#include <nsis/pluginapi.h> // nsis plugin
#define NSIS_DECLSPEC_DLLEXPORT __declspec(dllexport) // BUGBUG: Compiler specific

HINSTANCE g_hInstance;
HWND g_hwndParent;

TCHAR temp[1024];
TCHAR g_wndtitle[1024], g_wndtext[1024];
int dofont;
int docp;

int langs_num;
int visible_langs_num;

struct lang {
  TCHAR *name;
  TCHAR *id;
  UINT cp;
} *langs;

#ifndef UNICODE
static UINT AllowLang(struct lang*pL)
{
  UINT acp = GetACP(), lcp = pL->cp, allow = lcp == acp;
  /*
  ** Workaround for bug #1185:
  ** English and German can be displayed in cp1250
  */
  if (acp == 1250 && lcp == 1252)
  {
    const UINT lid = myatou(pL->id);
    if (lid == 1033 // English
     || lid == 1031 // German
    )
      ++allow;
  }
  return allow;
}
#endif

INT_PTR CALLBACK DialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  INT_PTR i;
  int size;
  TCHAR *selected_language = NULL;
  static HFONT font;
  switch (uMsg) {
    case WM_INITDIALOG:
      // add languages
      for (i = visible_langs_num - 1; i >= 0; i--) {
        INT_PTR cbi;

        cbi = SendDlgItemMessage(hwndDlg, IDC_LANGUAGE, CB_ADDSTRING, 0, (LPARAM) langs[i].name);
        SendDlgItemMessage(hwndDlg, IDC_LANGUAGE, CB_SETITEMDATA, cbi, (LPARAM) langs[i].id);

        // remember selected language
        if (!lstrcmp(langs[i].id, getuservariable(INST_LANG))) {
          selected_language = langs[i].name;
        }
      }
      // select the current language
      if (selected_language)
        SendDlgItemMessage(hwndDlg, IDC_LANGUAGE, CB_SELECTSTRING, (WPARAM) -1, (LPARAM) selected_language);
      else
        SendDlgItemMessage(hwndDlg, IDC_LANGUAGE, CB_SETCURSEL, 0, 0);
      // set texts
      SetDlgItemText(hwndDlg, IDC_TEXT, g_wndtext);
      SetWindowText(hwndDlg, g_wndtitle);
      SendDlgItemMessage(hwndDlg, IDC_APPICON, STM_SETICON, (LPARAM)LoadIcon(GetModuleHandle(0),MAKEINTRESOURCE(103)), 0);
      // set font
      if (dofont && !popstring(temp)) {
        size = myatou(temp);
        if (!popstring(temp)) {
          LOGFONT f;
          SecureZeroMemory(&f, sizeof(f));
          if (lstrcmp(temp, _T("MS Shell Dlg"))) {
            f.lfHeight = -MulDiv(size, GetDeviceCaps(GetDC(hwndDlg), LOGPIXELSY), 72);
            lstrcpy(f.lfFaceName, temp);
            font = CreateFontIndirect(&f);
            SendMessage(hwndDlg, WM_SETFONT, (WPARAM)font, 1);
            SendDlgItemMessage(hwndDlg, IDOK, WM_SETFONT, (WPARAM)font, 1);
            SendDlgItemMessage(hwndDlg, IDCANCEL, WM_SETFONT, (WPARAM)font, 1);
            SendDlgItemMessage(hwndDlg, IDC_LANGUAGE, WM_SETFONT, (WPARAM)font, 1);
            SendDlgItemMessage(hwndDlg, IDC_TEXT, WM_SETFONT, (WPARAM)font, 1);
          }
        }
      }
      // show window
      ShowWindow(hwndDlg, SW_SHOW);
      break;
    case WM_COMMAND:
      switch (LOWORD(wParam)) {
        case IDOK:
          // push result on the stack
          i = SendDlgItemMessage(hwndDlg, IDC_LANGUAGE, CB_GETCURSEL, 0, 0);
          i = SendDlgItemMessage(hwndDlg, IDC_LANGUAGE, CB_GETITEMDATA, i, 0);
          if (i != CB_ERR && i) {
            pushstring((TCHAR *) i);
          } else {
            // ?!
            pushstring(_T("cancel"));
          }
          // end dialog
          EndDialog(hwndDlg, 0);
          break;
        case IDCANCEL:
          // push "cancel" on the stack
          pushstring(_T("cancel"));
          // end dialog
          EndDialog(hwndDlg, 0);
          break;
      }
      break;
    case WM_DESTROY:
      // clean up
      if (font) DeleteObject(font);
      break;
    default:
      return FALSE; // message not processed
  }
  return TRUE; // message processed
}

void NSIS_DECLSPEC_DLLEXPORT LangDialog(HWND hwndParent, int string_size, 
                                      TCHAR *variables, stack_t **stacktop)
{
  g_hwndParent=hwndParent;
  EXDLL_INIT();

  {
    int i;
    int doauto = 0;
    BOOL pop_empty_string = FALSE;

    // get texts
    if (popstring(g_wndtitle)) return;
    if (popstring(g_wndtext)) return;

    // get flags
    if (popstring(temp)) return;

    // parse flags
    {
      TCHAR *p=temp;
      while (*p)
      {
        if (*p == _T('A')) doauto=1; // parse auto count flag
        if (*p == _T('F')) dofont=1; // parse font flag
        if (*p == _T('C')) docp=1;   // parse codepage flag
        p++;
      }
    }
 
    if (doauto) {
      // automatic language count
      stack_t *th;
      langs_num=0;
      th=(*g_stacktop);
      while (th && th->text[0]) {
        langs_num++;
        th = th->next;
      }
      if (!th) return;
      if (docp)
        langs_num /= 3;
      else
        langs_num /= 2;
      pop_empty_string = TRUE;
    } else {
      // use counts languages
      langs_num = myatou(temp);
    }

    // zero languages?
    if (!langs_num) return;

    // initialize visible languages count
    visible_langs_num = 0;

    // allocate language struct
    langs = (struct lang *)GlobalAlloc(GPTR, langs_num*sizeof(struct lang));
    if (!langs) return;

    // fill language struct
    for (i = 0; i < langs_num; i++) {
      if (popstring(temp)) { visible_langs_num = 0; break; }
      langs[visible_langs_num].name = (TCHAR*) GlobalAlloc(GPTR, (lstrlen(temp)+1)*sizeof(TCHAR));
      if (!langs[visible_langs_num].name) { visible_langs_num = 0; break; }
      lstrcpy(langs[visible_langs_num].name, temp);

      if (popstring(temp)) { visible_langs_num = 0; break; }
      langs[visible_langs_num].id = (TCHAR*) GlobalAlloc(GPTR, (lstrlen(temp)+1)*sizeof(TCHAR));
      if (!langs[visible_langs_num].id) { visible_langs_num = 0; break; }
      lstrcpy(langs[visible_langs_num].id, temp);

      if (docp)
      {
        if (popstring(temp)) { visible_langs_num = 0; break; }
        langs[visible_langs_num].cp = myatou(temp);
      }

      // If Unicode, show everything.
#ifdef UNICODE
      visible_langs_num++;
#else
      if (!docp || AllowLang(&langs[visible_langs_num]) || langs[visible_langs_num].cp == 0)
      {
        visible_langs_num++;
      }
      else
      {
        GlobalFree(langs[visible_langs_num].name);
        GlobalFree(langs[visible_langs_num].id);
      }
#endif
    }

    // pop the empty string to keep the stack clean
    if (pop_empty_string) {
      if (popstring(temp)) {
        visible_langs_num = 0;
      }
    }

    // start dialog
    if (visible_langs_num > 1)
    {
      DialogBox(g_hInstance, MAKEINTRESOURCE(IDD_DIALOG), 0, DialogProc);
    }
    else if (visible_langs_num == 0)
    {
      pushstring(_T(""));
    }
    else
    {
      pushstring(langs[0].id);
    }

    // free structs
    for (i = 0; i < visible_langs_num; i++) {
      if (langs[i].name) GlobalFree(langs[i].name);
      if (langs[i].id) GlobalFree(langs[i].id);
    }
    GlobalFree(langs);
  }
}

BOOL WINAPI DllMain(HINSTANCE hInst, ULONG ul_reason_for_call, LPVOID lpReserved)
{
  g_hInstance=hInst;
  return TRUE;
}
