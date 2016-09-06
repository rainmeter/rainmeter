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
#include <vector>

class LuaHelper
{
public:
	static bool LoadFile(lua_State* L, const std::wstring& file, bool& unicode);
	static bool IsFunction(lua_State* L, const char* funcName, const std::wstring& file, bool unicode);
	static bool RunFunction(lua_State* L, const char* funcName, const std::wstring& file, bool unicode);
	static bool RunFile(lua_State* L, const std::wstring& file, bool& unicode);
	static bool RunString(lua_State* L, const std::wstring& str, const std::wstring& file, bool unicode);
	static bool RunFunctionWithReturn(lua_State* L, const char* funcName, const std::wstring& file, bool unicode);

	static void ReportErrors(lua_State* L, const std::wstring& file);

	static void PushWide(lua_State* L, const WCHAR* str);
	static void PushWide(lua_State* L, const std::wstring& str);
	static std::wstring ToWide(lua_State* L, int narg);

	static std::pair<bool, std::wstring> IsUnicodeFile() { return m_UnicodeFile.back(); }

private:
	static std::vector<std::pair<bool, std::wstring>> m_UnicodeFile;
};

#endif
