// Copyright (c) Rainmeter Team. Source code licensed under GNU GPL v2 (see LICENSE file).

#pragma once

struct lua_State;
class Section;
class Skin;

namespace LuaSection {

int GetOption(lua_State* L, Section* section, bool allowMeterStyle = false);
int SetOption(lua_State* L, Section* section);
int SetOption(lua_State* L, Skin* skin, const wchar_t* sectionName, int optionIndex, bool group = false);

}  // namespace LuaSection
