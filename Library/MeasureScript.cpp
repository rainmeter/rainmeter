/* Copyright (C) 2010 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "StdAfx.h"
#include "MeasureScript.h"
#include "lua/LuaHelper.h"
#include "Util.h"
#include "Rainmeter.h"

const char* g_InitializeFunctionName = "Initialize";
const char* g_UpdateFunctionName = "Update";
const char* g_GetStringFunctionName = "GetStringValue";

MeasureScript::MeasureScript(Skin* skin, const WCHAR* name) : Measure(skin, name),
	m_HasUpdateFunction(false),
	m_HasGetStringFunction(false),
	m_ValueType(LUA_TNIL)
{
}

MeasureScript::~MeasureScript()
{
	UninitializeLuaScript();
}

void MeasureScript::UninitializeLuaScript()
{
	m_LuaScript.Uninitialize();

	m_HasUpdateFunction = false;
	m_HasGetStringFunction = false;
}

void MeasureScript::Initialize()
{
	Measure::Initialize();

	if (m_LuaScript.IsFunction(g_InitializeFunctionName))
	{
		m_LuaScript.RunFunction(g_InitializeFunctionName);
	}
}

/*
** Runs the function "Update()" in the script.
**
*/
void MeasureScript::UpdateValue()
{
	if (m_HasUpdateFunction)
	{
		m_ValueType = m_LuaScript.RunFunctionWithReturn(g_UpdateFunctionName, m_Value, m_StringValue);

		if (m_ValueType == LUA_TNIL && m_HasGetStringFunction)
		{
			// For backwards compatbility
			m_ValueType = m_LuaScript.RunFunctionWithReturn(g_GetStringFunctionName, m_Value, m_StringValue);
		}
	}
}

/*
** Returns the value as a string.
**
*/
const WCHAR* MeasureScript::GetStringValue()
{
	return (m_ValueType == LUA_TSTRING) ? CheckSubstitute(m_StringValue.c_str()) : nullptr;
}

/*
** Read the options specified in the ini file.
**
*/
void MeasureScript::ReadOptions(ConfigParser& parser, const WCHAR* section)
{
	Measure::ReadOptions(parser, section);

	std::wstring scriptFile = parser.ReadString(section, L"ScriptFile", L"");
	if (!scriptFile.empty())
	{
		if (m_Skin)
		{
			m_Skin->MakePathAbsolute(scriptFile);
		}

		if (!m_Initialized ||
			wcscmp(scriptFile.c_str(), m_LuaScript.GetFile().c_str()) != 0)
		{
			UninitializeLuaScript();

			if (m_LuaScript.Initialize(scriptFile))
			{
				bool hasInitializeFunction = m_LuaScript.IsFunction(g_InitializeFunctionName);
				m_HasUpdateFunction = m_LuaScript.IsFunction(g_UpdateFunctionName);

				auto L = m_LuaScript.GetState();
				lua_rawgeti(L, LUA_GLOBALSINDEX, m_LuaScript.GetRef());

				*(Skin**)lua_newuserdata(L, sizeof(Skin*)) = m_Skin;
				lua_getglobal(L, "MeterWindow");
				lua_setmetatable(L, -2);
				lua_setfield(L, -2, "SKIN");

				*(Measure**)lua_newuserdata(L, sizeof(Measure*)) = this;
				lua_getglobal(L, "Measure");
				lua_setmetatable(L, -2);
				lua_setfield(L, -2, "SELF");

				if (!m_LuaScript.IsUnicode())
				{
					// For backwards compatibility.

					m_HasGetStringFunction = m_LuaScript.IsFunction(g_GetStringFunctionName);
					if (m_HasGetStringFunction)
					{
						LogWarningF(this, L"Script: Using deprecated GetStringValue()");
					}

					lua_getfield(L, -1, "PROPERTIES");
					if (lua_isnil(L, -1) == 0)
					{
						lua_pushnil(L);

						// Look in the table for values to read from the section
						while (lua_next(L, -2))
						{
							lua_pop(L, 1);
							const char* strKey = lua_tostring(L, -1);
							const std::wstring wstrKey = StringUtil::Widen(strKey);
							const std::wstring& wstrValue =
								parser.ReadString(section, wstrKey.c_str(), L"");
							if (!wstrValue.empty())
							{
								const std::string strStrVal = StringUtil::Narrow(wstrValue);
								lua_pushstring(L, strStrVal.c_str());
								lua_setfield(L, -3, strKey);
							}
						}
					}

					// Pop PROPERTIES table.
					lua_pop(L, 1);
				}

				// Pop our table.
				lua_pop(L, 1);

				if (m_Initialized)
				{
					// If the measure is already initialized and the script has changed, we need to
					// manually call Initialize().
					Initialize();
				}

				// Valid script.
				return;
			}
		}
		else if (m_LuaScript.IsInitialized())
		{
			// Already initialized.
			return;
		}
	}

	LogErrorF(this, L"Script: File not valid");
	UninitializeLuaScript();
}

/*
** Executes a custom bang.
**
*/
void MeasureScript::Command(const std::wstring& command)
{
	m_LuaScript.RunString(command);
}

bool MeasureScript::CommandWithReturn(const std::wstring& command, std::wstring& strValue)
{
	// Scripts need to be initialized so that any variables declared in
	// the Initialize() function in the lua script file are accessible.
	// Return "0" so that no errors are thrown for forumlas.
	if (!m_Initialized)
	{
		strValue = L"0";
		return true;
	}

	size_t sPos = command.find_first_of(L'(');
	if (sPos != std::wstring::npos)
	{
		// Function call
		size_t ePos = command.find_last_of(L')');
		if (ePos == std::wstring::npos ||
			sPos > ePos ||
			command.size() < 3)
		{
			LogErrorF(this, L"Invalid function call: %s", command.c_str());
			return false;
		}

		std::wstring funcName = command.substr(0, sPos);
		auto args = ConfigParser::Tokenize2(
			command.substr(sPos + 1, ePos - sPos - 1),
			L',',
			PairedPunctuation::BothQuotes);

		if (!m_LuaScript.RunCustomFunction(funcName, args, strValue))
		{
			if (!strValue.empty())
			{
				LogErrorF(this, L"%s", strValue.c_str());
			}
			return false;
		}
	}
	else
	{
		if (!m_LuaScript.GetLuaVariable(command, strValue))
		{
			LogErrorF(this, L"%s", strValue.c_str());
			return false;
		}
	}

	return true;
}

//static void stackDump(lua_State *L)
//{
//	LuaHelper::LuaLogger::Debug(" ----------------  Stack Dump ----------------" );
//	for (int i = lua_gettop(L); i > 0; --i)
//	{
//		int t = lua_type(L, i);
//		switch (t)
//		{
//		case LUA_TSTRING:
//			LuaHelper::LuaLogger::Debug("%d:'%s'", i, lua_tostring(L, i));
//			break;
//
//		case LUA_TBOOLEAN:
//			LuaHelper::LuaLogger::Debug("%d: %s", i, lua_toboolean(L, i) ? "true" : "false");
//			break;
//
//		case LUA_TNUMBER:
//			LuaHelper::LuaLogger::Debug("%d: %g", i, lua_tonumber(L, i));
//			break;
//
//		default:
//			LuaHelper::LuaLogger::Debug("%d: %s", i, lua_typename(L, t));
//			break;
//		}
//	}
//	LuaHelper::LuaLogger::Debug("--------------- Stack Dump Finished ---------------" );
//}
