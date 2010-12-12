/*
** Lua binding: group
** Generated automatically by tolua++-1.0.92 on 11/22/10 21:20:13.
*/

#ifndef __cplusplus
#include "stdlib.h"
#endif

#include "string.h"

#include "tolua++.h"
#include "../LuaManager.h"
/* Exported function */
TOLUA_API int  luaopen_rainmeter_ext (lua_State* tolua_S);

#include "../../MeterBar.h"
#include "../../MeterBitmap.h"
#include "../../MeterButton.h"
#include "../../MeterHistogram.h"
#include "../../MeterImage.h"
#include "../../MeterLine.h"
#include "../../MeterRotator.h"
#include "../../MeterRoundline.h"
#include "../../MeterString.h"

static int AsMeterBar(lua_State* tolua_S)
{
	CMeterBar* meter = (CMeterBar*)  tolua_tousertype(tolua_S,1,0);
    tolua_pushusertype(tolua_S,(void*) meter, "CMeterBar");
	return 1;
}

static int AsMeterBitmap(lua_State* tolua_S)
{
	CMeterBitmap* meter = (CMeterBitmap*)  tolua_tousertype(tolua_S,1,0);
    tolua_pushusertype(tolua_S,(void*) meter, "CMeterBitmap");
	return 1;
}

static int AsMeterButton(lua_State* tolua_S)
{
	CMeterButton* meter = (CMeterButton*)  tolua_tousertype(tolua_S,1,0);
    tolua_pushusertype(tolua_S,(void*) meter, "CMeterButton");
	return 1;
}

static int AsMeterHistogram(lua_State* tolua_S)
{
	CMeterHistogram* meter = (CMeterHistogram*)  tolua_tousertype(tolua_S,1,0);
    tolua_pushusertype(tolua_S,(void*) meter, "CMeterHistogram");
	return 1;
}

static int AsMeterImage(lua_State* tolua_S)
{
	CMeterImage* meter = (CMeterImage*)  tolua_tousertype(tolua_S,1,0);
    tolua_pushusertype(tolua_S,(void*) meter, "CMeterImage");
	return 1;
}

static int AsMeterLine(lua_State* tolua_S)
{
	CMeterLine* meter = (CMeterLine*)  tolua_tousertype(tolua_S,1,0);
    tolua_pushusertype(tolua_S,(void*) meter, "CMeterLine");
	return 1;
}

static int AsMeterRotator(lua_State* tolua_S)
{
	CMeterRotator* meter = (CMeterRotator*)  tolua_tousertype(tolua_S,1,0);
    tolua_pushusertype(tolua_S,(void*) meter, "CMeterRotator");
	return 1;
}

static int AsMeterRoundline(lua_State* tolua_S)
{
	CMeterRoundLine* meter = (CMeterRoundLine*)  tolua_tousertype(tolua_S,1,0);
    tolua_pushusertype(tolua_S,(void*) meter, "CMeterRoundLine");
	return 1;
}

static int AsMeterString(lua_State* tolua_S)
{
	CMeterString* meterString = (CMeterString*)  tolua_tousertype(tolua_S,1,0);
    tolua_pushusertype(tolua_S,(void*) meterString, "CMeterString");
	return 1;
}

static int staticLuaLog(lua_State* tolua_S)
{
	const char* str = tolua_tostring(tolua_S,1,0);
	LuaManager::LuaLog(str);
	return 0;
}


/* list of functions in the module */
static const luaL_reg rainmeter_ext_funcs[] =
{
	{ "LuaLog", staticLuaLog},
	{ "MeterBar", AsMeterBar },
	{ "MeterBitmap", AsMeterBitmap },
	{ "MeterButton", AsMeterButton },
	{ "MeterHistogram", AsMeterHistogram },
	{ "MeterImage", AsMeterImage },
	{ "MeterLine", AsMeterLine },
	{ "MeterRotator", AsMeterRotator },
	{ "MeterRoundline", AsMeterRoundline },
    { "MeterString", AsMeterString },
    { NULL, NULL }
};

TOLUA_API int luaopen_rainmeter_ext (lua_State* L)
{
    luaL_register(L,"TO", rainmeter_ext_funcs);
    return 1;
}


