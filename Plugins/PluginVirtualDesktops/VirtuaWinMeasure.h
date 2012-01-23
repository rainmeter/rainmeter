/*
  Copyright (C) 2010 Patrick Dubbert

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
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#pragma once
#include "VDMeasure.h"

#include <windows.h>
#include <map>
#include <string>

class VirtuaWinMeasure :
	public VDMeasure
{
public:
	VirtuaWinMeasure(HMODULE, UINT id);
	virtual ~VirtuaWinMeasure(void);

	virtual UINT Initialize(LPCTSTR iniFile, LPCTSTR section);
	virtual void Finalize();
	virtual UINT Update();
	virtual LPCTSTR GetString(UINT flags);
	virtual void ExecuteBang(LPCTSTR args);

private:
	enum MeasureType
	{
		Unknown,
		VDMActive,
		DesktopCountRows,
		DesktopCountColumns,
		DesktopCountTotal,
		CurrentDesktop,
		SwitchDesktop,
	};

	static BOOL FindVirtuaWinWindow();

	MeasureType Type;
	static std::map<std::wstring, MeasureType> StringToType;
	static HWND vwHandle;

};
