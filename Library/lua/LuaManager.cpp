#include "../StdAfx.h"
#include "LuaManager.h"
#include "../Rainmeter.h"

#include "lua_measure.h"
#include "lua_meter.h"
#include "lua_group.h"
#include "lua_meterwindow.h"
#include "lua_rainmeter.h"
#include "lua_meterstring.h"
#include "lua_meterimage.h"

#include "lua_rainmeter_ext.h"

int LuaManager::c_RefCount = 0;
lua_State* LuaManager::c_pState = 0;

void LuaManager::Init()
{
	if (c_pState == 0)
	{
		// initialize Lua
		c_pState = lua_open();

		//load Lua base libraries
		luaL_openlibs(c_pState);

		tolua_measure_open(c_pState);
		tolua_group_open(c_pState);
		tolua_meter_open(c_pState);
		tolua_meterwindow_open(c_pState);
		tolua_rainmeter_open(c_pState);
		tolua_meter_string_open(c_pState);
		//tolua_meter_image_open(c_pState);

		luaopen_rainmeter_ext(c_pState);
	}

	++c_RefCount;
}

void LuaManager::CleanUp()
{
	if (c_RefCount > 0)
	{
		--c_RefCount;
	}

	if (c_RefCount == 0 && c_pState != 0)
	{
		lua_close(c_pState);
		c_pState = 0;
	}
}

void LuaManager::ReportErrors(lua_State * L)
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