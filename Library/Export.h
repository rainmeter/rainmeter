/*
  Copyright (C) 2004 Kimmo Pekkola

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

#ifndef __EXPORT_H__
#define __EXPORT_H__

#ifdef LIBRARY_EXPORTS
#define EXPORT_PLUGIN __declspec(dllexport)
#else
#define EXPORT_PLUGIN __declspec(dllimport)
#endif

// log level constants
#define LOG_ERROR 1
#define LOG_WARNING 2
#define LOG_NOTICE 3
#define LOG_DEBUG 4

#ifdef __cplusplus
extern "C"
{
#endif

	EXPORT_PLUGIN BOOL LSLog(int nLevel, LPCTSTR pszModule, LPCTSTR pszMessage);
	EXPORT_PLUGIN LPCTSTR ReadConfigString(LPCTSTR section, LPCTSTR key, LPCTSTR defValue);

#ifdef __cplusplus
}
#endif

#endif