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

	if (LuaHelper::RunFile(m_State, scriptFile, m_Unicode) != -1)
	{
		m_File = scriptFile;
		return true;
	}
	else
	{
		Uninitialize();
		return false;
	}
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
		bExists = LuaHelper::IsFunction(m_State, funcName, m_File, m_Unicode);
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
		LuaHelper::RunFunction(m_State, funcName, m_File, m_Unicode);
	}
}

/*
** Runs given function in script file and stores the retruned number or string.
**
*/
int LuaScript::RunFunctionWithReturn(const char* funcName, double& numValue, std::wstring& strValue)
{
	int type = LUA_TNIL;
	if (LuaHelper::RunFunctionWithReturn(m_State, funcName, m_File, m_Unicode)) {
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
		LuaHelper::RunString(m_State, str, m_File, m_Unicode);
	}
}
