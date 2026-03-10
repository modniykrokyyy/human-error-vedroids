//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//
#include "cbase.h"
#include "c_basehlcombatweapon.h"
#include "c_weapon__stubs.h"


// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

class C_Weapon_Turret : public C_BaseHLCombatWeapon 
{
	DECLARE_CLASS( C_Weapon_Turret, C_BaseHLCombatWeapon );
public:
	DECLARE_CLIENTCLASS();
	DECLARE_PREDICTABLE();

	virtual bool		VisibleInWeaponSelection( void ) { return true; }
	virtual bool		CanBeSelected( void ) { return true; }
	virtual bool		HasAnyAmmo( void ) { return true; }
	virtual bool		HasAmmo( void ) {  return true; }
};

STUB_WEAPON_CLASS_IMPLEMENT( weapon_turret, C_Weapon_Turret );

IMPLEMENT_CLIENTCLASS_DT( C_Weapon_Turret, DT_Weapon_Turret, CWeapon_Turret )
END_RECV_TABLE()
