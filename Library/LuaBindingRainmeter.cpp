// Copyright (c) Rainmeter Team. Source code licensed under GNU GPL v2 (see LICENSE file).

#include "StdAfx.h"
#include "LuaHelper.h"
#include "Skin.h"
#include "../Common/FileUtil.h"

// The functions of the module are not methods, so the skin a path is resolved against is read from
// the table the script runs in, where MeasureScript stores it once the script has been loaded.
static Skin* GetSkin(lua_State* L)
{
	// Stack: [scriptTable]
	lua_rawgeti(L, LUA_GLOBALSINDEX, LuaStateScope::GetCurrent()->GetRef());

	// Stack: [scriptTable, skin]
	lua_getfield(L, -1, "SKIN");
	void* skinData = lua_touserdata(L, -1);

	// Stack: []
	lua_pop(L, 2);

	return skinData ? *(Skin**)skinData : nullptr;
}

#define DECLARE_SKIN(L) \
	Skin* skin = GetSkin(L); \
	if (!skin) return luaL_error(L, "the skin is not available until the script has been loaded");

// Named with a "Lua" prefix to avoid confusion with the Win32 ReadFile()/WriteFile().
static int LuaReadFile(lua_State* L)
{
	DECLARE_SKIN(L)
	std::wstring path = LuaHelper::ToWide(1);
	skin->MakePathAbsolute(path);

	std::wstring text;
	if (!FileUtil::ReadTextFile(path, text))
	{
		lua_pushnil(L);
		LuaHelper::PushWide(L"Unable to read file: " + path);
		return 2;
	}

	LuaHelper::PushWide(text);

	return 1;
}

static int LuaWriteFile(lua_State* L)
{
	DECLARE_SKIN(L)
	std::wstring path = LuaHelper::ToWide(1);
	skin->MakePathAbsolute(path);

	const std::wstring text = LuaHelper::ToWide(2);

	auto encoding = FileUtil::Encoding::UTF16LE;
	if (lua_isstring(L, 3))
	{
		const std::wstring name = LuaHelper::ToWide(3);
		if (_wcsicmp(name.c_str(), L"ANSI") == 0)
		{
			encoding = FileUtil::Encoding::ANSI;
		}
		else if (_wcsicmp(name.c_str(), L"UTF8") == 0 || _wcsicmp(name.c_str(), L"UTF-8") == 0)
		{
			encoding = FileUtil::Encoding::UTF8;
		}
		else if (_wcsicmp(name.c_str(), L"UTF16") != 0 && _wcsicmp(name.c_str(), L"UTF-16") != 0)
		{
			lua_pushboolean(L, 0);
			LuaHelper::PushWide(L"Unknown encoding: " + name);
			return 2;
		}
	}

	if (!FileUtil::WriteTextFile(path, text, encoding))
	{
		lua_pushboolean(L, 0);
		LuaHelper::PushWide(L"Unable to write file: " + path);
		return 2;
	}

	lua_pushboolean(L, 1);
	return 1;
}

int luaopen_rm(lua_State* L)
{
	const luaL_Reg functions[] =
	{
		{ "ReadFile", LuaReadFile },
		{ "WriteFile", LuaWriteFile },
		{ nullptr, nullptr }
	};

	luaL_register(L, "rm", functions);

	return 1;
}
