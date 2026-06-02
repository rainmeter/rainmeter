/* Copyright (C) 2016 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#ifndef __LUAHELPER_H__
#define __LUAHELPER_H__

#include <vector>

extern "C"
{
#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
}

class LuaStateScope
{
public:
	static LuaStateScope* GetCurrent() { return c_ScopeStack.back(); }

	LuaStateScope(lua_State* state, bool unicode, int ref, std::wstring path);
	~LuaStateScope();

	operator lua_State*() { return m_State; }
	lua_State* GetState() { return m_State; }

	bool IsUnicode() { return m_Unicode; }
	int GetRef() { return m_Ref; }
	std::wstring GetSourceFile() { return m_File; }

private:
	lua_State* m_State;
	bool m_Unicode;
	int m_Ref;
	std::wstring m_File;

	static std::vector<LuaStateScope*> c_ScopeStack;
};

class LuaHelper
{
public:
	static void LogAndPopError();

	static void PushWide(const WCHAR* str);
	static void PushWide(const std::wstring& str);
	static std::wstring ToWide(int narg);
	static bool ToBool(int narg);

	static void StackDump();
};

#endif
