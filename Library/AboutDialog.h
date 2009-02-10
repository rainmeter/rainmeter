/*
  Copyright (C) 2000 Kimmo Pekkola

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/
/*
  $Header: /home/cvsroot/Rainmeter/Library/AboutDialog.h,v 1.1.1.1 2005/07/10 18:51:06 rainy Exp $

  $Log: AboutDialog.h,v $
  Revision 1.1.1.1  2005/07/10 18:51:06  rainy
  no message

  Revision 1.2  2004/03/13 16:13:50  rainy
  Added support for multiple configs.

  Revision 1.1  2002/07/01 15:35:54  rainy
  Intial version

*/

#ifndef _ABOUTDIALOG_H_
#define _ABOUTDIALOG_H_

#include "MeterWindow.h"

HWND OpenAboutDialog(HWND hwndOwner, HINSTANCE instance);
void UpdateAboutStatistics();

#endif

