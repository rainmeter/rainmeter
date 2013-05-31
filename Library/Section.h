/*
  Copyright (C) 2012 spx

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

#ifndef __SECTION_H__
#define __SECTION_H__

#include <windows.h>
#include <string>
#include "Group.h"

class MeterWindow;
class ConfigParser;

class __declspec(novtable) Section : public Group
{
public:
	virtual ~Section();

	virtual UINT GetTypeID() = 0;

	const WCHAR* GetName() const { return m_Name.c_str(); }
	const std::wstring& GetOriginalName() const { return m_Name; }

	bool HasDynamicVariables() const { return m_DynamicVariables; }
	void SetDynamicVariables(bool b) { m_DynamicVariables = b; }

	void ResetUpdateCounter() { m_UpdateCounter = m_UpdateDivider; }
	int GetUpdateCounter() const { return m_UpdateCounter; }
	int GetUpdateDivider() const { return m_UpdateDivider; }

	const std::wstring& GetOnUpdateAction() { return m_OnUpdateAction; }
	void DoUpdateAction();

	MeterWindow* GetMeterWindow() { return m_MeterWindow; }

protected:
	Section(MeterWindow* meterWindow, const WCHAR* name);

	virtual void ReadOptions(ConfigParser& parser, const WCHAR* section);

	bool UpdateCounter();

	const std::wstring m_Name;		// Name of this Section

	bool m_DynamicVariables;		// If true, the section contains dynamic variables
	int m_UpdateDivider;			// Divider for the update
	int m_UpdateCounter;			// Current update counter

	std::wstring m_OnUpdateAction;

	MeterWindow* m_MeterWindow;
};

#endif
