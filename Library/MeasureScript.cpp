#include "StdAfx.h"
#include "MeasureScript.h"
#include "lua/LuaManager.h"
#include "Litestep.h"
#include "Rainmeter.h"

const char* g_strInitFunction = "Initialize";
const char* g_strUpdateFunction = "Update";
const char* g_strGetValueFunction = "GetValue";
const char* g_strGetStringValueFunction = "GetStringValue";

CMeasureScript::CMeasureScript(CMeterWindow* meterWindow) : CMeasure(meterWindow)
{
	LuaManager::Init();

	m_pLuaScript = NULL;

	m_bUpdateDefined = false;
	m_bGetValueDefined = false;
	m_bGetStringValueDefined = false;
	m_bInitializeDefined = false;
}

CMeasureScript::~CMeasureScript()
{
	delete m_pLuaScript;

}

void CMeasureScript::Initialize()
{
	CMeasure::Initialize();

	if( m_bInitializeDefined  && m_pLuaScript->IsInitialized() )
	{
		m_pLuaScript->RunFunction(g_strInitFunction);
	}
}

bool CMeasureScript::Update()
{

	if (!CMeasure::PreUpdate())
	{
		return false;
	}

	if( m_bUpdateDefined && m_pLuaScript->IsInitialized() )
	{
		m_pLuaScript->RunFunction(g_strUpdateFunction);
	}

	return PostUpdate();
}

double CMeasureScript::GetValue()
{

	if( m_bGetValueDefined && m_pLuaScript->IsInitialized()  )
	{
		return m_pLuaScript->RunFunctionDouble(g_strGetValueFunction);
	}

	return CMeasure::GetValue();

}

void CMeasureScript::SetValue(double d)
{
	m_Value = d;
}

/*
** GetStringValue
**
** This method returns the value as text string. The actual value is
** get with GetValue() so we don't have to worry about m_Invert.
** 
** autoScale  If true, scale the value automatically to some sensible range.
** scale      The scale to use if autoScale is false.
** decimals   Number of decimals used in the value.
** percentual Return the value as % from the maximum value.
*/
const WCHAR* CMeasureScript::GetStringValue(bool autoScale, double scale, int decimals, bool percentual)
{

	if (m_bGetStringValueDefined && m_pLuaScript->IsInitialized() )
	{
		m_strValue = m_pLuaScript->RunFunctionString(g_strGetStringValueFunction) ;
		
		return CheckSubstitute(m_strValue.c_str());
	}

	return CMeasure::GetStringValue(autoScale, scale, decimals, percentual);
}

static void stackDump (lua_State *L) {
      int i=lua_gettop(L);
      LuaManager::LuaLog(" ----------------  Stack Dump ----------------" );
      while(  i   ) {
        int t = lua_type(L, i);
        switch (t) {
          case LUA_TSTRING:
			  LuaManager::LuaLog("%d:`%s'", i, lua_tostring(L, i));
          break;
          case LUA_TBOOLEAN:
            LuaManager::LuaLog("%d: %s",i,lua_toboolean(L, i) ? "true" : "false");
          break;
          case LUA_TNUMBER:
            LuaManager::LuaLog("%d: %g",  i, lua_tonumber(L, i));
         break;
         default: LuaManager::LuaLog("%d: %s", i, lua_typename(L, t)); break;
        }
       i--;
      }
     LuaManager::LuaLog("--------------- Stack Dump Finished ---------------" );
}


/*
** ReadConfig
**
** Reads the measure specific configs.
**
*/
void CMeasureScript::ReadConfig(CConfigParser& parser, const WCHAR* section)
{

	CMeasure::ReadConfig(parser, section);

	std::string strScriptFile = ConvertToAscii(parser.ReadString(section, L"ScriptFile", L"").c_str());
	std::string strTableName = ConvertToAscii(parser.ReadString(section, L"TableName", L"").c_str());

	if(strScriptFile.empty() == false)
	{

		if(strTableName.empty() == false)
		{
			lua_State* L = LuaManager::GetState();
			m_pLuaScript = new LuaScript(LuaManager::GetState(), strScriptFile.c_str(), strTableName.c_str());
			
			m_bUpdateDefined = m_pLuaScript->FunctionExists(g_strUpdateFunction);
			m_bInitializeDefined = m_pLuaScript->FunctionExists(g_strInitFunction);
			m_bGetValueDefined = m_pLuaScript->FunctionExists(g_strGetValueFunction);
			m_bGetStringValueDefined = m_pLuaScript->FunctionExists(g_strGetStringValueFunction);


			m_pLuaScript->PushTable();

			// Push the variable name we want to put a value in.
			lua_pushstring(L, "SELF");
			// Push the value
			tolua_pushusertype(L, this, "CMeasure");
			// Bind the variable
			lua_settable(L, -3);

			// Push the variable name we want to put a value in.
			lua_pushstring(L, "SKIN");
			// Push the value
			tolua_pushusertype(L, m_MeterWindow, "CMeterWindow");
			// Bind the variable
			lua_settable(L, -3);

			// Push the variable name we want to put a value in.
			lua_pushstring(L, "RAINMETER");
			// Push the value
			tolua_pushusertype(L, m_MeterWindow->GetMainObject(), "CRainmeter");
			// Bind the variable
			lua_settable(L, -3);

			// Look i nthe properties table for values to read from the section.
			lua_getfield(L, -1, "PROPERTIES");
			if(lua_isnil(L, -1) == 0)
			{
				lua_pushnil(L);

				while(lua_next(L, -2)) 
				{  
					lua_pop(L, 1);

					const char* strKey = lua_tostring(L, -1);

					std::wstring wstrKey = ConvertToWide(strKey);
					std::wstring wstrValue = parser.ReadString(section, wstrKey.c_str(), L"").c_str();
					
					if( !wstrValue.empty() )
					{
						const wchar_t* wstrValue2 = wstrValue.c_str();
						std::string strStrVal = ConvertToAscii(wstrValue2);
						const char* strValue = strStrVal.c_str();
						
						lua_pushstring(L,strValue);
						lua_settable(L, -3);

						lua_pushstring(L,strKey);
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
			LuaManager::LuaLog("%s : TableName missing.", m_ANSIName.c_str());
		}
	}
	else
	{
		LuaManager::LuaLog("%s : ScriptFile missing.", m_ANSIName.c_str());
	}

}
void CMeasureScript::RunFunctionWithMeter(const char* p_strFunction, CMeter* p_pMeter)
{

	// Get the Lua State
	lua_State* L = m_pLuaScript->GetState();

	// Push the script table
	m_pLuaScript->PushTable();

	// Push the function onto the stack
	lua_getfield(L,-1, p_strFunction);

	// Push the Meter
	tolua_pushusertype(L, (void*) p_pMeter ,"CMeter");

	if( lua_pcall(L, 1, 0, 0) )
	{
		LuaManager::ReportErrors(L);
	}

	// Clean up
	lua_pop(L, 1);
}

void CMeasureScript::MeterMouseEvent(CMeter* p_pMeter, MOUSE p_eMouse)
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