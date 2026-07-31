/* Copyright (C) 2026 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#pragma once

struct lua_State;
class Section;

namespace LuaSection {

int GetOption(lua_State* L, Section* section, bool allowMeterStyle = false);
int SetOption(lua_State* L, Section* section);

}  // namespace LuaSection
