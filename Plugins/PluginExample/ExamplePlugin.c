/*
  Copyright (C) 2001 Kimmo Pekkola

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

#pragma warning(disable: 4996)

#include <windows.h>
#include <math.h>
#include "..\..\Library\Export.h"	// Rainmeter's exported functions

/* The exported functions */
__declspec( dllexport ) UINT Initialize(HMODULE instance, LPCTSTR iniFile, LPCTSTR section, UINT id);
__declspec( dllexport ) void Finalize(HMODULE instance, UINT id);
__declspec( dllexport ) UINT Update(UINT id);
__declspec( dllexport ) UINT GetPluginVersion();
__declspec( dllexport ) LPCTSTR GetPluginAuthor();

/* Couple of globals */
static UINT g_Phase = 100;
static UINT g_CurrentPhase = 0;

/*
  This function is called when the measure is initialized.
  The function must return the maximum value that can be measured. 
  The return value can also be 0, which means that Rainmeter will
  track the maximum value automatically. The parameters for this
  function are:

  instance  The instance of this DLL
  iniFile   The name of the ini-file (usually Rainmeter.ini)
  section   The name of the section in the ini-file for this measure
  id        The identifier for the measure. This is used to identify the measures that use the same plugin.
*/
UINT Initialize(HMODULE instance, LPCTSTR iniFile, LPCTSTR section, UINT id)
{
	/* 
	  Read our own settings from the ini-file 
	  The ReadConfigString can be used for this purpose. Plugins
	  can also read the config some other way (e.g. with 
	  GetPrivateProfileInt, but in that case the variables
	  do not work.
	*/
	LPCTSTR data = ReadConfigString(section, L"PhaseShift", L"0");
	if (data)
	{
		g_CurrentPhase = _wtoi(data);
	}

	data = ReadConfigString(section, L"Phase", L"100");
	if (data)
	{
		g_Phase = _wtoi(data);
	}
	
	return 1000;	/* We'll return values from 0 to 1000 */
}

/*
  This function is called when new value should be measured.
  The function returns the new value.
*/
UINT Update(UINT id)
{
	/* 
	  This measure doesn't measure anything. It's just an
	  example how to create Rainmeter plugins. We'll just
	  return sine curve so that the meter shows something.
	*/

	double value = 6.283185 * ((double)g_CurrentPhase / (double)g_Phase);
	value = sin(value);
	
	g_CurrentPhase++;
	if(g_CurrentPhase > g_Phase)
	{
		g_CurrentPhase = 0;
	}

	return (UINT)((value + 1.0) * 500.0);
}

/*
  If the measure needs to free resources before quitting.
  The plugin can export Finalize function, which is called
  when Rainmeter quits (or refreshes).
*/
void Finalize(HMODULE instance, UINT id)
{
	/* Nothing to do here */
}

/*
  Returns the version number of the plugin. The value
  can be calculated like this: Major * 1000 + Minor.
  So, e.g. 2.31 would be 2031.
*/
UINT GetPluginVersion()
{
	return 1001;
}

/*
  Returns the author of the plugin for the about dialog.
*/
LPCTSTR GetPluginAuthor()
{
	return L"Rainy (rainy@iki.fi)";
}
