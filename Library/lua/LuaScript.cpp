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

LuaScript* LuaScript::m_ActiveScript = nullptr;

LuaScript::LuaScript() :
	m_State(nullptr),
	m_Unicode(false)
{
}

LuaScript::~LuaScript()
{
	Uninitialize();
}

bool LuaScript::Initialize(const std::wstring& scriptFile)
{
	assert(!IsInitialized());

	if (!m_State)
	{
		// Initialize Lua
		m_State = luaL_newstate();

		luaL_openlibs(m_State);

		// Register custom types and functions
		RegisterGlobal(m_State);
		RegisterMeasure(m_State);
		RegisterMeter(m_State);
		RegisterSkin(m_State);
	}

	size_t fileSize = 0;
	auto fileData = FileUtil::ReadFullFile(scriptFile, &fileSize);
	if (!fileData)
	{
		return false;
	}

	bool scriptLoaded = false;

	// Treat the script as Unicode if it has the UTF-16 LE BOM.
	m_Unicode = fileSize > 2 && fileData[0] == 0xFF && fileData[1] == 0xFE;
	if (m_Unicode)
	{
		const std::string utf8Data = 
			StringUtil::NarrowUTF8((WCHAR*)(fileData.get() + 2), (fileSize - 2) / sizeof(WCHAR));
		scriptLoaded = luaL_loadbuffer(m_State, utf8Data.c_str(), utf8Data.length(), "") == 0;
	}
	else
	{
		scriptLoaded = luaL_loadbuffer(m_State, (char*)fileData.get(), fileSize, "") == 0;
	}

	if (scriptLoaded)
	{
		m_ActiveScript = this;

		// Execute the Lua script
		int result = lua_pcall(m_State, 0, 0, 0);

		m_ActiveScript = nullptr;

		if (result == 0)
		{
			m_File = scriptFile;
			return true;
		}
		else
		{
			LuaHelper::ReportErrors(m_State, scriptFile);
			Uninitialize();
		}
	}
	else
	{
		LuaHelper::ReportErrors(m_State, scriptFile);
	}

	return false;
}

void LuaScript::Uninitialize()
{
	if (m_State)
	{
		lua_close(m_State);
		m_State = nullptr;
		m_File.clear();
	}
}

/*
** Checks if given function is defined in the script file.
**
*/
bool LuaScript::IsFunction(const char* funcName)
{
	bool bExists = false;

	if (IsInitialized())
	{
		// Push the function onto the stack
		lua_getglobal(m_State, funcName);

		bExists = lua_isfunction(m_State, -1);

		// Pop both the table and the function off the stack.
		lua_pop(m_State, 1);
	}

	return bExists;
}

/*
** Runs given function in script file.
**
*/
void LuaScript::RunFunction(const char* funcName)
{
	if (IsInitialized())
	{
		// Push the function onto the stack
		lua_getglobal(m_State, funcName);

		m_ActiveScript = this;

		if (lua_pcall(m_State, 0, 0, 0))
		{
			LuaHelper::ReportErrors(m_State, m_File);
		}

		m_ActiveScript = nullptr;
	}
}

/*
** Runs given function in script file and stores the retruned number or string.
**
*/
int LuaScript::RunFunctionWithReturn(const char* funcName, double& numValue, std::wstring& strValue)
{
	int type = LUA_TNIL;

	if (IsInitialized())
	{
		// Push the function onto the stack
		lua_getglobal(m_State, funcName);

		m_ActiveScript = this;

		if (lua_pcall(m_State, 0, 1, 0))
		{
			LuaHelper::ReportErrors(m_State, m_File);
		}
		else
		{
			type = lua_type(m_State, -1);
			if (type == LUA_TNUMBER)
			{
				numValue = lua_tonumber(m_State, -1);
			}
			else if (type == LUA_TSTRING)
			{
				size_t strLen = 0;
				const char* str = lua_tolstring(m_State, -1, &strLen);
				strValue = m_Unicode ?
					StringUtil::WidenUTF8(str, (int)strLen) : StringUtil::Widen(str, (int)strLen);
				numValue = strtod(str, nullptr);
			}

			lua_pop(m_State, 1);
		}

		m_ActiveScript = nullptr;
	}

	return type;
}

/*
** Runs given string in the context of the script file.
**
*/
void LuaScript::RunString(const std::wstring& str)
{
	if (IsInitialized())
	{
		const std::string narrowStr = m_Unicode ?
			StringUtil::NarrowUTF8(str) : StringUtil::Narrow(str);

		// Load the string as a Lua chunk
		if (luaL_loadstring(m_State, narrowStr.c_str()))
		{
			LuaHelper::ReportErrors(m_State, m_File);
		}

		m_ActiveScript = this;

		if (lua_pcall(m_State, 0, 0, 0))
		{
			LuaHelper::ReportErrors(m_State, m_File);
		}

		m_ActiveScript = nullptr;
	}
}
