////////////////////////////////////////////////////////////////////////////
//	Module 		: xrServer_Objects_ALife_Items_script.cpp
//	Created 	: 19.09.2002
//  Modified 	: 04.06.2003
//	Author		: Dmitriy Iassenev
//	Description : Server items for ALife simulator, script export
////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "xrServer_Objects_ALife_Items.h"
#include "xrServer_script_macroses.h"

using namespace luabind;


void CSE_ALifeInventoryItem::script_register(lua_State* L)
{
    module(L)[class_<CSE_ALifeInventoryItem>("cse_alife_inventory_item")
                  //			.def(		constructor<LPCSTR>())
                  .def_readwrite("item_condition", &CSE_ALifeInventoryItem::m_fCondition)];
}

void CSE_ALifeItem::script_register(lua_State* L)
{
    module(L)[
        //		luabind_class_item2(
        luabind_class_abstract2(CSE_ALifeItem, "cse_alife_item", CSE_ALifeDynamicObjectVisual, CSE_ALifeInventoryItem)];
}

void CSE_ALifeItemTorch::script_register(lua_State* L) { module(L)[luabind_class_item1(CSE_ALifeItemTorch, "cse_alife_item_torch", CSE_ALifeItem)]; }

void CSE_ALifeItemAmmo::script_register(lua_State* L) { module(L)[luabind_class_item1(CSE_ALifeItemAmmo, "cse_alife_item_ammo", CSE_ALifeItem)]; }

void CSE_ALifeItemWeapon::script_register(lua_State* L)
{
    module(L)[luabind_class_item1(CSE_ALifeItemWeapon, "cse_alife_item_weapon", CSE_ALifeItem)
                  .def_readwrite("ammo_current", &CSE_ALifeItemWeapon::a_current)
                  .def_readwrite("ammo_elapsed", &CSE_ALifeItemWeapon::a_elapsed)
                  .def_readwrite("weapon_state", &CSE_ALifeItemWeapon::wpn_state)
                  .def_readwrite("addon_flags", &CSE_ALifeItemWeapon::m_addon_flags)
                  .def_readwrite("ammo_type", &CSE_ALifeItemWeapon::ammo_type)];
}

void CSE_ALifeItemWeaponShotGun::script_register(lua_State* L) { module(L)[luabind_class_item1(CSE_ALifeItemWeaponShotGun, "cse_alife_item_weapon_shotgun", CSE_ALifeItemWeapon)]; }

void CSE_ALifeItemDetector::script_register(lua_State* L) { module(L)[luabind_class_item1(CSE_ALifeItemDetector, "cse_alife_item_detector", CSE_ALifeItem)]; }

void CSE_ALifeItemArtefact::script_register(lua_State* L) { module(L)[luabind_class_item1(CSE_ALifeItemArtefact, "cse_alife_item_artefact", CSE_ALifeItem)]; }
