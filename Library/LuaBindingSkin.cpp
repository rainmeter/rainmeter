// Copyright (c) Rainmeter Team. Source code licensed under GNU GPL v2 (see LICENSE file).

#include "StdAfx.h"
#include "LuaBindingSection.h"
#include "LuaHelper.h"
#include "LuaScript.h"
#include "Rainmeter.h"
#include "Skin.h"
#include "MeterString.h"
#include "../Common/FileUtil.h"

#define DECLARE_SELF(L) \
	void* selfData = lua_touserdata(L, 1); \
	if (!selfData) return 0; \
	Skin* self = *(Skin**)selfData;

static int Bang(lua_State* L)
{
	DECLARE_SELF(L)
	ConfigParser& parser = self->GetParser();

	std::wstring bang = LuaHelper::ToWideString(2);

	int top = lua_gettop(L);
	if (top == 2)	// 1 argument
	{
		parser.ReplaceVariables(bang);
		GetRainmeter().ExecuteCommand(bang.c_str(), self);
	}
	else
	{
		std::wstring_view bangSz = bang;
		if (bangSz.starts_with(L'!'))
		{
			bangSz.remove_prefix(1);
			std::vector<std::wstring> args;
			for (int i = 3; i <= top; ++i)
			{
				std::wstring tmpSz = LuaHelper::ToWideString(i);
				parser.ReplaceVariables(tmpSz);
				args.push_back(tmpSz);
			}

			GetRainmeter().ExecuteBang(bangSz, args, self);
		}
	}

	return 0;
}

static int GetMeter(lua_State* L)
{
	DECLARE_SELF(L)
	const auto meterName = LuaHelper::ToWide(2);

	Meter* meter = self->GetMeter(meterName);
	if (meter)
	{
		*(Meter**)lua_newuserdata(L, sizeof(Meter*)) = meter;
		lua_getglobal(L, "Meter");
		lua_setmetatable(L, -2);
	}
	else
	{
		lua_pushnil(L);
	}

	return 1;
}

static int GetMeasure(lua_State* L)
{
	DECLARE_SELF(L)
	const auto measureName = LuaHelper::ToWide(2);

	Measure* measure = self->GetMeasure(measureName);
	if (measure)
	{
		*(Measure**)lua_newuserdata(L, sizeof(Measure*)) = measure;
		lua_getglobal(L, "Measure");
		lua_setmetatable(L, -2);
	}
	else
	{
		lua_pushnil(L);
	}

	return 1;
}

static int SetOption(lua_State* L)
{
	DECLARE_SELF(L)
	const auto section = LuaHelper::ToWide(2);
	return LuaSection::SetOption(L, self, section.c_str(), 3);
}

static int SetOptionGroup(lua_State* L)
{
	DECLARE_SELF(L)
	const auto group = LuaHelper::ToWide(2);
	return LuaSection::SetOption(L, self, group.c_str(), 3, true);
}

static int GetVariable(lua_State* L)
{
	DECLARE_SELF(L)

	const auto name = LuaHelper::ToWide(2);
	std::wstring value;
	if (self->GetParser().GetVariable(name, value))
	{
		LuaHelper::PushWide(value);
	}
	else if (lua_gettop(L) >= 3)
	{
		lua_pushvalue(L, 3);
	}
	else
	{
		lua_pushnil(L);
	}

	return 1;
}

static int SetVariable(lua_State* L)
{
	DECLARE_SELF(L)
	if (lua_istable(L, 2))
	{
		const int top = lua_gettop(L);
		for (int i = 2; i <= top; ++i)
		{
			if (!lua_istable(L, i))
			{
				continue;
			}

			lua_rawgeti(L, i, 1);
			lua_rawgeti(L, i, 2);
			if (lua_type(L, -2) == LUA_TSTRING && lua_isstring(L, -1))
			{
				const auto value = LuaHelper::ToWide(-1);
				const auto name = LuaHelper::ToWide(-2);
				self->SetVariable(name, value);
			}

			lua_pop(L, 2);
		}
	}
	else if (lua_isstring(L, 2) && lua_isstring(L, 3))
	{
		const auto name = LuaHelper::ToWide(2);
		const auto value = LuaHelper::ToWide(3);
		self->SetVariable(name, value);
	}

	return 0;
}

