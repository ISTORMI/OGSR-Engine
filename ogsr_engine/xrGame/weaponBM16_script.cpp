#include "stdafx.h"
#include "WeaponBM16.h"

using namespace luabind;


void CWeaponBM16::script_register(lua_State* L) { module(L)[class_<CWeaponBM16, CGameObject>("CWeaponBM16").def(constructor<>())]; }
