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
	m_HasGetStringFunction(false)
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
	if (m_LuaScript)
	{
		delete m_LuaScript;
		m_LuaScript = NULL;
	}

	m_HasInitializeFunction = false;
	m_HasUpdateFunction = false;
	m_HasGetStringFunction = false;
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
		bool ret = m_LuaScript->RunFunctionWithReturn(g_UpdateFunctionName, m_Value, m_StringValue);

		if (!ret)
		{
			// Update() didn't return anything. For backwards compatibility, check for GetStringValue() first
			if (m_HasGetStringFunction)
			{
				m_LuaScript->RunFunctionWithReturn(g_GetStringFunctionName, m_Value, m_StringValue);
			}
			else
			{
				std::wstring error = L"Script: Update() in measure [";
				error += m_Name;
				error += L"] is not returning a valid number or string.";
				Log(LOG_WARNING, error.c_str());
			}
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
	return m_StringValue.c_str();
}

/*
** ReadConfig
**
** Reads the measure specific configs.
**
*/
void CMeasureScript::ReadConfig(CConfigParser& parser, const WCHAR* section)
{
	// Store the current values
	std::string oldScriptFile = m_ScriptFile;

	// Read common configs
	CMeasure::ReadConfig(parser, section);

	m_ScriptFile = ConvertToAscii(parser.ReadString(section, L"ScriptFile", L"").c_str());

	if (!m_ScriptFile.empty())
	{
		if (!m_Initialized ||
			oldScriptFile != m_ScriptFile)
		{
			lua_State* L = LuaManager::GetState();

			DeleteLuaScript();
			m_LuaScript = new LuaScript(LuaManager::GetState(), m_ScriptFile.c_str());

			if (m_LuaScript->IsInitialized())
			{
				m_HasInitializeFunction = m_LuaScript->IsFunction(g_InitializeFunctionName);
				m_HasUpdateFunction = m_LuaScript->IsFunction(g_UpdateFunctionName);
				m_HasGetStringFunction = m_LuaScript->IsFunction(g_GetStringFunctionName);  // For backwards compatbility

				if (m_HasGetStringFunction)
				{
					std::wstring error = L"Script: GetStringValue() used in measure [";
					error += m_Name;
					error += L"] has been deprecated. Check manual to ensure future compatibility.";
					Log(LOG_WARNING, error.c_str());
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

				lua_pushstring(L, "RAINMETER");
				tolua_pushusertype(L, m_MeterWindow->GetMainObject(), "CRainmeter");
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
						std::wstring wstrValue = parser.ReadString(section, wstrKey.c_str(), L"");

						if (!wstrValue.empty())
						{
							std::string strStrVal = ConvertToAscii(wstrValue.c_str());
							const char* strValue = strStrVal.c_str();

							lua_pushstring(L, strValue);
							lua_setfield(L, -3, strKey);
						}
					}
				}

				// Pop PROPERTIES table
				lua_pop(L, 1);

				// Pop the table
				lua_pop(L, 1);
			}
			else
			{
				DeleteLuaScript();
			}
		}
	}
	else
	{
		std::wstring error = L"Script: ScriptFile= is not valid in measure [";
		error += m_Name;
		error += L"].";
		Log(LOG_WARNING, error.c_str());

		DeleteLuaScript();
	}
}

void CMeasureScript::RunFunctionWithMeter(const char* p_strFunction, CMeter* p_pMeter)
{
	// Get the Lua State
	lua_State* L = m_LuaScript->GetState();

	// Push the script table
	m_LuaScript->PushTable();

	// Push the function onto the stack
	lua_getfield(L, -1, p_strFunction);

	// Check if the function exists
	if (!lua_isnil(L, -1))
	{
		// Push the Meter
		tolua_pushusertype(L, (void*) p_pMeter, "CMeter");

		if (lua_pcall(L, 1, 0, 0))
		{
			LuaManager::ReportErrors(L);
		}

		lua_pop(L, 1);
	}
	else
	{
		lua_pop(L, 2);
	}
}

void CMeasureScript::MeterMouseEvent(CMeter* p_pMeter, MOUSE p_eMouse)
{
	if (m_LuaScript && m_LuaScript->IsInitialized())
	{
		switch (p_eMouse)
		{
		case MOUSE_LMB_DOWN:
			RunFunctionWithMeter("LeftMouseDown", p_pMeter);
			break;

		case MOUSE_LMB_UP:
			RunFunctionWithMeter("LeftMouseUp", p_pMeter);
			break;

		case MOUSE_LMB_DBLCLK:
			RunFunctionWithMeter("LeftMouseDoubleClick", p_pMeter);
			break;

		case MOUSE_RMB_DOWN:
			RunFunctionWithMeter("RightMouseDown", p_pMeter);
			break;

		case MOUSE_RMB_UP:
			RunFunctionWithMeter("RightMouseUp", p_pMeter);
			break;

		case MOUSE_RMB_DBLCLK:
			RunFunctionWithMeter("RightMouseDoubleClick", p_pMeter);
			break;

		case MOUSE_MMB_DOWN:
			RunFunctionWithMeter("MiddleMouseDown", p_pMeter);
			break;

		case MOUSE_MMB_UP:
			RunFunctionWithMeter("MiddleMouseUp", p_pMeter);
			break;

		case MOUSE_MMB_DBLCLK:
			RunFunctionWithMeter("MiddleMouseDoubleClick", p_pMeter);
			break;
		}
	}
}

static void stackDump(lua_State *L)
{
	int i = lua_gettop(L);
	LuaManager::LuaLog(LOG_DEBUG, " ----------------  Stack Dump ----------------" );
	while (i)
	{
		int t = lua_type(L, i);
		switch (t)
		{
		case LUA_TSTRING:
			LuaManager::LuaLog(LOG_DEBUG, "%d:`%s'", i, lua_tostring(L, i));
			break;

		case LUA_TBOOLEAN:
			LuaManager::LuaLog(LOG_DEBUG, "%d: %s",i,lua_toboolean(L, i) ? "true" : "false");
			break;

		case LUA_TNUMBER:
			LuaManager::LuaLog(LOG_DEBUG, "%d: %g",  i, lua_tonumber(L, i));
			break;

		default:
			LuaManager::LuaLog(LOG_DEBUG, "%d: %s", i, lua_typename(L, t));
			break;
		}
		i--;
	}
	LuaManager::LuaLog(LOG_DEBUG, "--------------- Stack Dump Finished ---------------" );
}
