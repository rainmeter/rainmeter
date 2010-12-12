#include "LuaScript.h"
#include "LuaManager.h"
#include "../Rainmeter.h"

LuaScript::LuaScript(lua_State* p_pState, const char* p_strFile, const char* p_strTableName) : 
	m_pState(p_pState)
{
	m_bInitialized = true; 

	int result = luaL_loadfile(m_pState, p_strFile);

	m_strTableName = strdup(p_strTableName);
	
	// If the file loaded okay.
	if(result == 0)
	{
		// Create the table this script will reside in
		lua_newtable(m_pState);

		// Create the metatable that will store the global table
		lua_createtable(m_pState,0,1); 
		
		// Push the global teble
		lua_pushvalue(m_pState,LUA_GLOBALSINDEX);
		
		// Set the __index of the table to be the global table
		lua_setfield(m_pState,-2, "__index");

		// Set the metatable for the script's table
		lua_setmetatable(m_pState, -2);

		// Put the table into the global table
		lua_setglobal(m_pState, p_strTableName);

		lua_getglobal(m_pState, p_strTableName);

		// Set the environment for the function to be run in to be the table that
		// has been created for the script/
		lua_setfenv(m_pState,-2);

		// Execute the Lua script
		result = lua_pcall(m_pState, 0, LUA_MULTRET, 0);

		if(result)
		{
			m_bInitialized = false;
			LuaManager::LuaLog("Lua cannot run file:");
			LuaManager::LuaLog("%s", lua_tostring(m_pState, -1) );
		}
	}
	else
	{
		m_bInitialized = false;
		LuaManager::LuaLog("Lua cannot run file:");
		LuaManager::LuaLog("%s", lua_tostring(m_pState, -1) );
	}

}

LuaScript::~LuaScript(void)
{
	delete[] m_strTableName;
}

void LuaScript::BindVariable(const char* p_strName, void* p_pValue, const char* p_strTypeName)
{

	PushTable();

	/*
	// Push the variable name we want to put a value in.
	lua_pushstring(m_pState, p_strName);
	// Push the value
	tolua_pushusertype(m_pState, p_pValue, p_strTypeName);
	// Bind the variable
	lua_settable(m_pState, -3);

	// Pop our table off of the stack
	lua_pop(m_pState, 1);

	*/

	// Push the variable name we want to put a value in.
	lua_pushstring(m_pState, "SKIN");
	// Push the value
	tolua_pushusertype(m_pState, p_pValue, "CMeterWindow");
	// Bind the variable
	lua_settable(m_pState, -3);

	//lua_pop(m_pLuaScript->GetState(), 1);
}

double LuaScript::RunFunctionDouble(const char* p_strFuncName)
{
	if( m_bInitialized && p_strFuncName )
	{
		// Push our table onto the stack
		lua_getglobal(m_pState, m_strTableName);

		// Push the function onto the stack
		lua_getfield(m_pState,-1, p_strFuncName);

		if( lua_pcall(m_pState, 0, 1, 0) )
		{
			LuaManager::ReportErrors(m_pState);
		}
		else
		{
			if (!lua_isnumber(m_pState, -1))
			{
				LuaManager::LuaLog("Function `%s:%s' must return a number",m_strTableName, p_strFuncName);
			}

			double d = lua_tonumber(m_pState, -1);

			lua_pop(m_pState, 1);

			return d;
		}

		lua_pop(m_pState, 1);
	}

	return -1;
}


std::wstring LuaScript::RunFunctionString(const char* p_strFuncName)
{
	if( m_bInitialized && p_strFuncName )
	{
		// Push our table onto the stack
		lua_getglobal(m_pState, m_strTableName);

		// Push the function onto the stack
		lua_getfield(m_pState,-1, p_strFuncName);

		if( lua_pcall(m_pState, 0, 1, 0) )
		{
			LuaManager::ReportErrors(m_pState);
		}
		else
		{
			if (!lua_isstring(m_pState, -1))
			{
				LuaManager::LuaLog("Function `%s:%s' must return a string",m_strTableName, p_strFuncName);
			}

			const char* str = lua_tostring(m_pState, -1);

			lua_pop(m_pState, 1);

			return ConvertToWide(str);
		}

		lua_pop(m_pState, 1);
	}

	return 0;
}

void LuaScript::RunFunction(const char* p_strFuncName)
{
	if( m_bInitialized && p_strFuncName )
	{
		// Push our table onto the stack
		lua_getglobal(m_pState, m_strTableName);

		// Push the function onto the stack
		lua_getfield(m_pState,-1, p_strFuncName);

		if( lua_pcall(m_pState, 0, 0, 0) )
		{
			LuaManager::ReportErrors(m_pState);
		}

		lua_pop(m_pState, 1);
	}
}

bool LuaScript::FunctionExists(const char* p_strFuncName)
{
	bool bExists = false;

	if( m_bInitialized && p_strFuncName )
	{
		// Push our table onto the stack
		lua_getglobal(m_pState, m_strTableName);

		// Push the function onto the stack
		lua_getfield(m_pState,-1, p_strFuncName);

		if( lua_isfunction( m_pState, lua_gettop(m_pState) ) ) 
		{
			bExists = true;
		}

		// Pop both the table and the function off the stack.
		lua_pop(m_pState, 2);
	}

	return bExists;
}


