

#include "cbase.h"
#include "basehlcombatweapon.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

class CWeaponToothbrush : public CBaseHLCombatWeapon
{
	DECLARE_CLASS( CWeaponToothbrush, CBaseHLCombatWeapon );
public:

	DECLARE_SERVERCLASS();
	DECLARE_DATADESC();	
	DECLARE_ACTTABLE();

	void ItemPostFrame( void );
	void Drop( const Vector &vecVelocity );
};

IMPLEMENT_SERVERCLASS_ST(CWeaponToothbrush, DT_WeaponToothBrush)
END_SEND_TABLE()

BEGIN_DATADESC( CWeaponToothbrush )
END_DATADESC()

LINK_ENTITY_TO_CLASS( weapon_toothbrush, CWeaponToothbrush );
PRECACHE_WEAPON_REGISTER(weapon_toothbrush);

acttable_t	CWeaponToothbrush::m_acttable[] = 
{
	{ ACT_IDLE,						ACT_IDLE,					false },
};
IMPLEMENT_ACTTABLE(CWeaponToothbrush);

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponToothbrush::ItemPostFrame( void )
{
	// Do nothing
}

//-----------------------------------------------------------------------------
// Purpose: Remove the citizen package if it's ever dropped
//-----------------------------------------------------------------------------
void CWeaponToothbrush::Drop( const Vector &vecVelocity )
{
	BaseClass::Drop( vecVelocity );
	UTIL_Remove( this );
}

