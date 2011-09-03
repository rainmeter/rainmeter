/*
  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful, 
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/

#include "../StdAfx.h"
#include "LuaManager.h"
#include "../Rainmeter.h"

int LuaManager::c_RefCount = 0;
lua_State* LuaManager::c_State = 0;

void LuaManager::Init()
{
	if (c_State == NULL)
	{
		// Initialize Lua
		c_State = lua_open();

		// Load Lua base libraries
		luaL_openlibs(c_State);

		// Initialize tolua
		tolua_open(c_State);

		// Register custom types and functions
		tolua_module(c_State, NULL, 0);
		tolua_beginmodule(c_State, NULL);
		RegisterGlobal(c_State);
		RegisterMeasure(c_State);
		RegisterMeasure(c_State);
		RegisterMeter(c_State);
		RegisterMeterWindow(c_State);
		RegisterMeterString(c_State);
		tolua_endmodule(c_State);
	}

	++c_RefCount;
}

void LuaManager::CleanUp()
{
	if (c_RefCount > 0)
	{
		--c_RefCount;
	}

	if (c_RefCount == 0 && c_State != NULL)
	{
		lua_close(c_State);
		c_State = NULL;
	}
}

void LuaManager::ReportErrors(lua_State* L)
{
	std::string error = lua_tostring(L, -1);
	lua_pop(L, 1);

	// Get rid of everything up to the filename
	std::string::size_type pos = 4; // Skip the drive
	pos = error.find_first_of(':', pos);
	pos = error.find_last_of('\\', pos);
	if (pos != std::string::npos)
	{
		error.erase(0, ++pos);
	}

	std::wstring str = L"Script: ";
	str += ConvertUTF8ToWide(error.c_str());
	Log(LOG_ERROR, str.c_str());
}

void LuaManager::LuaLog(int nLevel, const char* format, ... )
{
	char* buffer = new char[4096];
	va_list args;
	va_start( args, format );

	_invalid_parameter_handler oldHandler = _set_invalid_parameter_handler(RmNullCRTInvalidParameterHandler);
	_CrtSetReportMode(_CRT_ASSERT, 0);

	errno = 0;
	_vsnprintf_s( buffer, 4096, _TRUNCATE, format, args );
	if (errno != 0)
	{
		_snprintf_s(buffer, 4096, _TRUNCATE, "Script: LuaLog internal error: %s", format);
	}

	_set_invalid_parameter_handler(oldHandler);

	std::wstring str = ConvertUTF8ToWide(buffer);
	Log(nLevel, str.c_str());
	va_end(args);

	delete [] buffer;
}

void LuaManager::PushWide(lua_State* L, const WCHAR* str)
{
	lua_pushstring(L, ConvertToUTF8(str).c_str());
}

std::wstring LuaManager::ToWide(lua_State* L, int narg)
{
	return ConvertUTF8ToWide(lua_tostring(L, narg));
}
