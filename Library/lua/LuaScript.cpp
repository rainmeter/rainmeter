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
LuaScript::LuaScript(const WCHAR* scriptFile) :
	m_Ref(LUA_NOREF),
	m_Initialized(false)
{
	lua_State* L = LuaManager::GetState();

	FILE* file = _wfopen(scriptFile, L"rb");
	if (!file)
	{
		return;
	}

	fseek(file, 0, SEEK_END);
	long fileSize = ftell(file);
	if (fileSize < 3)
	{
		return;
	}

	int load = 0;
	std::string scriptName = ConvertToUTF8(wcsrchr(scriptFile, L'\\') + 1);
	const char* scriptNameSz = scriptName.c_str();

	BYTE* fileData = new BYTE[fileSize];
	fseek(file, 0, SEEK_SET);
	fread(fileData, fileSize, 1, file);

	if (fileData[0] == 0xEF && fileData[1] == 0xBB && fileData[2] == 0xBF)
	{
		// Has UTF8 BOM, so assume that data is already in UTF8.
		const char* utf8Data = (char*)fileData + 3;
		int utf8Size = fileSize - 3;

		load = luaL_loadbuffer(L, utf8Data, utf8Size, scriptNameSz);
		delete [] fileData;
	}
	else
	{
		// Convert the file contents first to WCHAR with respect to the current
		// code page and then to UTF8 in order to preserve code page specific chars.

		int wideSize = MultiByteToWideChar(CP_ACP, 0, (char*)fileData, fileSize, NULL, 0);
		WCHAR* wideData = new WCHAR[wideSize];
		MultiByteToWideChar(CP_ACP, 0, (char*)fileData, fileSize, wideData, wideSize);
		delete [] fileData;

		int utf8Size = WideCharToMultiByte(CP_UTF8, 0, wideData, wideSize, NULL, 0, NULL, NULL);
		char* utf8Data = new char[utf8Size];
		WideCharToMultiByte(CP_UTF8, 0, wideData, wideSize, utf8Data, utf8Size, NULL, NULL);
		delete [] wideData;

		load = luaL_loadbuffer(L, utf8Data, utf8Size, scriptNameSz);
		delete [] utf8Data;
	}

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
		if (lua_pcall(L, 0, 0, 0) == 0)
		{
			m_Initialized = true;
		}
		else
		{
			LuaManager::ReportErrors(L);

			luaL_unref(L, LUA_GLOBALSINDEX, m_Ref);
			m_Ref = LUA_NOREF;
		}
	}
	else
	{
		LuaManager::ReportErrors(L);
	}

	fclose(file);
}

/*
** The destructor
**
*/
LuaScript::~LuaScript()
{
	lua_State* L = LuaManager::GetState();
	luaL_unref(L, LUA_GLOBALSINDEX, m_Ref);
}

/*
** Checks if given function is defined in the script file.
**
*/
bool LuaScript::IsFunction(const char* funcName)
{
	lua_State* L = LuaManager::GetState();
	bool bExists = false;

	if (m_Initialized)
	{
		// Push our table onto the stack
		lua_rawgeti(L, LUA_GLOBALSINDEX, m_Ref);

		// Push the function onto the stack
		lua_getfield(L, -1, funcName);

		if (lua_isfunction(L, -1))
		{
			bExists = true;
		}

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

	if (m_Initialized)
	{
		// Push our table onto the stack
		lua_rawgeti(L, LUA_GLOBALSINDEX, m_Ref);

		// Push the function onto the stack
		lua_getfield(L, -1, funcName);

		if (lua_pcall(L, 0, 0, 0))
		{
			LuaManager::ReportErrors(L);
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

	if (m_Initialized)
	{
		// Push our table onto the stack
		lua_rawgeti(L, LUA_GLOBALSINDEX, m_Ref);

		// Push the function onto the stack
		lua_getfield(L, -1, funcName);

		if (lua_pcall(L, 0, 1, 0))
		{
			LuaManager::ReportErrors(L);
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
				const char* str = lua_tostring(L, -1);
				strValue = ConvertUTF8ToWide(str);
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

	if (m_Initialized)
	{
		// Load the string as a Lua chunk
		if (luaL_loadstring(L, str))
		{
			LuaManager::ReportErrors(L);
		}

		// Push our table onto the stack
		lua_rawgeti(L, LUA_GLOBALSINDEX, m_Ref);

		// Pop table and set the environment of the loaded chunk to it
		lua_setfenv(L, -2);

		if (lua_pcall(L, 0, 0, 0))
		{
			LuaManager::ReportErrors(L);
		}
	}
}
