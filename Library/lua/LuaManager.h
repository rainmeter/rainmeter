/* Copyright (C) 2010 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#ifndef __LUAMANAGER_H__
#define __LUAMANAGER_H__

extern "C"
{
#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
}

#include <vector>

class LuaManager
{
public:
	class ScopedLuaState
	{
	public:
		ScopedLuaState(bool unicode) { LuaManager::c_UnicodeStateStack.push_back(unicode); }
		~ScopedLuaState() { LuaManager::c_UnicodeStateStack.pop_back(); }
		operator lua_State*() { return LuaManager::c_State; }
	};

	static void Initialize();
	static void Finalize();

	static ScopedLuaState GetState(bool unicode) { return ScopedLuaState(unicode); }

	static bool IsUnicodeState() { return c_UnicodeStateStack.back(); }

	static void ReportErrors(const std::wstring& file);

	static void PushWide(const WCHAR* str);
	static void PushWide(const std::wstring& str);
	static std::wstring ToWide(int narg);

	static int GetActiveScriptRef();
	static void SetActiveScriptRef(int ref);

protected:
	static int c_RefCount;
	static lua_State* c_State;

private:
	static void RegisterGlobal(lua_State* L);
	static void RegisterMeasure(lua_State* L);
	static void RegisterMeter(lua_State* L);
	static void RegisterSkin(lua_State* L);
	static void RegisterMeterString(lua_State* L);

	// If the back of the vector is |true|, Lua strings converted to/from as if they were encoded
	// in UTF-8. Otherwise Lua strings are treated as if they are encoded in the default system
	// encoding.
	static std::vector<bool> c_UnicodeStateStack;

	static int m_ActiveScriptRef;
};

#endif
