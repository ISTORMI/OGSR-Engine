#include "stdafx.h"
#include "flesh.h"

using namespace luabind;


void CAI_Flesh::script_register(lua_State* L) { module(L)[class_<CAI_Flesh, CGameObject>("CAI_Flesh").def(constructor<>())]; }
