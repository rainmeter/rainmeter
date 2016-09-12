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

class LuaHelper
{
public:
	class UnicodeScript
	{
	public:
		UnicodeScript(lua_State* state, bool unicode, int ref, std::wstring path);
		~UnicodeScript();

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
	};

	static UnicodeScript GetState(lua_State* state, bool unicode, int ref,
		const std::wstring& path) { return UnicodeScript(state, unicode, ref, path); }

	static UnicodeScript* GetCurrentScript() { return c_ScriptStack.back(); }

	static void ReportErrors(const std::wstring& file);

	static void PushWide(const WCHAR* str);
	static void PushWide(const std::wstring& str);
	static std::wstring ToWide(int narg);

private:
	static std::vector<UnicodeScript*> c_ScriptStack;
};

#endif
