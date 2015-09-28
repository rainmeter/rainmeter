/* Copyright (C) 2010 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

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
