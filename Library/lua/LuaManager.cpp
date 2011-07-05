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
lua_State* LuaManager::c_pState = 0;

void LuaManager::Init()
{
	if (c_pState == NULL)
	{
		// Initialize Lua
		c_pState = lua_open();

		// Load Lua base libraries
		luaL_openlibs(c_pState);

		// Initialize tolua
		tolua_open(c_pState);

		// Register custom types and functions
		RegisterTypes(c_pState);

		tolua_module(c_pState, NULL, 0);
		tolua_beginmodule(c_pState, NULL);
		RegisterGlobal(c_pState);
		RegisterMeasure(c_pState);
		RegisterGroup(c_pState);
		RegisterMeasure(c_pState);
		RegisterMeter(c_pState);
		RegisterMeterWindow(c_pState);
		RegisterRainmeter(c_pState);
		RegisterMeterString(c_pState);
		tolua_endmodule(c_pState);
	}

	++c_RefCount;
}

void LuaManager::CleanUp()
{
	if (c_RefCount > 0)
	{
		--c_RefCount;
	}

	if (c_RefCount == 0 && c_pState != NULL)
	{
		lua_close(c_pState);
		c_pState = NULL;
	}
}

void LuaManager::RegisterTypes(lua_State* L)
{
}

void LuaManager::ReportErrors(lua_State* L)
{
	LuaLog(LOG_ERROR, "Script: %s", lua_tostring(L, -1));
	lua_pop(L, 1);
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

#ifndef _DEBUG
	// Forcing output to the Debug Output window!
	OutputDebugStringA(buffer);
	OutputDebugStringA("\n");
#endif

	std::wstring str = ConvertToWide(buffer);

	LSLog(nLevel, L"Rainmeter", str.c_str());
	va_end(args);

	delete [] buffer;
}