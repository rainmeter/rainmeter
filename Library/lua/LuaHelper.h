/* Copyright (C) 2016 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#ifndef __LUAHELPER_H__
#define __LUAHELPER_H__

extern "C"
{
#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
}

class LuaHelper
{
public:	
	static bool LoadFile(lua_State* L, const std::wstring& file);
	static int RunFile(lua_State* L, const std::wstring& file, int n);

	static void ReportErrors(lua_State* L, const std::wstring& file);

	static void PushWide(lua_State* L, const WCHAR* str);
	static void PushWide(lua_State* L, const std::wstring& str);
	static std::wstring ToWide(lua_State* L, int narg);

};
#endif
