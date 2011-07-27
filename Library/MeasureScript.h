/*
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

#ifndef __MEASURESCRIPT_H__
#define __MEASURESCRIPT_H__

#include "Measure.h"
#include "lua/LuaScript.h"
#include "MeterWindow.h"

class CMeasureScript : public CMeasure
{
public:
	CMeasureScript(CMeterWindow* meterWindow, const WCHAR* name);
	virtual ~CMeasureScript();

	virtual void ReadConfig(CConfigParser& parser, const WCHAR* section);
	virtual void Initialize();
	virtual bool Update();
	virtual const WCHAR* GetStringValue(AUTOSCALE autoScale, double scale, int decimals, bool percentual);
	virtual void ExecuteBang(const WCHAR* args);

	void DeleteLuaScript();

protected:
	LuaScript* m_LuaScript;
	
	bool m_HasInitializeFunction;
	bool m_HasUpdateFunction;
	bool m_HasGetStringFunction;

	std::wstring m_StringValue;

	std::string m_ScriptFile;
};

#endif