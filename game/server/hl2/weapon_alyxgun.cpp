//========= Copyright � 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
//=============================================================================//

#include "cbase.h"

#include "weapon_alyxgun.h"
#include "npcevent.h"
#include "ai_basenpc.h"
#include "globalstate.h"
#include "weapon_flaregun.h"
#include "rumble_shared.h"
#include "gamestats.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

IMPLEMENT_SERVERCLASS_ST(CWeaponAlyxGun, DT_WeaponAlyxGun)
END_SEND_TABLE()

LINK_ENTITY_TO_CLASS( weapon_alyxgun, CWeaponAlyxGun );
PRECACHE_WEAPON_REGISTER(weapon_alyxgun);

BEGIN_DATADESC( CWeaponAlyxGun )
END_DATADESC()

acttable_t	CWeaponAlyxGun::m_acttable[] = 
{
	{ ACT_IDLE,						ACT_IDLE_PISTOL,				true },
	{ ACT_IDLE_ANGRY,				ACT_IDLE_ANGRY_PISTOL,			true },
	{ ACT_RANGE_ATTACK1,			ACT_RANGE_ATTACK_PISTOL,		true },
	{ ACT_RELOAD,					ACT_RELOAD_PISTOL,				true },
	{ ACT_WALK_AIM,					ACT_WALK_AIM_PISTOL,			true },
	{ ACT_RUN_AIM,					ACT_RUN_AIM_PISTOL,				true },
	{ ACT_COVER_LOW,				ACT_COVER_PISTOL_LOW,			true },
	{ ACT_RANGE_AIM_LOW,			ACT_RANGE_AIM_PISTOL_LOW,		true },
	{ ACT_GESTURE_RANGE_ATTACK1,	ACT_GESTURE_RANGE_ATTACK_PISTOL,true },
	{ ACT_RELOAD_LOW,				ACT_RELOAD_PISTOL_LOW,			true },
	{ ACT_RANGE_ATTACK1_LOW,		ACT_RANGE_ATTACK_PISTOL_LOW,	true },
	{ ACT_GESTURE_RELOAD,			ACT_GESTURE_RELOAD_PISTOL,		true },

	// Readiness activities (not aiming)
	{ ACT_IDLE_RELAXED,				ACT_IDLE_PISTOL,				false },//never aims
	{ ACT_IDLE_STIMULATED,			ACT_IDLE_STIMULATED,			false },
	{ ACT_IDLE_AGITATED,			ACT_IDLE_ANGRY_PISTOL,			false },//always aims
	{ ACT_IDLE_STEALTH,				ACT_IDLE_STEALTH_PISTOL,		false },

	{ ACT_WALK_RELAXED,				ACT_WALK,						false },//never aims
	{ ACT_WALK_STIMULATED,			ACT_WALK_STIMULATED,			false },
	{ ACT_WALK_AGITATED,			ACT_WALK_AIM_PISTOL,			false },//always aims
	{ ACT_WALK_STEALTH,				ACT_WALK_STEALTH_PISTOL,		false },

	{ ACT_RUN_RELAXED,				ACT_RUN,						false },//never aims
	{ ACT_RUN_STIMULATED,			ACT_RUN_STIMULATED,				false },
	{ ACT_RUN_AGITATED,				ACT_RUN_AIM_PISTOL,				false },//always aims
	{ ACT_RUN_STEALTH,				ACT_RUN_STEALTH_PISTOL,			false },

	// Readiness activities (aiming)
	{ ACT_IDLE_AIM_RELAXED,			ACT_IDLE_PISTOL,				false },//never aims	
	{ ACT_IDLE_AIM_STIMULATED,		ACT_IDLE_ANGRY_PISTOL,			false },
	{ ACT_IDLE_AIM_AGITATED,		ACT_IDLE_ANGRY_PISTOL,			false },//always aims
	{ ACT_IDLE_AIM_STEALTH,			ACT_IDLE_STEALTH_PISTOL,		false },

	{ ACT_WALK_AIM_RELAXED,			ACT_WALK,						false },//never aims
	{ ACT_WALK_AIM_STIMULATED,		ACT_WALK_AIM_PISTOL,			false },
	{ ACT_WALK_AIM_AGITATED,		ACT_WALK_AIM_PISTOL,			false },//always aims
	{ ACT_WALK_AIM_STEALTH,			ACT_WALK_AIM_STEALTH_PISTOL,	false },//always aims

	{ ACT_RUN_AIM_RELAXED,			ACT_RUN,						false },//never aims
	{ ACT_RUN_AIM_STIMULATED,		ACT_RUN_AIM_PISTOL,				false },
	{ ACT_RUN_AIM_AGITATED,			ACT_RUN_AIM_PISTOL,				false },//always aims
	{ ACT_RUN_AIM_STEALTH,			ACT_RUN_AIM_STEALTH_PISTOL,		false },//always aims
	//End readiness activities

	// Crouch activities
	{ ACT_CROUCHIDLE_STIMULATED,	ACT_CROUCHIDLE_STIMULATED,		false },
	{ ACT_CROUCHIDLE_AIM_STIMULATED,ACT_RANGE_AIM_PISTOL_LOW,		false },//always aims
	{ ACT_CROUCHIDLE_AGITATED,		ACT_RANGE_AIM_PISTOL_LOW,		false },//always aims

	// Readiness translations
	{ ACT_READINESS_RELAXED_TO_STIMULATED, ACT_READINESS_PISTOL_RELAXED_TO_STIMULATED, false },
	{ ACT_READINESS_RELAXED_TO_STIMULATED_WALK, ACT_READINESS_PISTOL_RELAXED_TO_STIMULATED_WALK, false },
	{ ACT_READINESS_AGITATED_TO_STIMULATED, ACT_READINESS_PISTOL_AGITATED_TO_STIMULATED, false },
	{ ACT_READINESS_STIMULATED_TO_RELAXED, ACT_READINESS_PISTOL_STIMULATED_TO_RELAXED, false },


//	{ ACT_ARM,				ACT_ARM_PISTOL,					true },
//	{ ACT_DISARM,			ACT_DISARM_PISTOL,				true },
};

