/* Copyright (C) 2010 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#ifndef __LUASCRIPT_H__
#define __LUASCRIPT_H__

#include "LuaManager.h"

class LuaScript
{
public:
	LuaScript();
	~LuaScript();

	bool Initialize(const std::wstring& scriptFile);
	void Uninitialize();
	bool IsInitialized() { return m_State != nullptr && m_Ref != LUA_NOREF; }

	const std::wstring& GetFile() { return m_File; }
	int GetRef() { return m_Ref; }
	bool IsUnicode() const { return m_Unicode; }

	lua_State* GetState() { return m_State; }

	bool IsFunction(const char* funcName);
	void RunFunction(const char* funcName);
	int RunFunctionWithReturn(const char* funcName, double& numValue, std::wstring& strValue);
	void RunString(const std::wstring& str);

	static LuaScript* GetActiveScript() { return m_ActiveScript; }

protected:
	static void RegisterGlobal(lua_State* L);
	static void RegisterMeasure(lua_State* L);
	static void RegisterMeter(lua_State* L);
	static void RegisterSkin(lua_State* L);

	std::wstring m_File;
	int m_Ref;
	bool m_Unicode;
	lua_State* m_State;

private:
	static LuaScript* m_ActiveScript;
};

#endif

