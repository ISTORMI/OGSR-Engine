#include "stdafx.h"
#include "weaponsvu.h"

CWeaponSVU::CWeaponSVU(void) : CWeaponCustomPistol("SVU") {}

CWeaponSVU::~CWeaponSVU(void) {}

using namespace luabind;


void CWeaponSVU::script_register(lua_State* L) { module(L)[class_<CWeaponSVU, CGameObject>("CWeaponSVU").def(constructor<>())]; }
