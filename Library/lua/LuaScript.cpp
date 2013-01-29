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

#include "../StdAfx.h"
#include "LuaScript.h"
#include "LuaManager.h"
#include "../Rainmeter.h"

/*
** The constructor
**
*/
LuaScript::LuaScript() :
	m_Ref(LUA_NOREF)
{
}

/*
** The destructor
**
*/
LuaScript::~LuaScript()
{
	Uninitialize();
}

bool LuaScript::Initialize(const std::wstring& scriptFile)
{
	assert(!IsInitialized());

	lua_State* L = LuaManager::GetState();

	// Load file into a buffer as luaL_loadfile does not support Unicode paths.
	FILE* file = _wfopen(scriptFile.c_str(), L"rb");
	if (!file)
	{
		return false;
	}

	fseek(file, 0, SEEK_END);
	long fileSize = ftell(file);

	char* fileData = new char[fileSize];
	fseek(file, 0, SEEK_SET);
	fread(fileData, fileSize, 1, file);

	int load = luaL_loadbuffer(L, fileData, fileSize, "");
	delete [] fileData;

	if (load == 0)
	{
		// Create the table this script will reside in
		lua_newtable(L);

		// Create the metatable that will store the global table
		lua_createtable(L, 0, 1);

		// Push the global teble
		lua_pushvalue(L, LUA_GLOBALSINDEX);

		// Set the __index of the table to be the global table
		lua_setfield(L, -2, "__index");

		// Set the metatable for the script's table
		lua_setmetatable(L, -2);

		// Put the table into the global table
		m_Ref = luaL_ref(L, LUA_GLOBALSINDEX);

		lua_rawgeti(L, LUA_GLOBALSINDEX, m_Ref);

		// Set the environment for the function to be run in to be the table that
		// has been created for the script/
		lua_setfenv(L, -2);

		// Execute the Lua script
		int result = lua_pcall(L, 0, 0, 0);
		if (result == 0)
		{
			m_File = scriptFile;
			return true;
		}
		else
		{
			LuaManager::ReportErrors(L, scriptFile);
			Uninitialize();
		}
	}
	else
	{
		LuaManager::ReportErrors(L, scriptFile);
	}

	return false;
}

void LuaScript::Uninitialize()
{
	lua_State* L = LuaManager::GetState();

	if (m_Ref != LUA_NOREF)
	{
		luaL_unref(L, LUA_GLOBALSINDEX, m_Ref);
		m_Ref = LUA_NOREF;
		m_File.clear();
	}
}

/*
** Checks if given function is defined in the script file.
**
*/
bool LuaScript::IsFunction(const char* funcName)
{
	lua_State* L = LuaManager::GetState();
	bool bExists = false;

	if (IsInitialized())
	{
		// Push our table onto the stack
		lua_rawgeti(L, LUA_GLOBALSINDEX, m_Ref);

		// Push the function onto the stack
		lua_getfield(L, -1, funcName);

		bExists = lua_isfunction(L, -1);

		// Pop both the table and the function off the stack.
		lua_pop(L, 2);
	}

	return bExists;
}

/*
** Runs given function in script file.
**
*/
void LuaScript::RunFunction(const char* funcName)
{
	lua_State* L = LuaManager::GetState();

	if (IsInitialized())
	{
		// Push our table onto the stack
		lua_rawgeti(L, LUA_GLOBALSINDEX, m_Ref);

		// Push the function onto the stack
		lua_getfield(L, -1, funcName);

		if (lua_pcall(L, 0, 0, 0))
		{
			LuaManager::ReportErrors(L, m_File);
		}

		lua_pop(L, 1);
	}
}

/*
** Runs given function in script file and stores the retruned number or string.
**
*/
int LuaScript::RunFunctionWithReturn(const char* funcName, double& numValue, std::wstring& strValue)
{
	lua_State* L = LuaManager::GetState();
	int type = LUA_TNIL;

	if (IsInitialized())
	{
		// Push our table onto the stack
		lua_rawgeti(L, LUA_GLOBALSINDEX, m_Ref);

		// Push the function onto the stack
		lua_getfield(L, -1, funcName);

		if (lua_pcall(L, 0, 1, 0))
		{
			LuaManager::ReportErrors(L, m_File);
			lua_pop(L, 1);
		}
		else
		{
			type = lua_type(L, -1);
			if (type == LUA_TNUMBER)
			{
				numValue = lua_tonumber(L, -1);
			}
			else if (type == LUA_TSTRING)
			{
				size_t strLen = 0;
				const char* str = lua_tolstring(L, -1, &strLen);
				strValue = StringUtil::Widen(str, (int)strLen);
				numValue = strtod(str, NULL);
			}

			lua_pop(L, 2);
		}
	}

	return type;
}

/*
** Runs given string in the context of the script file.
**
*/
void LuaScript::RunString(const char* str)
{
	lua_State* L = LuaManager::GetState();

	if (IsInitialized())
	{
		// Load the string as a Lua chunk
		if (luaL_loadstring(L, str))
		{
			LuaManager::ReportErrors(L, m_File);
		}

		// Push our table onto the stack
		lua_rawgeti(L, LUA_GLOBALSINDEX, m_Ref);

		// Pop table and set the environment of the loaded chunk to it
		lua_setfenv(L, -2);

		if (lua_pcall(L, 0, 0, 0))
		{
			LuaManager::ReportErrors(L, m_File);
		}
	}
}
