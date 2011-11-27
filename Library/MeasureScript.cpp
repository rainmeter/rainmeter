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

#include "StdAfx.h"
#include "MeasureScript.h"
#include "lua/LuaManager.h"
#include "Litestep.h"
#include "Rainmeter.h"

const char* g_InitializeFunctionName = "Initialize";
const char* g_UpdateFunctionName = "Update";
const char* g_GetStringFunctionName = "GetStringValue";

/*
** CMeasureScript
**
** The constructor
**
*/
CMeasureScript::CMeasureScript(CMeterWindow* meterWindow, const WCHAR* name) : CMeasure(meterWindow, name),
	m_LuaScript(),
	m_HasInitializeFunction(false),
	m_HasUpdateFunction(false),
	m_HasGetStringFunction(false),
	m_ValueType(LUA_TNIL)
{
	LuaManager::Init();
}

/*
** ~CMeasureScript
**
** The destructor
**
*/
CMeasureScript::~CMeasureScript()
{
	DeleteLuaScript();
	LuaManager::CleanUp();
}

void CMeasureScript::DeleteLuaScript()
{
	delete m_LuaScript;
	m_LuaScript = NULL;

	m_HasInitializeFunction = false;
	m_HasUpdateFunction = false;
	m_HasGetStringFunction = false;

	m_ScriptFile.clear();
}

/*
** Initialize
**
** Initializes the measure.
**
*/
void CMeasureScript::Initialize()
{
	CMeasure::Initialize();

	if (m_HasInitializeFunction)
	{
		m_LuaScript->RunFunction(g_InitializeFunctionName);
	}
}

/*
** Update
**
** Updates the current disk free space value.
**
*/
bool CMeasureScript::Update()
{
	if (!CMeasure::PreUpdate())
	{
		return false;
	}

	if (m_HasUpdateFunction)
	{
		m_ValueType = m_LuaScript->RunFunctionWithReturn(g_UpdateFunctionName, m_Value, m_StringValue);

		if (m_ValueType == LUA_TNIL && m_HasGetStringFunction)
		{
			// For backwards compatbility
			m_ValueType = m_LuaScript->RunFunctionWithReturn(g_GetStringFunctionName, m_Value, m_StringValue);
		}
	}

	return PostUpdate();
}

/*
** GetStringValue
**
** Returns the time as string.
**
*/
const WCHAR* CMeasureScript::GetStringValue(AUTOSCALE autoScale, double scale, int decimals, bool percentual)
{
	if (m_ValueType == LUA_TSTRING)
	{
		return CheckSubstitute(m_StringValue.c_str());
	}

	return CMeasure::GetStringValue(autoScale, scale, decimals, percentual);
}

/*
** ReadConfig
**
** Reads the measure specific configs.
**
*/
void CMeasureScript::ReadConfig(CConfigParser& parser, const WCHAR* section)
{
	// Read common configs
	CMeasure::ReadConfig(parser, section);

	const std::wstring& file = parser.ReadString(section, L"ScriptFile", L"");

	if (!file.empty())
	{
		std::string scriptFile = ConvertToAscii(m_MeterWindow->MakePathAbsolute(file).c_str());

		if (!m_Initialized ||
			strcmp(scriptFile.c_str(), m_ScriptFile.c_str()) != 0)
		{
			DeleteLuaScript();

			lua_State* L = LuaManager::GetState();
			m_ScriptFile = scriptFile;
			m_LuaScript = new LuaScript(LuaManager::GetState(), m_ScriptFile.c_str());

			if (m_LuaScript->IsInitialized())
			{
				m_HasInitializeFunction = m_LuaScript->IsFunction(g_InitializeFunctionName);
				m_HasUpdateFunction = m_LuaScript->IsFunction(g_UpdateFunctionName);
				m_HasGetStringFunction = m_LuaScript->IsFunction(g_GetStringFunctionName);  // For backwards compatbility

				if (m_HasGetStringFunction)
				{
					LogWithArgs(LOG_WARNING, L"Script: Using deprecated GetStringValue() in [%s]", m_Name.c_str());
				}

				m_LuaScript->PushTable();

				// Push the variable name we want to put a value in.
				lua_pushstring(L, "SELF");
				// Push the value
				tolua_pushusertype(L, this, "CMeasure");
				// Bind the variable
				lua_settable(L, -3);

				lua_pushstring(L, "SKIN");
				tolua_pushusertype(L, m_MeterWindow, "CMeterWindow");
				lua_settable(L, -3);

				// Look in the properties table for values to read from the section.
				lua_getfield(L, -1, "PROPERTIES");
				if (lua_isnil(L, -1) == 0)
				{
					lua_pushnil(L);

					while (lua_next(L, -2))
					{
						lua_pop(L, 1);
						const char* strKey = lua_tostring(L, -1);

						std::wstring wstrKey = ConvertToWide(strKey);
						const std::wstring& wstrValue = parser.ReadString(section, wstrKey.c_str(), L"");

						if (!wstrValue.empty())
						{
							std::string strStrVal = ConvertToAscii(wstrValue.c_str());
							const char* strValue = strStrVal.c_str();

							lua_pushstring(L, strValue);
							lua_setfield(L, -3, strKey);
						}
					}
				}

				// Pop PROPERTIES table and our table
				lua_pop(L, 2);
			}
			else
			{
				DeleteLuaScript();
			}
		}
	}
	else
	{
		LogWithArgs(LOG_ERROR, L"Script: File not valid in [%s]", m_Name.c_str());
		DeleteLuaScript();
	}
}

/*
** ExecuteBang
**
** Sends a bang to the measure.
**
*/
void CMeasureScript::ExecuteBang(const WCHAR* args)
{
	std::string str = ConvertToAscii(args);
	m_LuaScript->RunString(str.c_str());
}

//static void stackDump(lua_State *L)
//{
//	LuaManager::LuaLog(LOG_DEBUG, " ----------------  Stack Dump ----------------" );
//	for (int i = lua_gettop(L); i > 0; --i)
//	{
//		int t = lua_type(L, i);
//		switch (t)
//		{
//		case LUA_TSTRING:
//			LuaManager::LuaLog(LOG_DEBUG, "%d:'%s'", i, lua_tostring(L, i));
//			break;
//
//		case LUA_TBOOLEAN:
//			LuaManager::LuaLog(LOG_DEBUG, "%d: %s", i, lua_toboolean(L, i) ? "true" : "false");
//			break;
//
//		case LUA_TNUMBER:
//			LuaManager::LuaLog(LOG_DEBUG, "%d: %g", i, lua_tonumber(L, i));
//			break;
//
//		default:
//			LuaManager::LuaLog(LOG_DEBUG, "%d: %s", i, lua_typename(L, t));
//			break;
//		}
//	}
//	LuaManager::LuaLog(LOG_DEBUG, "--------------- Stack Dump Finished ---------------" );
//}
