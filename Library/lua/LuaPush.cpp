#include "../StdAfx.h"
#include "LuaPush.h"

#include "../Litestep.h"

void push_wstring (lua_State* L, void* value, const char* type)
{	
	std::string str2 = std::string( ConvertToAscii((const wchar_t*)value) );
	lua_pushstring(L,str2.c_str());
}


void push_wchar (lua_State* L, void* value, const char* type)
{
	std::string str2 =  ConvertToAscii( (WCHAR*) value );
	lua_pushstring(L,str2.c_str());
}


std::wstring to_wstring (lua_State* L, int arg, void* type)
{
	// We have a static wstring here so we can keep a copy of the string 
	// passed in alive while its being passed around.
	// This isn't exactly safe, but we shouldn't really have to worry as
	// Rainmeter isn't threaded.
	static std::wstring str2 = std::wstring(L"");

	str2 = ConvertToWide( lua_tostring(L,arg) );

	return str2;
}

const wchar_t* to_wchar (lua_State* L, int arg, void* type)
{
	// We have a static wstring here so we can keep a copy of the string 
	// passed in alive while its being passed around.
	// This isn't exactly safe, but we shouldn't really have to worry as
	// Rainmeter isn't threaded.
	static std::wstring str = std::wstring(L"");

	str = ConvertToWide( lua_tostring(L,arg) );
		
	return str.c_str();
}

int is_wstring (lua_State* L, int lo, const char* type, int def, tolua_Error* err)
{
	return is_wchar(L,lo,type,def,err);
}

int is_wchar (lua_State* L, int lo, const char* type, int def, tolua_Error* err)
{
	if (def && lua_gettop(L)<abs(lo))
		return 1;
	if (lua_isnil(L,lo) || lua_isstring(L,lo))
		return 1;

	err->index = lo;
	err->array = 0;
	err->type = type;
	return 0;
}