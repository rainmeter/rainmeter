/* Copyright (C) 2010 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "StdAfx.h"
#include "../../Common/StringUtil.h"
#include "../../Common/FileUtil.h"
#include "LuaScript.h"
#include "LuaManager.h"
#include "../Logger.h"

LuaScript::LuaScript() :
	m_Ref(LUA_NOREF),
	m_Unicode(false)
{
}

LuaScript::~LuaScript()
{
	Uninitialize();
}

bool LuaScript::Initialize(const std::wstring& scriptFile, const std::wstring& packagePath)
{
	assert(!IsInitialized());

	size_t fileSize = 0;
	auto fileData = FileUtil::ReadFullFile(scriptFile, &fileSize);
	if (!fileData)
	{
		return false;
	}

	auto L = GetState();
	bool scriptLoaded = false;

	// Treat the script as Unicode if it has the UTF-16 LE BOM.
	m_Unicode = fileSize > 2 && fileData[0] == 0xFF && fileData[1] == 0xFE;
	if (m_Unicode)
	{
		const std::string utf8Data = 
			StringUtil::NarrowUTF8((WCHAR*)(fileData.get() + 2), (fileSize - 2) / sizeof(WCHAR));
		scriptLoaded = luaL_loadbuffer(L, utf8Data.c_str(), utf8Data.length(), "") == 0;
	}
	else
	{
		scriptLoaded = luaL_loadbuffer(L, (char*)fileData.get(), fileSize, "") == 0;
	}

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

		// Get the original package path
		std::wstring oldPackagePath = GetPackagePath();

		// Append package path for script's require function to use
		if (!packagePath.empty()) {
			m_scriptResourceFolder = packagePath;
			SetPackagePath(packagePath);
		}

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

		// Reset the package path to the original value
		if (!packagePath.empty()) {
			SetPackagePath(oldPackagePath);
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

		// Pop table and set the environment of the loaded chunk to it
		lua_setfenv(L, -2);

		// Push our table onto the stack
		lua_rawgeti(L, LUA_GLOBALSINDEX, m_Ref);

		// Push the function onto the stack
		lua_getfield(L, -1, funcName);

		// Get the original package path
		std::wstring oldPackagePath = GetPackagePath();

		// Append package path for script's require function to use
		if (!m_scriptResourceFolder.empty()) {
			SetPackagePath(m_scriptResourceFolder);
		}

		if (lua_pcall(L, 0, 0, 0))
		{
			LuaManager::ReportErrors(m_File);
		}

		// Reset the package path to the original value
		if (!m_scriptResourceFolder.empty()) {
			SetPackagePath(oldPackagePath);
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

		// Pop table and set the environment of the loaded chunk to it
		lua_setfenv(L, -2);

		// Push our table onto the stack
		lua_rawgeti(L, LUA_GLOBALSINDEX, m_Ref);

		// Push the function onto the stack
		lua_getfield(L, -1, funcName);

		// Get the original package path
		std::wstring oldPackagePath = GetPackagePath();

		// Append package path for script's require function to use
		if (!m_scriptResourceFolder.empty()) {
			SetPackagePath(m_scriptResourceFolder);
		}

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

		// Reset the package path to the original value
		if (!m_scriptResourceFolder.empty()) {
			SetPackagePath(oldPackagePath);
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

		// Get the original package path
		std::wstring oldPackagePath = GetPackagePath();

		// Append package path for script's require function to use
		if (!m_scriptResourceFolder.empty()) {
			SetPackagePath(m_scriptResourceFolder);
		}

		if (lua_pcall(L, 0, 0, 0))
		{
			LuaManager::ReportErrors(m_File);
		}

		// Reset the package path to the original value
		if (!m_scriptResourceFolder.empty()) {
			SetPackagePath(oldPackagePath);
		}
	}
}

void LuaScript::SetPackagePath(const std::wstring& path)
{
	auto L = GetState();
	lua_getglobal(L, "package");
	std::wstring packagepath = path;
	packagepath.append(L"?.lua");
	LuaManager::PushWide(packagepath);
	lua_setfield(L, -2, "path");
	lua_pop(L, 1);
}

std::wstring LuaScript::GetPackagePath()
{
	auto L = GetState();
	lua_getglobal(L, "package");
	lua_getfield(L, -1, "path");
	std::wstring curPath = LuaManager::ToWide(-1);
	lua_pop(L, 1);
	lua_pop(L, 1);
	return curPath;
}
