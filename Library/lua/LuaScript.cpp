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
#include "../../Common/StringUtil.h"
#include "LuaScript.h"
#include "LuaManager.h"

/*
** The constructor
**
*/
LuaScript::LuaScript() :
	m_Ref(LUA_NOREF),
	m_Unicode(false)
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

	FILE* file = _wfopen(scriptFile.c_str(), L"rb");
	if (!file) return false;

	fseek(file, 0, SEEK_END);
	const long fileSize = ftell(file);
	BYTE* fileData = new BYTE[fileSize];
	fseek(file, 0, SEEK_SET);
	fread(fileData, fileSize, 1, file);
	fclose(file);
	file = nullptr;

	auto L = GetState();
	bool scriptLoaded = false;

	// Treat the script as Unicode if it has the UTF-16 LE BOM.
	m_Unicode = fileSize > 2 && fileData[0] == 0xFF && fileData[1] == 0xFE;
	if (m_Unicode)
	{
		const std::string utf8Data = 
			StringUtil::NarrowUTF8((WCHAR*)(fileData + 2), (fileSize - 2) / sizeof(WCHAR));
		scriptLoaded = luaL_loadbuffer(L, utf8Data.c_str(), utf8Data.length(), "") == 0;
	}
	else
	{
		scriptLoaded = luaL_loadbuffer(L, (char*)fileData, fileSize, "") == 0;
	}

	delete [] fileData;

	if (scriptLoaded)
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
			LuaManager::ReportErrors(scriptFile);
			Uninitialize();
		}
	}
	else
	{
		LuaManager::ReportErrors(scriptFile);
	}

	return false;
}

void LuaScript::Uninitialize()
{
	auto L = GetState();

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
	auto L = GetState();
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
	auto L = GetState();

	if (IsInitialized())
	{
		// Push our table onto the stack
		lua_rawgeti(L, LUA_GLOBALSINDEX, m_Ref);

		// Push the function onto the stack
		lua_getfield(L, -1, funcName);

		if (lua_pcall(L, 0, 0, 0))
		{
			LuaManager::ReportErrors(m_File);
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
	auto L = GetState();
	int type = LUA_TNIL;

	if (IsInitialized())
	{
		// Push our table onto the stack
		lua_rawgeti(L, LUA_GLOBALSINDEX, m_Ref);

		// Push the function onto the stack
		lua_getfield(L, -1, funcName);

		if (lua_pcall(L, 0, 1, 0))
		{
			LuaManager::ReportErrors(m_File);
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
				strValue = m_Unicode ?
					StringUtil::WidenUTF8(str, (int)strLen) : StringUtil::Widen(str, (int)strLen);
				numValue = strtod(str, nullptr);
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
void LuaScript::RunString(const std::wstring& str)
{
	auto L = GetState();

	if (IsInitialized())
	{
		const std::string narrowStr = m_Unicode ?
			StringUtil::NarrowUTF8(str) : StringUtil::Narrow(str);

		// Load the string as a Lua chunk
		if (luaL_loadstring(L, narrowStr.c_str()))
		{
			LuaManager::ReportErrors(m_File);
		}

		// Push our table onto the stack
		lua_rawgeti(L, LUA_GLOBALSINDEX, m_Ref);

		// Pop table and set the environment of the loaded chunk to it
		lua_setfenv(L, -2);

		if (lua_pcall(L, 0, 0, 0))
		{
			LuaManager::ReportErrors(m_File);
		}
	}
}
