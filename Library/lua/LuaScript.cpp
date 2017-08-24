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
#include "LuaHelper.h"
#include "Measure.h"

LuaScript::LuaScript() :
	m_Ref(LUA_NOREF),
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

	if (m_State == nullptr)
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

	auto L = GetState();
	bool scriptLoaded = false;

	// Treat the script as Unicode if it has the UTF-16 LE BOM.
	m_Unicode = fileSize > 2 && fileData[0] == 0xFF && fileData[1] == 0xFE;
	if (m_Unicode)
	{
		const std::string utf8Data =
			StringUtil::NarrowUTF8((WCHAR*)(fileData.get() + 2), (int)((fileSize - 2) / sizeof(WCHAR)));
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

		// Execute the Lua script
		int result = lua_pcall(L, 0, 0, 0);
		if (result == 0)
		{
			m_File = scriptFile;
			return true;
		}
		else
		{
			LuaHelper::ReportErrors(scriptFile);
			Uninitialize();
		}
	}
	else
	{
		LuaHelper::ReportErrors(scriptFile);
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
	auto L = GetState();
	bool bExists = false;

	if (IsInitialized())
	{
		// Push our table onto the stack
		lua_rawgeti(L, LUA_GLOBALSINDEX, m_Ref);

		// Push the function onto the stack
		lua_getfield(L, -1, funcName);

		bExists = lua_isfunction(L, -1);

		// Pop the function off the stack.
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
			LuaHelper::ReportErrors(m_File);
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
			LuaHelper::ReportErrors(m_File);
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
			LuaHelper::ReportErrors(m_File);
		}

		// Push our table onto the stack
		lua_rawgeti(L, LUA_GLOBALSINDEX, m_Ref);

		// Pop table and set the environment of the loaded chunk to it
		lua_setfenv(L, -2);

		if (lua_pcall(L, 0, 0, 0))
		{
			LuaHelper::ReportErrors(m_File);
		}
	}
}

bool LuaScript::RunCustomFunction(const std::wstring& funcName, const std::vector<std::wstring>& args, std::wstring& strValue)
{
	if (!IsInitialized()) return false;

	auto L = GetState();

	const std::string nFuncName = m_Unicode ?
		StringUtil::NarrowUTF8(funcName) : StringUtil::Narrow(funcName);
	if (!IsFunction(nFuncName.c_str()))
	{
		strValue = L"Not a valid function name: \"" + funcName + L"\"";
		return false;
	}

	// Push our table onto the stack
	lua_rawgeti(L, LUA_GLOBALSINDEX, m_Ref);

	// Push the function onto the stack
	lua_getfield(L, -1, nFuncName.c_str());

	// Add args
	int numArgs = 0;
	if (args.size() > 0)
	{
		for (const auto& iter : args)
		{
			std::string arg = m_Unicode ?
				StringUtil::NarrowUTF8(iter) : StringUtil::Narrow(iter);
			size_t argSize = arg.size();
			if ((arg[0] == '\"' || arg[0] == '\'') && argSize > 1)
			{
				arg.erase(0, 1); // strip begin quote
				--argSize;

				auto ch = arg.back();
				if (ch == '\"' || ch == '\'')
				{
					arg.pop_back();  // strip last quote
					--argSize;
				}

				lua_pushlstring(L, arg.c_str(), argSize);
			}
			else if (arg == "true")
			{
				lua_pushboolean(L, 1);
			}
			else if (arg == "false")
			{
				lua_pushboolean(L, 0);
			}
			else
			{
				double num = strtod(arg.c_str(), nullptr);
				lua_pushnumber(L, num);
			}
			++numArgs;
		}
	}

	bool result = true;

	if (lua_pcall(L, numArgs, 1, 0))
	{
		LuaHelper::ReportErrors(m_File);
		strValue.clear();
		result = false;
	}
	else
	{
		int type = lua_type(L, -1);
		if (type == LUA_TNUMBER)
		{
			double val = lua_tonumber(L, -1);
			WCHAR buffer[128];
			int bufferLen = _snwprintf_s(buffer, _TRUNCATE, L"%.5f", val);
			Measure::RemoveTrailingZero(buffer, bufferLen);
			strValue = buffer;
		}
		else if (type == LUA_TBOOLEAN)
		{
			strValue = (lua_toboolean(L, -1) == 0) ? L"0" : L"1";
		}
		else if (type == LUA_TSTRING)
		{
			size_t strLen = 0;
			const char* str = lua_tolstring(L, -1, &strLen);
			strValue = m_Unicode ?
				StringUtil::WidenUTF8(str, (int)strLen) : StringUtil::Widen(str, (int)strLen);
		}
		else if (type == LUA_TNONE || type == LUA_TNIL)
		{
			strValue = L"Return type in function \"";
			strValue += funcName;
			strValue += L"\" not found or is nil";
			result = false;
		}
		else
		{
			const char* t = lua_typename(L, type);
			strValue = L"Invalid return type in function \"";
			strValue += funcName;
			strValue += L"\" (";
			strValue += m_Unicode ?
				StringUtil::WidenUTF8(t) : StringUtil::Widen(t);
			strValue += L")";
			result = false;
		}
	}

	lua_settop(L, 0);
	return result;
}

bool LuaScript::GetLuaVariable(const std::wstring& varName, std::wstring& strValue)
{
	if (!IsInitialized()) return false;

	auto L = GetState();

	bool result = true;

	const std::string nVarName = m_Unicode ?
		StringUtil::NarrowUTF8(varName) : StringUtil::Narrow(varName);

	// Push our table onto the stack
	lua_rawgeti(L, LUA_GLOBALSINDEX, m_Ref);

	// Push the variable onto the stack
	lua_getfield(L, -1, nVarName.c_str());

	int type = lua_type(L, -1);
	if (type == LUA_TNUMBER)
	{
		double val = lua_tonumber(L, -1);
		WCHAR buffer[128];
		int bufferLen = _snwprintf_s(buffer, _TRUNCATE, L"%.5f", val);
		Measure::RemoveTrailingZero(buffer, bufferLen);
		strValue = buffer;
	}
	else if (type == LUA_TBOOLEAN)
	{
		strValue = (lua_toboolean(L, -1) == 0) ? L"0" : L"1";
	}
	else if (type == LUA_TSTRING)
	{
		size_t strLen = 0;
		const char* str = lua_tolstring(L, -1, &strLen);
		strValue = m_Unicode ?
			StringUtil::WidenUTF8(str, (int)strLen) : StringUtil::Widen(str, (int)strLen);
	}
	else if (type == LUA_TNONE || type == LUA_TNIL)
	{
		strValue = L"Variable \"";
		strValue += varName;
		strValue += L"\" not found or is nil";
		result = false;
	}
	else
	{	
		const char* t = lua_typename(L, type);
		strValue = L"Invalid variable type (";
		strValue += m_Unicode ?
			StringUtil::WidenUTF8(t) : StringUtil::Widen(t);
		strValue += L")";
		result = false;
	}

	lua_settop(L, 0);
	return result;
}
