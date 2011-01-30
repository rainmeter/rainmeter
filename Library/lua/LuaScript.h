#ifndef LUA_SCRIPT_H
#define LUA_SCRIPT_H

#include <stdio.h>
#include "string"

#include "lua.hpp"

class LuaScript
{
public:

	LuaScript(lua_State* p_pState, const char* p_strFile, const char* p_strTableName);

	~LuaScript(void);
	
	bool FunctionExists(const char* p_strFuncName);

	void RunFunction(const char* p_strFuncName);

	double RunFunctionDouble(const char* p_strFuncName);

	std::wstring RunFunctionString(const char* p_strFuncName);

	lua_State* GetState() { return m_pState; }

	bool IsInitialized() { return m_bInitialized; }

	void BindVariable(const char* p_strName, void* p_pValue, const char* p_strTypeName);

	void PushTable() { lua_getglobal(m_pState, m_strTableName); }

	static void ReportErrors(lua_State * L);

protected:

	lua_State* m_pState;

	char* m_strTableName;

	bool m_bInitialized;
};

#endif

