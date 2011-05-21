/*
** Copyright (C) 2003 Nullsoft, Inc.
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

#ifndef _WA_DLG_H_
#define _WA_DLG_H_

#include "wa_ipc.h"

/*
  
  dont know where to put this yet :)
       1) gen.bmp has a generic window frame for plugins to use.
          its format is similar to the minibrowser's.
          In addition gen.bmp includes a font for the titlebar, in both
          highlight and no-highlight modes. The font is variable width,
          and it uses the first color before the letter A as the delimiter.
          The no-highlight form of letter must be the same width as the
          highlight form.
       2) genex.bmp has button and scrollbar images, as well as some individual
          pixels that describe the colors for the dialog. The button and
          scrollbar images should be self explanatory (note that the buttons
          have 4 pixel sized edges that are not stretched, and the center is
          stretched), and the scrollbars do something similar.
          The colors start at (48,0) and run every other pixel. The meaning
          of each pixel is:
            x=48: item background (background to edits, listviews etc)
            x=50: item foreground (text color of edit/listview, etc)
            x=52: window background (used to set the bg color for the dialog)
            x=54: button text color
            x=56: window text color
            x=58: color of dividers and sunken borders
            x=60: selection color for listviews/playlists/etc
            x=62: listview header background color
            x=64: listview header text color
            x=66: listview header frame top color
            x=68: listview header frame middle color
            x=70: listview header frame bottom color
            x=72: listview header empty color
            x=74: scrollbar foreground color
            x=76: scrollbar background color
            x=78: inverse scrollbar foreground color
            x=80: inverse scrollbar background color
            x=82: scrollbar dead area color
*/


#define DCW_SUNKENBORDER 0x00010000
#define DCW_DIVIDER 0x00020000

enum
{
  WADLG_ITEMBG,
  WADLG_ITEMFG,
  WADLG_WNDBG,
  WADLG_BUTTONFG,
  WADLG_WNDFG,
  WADLG_HILITE,
  WADLG_SELCOLOR,
  WADLG_LISTHEADER_BGCOLOR,
  WADLG_LISTHEADER_FONTCOLOR,
  WADLG_LISTHEADER_FRAME_TOPCOLOR,
  WADLG_LISTHEADER_FRAME_MIDDLECOLOR,
  WADLG_LISTHEADER_FRAME_BOTTOMCOLOR,
  WADLG_LISTHEADER_EMPTY_BGCOLOR,
  WADLG_SCROLLBAR_FGCOLOR,
  WADLG_SCROLLBAR_BGCOLOR,
  WADLG_SCROLLBAR_INV_FGCOLOR,
  WADLG_SCROLLBAR_INV_BGCOLOR,
  WADLG_SCROLLBAR_DEADAREA_COLOR,
  WADLG_NUM_COLORS
};



void WADlg_init(HWND hwndWinamp); // call this on init, or on WM_DISPLAYCHANGE
void WADlg_close();

int WADlg_getColor(int idx);

int WADlg_handleDialogMsgs(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam); //
void WADlg_DrawChildWindowBorders(HWND hwndDlg, int *tab, int tabsize); // each entry in tab would be the id | DCW_*

HBITMAP WADlg_getBitmap();


/// define WA_DLG_IMPLEMENT in one of your source files before including this .h
// if you are making a media library plugin, you dont need to do this, look at view_ex for 
// an example of how to get the function *'s via an IPC message.

#ifdef WA_DLG_IMPLEMENT

static HBRUSH wadlg_lastbrush;
static HBITMAP wadlg_bitmap; // load this manually
static int wadlg_colors[WADLG_NUM_COLORS];
static int wadlg_defcolors[WADLG_NUM_COLORS]=
{
RGB(0,0,0),
RGB(0,255,0),
RGB(36,36,60),
RGB(57,56,66),
RGB(255,255,255),
RGB(132,148,165),  
RGB(0,0,198),
RGB(36*2,36*2,60*2),
RGB(255,255,255),
RGB(36*3,36*3,60*3),
RGB(36,36,60),
RGB(36*0.5,36*0.5,60*0.5),
RGB(36,36,60),
RGB(36*1,36*1,60*1),
RGB(36*1,36*1,60*1),
RGB(121,130,150),
RGB(78,88,110),
RGB(36*1,36*1,60*1),
};

int WADlg_getColor(int idx)
{
  if (idx < 0 || idx >= WADLG_NUM_COLORS) return 0;
  return wadlg_colors[idx];
}

HBITMAP WADlg_getBitmap()
{
  return wadlg_bitmap;
}

