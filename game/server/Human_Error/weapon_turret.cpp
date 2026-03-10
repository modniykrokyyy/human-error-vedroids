#include "cbase.h"
#include "npcevent.h"
#include "basegrenade_shared.h"
#include "basehlcombatweapon.h"
#include "in_buttons.h"
#include "npc_turret_floor.h"

#include "Human_Error/hlss_weapon_id.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"



class CWeapon_Turret : public CBaseHLCombatWeapon
{
public:
	DECLARE_CLASS( CWeapon_Turret, CBaseHLCombatWeapon );
	DECLARE_SERVERCLASS();

	virtual const int		HLSS_GetWeaponId() { return HLSS_WEAPON_ID_TURRET; }

	void				Spawn( void );
	void				Precache( void );

	int					CapabilitiesGet( void ) { return bits_CAP_WEAPON_RANGE_ATTACK1; }
	void				PrimaryAttack( void );
	void				SecondaryAttack( void );
	void				WeaponIdle( void );
	
	void				ItemPostFrame( void );	
	void				ItemBusyFrame( void );
	bool				Reload( void );

	bool				SetUpTurret( CBasePlayer *pOwner );
	virtual void		Drop( const Vector &vecVelocity );

	virtual bool		VisibleInWeaponSelection( void ) { return true; }
	virtual bool		CanBeSelected( void ) { return true; }
	virtual bool		HasAnyAmmo( void ) { return true; }
	virtual bool		HasAmmo( void ) {  return true; }

	bool				Deploy( void );
	//bool				Holster( CBaseCombatWeapon *pSwitchingTo = NULL );*/

	//int					m_iTurretHealth;

	DECLARE_ACTTABLE();
	DECLARE_DATADESC();
};

acttable_t	CWeapon_Turret::m_acttable[] = 
{
	{ ACT_RANGE_ATTACK1, ACT_RANGE_ATTACK_SLAM, true },
};
IMPLEMENT_ACTTABLE(CWeapon_Turret);

BEGIN_DATADESC( CWeapon_Turret )
END_DATADESC()

IMPLEMENT_SERVERCLASS_ST(CWeapon_Turret, DT_Weapon_Turret)
END_SEND_TABLE()

LINK_ENTITY_TO_CLASS( weapon_turret, CWeapon_Turret );
PRECACHE_WEAPON_REGISTER( weapon_turret );

void CWeapon_Turret::Precache( void )
{
	BaseClass::Precache();

	UTIL_PrecacheOther( "npc_turret_floor" );

}

bool CWeapon_Turret::Reload( void )
{
	WeaponIdle( );
	return true;
}

//-----------------------------------------------------------------------------
// Purpose:
// Input  :
// Output :
//-----------------------------------------------------------------------------
void CWeapon_Turret::PrimaryAttack( void )
{
	CBasePlayer *pPlayer  = ToBasePlayer( GetOwner() );

	if (SetUpTurret(pPlayer))
	{
		pPlayer->Weapon_Drop( this, NULL, NULL);
		UTIL_Remove( this );
	}
}

void CWeapon_Turret::Drop( const Vector &vecVelocity )
{
	CBasePlayer *pPlayer  = ToBasePlayer( GetOwner() );

	BaseClass::Drop( vecVelocity );

	if (SetUpTurret(pPlayer))
	{
		UTIL_Remove( this );
	}
}

bool CWeapon_Turret::SetUpTurret( CBasePlayer *pOwner )
{
	if (!pOwner)
	{ 
		return false;
	}

	m_flNextPrimaryAttack = gpGlobals->curtime + 0.1f;

	Vector vecStart = pOwner->WorldSpaceCenter();
	Vector forward;
	Vector down = pOwner->GetAbsOrigin() - vecStart;

	pOwner->GetVectors(&forward, NULL, NULL);
	//QAngle angForward = pOwner->GetAbsAngles();
	//AngleVectors(angForward, &forward);

	vecStart = vecStart + (forward * 45.0f);

	trace_t tr;
	UTIL_TraceLine( vecStart, vecStart + down, MASK_NPCSOLID, pOwner, COLLISION_GROUP_NONE, &tr );

	UTIL_TraceHull( tr.endpos + Vector(0,0,1), tr.endpos, Vector(-16,-16,0), Vector(16,16,72), MASK_NPCSOLID, pOwner, COLLISION_GROUP_NONE, &tr );
	
	if (tr.fraction == 1.0)
	{
		CNPC_FloorTurret *pTurret = (CNPC_FloorTurret *)CBaseEntity::Create( "npc_turret_floor", tr.endpos, pOwner->GetAbsAngles(), NULL );

		if (pTurret == NULL)
			return false;
	
		pTurret->AddSpawnFlags( SF_FLOOR_TURRET_AUTOACTIVATE );
		pTurret->AddSpawnFlags( SF_FLOOR_TURRET_CAN_BE_CARRIED );

		bool bUnbreakable = true;

		if (m_iHealth >= 0)
		{
			bUnbreakable = false;
			pTurret->AddSpawnFlags( SF_FLOOR_TURRET_BREAKABLE );
		}

		pTurret->Spawn();
		pTurret->Activate();

		if (bUnbreakable)
		{
			pTurret->m_iHealth = CNPC_FloorTurret::GetBreakableTurretHealth();
			pTurret->m_iMaxHealth = pTurret->m_iHealth;
		}
		else
		{
			pTurret->m_iHealth = m_iHealth;
		}

		pOwner->SetAmmoCount( 0, m_iPrimaryAmmoType );

		return true;
	}

	return false;
}

void CWeapon_Turret::SecondaryAttack( void )
{
}

void CWeapon_Turret::ItemBusyFrame( void )
{
	BaseClass::ItemBusyFrame();
}

void CWeapon_Turret::ItemPostFrame( void )
{
	CBasePlayer *pOwner = ToBasePlayer( GetOwner() );
	if (!pOwner)
	{
		return;
	}

	if ((pOwner->m_nButtons & IN_ATTACK) && (m_flNextPrimaryAttack <= gpGlobals->curtime))
	{
		PrimaryAttack();
	}

	// -----------------------
	//  No buttons down
	// -----------------------
	else 
	{
		//TERO: yes, we update every frame
		if (m_iHealth>=0)
		{
			pOwner->SetAmmoCount( m_iHealth, m_iPrimaryAmmoType );
		}
		else
		{
			pOwner->SetAmmoCount( 100, m_iPrimaryAmmoType );
		}

		WeaponIdle( );
		return;
	}
}

void CWeapon_Turret::Spawn()
{
	BaseClass::Spawn();

	m_iHealth = 100;

	SendWeaponAnim( ACT_VM_IDLE );
}

bool CWeapon_Turret::Deploy( void )
{
	if (BaseClass::Deploy())
	{
		SendWeaponAnim( ACT_VM_DRAW );
		m_flNextPrimaryAttack = 0;
		return true;
	}

	return false;
}

void CWeapon_Turret::WeaponIdle( void )
{
	// Ready to switch animations?
 	if ( HasWeaponIdleTimeElapsed() )
	{
		SendWeaponAnim( ACT_VM_IDLE );
	}
}