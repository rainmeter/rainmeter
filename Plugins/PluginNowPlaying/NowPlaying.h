/*
  Copyright (C) 2011 Birunthan Mohanathas (www.poiru.net)

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

#ifndef __NOWPLAYING_H__
#define __NOWPLAYING_H__

#include "Player.h"

struct ParentMeasure
{
	ParentMeasure() :
		player(NULL),
		childCount(1),
		trackCount(0),
		disableLeadingZero(false)
	{}

	UINT id;
	UINT childCount;
	UINT trackCount;
	CPlayer* player;
	HWND window;
	std::wstring name;
	std::wstring iniFile;
	std::wstring trackChangeAction;
	std::wstring playerPath;
	bool disableLeadingZero;
};

struct ChildMeasure
{
	ChildMeasure() :
		type(MEASURE_NONE),
		parent(NULL)
	{}

	MEASURETYPE type;
	ParentMeasure* parent;
};

void SecondsToTime(UINT seconds, bool leadingZero, WCHAR* buffer);
void ExecuteCommand(std::wstring& command, HWND wnd);
bool BelongToSameProcess(HWND wnd);
HWND FindMeterWindow(HWND parent);
HWND FindMeterWindow(const std::wstring& iniFile);

/* The exported functions */
extern "C"
{
__declspec( dllexport ) UINT Initialize(HMODULE instance, LPCTSTR iniFile, LPCTSTR section, UINT id);
__declspec( dllexport ) void Finalize(HMODULE instance, UINT id);
__declspec( dllexport ) UINT Update(UINT id);
__declspec( dllexport ) LPCTSTR GetString(UINT id, UINT flags);
__declspec( dllexport ) LPCTSTR GetPluginAuthor();
__declspec( dllexport ) UINT GetPluginVersion();
__declspec( dllexport ) void ExecuteBang(LPCTSTR bang, UINT id);
}

#endif
