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

#ifndef __ADVANCEDCPU_H__
#define __ADVANCEDCPU_H__

#include "../PluginPerfMon/titledb.h"
#include "../PluginPerfMon/perfsnap.h"
#include "../PluginPerfMon/objlist.h"
#include "../PluginPerfMon/perfobj.h"
#include "../PluginPerfMon/objinst.h"
#include "../PluginPerfMon/perfcntr.h"

#pragma warning ( disable: 4786 )

/* The exported functions */
extern "C"
{
__declspec( dllexport ) UINT Initialize(HMODULE instance, LPCTSTR iniFile, LPCTSTR section, UINT id);
__declspec( dllexport ) void Finalize(HMODULE instance, UINT id);
__declspec( dllexport ) double Update2(UINT id);
__declspec( dllexport ) UINT GetPluginVersion();
__declspec( dllexport ) LPCTSTR GetString(UINT id, UINT flags);
__declspec( dllexport ) LPCTSTR GetPluginAuthor();
}

#endif