IMPLEMENT_ACTTABLE(CWeaponAlyxGun);

#define TOOCLOSETIMER_OFF	0.0f
#define ALYX_TOOCLOSETIMER	1.0f		// Time an enemy must be tooclose before Alyx is allowed to shoot it.

//=========================================================
CWeaponAlyxGun::CWeaponAlyxGun( )
{
	m_fMinRange1		= 1;
	m_fMaxRange1		= 5000;

	m_flTooCloseTimer	= TOOCLOSETIMER_OFF;

#ifdef HL2_EPISODIC
	m_fMinRange1		= 60;
	m_fMaxRange1		= 2048;
#endif//HL2_EPISODIC

	m_iFireMode = FIREMODE_3RNDBURST;
	m_iSecondaryAmmoType = -1;
}

CWeaponAlyxGun::~CWeaponAlyxGun( )
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponAlyxGun::Precache( void )
{
	BaseClass::Precache();
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CWeaponAlyxGun::Equip( CBaseCombatCharacter *pOwner )
{
	BaseClass::Equip( pOwner );
}

//-----------------------------------------------------------------------------
// Purpose: Try to encourage Alyx not to use her weapon at point blank range,
//			but don't prevent her from defending herself if cornered.
// Input  : flDot - 
//			flDist - 
// Output : int
//-----------------------------------------------------------------------------
int CWeaponAlyxGun::WeaponRangeAttack1Condition( float flDot, float flDist )
{
#ifdef HL2_EPISODIC
	
	if( flDist < m_fMinRange1 )
	{
		// If Alyx is not able to fire because an enemy is too close, start a timer.
		// If the condition persists, allow her to ignore it and defend herself. The idea
		// is to stop Alyx being content to fire point blank at enemies if she's able to move
		// away, without making her defenseless if she's not able to move.
		float flTime;

		if( m_flTooCloseTimer == TOOCLOSETIMER_OFF )
		{
			m_flTooCloseTimer = gpGlobals->curtime;
		}

		flTime = gpGlobals->curtime - m_flTooCloseTimer;

		if( flTime > ALYX_TOOCLOSETIMER )
		{
			// Fake the range to allow Alyx to shoot.
			flDist = m_fMinRange1 + 1.0f;
		}
	}
	else
	{
		m_flTooCloseTimer = TOOCLOSETIMER_OFF;
	}

	int nBaseCondition = BaseClass::WeaponRangeAttack1Condition( flDot, flDist );

	// While in a vehicle, we extend our aiming cone (this relies on COND_NOT_FACING_ATTACK 
	// TODO: This needs to be rolled in at the animation level
	if ( GetOwner()->IsInAVehicle() )
	{
		Vector vecRoughDirection = ( GetOwner()->GetEnemy()->WorldSpaceCenter() - WorldSpaceCenter() );
		Vector vecRight;
		GetVectors( NULL, &vecRight, NULL );
		bool bRightSide = ( DotProduct( vecRoughDirection, vecRight ) > 0.0f );
		float flTargetDot = ( bRightSide ) ? -0.7f : 0.0f;
		
		if ( nBaseCondition == COND_NOT_FACING_ATTACK && flDot >= flTargetDot )
		{
			nBaseCondition = COND_CAN_RANGE_ATTACK1;
		}
	}

	return nBaseCondition;

#else 

	return BaseClass::WeaponRangeAttack1Condition( flDot, flDist );

#endif//HL2_EPISODIC
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : flDot - 
//			flDist - 
// Output : int
//-----------------------------------------------------------------------------
int CWeaponAlyxGun::WeaponRangeAttack2Condition( float flDot, float flDist )
{
	return COND_NONE;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pOperator - 
//-----------------------------------------------------------------------------
void CWeaponAlyxGun::FireNPCPrimaryAttack( CBaseCombatCharacter *pOperator, bool bUseWeaponAngles )
{
  	Vector vecShootOrigin, vecShootDir;
	CAI_BaseNPC *npc = pOperator->MyNPCPointer();
	ASSERT( npc != NULL );

	if ( bUseWeaponAngles )
	{
		QAngle	angShootDir;
		GetAttachment( LookupAttachment( "muzzle" ), vecShootOrigin, angShootDir );
		AngleVectors( angShootDir, &vecShootDir );
	}
	else 
	{
		vecShootOrigin = pOperator->Weapon_ShootPosition();
 		vecShootDir = npc->GetActualShootTrajectory( vecShootOrigin );
	}

	WeaponSound( SINGLE_NPC );

	if( hl2_episodic.GetBool() )
	{
		pOperator->FireBullets( 1, vecShootOrigin, vecShootDir, VECTOR_CONE_PRECALCULATED, MAX_TRACE_LENGTH, m_iPrimaryAmmoType, 1 );
	}
	else
	{
		pOperator->FireBullets( 1, vecShootOrigin, vecShootDir, VECTOR_CONE_PRECALCULATED, MAX_TRACE_LENGTH, m_iPrimaryAmmoType, 2 );
	}

	pOperator->DoMuzzleFlash();

	if( hl2_episodic.GetBool() )
	{
		// Never fire Alyx's last bullet just in case there's an emergency
		// and she needs to be able to shoot without reloading.
		if( m_iClip1 > 1 )
		{
			m_iClip1 = m_iClip1 - 1;
		}
	}
	else
	{
		m_iClip1 = m_iClip1 - 1;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponAlyxGun::Operator_ForceNPCFire( CBaseCombatCharacter *pOperator, bool bSecondary )
{
	// Ensure we have enough rounds in the clip
	m_iClip1++;

	// HACK: We need the gun to fire its muzzle flash
	if ( bSecondary == false )
	{
		SetActivity( ACT_RANGE_ATTACK_PISTOL, 0.0f );
	}

	FireNPCPrimaryAttack( pOperator, true );
}

//-----------------------------------------------------------------------------
void CWeaponAlyxGun::Operator_HandleAnimEvent( animevent_t *pEvent, CBaseCombatCharacter *pOperator )
{
	switch( pEvent->event )
	{
		case EVENT_WEAPON_PISTOL_FIRE:
		{
			FireNPCPrimaryAttack( pOperator, false );
			break;
		}
		
		default:
			BaseClass::Operator_HandleAnimEvent( pEvent, pOperator );
			break;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Whether or not Alyx is in the injured mode
//-----------------------------------------------------------------------------
bool IsAlyxInInjuredMode( void )
{
	if ( hl2_episodic.GetBool() == false )
		return false;

	return ( GlobalEntity_GetState("ep2_alyx_injured") == GLOBAL_ON );
}

//-----------------------------------------------------------------------------
const Vector& CWeaponAlyxGun::GetBulletSpread( void )
{
	static const Vector cone = VECTOR_CONE_2DEGREES;
	static const Vector autoCone = VECTOR_CONE_6DEGREES;

	/*if ( IsAlyxInInjuredMode() )
		return injuredCone;*/

	if (m_iFireMode == FIREMODE_FULLAUTO)
	{
		return autoCone;
	}

	return cone;
}

//-----------------------------------------------------------------------------
float CWeaponAlyxGun::GetMinRestTime( void )
{
	if ( IsAlyxInInjuredMode() )
		return 1.5f;

	return BaseClass::GetMinRestTime();
}

//-----------------------------------------------------------------------------
float CWeaponAlyxGun::GetMaxRestTime( void )
{
	if ( IsAlyxInInjuredMode() )
		return 3.0f;

	return BaseClass::GetMaxRestTime();
}


/*void CWeaponAlyxGun::SecondaryAttack( void )
{
	BaseClass::SecondaryAttack();	

	CBasePlayer *pOwner = ToBasePlayer( GetOwner() );
	
	if ( pOwner == NULL )
		return;

	if ( ( pOwner->GetAmmoCount( m_iSecondaryAmmoType ) <= 0 ) || ( pOwner->GetWaterLevel() == 3 ) )
	{
		return;
	}

	CFlare *pFlare = CFlare::Create( pOwner->Weapon_ShootPosition(), pOwner->EyeAngles(), pOwner, FLARE_DURATION );

	if ( pFlare == NULL )
		return;

	// Can blow up after a short delay (so have time to release mouse button)
	m_flNextSecondaryAttack = gpGlobals->curtime + 1.0f;

	Vector forward;
	pOwner->EyeVectors( &forward );

	pFlare->SetAbsVelocity( forward * 1500 );

	WeaponSound( SPECIAL1 );

	// Decrease ammo
	pOwner->RemoveAmmo( 1, m_iSecondaryAmmoType );
	//pOwner->RumbleEffect( RUMBLE_357, 0, RUMBLE_FLAGS_NONE );

	m_iSecondaryAttacks++;
	gamestats->Event_WeaponFired( pOwner, false, GetClassname() );
}*/

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool CWeaponAlyxGun::Reload( void )
{
	bool fRet;
	float fCacheTime = m_flNextSecondaryAttack;

	fRet = DefaultReload( GetMaxClip1(), GetMaxClip2(), ACT_VM_RELOAD );
	if ( fRet )
	{
		// Undo whatever the reload process has done to our secondary
		// attack timer. We allow you to interrupt reloading to fire
		// a grenade.
		m_flNextSecondaryAttack = GetOwner()->m_flNextAttack = fCacheTime;

		WeaponSound( RELOAD );
	}

	return fRet;
}

//-----------------------------------------------------------------------------
// Purpose: 
//
//
//-----------------------------------------------------------------------------
void CWeaponAlyxGun::SecondaryAttack( void )
{
	// change fire modes.

	switch( m_iFireMode )
	{
	case FIREMODE_FULLAUTO:
		//Msg( "Burst\n" );
		m_iFireMode = FIREMODE_3RNDBURST;
		m_iSecondaryAmmoType = -1;
		WeaponSound(SPECIAL2);
		break;

	case FIREMODE_3RNDBURST:
		//Msg( "Auto\n" );
		m_iFireMode = FIREMODE_FULLAUTO;
		m_iSecondaryAmmoType = -2;
		WeaponSound(SPECIAL1);
		break;
	}
	
	SendWeaponAnim( GetSecondaryAttackActivity() );

	m_flNextSecondaryAttack = GetOwner()->m_flNextAttack = gpGlobals->curtime + 0.5;
	m_flNextPrimaryAttack	= gpGlobals->curtime + 0.5f;

	CBasePlayer *pOwner = ToBasePlayer( GetOwner() );
	if ( pOwner )
	{
		m_iSecondaryAttacks++;
		gamestats->Event_WeaponFired( pOwner, false, GetClassname() );
	}
}

