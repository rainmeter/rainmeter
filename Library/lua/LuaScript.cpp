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

#include "../StdAfx.h"
#include "LuaScript.h"
#include "LuaManager.h"
#include "../Rainmeter.h"

/*
** LuaScript
**
** The constructor
**
*/
LuaScript::LuaScript(lua_State* state, const char* file) : m_State(state),
	m_iRef(LUA_NOREF),
	m_Initialized(true)
{
	int result = luaL_loadfile(m_State, file);

	// If the file loaded okay.
	if (result == 0)
	{
		// Create the table this script will reside in
		lua_newtable(m_State);

		// Create the metatable that will store the global table
		lua_createtable(m_State, 0, 1);

		// Push the global teble
		lua_pushvalue(m_State, LUA_GLOBALSINDEX);

		// Set the __index of the table to be the global table
		lua_setfield(m_State, -2, "__index");

		// Set the metatable for the script's table
		lua_setmetatable(m_State, -2);

		// Put the table into the global table
		m_iRef = luaL_ref(m_State, LUA_GLOBALSINDEX);

		PushTable();

		// Set the environment for the function to be run in to be the table that
		// has been created for the script/
		lua_setfenv(m_State, -2);

		// Execute the Lua script
		result = lua_pcall(m_State, 0, LUA_MULTRET, 0);

		if (result)
		{
			m_Initialized = false;
			LuaManager::ReportErrors(m_State);

			luaL_unref(m_State, LUA_GLOBALSINDEX, m_iRef);
			m_iRef = LUA_NOREF;
		}
	}
	else
	{
		m_Initialized = false;
		LuaManager::ReportErrors(m_State);
	}
}

/*
** ~LuaScript
**
** The destructor
**
*/
LuaScript::~LuaScript()
{
	luaL_unref(m_State, LUA_GLOBALSINDEX, m_iRef);
}

/*
** IsFunction
**
** Checks if given function is defined in the script file.
**
*/
bool LuaScript::IsFunction(const char* funcName)
{
	bool bExists = false;

	if (m_Initialized)
	{
		// Push our table onto the stack
		PushTable();

		// Push the function onto the stack
		lua_getfield(m_State, -1, funcName);

		if (lua_isfunction(m_State, -1))
		{
			bExists = true;
		}

		// Pop both the table and the function off the stack.
		lua_pop(m_State, 2);
	}

	return bExists;
}

/*
** RunFunction
**
** Runs given function in script file.
**
*/
void LuaScript::RunFunction(const char* funcName)
{
	if (m_Initialized)
	{
		// Push our table onto the stack
		PushTable();

		// Push the function onto the stack
		lua_getfield(m_State, -1, funcName);

		if (lua_pcall(m_State, 0, 0, 0))
		{
			LuaManager::ReportErrors(m_State);
		}

		lua_pop(m_State, 1);
	}
}

/*
** RunFunctionWithReturn
**
** Runs given function in script file and stores the retruned number and/or string.
** Returns true if the executed function returns a valid value.
**
*/
bool LuaScript::RunFunctionWithReturn(const char* funcName, double& numValue, std::wstring& strValue)
{
	bool ret = false;

	if (m_Initialized)
	{
		// Push our table onto the stack
		PushTable();

		// Push the function onto the stack
		lua_getfield(m_State, -1, funcName);

		if (lua_pcall(m_State, 0, 1, 0))
		{
			LuaManager::ReportErrors(m_State);
			lua_pop(m_State, 1);
		}
		else
		{
			if (lua_isstring(m_State, -1))
			{
				const char* str = lua_tostring(m_State, -1);
				strValue = ConvertToWide(str);

				// A number is a string and numerical string (e.g. "10") is a number, so check for it here
				if (lua_isnumber(m_State, -1))
				{
					numValue = lua_tonumber(m_State, -1);
				}

				ret = true;
			}

			lua_pop(m_State, 2);
		}
	}

	return ret;
}

/*
** RunString
**
** Runs given string in the context of the script file.
**
*/
void LuaScript::RunString(const char* str)
{
	if (m_Initialized)
	{
		// Load the string as a Lua chunk
		if (luaL_loadstring(m_State, str))
		{
			LuaManager::ReportErrors(m_State);
		}

		// Push our table onto the stack
		PushTable();

		// Pop table and set the environment of the loaded chunk to it
		lua_setfenv(m_State, -2);

		if (lua_pcall(m_State, 0, 0, 0))
		{
			LuaManager::ReportErrors(m_State);
		}
	}
}

//void LuaScript::BindVariable(const char* p_strName, void* p_pValue, const char* p_strTypeName)
//{
//	PushTable();
//
//	/*
//	// Push the variable name we want to put a value in.
//	lua_pushstring(m_State, p_strName);
//	// Push the value
//	tolua_pushusertype(m_State, p_pValue, p_strTypeName);
//	// Bind the variable
//	lua_settable(m_State, -3);
//
//	// Pop our table off of the stack
//	lua_pop(m_State, 1);
//
//	*/
//
//	// Push the variable name we want to put a value in.
//	lua_pushstring(m_State, "SKIN");
//	// Push the value
//	tolua_pushusertype(m_State, p_pValue, "CMeterWindow");
//	// Bind the variable
//	lua_settable(m_State, -3);
//
//	//lua_pop(m_pLuaScript->GetState(), 1);
//}
//