void WADlg_init(HWND hwndWinamp) // call this on init, or on WM_DISPLAYCHANGE
{
  if (wadlg_bitmap) DeleteObject(wadlg_bitmap);
  wadlg_bitmap = (HBITMAP) SendMessage(hwndWinamp,WM_WA_IPC,0,IPC_GET_GENSKINBITMAP);
  if (wadlg_bitmap)
  {
    HDC tmpDC=CreateCompatibleDC(NULL);
    HGDIOBJ o=SelectObject(tmpDC,(HGDIOBJ)wadlg_bitmap);
    int x;
    for (x = 0; x < WADLG_NUM_COLORS; x ++)
    {
      int a=GetPixel(tmpDC,48+x*2,0);
      if (a == CLR_INVALID || a == RGB(0,198,255)) a=wadlg_defcolors[x];
      wadlg_colors[x]=a;
    }

    SelectObject(tmpDC,o);
    DeleteDC(tmpDC);
  }
}

void WADlg_close()
{
  if (wadlg_bitmap) DeleteObject(wadlg_bitmap);
  wadlg_bitmap=0;
  if (wadlg_lastbrush) DeleteObject(wadlg_lastbrush);
  wadlg_lastbrush=0;
}

void WADlg_dotLine(HDC hdc, int left, int top, int len, int vert)
{
  for(int i=(top&1);i<len-1;i+=2)
  {
    if(vert) {
      MoveToEx(hdc,left,top+i,NULL);
      LineTo(hdc,left,top+i+1);
    } else {
      MoveToEx(hdc,left+i,top,NULL);
      LineTo(hdc,left+i+1,top);
    }
  }
}

int WADlg_handleDialogMsgs(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (uMsg == WM_DRAWITEM)
	{
		DRAWITEMSTRUCT *di = (DRAWITEMSTRUCT *)lParam;
		if (di->CtlType == ODT_BUTTON) {
			char wt[256];
			RECT r;
			GetDlgItemText(hwndDlg,wParam,wt,sizeof(wt));

      HDC hdc = CreateCompatibleDC(di->hDC);
      SelectObject(hdc,wadlg_bitmap);

			r=di->rcItem;
      SetStretchBltMode(di->hDC,COLORONCOLOR);


      int yoffs = (di->itemState & ODS_SELECTED) ? 15 : 0;

      BitBlt(di->hDC,r.left,r.top,4,4,hdc,0,yoffs,SRCCOPY); // top left
      StretchBlt(di->hDC,r.left+4,r.top,r.right-r.left-4-4,4,hdc,4,yoffs,47-4-4,4,SRCCOPY); // top center
      BitBlt(di->hDC,r.right-4,r.top,4,4,hdc,47-4,yoffs,SRCCOPY); // top right

      StretchBlt(di->hDC,r.left,r.top+4,4,r.bottom-r.top-4-4,hdc,0,4+yoffs,4,15-4-4,SRCCOPY); // left edge
      StretchBlt(di->hDC,r.right-4,r.top+4,4,r.bottom-r.top-4-4,hdc,47-4,4+yoffs,4,15-4-4,SRCCOPY); // right edge

      // center
      StretchBlt(di->hDC,r.left+4,r.top+4,r.right-r.left-4-4,r.bottom-r.top-4-4,hdc,4,4+yoffs,47-4-4,15-4-4,SRCCOPY);


      BitBlt(di->hDC,r.left,r.bottom-4,4,4,hdc,0,15-4+yoffs,SRCCOPY); // bottom left
      StretchBlt(di->hDC,r.left+4,r.bottom-4,r.right-r.left-4-4,4,hdc,4,15-4+yoffs,47-4-4,4,SRCCOPY); // bottom center
      BitBlt(di->hDC,r.right-4,r.bottom-4,4,4,hdc,47-4,15-4+yoffs,SRCCOPY); // bottom right


			// draw text
      SetBkMode(di->hDC,TRANSPARENT);
			SetTextColor(di->hDC,wadlg_colors[WADLG_BUTTONFG]);
      if (di->itemState & ODS_SELECTED) {r.left+=2; r.top+=2;}
			DrawText(di->hDC,wt,-1,&r,DT_VCENTER|DT_SINGLELINE|DT_CENTER);
      DeleteDC(hdc);

      if(GetFocus()==di->hwndItem) {
        HPEN pen=CreatePen(PS_SOLID,0,RGB(0,0,0));
        SelectObject(di->hDC,pen);
        WADlg_dotLine(di->hDC,r.left+2,r.top+2,r.right-r.left-3,0);
        WADlg_dotLine(di->hDC,r.right-3,r.top+2,r.bottom-r.top-3,1);
        WADlg_dotLine(di->hDC,r.left+2,r.top+2,r.bottom-r.top-3,1);
        WADlg_dotLine(di->hDC,r.left+2,r.bottom-3,r.right-r.left-3,0);
        DeleteObject(pen);
      }
		}	
	}
  switch(uMsg) 
  {
    case WM_CTLCOLORLISTBOX:
    case WM_CTLCOLORDLG:
    case WM_CTLCOLORBTN:
    case WM_CTLCOLORSTATIC:
    case WM_CTLCOLOREDIT:
    {
      int bgcolor=(uMsg == WM_CTLCOLOREDIT || uMsg == WM_CTLCOLORLISTBOX) ? wadlg_colors[WADLG_ITEMBG] : (uMsg == WM_CTLCOLORBTN ? wadlg_colors[WADLG_ITEMBG] : wadlg_colors[WADLG_WNDBG]);
      LOGBRUSH lb={BS_SOLID,GetNearestColor((HDC)wParam,bgcolor)};
      if (wadlg_lastbrush) DeleteObject(wadlg_lastbrush);
      wadlg_lastbrush=CreateBrushIndirect(&lb);
      SetTextColor((HDC)wParam,uMsg == WM_CTLCOLORSTATIC ? wadlg_colors[WADLG_WNDFG] : wadlg_colors[WADLG_ITEMFG]);
      SetBkColor((HDC)wParam,lb.lbColor);
      return (int)wadlg_lastbrush;
    }
  }
  return 0;
}

