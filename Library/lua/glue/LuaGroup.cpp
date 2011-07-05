#include "../../StdAfx.h"
#include "../LuaManager.h"
#include "../../Group.h"

static int Group_BelongsToGroup(lua_State* L)
{
	CGroup* self = (CGroup*)tolua_tousertype(L, 1, 0);
	const std::wstring group = (const std::wstring)to_wstring(L, 2, 0);
	bool val = self->BelongsToGroup(group);
	tolua_pushboolean(L, val);

	return 1;
}

void LuaManager::RegisterGroup(lua_State* L)
{
	tolua_usertype(L, "CGroup");
	tolua_cclass(L, "CGroup", "CGroup", "", NULL);

	tolua_beginmodule(L, "CGroup");
	tolua_function(L, "BelongsToGroup", Group_BelongsToGroup);
	tolua_endmodule(L);
}
