/*
  Copyright (C) 2010 Matt King, Birunthan Mohanathas

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

#ifndef __LUASCRIPT_H__
#define __LUASCRIPT_H__

#include "lua.hpp"

class LuaScript
{
public:
	LuaScript(lua_State* state, const char* file);
	~LuaScript();
	
	bool IsInitialized() { return m_Initialized; }

	lua_State* GetState() { return m_State; }
	void PushTable() { lua_rawgeti(m_State, LUA_GLOBALSINDEX, m_iRef); }

	bool IsFunction(const char* funcName);
	void RunFunction(const char* funcName);
	int RunFunctionWithReturn(const char* funcName, double& numValue, std::wstring& strValue);
	void RunString(const char* str);

protected:
	lua_State* m_State;

	int m_iRef;
	bool m_Initialized;
};

#endif