static int RectInRect(RECT *rect1, RECT *rect2)
{ 
  // this has a bias towards true

  // this could probably be optimized a lot
  return ((rect1->top >= rect2->top && rect1->top <= rect2->bottom) ||
      (rect1->bottom >= rect2->top && rect1->bottom <= rect2->bottom) ||
      (rect2->top >= rect1->top && rect2->top <= rect1->bottom) ||
      (rect2->bottom >= rect1->top && rect2->bottom <= rect1->bottom)) // vertical intersect
      &&
      ((rect1->left >= rect2->left && rect1->left <= rect2->right) ||
      (rect1->right >= rect2->left && rect1->right <= rect2->right) ||
      (rect2->left >= rect1->left && rect2->left <= rect1->right) ||
      (rect2->right >= rect1->left && rect2->right <= rect1->right)) // horiz intersect
      ;
}

static void WADlg_removeFromRgn(HRGN hrgn, int left, int top, int right, int bottom)
{
  HRGN rgn2=CreateRectRgn(left,top,right,bottom);
  CombineRgn(hrgn,hrgn,rgn2,RGN_DIFF);
  DeleteObject(rgn2);
}

void WADlg_DrawChildWindowBorders(HWND hwndDlg, int *tab, int tabsize)
{
  PAINTSTRUCT ps;
  BeginPaint(hwndDlg,&ps);

  HRGN hrgn=NULL;
  if(ps.fErase)
  {
    RECT r=ps.rcPaint;
    hrgn=CreateRectRgn(r.left,r.top,r.right,r.bottom);
  }

  HPEN pen=CreatePen(PS_SOLID,0,wadlg_colors[WADLG_HILITE]);
  HGDIOBJ o=SelectObject(ps.hdc,pen);

  while (tabsize--)
  {
    RECT r;
    int a=*tab++;
    GetWindowRect(GetDlgItem(hwndDlg,a&0xffff),&r);
    ScreenToClient(hwndDlg,(LPPOINT)&r);
    ScreenToClient(hwndDlg,((LPPOINT)&r)+1);

    if (RectInRect(&ps.rcPaint,&r)) 
    {
      if ((a & 0xffff0000) == DCW_SUNKENBORDER)
      {
        MoveToEx(ps.hdc,r.left,r.bottom,NULL);
        LineTo(ps.hdc,r.right,r.bottom);
        LineTo(ps.hdc,r.right,r.top-1);
        if(hrgn)
        {
          WADlg_removeFromRgn(hrgn,r.left,r.bottom,r.right,r.bottom+1);
          WADlg_removeFromRgn(hrgn,r.right,r.top,r.right+1,r.bottom);
        }
      }
      else if ((a & 0xffff0000) == DCW_DIVIDER)
      {
        if (r.right - r.left < r.bottom - r.top) // vertical
        {
          int left=(r.left+r.right)/2;
          MoveToEx(ps.hdc,left,r.top,NULL);
          LineTo(ps.hdc,left,r.bottom+1);
          if(hrgn) WADlg_removeFromRgn(hrgn,left,r.top,left+1,r.bottom);
        }
        else // horiz
        {
          int top=(r.top+r.bottom)/2;
          MoveToEx(ps.hdc,r.left,top,NULL);
          LineTo(ps.hdc,r.right+1,top);
          if(hrgn) WADlg_removeFromRgn(hrgn,r.left,top,r.right,top+1);
        }
      }
    }
  }

  SelectObject(ps.hdc,o);
  DeleteObject(pen);

  if(hrgn) {
    //erase bkgnd while clipping out our own drawn stuff (for flickerless display)
    HBRUSH b=CreateSolidBrush(wadlg_colors[WADLG_WNDBG]);
    FillRgn(ps.hdc,hrgn,b);
    DeleteObject(b);
    DeleteObject(hrgn);
  }
  EndPaint(hwndDlg,&ps);
}
#endif

#endif//_WA_DLG_H_