/* Copyright (C) 2010 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

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
