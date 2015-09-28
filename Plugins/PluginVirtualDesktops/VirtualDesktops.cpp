/* Copyright (C) 2010 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

/*

  VirtualDesktops Plugin
  ----------------------

  This plugin aims to give Rainmeter skins access to various virtual desktop
  managers through a common interface. The following VDMs are supported:

	* Dexpot
	* VirtuaWin

  To add support for another virtual desktop manager,

	1) implement a new class derived from VDMeasure
	2) include its header file below
	3) add a new case for the "VDManager" config string in the Initialize
	   funtion below

  Different types of measures are identified using the "VDMeasureType" config
  string, i.e.

	[VirtualDesktopsMeasure]
	Measure=Plugin
	Plugin=VirtualDesktops.dll
	VDManager=SomeVDM
	VDMeasureType=...

  The following basic measure types have to be implemented:

	* VDMActive: returns 1 when the VDM is running, 0 otherwise

	* DesktopCount: returns the number of virtual desktops available; when
					"VDDesktopCount=X" or "VDDesktopCount=Y" is given, returns
					the number of columns or rows, respectively, in a grid of
					desktops

	* CurrentDesktop: returns the number of the currently active desktop

	* SwitchDesktop: when sent a desktop number as a bang, switches to the
					 corresponding desktop

  You're welcome to add any other measure types that suit the feature set of
  the virtual desktop manager in question. Examples can be found in the
  existing implementations.

*/

#include "DexpotMeasure.h"
#include "VirtuaWinMeasure.h"

#include <windows.h>
#include <tchar.h>
#include <map>

#include "../../Library/Export.h"

extern "C"
{
	__declspec(dllexport) UINT Initialize(HMODULE instance, LPCTSTR iniFile, LPCTSTR section, UINT id);
	__declspec(dllexport) void Finalize(HMODULE instance, UINT id);
	__declspec(dllexport) UINT Update(UINT id);
	__declspec(dllexport) LPCTSTR GetString(UINT id, UINT flags);
	__declspec(dllexport) void ExecuteBang(LPCTSTR args, UINT id);
	__declspec(dllexport) UINT GetPluginVersion();
	__declspec(dllexport) LPCTSTR GetPluginAuthor();
}

std::map<UINT, VDMeasure*> Measures;
HMODULE VDMeasure::hInstance;

UINT Initialize(HMODULE instance, LPCTSTR iniFile, LPCTSTR section, UINT id)
{
	VDMeasure *Measure = nullptr;
	LPCTSTR VDManager = ReadConfigString(section, _T("VDManager"), _T(""));

	if (_tcsicmp(VDManager, _T("Dexpot")) == 0)
	{
		Measure = DexpotMeasure::CreateMeasure(instance, id, iniFile, section);
	}
	else if (_tcsicmp(VDManager, _T("VirtuaWin")) == 0)
	{
		Measure = new VirtuaWinMeasure(instance, id);
	}

	if (Measure)
	{
		Measures.insert(std::make_pair(id, Measure));
		return Measure->Initialize(iniFile, section);
	}

	return 0;
}

UINT Update(UINT id)
{
	std::map<UINT, VDMeasure*>::iterator i = Measures.find(id);
	if (i != Measures.end())
	{
		return i->second->Update();
	}

	return 0;
}

LPCTSTR GetString(UINT id, UINT flags)
{
	std::map<UINT, VDMeasure*>::iterator i = Measures.find(id);
	if (i != Measures.end())
	{
		return i->second->GetString(flags);
	}

	return _T("");
}

void ExecuteBang(LPCTSTR args, UINT id)
{
	std::map<UINT, VDMeasure*>::iterator i = Measures.find(id);
	if (i != Measures.end())
	{
		i->second->ExecuteBang(args);
	}
}

void Finalize(HMODULE instance, UINT id)
{
	std::map<UINT, VDMeasure*>::iterator i = Measures.find(id);
	if (i != Measures.end())
	{
		i->second->Finalize();
		delete i->second;
		Measures.erase(i);
	}
}

UINT GetPluginVersion()
{
	return 1001;
}

LPCTSTR GetPluginAuthor()
{
	return _T("Dexpot GbR (info@dexpot.de)");
}
