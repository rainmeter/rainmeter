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

#include <windows.h>

class VDMeasure
{
public:
	VDMeasure(HMODULE instance, UINT id) { hInstance = instance; ID = id; };
	virtual ~VDMeasure(void) {};

/*
  This function is called when the measure is initialized.
  The function must return the maximum value that can be measured. 
  The return value can also be 0, which means that Rainmeter will
  track the maximum value automatically. The parameters for this
  function are:

  iniFile   The name of the ini-file (usually Rainmeter.ini)
  section   The name of the section in the ini-file for this measure
*/
	virtual UINT Initialize(LPCTSTR iniFile, LPCTSTR section) = 0;

/*
  If the measure needs to free resources before quitting.
  The plugin can export Finalize function, which is called
  when Rainmeter quits (or refreshes).
*/
	virtual void Finalize() = 0;

/*
  This function is called when new value should be measured.
  The function returns the new value.
*/
	virtual UINT Update() = 0;

/*
  This function is called when the value should be
  returned as a string.
*/
	virtual LPCTSTR GetString(UINT flags) = 0;

	virtual void ExecuteBang(LPCTSTR args) = 0;

protected:
	static HMODULE hInstance;
	UINT ID;
};