static int ReplaceVariables(lua_State* L)
{
	DECLARE_SELF(L)
	std::wstring strTmp = LuaHelper::ToWideString(2);

	self->GetParser().ReplaceVariables(strTmp);
	self->GetParser().ReplaceMeasures(strTmp);
	LuaHelper::PushWide(strTmp);

	return 1;
}

static int ParseFormula(lua_State* L)
{
	DECLARE_SELF(L)
	const auto strTmp = LuaHelper::ToWide(2);

	double result;
	if (!self->GetParser().ParseFormula(strTmp, &result))
	{
		result = lua_tonumber(L, 2);
	}

	lua_pushnumber(L, result);

	return 1;
}

static int MoveWindow(lua_State* L)
{
	DECLARE_SELF(L)
	int x = (int)lua_tonumber(L, 2);
	int y = (int)lua_tonumber(L, 3);
	self->MoveWindow(x, y, SkinPositionSpace::Virtualized);

	return 0;
}

static int FadeWindow(lua_State* L)
{
	DECLARE_SELF(L)
	int from = (int)lua_tonumber(L, 2);
	int to = (int)lua_tonumber(L, 3);
	self->FadeWindow(from, to);

	return 0;
}

static int GetW(lua_State* L)
{
	DECLARE_SELF(L)
	lua_pushnumber(L, self->GetW());

	return 1;
}

static int GetH(lua_State* L)
{
	DECLARE_SELF(L)
	lua_pushnumber(L, self->GetH());

	return 1;
}

static int GetX(lua_State* L)
{
	DECLARE_SELF(L)
	lua_pushnumber(L, self->GetPositionAsVirtualized().x);

	return 1;
}

static int GetY(lua_State* L)
{
	DECLARE_SELF(L)
	lua_pushnumber(L, self->GetPositionAsVirtualized().y);

	return 1;
}

static int MakePathAbsolute(lua_State* L)
{
	DECLARE_SELF(L)
	std::wstring path = LuaHelper::ToWideString(2);
	self->MakePathAbsolute(path);
	LuaHelper::PushWide(path);

	return 1;
}

static int ReadTextFile(lua_State* L)
{
	DECLARE_SELF(L)
	std::wstring path = LuaHelper::ToWideString(2);
	self->MakePathAbsolute(path);

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

static int WriteTextFile(lua_State* L)
{
	DECLARE_SELF(L)
	std::wstring path = LuaHelper::ToWideString(2);
	self->MakePathAbsolute(path);

	const auto text = LuaHelper::ToWide(3);

	auto encoding = FileUtil::Encoding::UTF16LE;
	if (lua_isstring(L, 4))
	{
		const auto name = LuaHelper::ToWide(4);
		if (_wcsicmp(name.c_str(), L"ANSI") == 0)
		{
			encoding = FileUtil::Encoding::ANSI;
		}
		else if (_wcsicmp(name.c_str(), L"UTF-8") == 0)
		{
			encoding = FileUtil::Encoding::UTF8;
		}
		else if (_wcsicmp(name.c_str(), L"UTF-16") != 0)
		{
			lua_pushboolean(L, 0);
			LuaHelper::PushWide(fmt::format(L"Unknown encoding: {}", std::wstring_view(name)));
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

void LuaScript::RegisterSkin(lua_State* L)
{
	const luaL_Reg functions[] =
	{
		{ "Bang", Bang },
		{ "GetMeter", GetMeter },
		{ "GetMeasure", GetMeasure },
		{ "SetOption", SetOption },
		{ "SetOptionGroup", SetOptionGroup },
		{ "GetVariable", GetVariable },
		{ "SetVariable", SetVariable },
		{ "ReplaceVariables", ReplaceVariables },
		{ "ParseFormula", ParseFormula },
		{ "MoveWindow", MoveWindow },
		{ "FadeWindow", FadeWindow },
		{ "GetW", GetW },
		{ "GetH", GetH },
		{ "GetX", GetX },
		{ "GetY", GetY },
		{ "MakePathAbsolute", MakePathAbsolute },
		{ "ReadTextFile", ReadTextFile },
		{ "WriteTextFile", WriteTextFile },
		{ nullptr, nullptr }
	};

	// Retaining old MeterWindow name for BWC.
	luaL_register(L, "MeterWindow", functions);
	lua_pushvalue(L, -1);
	lua_setfield(L, -2, "__index");
	lua_pop(L, 1);
}
