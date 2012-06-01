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
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
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
	LuaManager::Initialize();
}

/*
** The destructor
**
*/
CMeasureScript::~CMeasureScript()
{
	DeleteLuaScript();
	LuaManager::Finalize();
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
** Updates the current disk free space value.
**
*/
void CMeasureScript::UpdateValue()
{
	if (m_HasUpdateFunction)
	{
		m_ValueType = m_LuaScript->RunFunctionWithReturn(g_UpdateFunctionName, m_Value, m_StringValue);

		if (m_ValueType == LUA_TNIL && m_HasGetStringFunction)
		{
			// For backwards compatbility
			m_ValueType = m_LuaScript->RunFunctionWithReturn(g_GetStringFunctionName, m_Value, m_StringValue);
		}
	}
}

/*
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
** Read the options specified in the ini file.
**
*/
void CMeasureScript::ReadOptions(CConfigParser& parser, const WCHAR* section)
{
	CMeasure::ReadOptions(parser, section);

	std::wstring file = parser.ReadString(section, L"ScriptFile", L"");

	if (!file.empty())
	{
		if (m_MeterWindow)
		{
			m_MeterWindow->MakePathAbsolute(file);
		}
		std::string scriptFile = ConvertToAscii(file.c_str());

		if (!m_Initialized ||
			strcmp(scriptFile.c_str(), m_ScriptFile.c_str()) != 0)
		{
			DeleteLuaScript();

			lua_State* L = LuaManager::GetState();
			m_ScriptFile = scriptFile;
			m_LuaScript = new LuaScript(m_ScriptFile.c_str());

			if (m_LuaScript->IsInitialized())
			{
				m_HasInitializeFunction = m_LuaScript->IsFunction(g_InitializeFunctionName);
				m_HasUpdateFunction = m_LuaScript->IsFunction(g_UpdateFunctionName);
				m_HasGetStringFunction = m_LuaScript->IsFunction(g_GetStringFunctionName);  // For backwards compatbility

				if (m_HasGetStringFunction)
				{
					LogWithArgs(LOG_WARNING, L"Script: Using deprecated GetStringValue() in [%s]", m_Name.c_str());
				}

				lua_rawgeti(L, LUA_GLOBALSINDEX, m_LuaScript->GetRef());

				*(CMeterWindow**)lua_newuserdata(L, sizeof(CMeterWindow*)) = m_MeterWindow;
				lua_getglobal(L, "CMeterWindow");
				lua_setmetatable(L, -2);
				lua_setfield(L, -2, "SKIN");

				*(CMeasure**)lua_newuserdata(L, sizeof(CMeasure*)) = this;
				lua_getglobal(L, "CMeasure");
				lua_setmetatable(L, -2);
				lua_setfield(L, -2, "SELF");

				// For backwards compatibility
				lua_getfield(L, -1, "PROPERTIES");
				if (lua_isnil(L, -1) == 0)
				{
					lua_pushnil(L);
					
					// Look in the table for values to read from the section
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
** Executes a custom bang.
**
*/
void CMeasureScript::Command(const std::wstring& command)
{
	std::string str = ConvertToAscii(command.c_str());
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
