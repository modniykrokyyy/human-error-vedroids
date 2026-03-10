//========= Copyright � 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose:		Stun Stick- beating stick with a zappy end
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "npcevent.h"
#include "npc_metropolice.h"
#include "weapon_stunstick.h"
#include "IEffects.h"

#include "rumble_shared.h"
#include "physics_prop_ragdoll.h"
#include "RagdollBoogie.h"
#include "gamestats.h"
#include "ammodef.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

ConVar    sk_plr_dmg_stunstick			( "sk_plr_dmg_stunstick","0");
ConVar    sk_plr_dmg_stunstick_nopower	( "sk_plr_dmg_stunstick_nopower","0");
ConVar    sk_npc_dmg_stunstick			( "sk_npc_dmg_stunstick","0");
ConVar	  hlss_stunstick_only_reduce_ammo_when_hit( "hlss_stunstick_only_reduce_ammo_when_hit", "0");

extern ConVar metropolice_move_and_melee;

//-----------------------------------------------------------------------------
// CWeaponStunStick
//-----------------------------------------------------------------------------

IMPLEMENT_SERVERCLASS_ST(CWeaponStunStick, DT_WeaponStunStick)
	SendPropInt( SENDINFO( m_bActive ), 1, SPROP_UNSIGNED ),
	SendPropBool( SENDINFO( m_bInSwing ) ),
END_SEND_TABLE()

#ifndef HL2MP
LINK_ENTITY_TO_CLASS( weapon_stunstick, CWeaponStunStick );
PRECACHE_WEAPON_REGISTER( weapon_stunstick );
#endif

acttable_t CWeaponStunStick::m_acttable[] = 
{
	{ ACT_MELEE_ATTACK1,	ACT_MELEE_ATTACK_SWING,	true },
	{ ACT_IDLE_ANGRY,		ACT_IDLE_ANGRY_MELEE,	true },
};

IMPLEMENT_ACTTABLE(CWeaponStunStick);


BEGIN_DATADESC( CWeaponStunStick )

	DEFINE_FIELD( m_bActive,					FIELD_BOOLEAN ),
	DEFINE_FIELD( m_bInSwing,					FIELD_BOOLEAN ),
	DEFINE_FIELD( m_bHasPower,					FIELD_BOOLEAN ),
	DEFINE_FIELD( m_iStunstickHints,			FIELD_INTEGER ),
	DEFINE_FIELD( m_iStunstickDepletedHints,	FIELD_INTEGER ),

END_DATADESC()



//-----------------------------------------------------------------------------
// Constructor
//-----------------------------------------------------------------------------
CWeaponStunStick::CWeaponStunStick( void )
{
	// HACK:  Don't call SetStunState because this tried to Emit a sound before
	//  any players are connected which is a bug
	m_bActive = false;
	m_bInSwing = false;
	m_bHasPower = true;
	m_iStunstickHints = 0;
	m_iStunstickDepletedHints = 0;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CWeaponStunStick::Spawn()
{
	Precache();


	BaseClass::Spawn();
	AddSolidFlags( FSOLID_NOT_STANDABLE );
}

void CWeaponStunStick::Precache()
{
	BaseClass::Precache();

	PrecacheScriptSound( "Weapon_StunStick.Activate" );
	PrecacheScriptSound( "Weapon_StunStick.Deactivate" );

	PrecacheScriptSound( "Weapon_Crowbar.Melee_Hit" );
	PrecacheScriptSound( "Weapon_Crowbar.Single" );

	PrecacheScriptSound( "ItemBattery.Touch" );

}

//-----------------------------------------------------------------------------
// Purpose: Get the damage amount for the animation we're doing
// Input  : hitActivity - currently played activity
// Output : Damage amount
//-----------------------------------------------------------------------------
float CWeaponStunStick::GetDamageForActivity( Activity hitActivity )
{
	if ( ( GetOwner() != NULL ) && ( GetOwner()->IsPlayer() ) )
	{
		if (m_bHasPower)
			return sk_plr_dmg_stunstick.GetFloat();
		else
			return sk_plr_dmg_stunstick_nopower.GetFloat();
	}
	
	return sk_npc_dmg_stunstick.GetFloat();
}

//-----------------------------------------------------------------------------
// Attempt to lead the target (needed because citizens can't hit manhacks with the crowbar!)
//-----------------------------------------------------------------------------
extern ConVar sk_crowbar_lead_time;

int CWeaponStunStick::WeaponMeleeAttack1Condition( float flDot, float flDist )
{
	// Attempt to lead the target (needed because citizens can't hit manhacks with the crowbar!)
	CAI_BaseNPC *pNPC	= GetOwner()->MyNPCPointer();
	CBaseEntity *pEnemy = pNPC->GetEnemy();
	if (!pEnemy)
		return COND_NONE;

	Vector vecVelocity;
	AngularImpulse angVelocity;
	pEnemy->GetVelocity( &vecVelocity, &angVelocity );

	// Project where the enemy will be in a little while, add some randomness so he doesn't always hit
	float dt = sk_crowbar_lead_time.GetFloat();
	dt += random->RandomFloat( -0.3f, 0.2f );
	if ( dt < 0.0f )
		dt = 0.0f;

	Vector vecExtrapolatedPos;
	VectorMA( pEnemy->WorldSpaceCenter(), dt, vecVelocity, vecExtrapolatedPos );

	Vector vecDelta;
	VectorSubtract( vecExtrapolatedPos, pNPC->WorldSpaceCenter(), vecDelta );

	if ( fabs( vecDelta.z ) > 70 )
	{
		return COND_TOO_FAR_TO_ATTACK;
	}

	Vector vecForward = pNPC->BodyDirection2D( );
	vecDelta.z = 0.0f;
	float flExtrapolatedDot = DotProduct2D( vecDelta.AsVector2D(), vecForward.AsVector2D() );
	if ((flDot < 0.7) && (flExtrapolatedDot < 0.7))
	{
		return COND_NOT_FACING_ATTACK;
	}

	float flExtrapolatedDist = Vector2DNormalize( vecDelta.AsVector2D() );

	if( pEnemy->IsPlayer() )
	{
		//Vector vecDir = pEnemy->GetSmoothedVelocity();
		//float flSpeed = VectorNormalize( vecDir );

		// If player will be in front of me in one-half second, clock his arse.
		Vector vecProjectEnemy = pEnemy->GetAbsOrigin() + (pEnemy->GetAbsVelocity() * 0.35);
		Vector vecProjectMe = GetAbsOrigin();

		if( (vecProjectMe - vecProjectEnemy).Length2D() <= 48.0f )
		{
			return COND_CAN_MELEE_ATTACK1;
		}
	}
/*
	if( metropolice_move_and_melee.GetBool() )
	{
		if( pNPC->IsMoving() )
		{
			flTargetDist *= 1.5f;
		}
	}
*/
	float flTargetDist = 48.0f;
	if ((flDist > flTargetDist) && (flExtrapolatedDist > flTargetDist))
	{
		return COND_TOO_FAR_TO_ATTACK;
	}

	return COND_CAN_MELEE_ATTACK1;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponStunStick::ImpactEffect( trace_t &traceHit )
{
	//Glowing spark effect for hit
	//UTIL_DecalTrace( &m_trLineHit, "PlasmaGlowFade" );
	
	//FIXME: need new decals
	UTIL_ImpactTrace( &traceHit, DMG_CLUB );
}

void CWeaponStunStick::Operator_HandleAnimEvent( animevent_t *pEvent, CBaseCombatCharacter *pOperator )
{
	switch( pEvent->event )
	{
		case EVENT_WEAPON_MELEE_HIT:
		{
			// Trace up or down based on where the enemy is...
			// But only if we're basically facing that direction
			Vector vecDirection;
			AngleVectors( GetAbsAngles(), &vecDirection );

			CBaseEntity *pEnemy = pOperator->MyNPCPointer() ? pOperator->MyNPCPointer()->GetEnemy() : NULL;
			if ( pEnemy )
			{
				Vector vecDelta;
				VectorSubtract( pEnemy->WorldSpaceCenter(), pOperator->Weapon_ShootPosition(), vecDelta );
				VectorNormalize( vecDelta );
				
				Vector2D vecDelta2D = vecDelta.AsVector2D();
				Vector2DNormalize( vecDelta2D );
				if ( DotProduct2D( vecDelta2D, vecDirection.AsVector2D() ) > 0.8f )
				{
					vecDirection = vecDelta;
				}
			}

			Vector vecEnd;
			VectorMA( pOperator->Weapon_ShootPosition(), 32, vecDirection, vecEnd );
			// Stretch the swing box down to catch low level physics objects
			CBaseEntity *pHurt = pOperator->CheckTraceHullAttack( pOperator->Weapon_ShootPosition(), vecEnd, 
				Vector(-16,-16,-40), Vector(16,16,16), GetDamageForActivity( GetActivity() ), DMG_CLUB, 0.5f, false );
			
			// did I hit someone?
			if ( pHurt )
			{
				// play sound
				WeaponSound( MELEE_HIT );

				CBasePlayer *pPlayer = ToBasePlayer( pHurt );

				CNPC_MetroPolice *pCop = dynamic_cast<CNPC_MetroPolice *>(pOperator);
				bool bFlashed = false;

				if ( pCop != NULL && pPlayer != NULL )
				{
					// See if we need to knock out this target
					if ( pCop->ShouldKnockOutTarget( pHurt ) )
					{
						float yawKick = random->RandomFloat( -48, -24 );

						//Kick the player angles
						pPlayer->ViewPunch( QAngle( -16, yawKick, 2 ) );

						color32 white = {255,255,255,255};
						UTIL_ScreenFade( pPlayer, white, 0.2f, 1.0f, FFADE_OUT|FFADE_PURGE|FFADE_STAYOUT );
						bFlashed = true;
						
						pCop->KnockOutTarget( pHurt );

						break;
					}
					else
					{
						// Notify that we've stunned a target
						pCop->StunnedTarget( pHurt );
					}
				}
				
				// Punch angles
				if ( pPlayer != NULL && !(pPlayer->GetFlags() & FL_GODMODE) )
				{
					float yawKick = random->RandomFloat( -48, -24 );

					//Kick the player angles
					pPlayer->ViewPunch( QAngle( -16, yawKick, 2 ) );

					Vector	dir = pHurt->GetAbsOrigin() - GetAbsOrigin();

					// If the player's on my head, don't knock him up
					if ( pPlayer->GetGroundEntity() == pOperator )
					{
						dir = vecDirection;
						dir.z = 0;
					}

					VectorNormalize(dir);

					dir *= 500.0f;

					//If not on ground, then don't make them fly!
					if ( !(pPlayer->GetFlags() & FL_ONGROUND ) )
						 dir.z = 0.0f;

					//Push the target back
					pHurt->ApplyAbsVelocityImpulse( dir );

					if ( !bFlashed )
					{
						color32 red = {128,0,0,128};
						UTIL_ScreenFade( pPlayer, red, 0.5f, 0.1f, FFADE_IN );
					}
					
					// Force the player to drop anyting they were holding
					pPlayer->ForceDropOfCarriedPhysObjects();
				}
				
				// do effect?
			}
			else
			{
				WeaponSound( MELEE_MISS );
			}
		}
		break;
		default:
			BaseClass::Operator_HandleAnimEvent( pEvent, pOperator );
			break;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Sets the state of the stun stick
//-----------------------------------------------------------------------------
void CWeaponStunStick::SetStunState( bool state )
{
	m_bActive = state;

	CBasePlayer *pOwner  = ToBasePlayer( GetOwner() );

	if ( m_bActive )
	{
		//FIXME: START - Move to client-side

		Vector vecAttachment;

		GetAttachment( 1, vecAttachment );
		g_pEffects->Sparks( vecAttachment );

		//FIXME: END - Move to client-side

		if (pOwner && pOwner->GetAmmoCount(m_iPrimaryAmmoType) > 0)
		{
			EmitSound( "Weapon_StunStick.Activate" );
		}
	}
	else if (pOwner && pOwner->GetAmmoCount(m_iPrimaryAmmoType) > 0)
	{
		EmitSound( "Weapon_StunStick.Deactivate" );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CWeaponStunStick::Deploy( void )
{
	SetStunState( true );

	if (m_iStunstickHints < 3) 
	{
		CBasePlayer *pOwner  = ToBasePlayer( GetOwner() );
		if (pOwner && (m_iStunstickHints <= 0 || pOwner->GetAmmoCount(m_iPrimaryAmmoType) <= 3))
		{
			m_iStunstickHints++;
			UTIL_HudHintText( pOwner , "#HLSS_StunstickRecharge" );
		}
	}

	return BaseClass::Deploy();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CWeaponStunStick::Holster( CBaseCombatWeapon *pSwitchingTo )
{
	if ( BaseClass::Holster( pSwitchingTo ) == false )
		return false;

	m_bInSwing = false;

	SetStunState( false );

	return true;
}

void CWeaponStunStick::ItemPostFrame( void )
{
	int activity = GetActivity();

	if ( m_bHasPower &&
		(activity == GetPrimaryAttackActivity() || 
		 activity == GetSecondaryAttackActivity() ||
		 activity == ACT_VM_MISSCENTER ||
		 activity == ACT_VM_MISSCENTER2 ))
	{
		m_bInSwing = true;
	}
	else
	{
		m_bInSwing = false;
	}

	BaseClass::ItemPostFrame();
}

void CWeaponStunStick::PrimaryAttack()
{
	Swing_StunStick();
	BaseClass::PrimaryAttack();
}

void CWeaponStunStick::SecondaryAttack()
{
	CBasePlayer *pOwner  = ToBasePlayer( GetOwner() );
	// Try a ray
	if ( !pOwner )
		return;

	m_flNextSecondaryAttack = gpGlobals->curtime + 0.2f;

	if (DecreamentEnergy( pOwner, true) )
	{
		EmitSound( "ItemBattery.Touch" );

		m_iStunstickHints++;
	}
	else if (pOwner->GetAmmoCount(m_iPrimaryAmmoType) <= 0)
	{
		//if (m_bHasPower && (m_iStunstickDepletedHints < 2))
		if (!m_bHasPower && m_iStunstickDepletedHints < 2)
		{
			m_iStunstickDepletedHints++;
			UTIL_HudHintText( pOwner , "#HLSS_StunstickDepleted" );
		}

		EmitSound("SuitRecharge.Deny");
	}
}

bool CWeaponStunStick::DecreamentEnergy( CBasePlayer *pOwner, bool bTakeAmmo )
{
	int iCount = pOwner->GetAmmoCount(m_iPrimaryAmmoType);

	if ( pOwner->ArmorValue() >= STUNSTICK_POWER_NEEDED && ( !bTakeAmmo || iCount < GetAmmoDef()->MaxCarry( m_iPrimaryAmmoType )))
	{
		pOwner->DecrementArmorValue( STUNSTICK_POWER_NEEDED );

		if (bTakeAmmo)
		{
			pOwner->SetAmmoCount( iCount + 1, m_iPrimaryAmmoType );
		}

		return true;
	}

	return false;
}

void CWeaponStunStick::Swing_StunStick()
{
	//TERO: This used t obe here, moved to Hit so that
	//		you would loose ammo only when hitting an NPC
	CBasePlayer *pOwner  = ToBasePlayer( GetOwner() );
	if ( !pOwner )
		return;

	if (pOwner->GetAmmoCount(m_iPrimaryAmmoType) > 0)
	{
		m_bHasPower = true; 
		//pOwner->RemoveAmmo( 1, m_iPrimaryAmmoType );
	}
	else 
	{
		if (!m_bHasPower && (m_iStunstickHints < 4))
		{
			UTIL_HudHintText( pOwner , "#HLSS_StunstickRecharge" );
		}

		m_bHasPower = false;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &vecVelocity - 
//-----------------------------------------------------------------------------
void CWeaponStunStick::Drop( const Vector &vecVelocity )
{
	m_bInSwing = false;
	SetStunState( false );

	BaseClass::Drop( vecVelocity );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CWeaponStunStick::GetStunState( void )
{
	return m_bActive;
}

//------------------------------------------------------------------------------
// Purpose: Implement impact function
//------------------------------------------------------------------------------
void CWeaponStunStick::Hit( trace_t &traceHit, Activity nHitActivity, bool bIsSecondary )
{
	CBasePlayer *pPlayer = ToBasePlayer( GetOwner() );
	
	//Do view kick
	AddViewKick();

	//Make sound for the AI
	CSoundEnt::InsertSound( SOUND_BULLET_IMPACT, traceHit.endpos, 400, 0.2f, pPlayer );

	// This isn't great, but it's something for when the crowbar hits.
	pPlayer->RumbleEffect( RUMBLE_AR2, 0, RUMBLE_FLAG_RESTART );

	CBaseEntity	*pHitEntity = traceHit.m_pEnt;

	//Apply damage to a hit target
	if ( pHitEntity != NULL )
	{
		if (hlss_stunstick_only_reduce_ammo_when_hit.GetBool())
		{
			if ( pHitEntity->IsNPC() || pHitEntity->IsPlayer() )
			{	
				CBasePlayer *pOwner  = ToBasePlayer( GetOwner() );

				if (pOwner && (pOwner->IRelationType(pHitEntity) != D_LI))
				{
					if (pOwner->GetAmmoCount(m_iPrimaryAmmoType) > 0)
					{
						pOwner->RemoveAmmo( 1, m_iPrimaryAmmoType );
					}
				}
			}
		}
		else
		{
			CBasePlayer *pOwner  = ToBasePlayer( GetOwner() );

			if (pOwner->GetAmmoCount(m_iPrimaryAmmoType) > 0)
			{
				pOwner->RemoveAmmo( 1, m_iPrimaryAmmoType );
			}
		}


		/*if (pHitEntity->MyNPCPointer())
		{
			DevMsg("weapon_stunstick: sum damage before hit %d\n", pHitEntity->MyNPCPointer()->SumDamage());
		}*/

		Vector hitDirection;
		pPlayer->EyeVectors( &hitDirection, NULL, NULL );
		VectorNormalize( hitDirection );

		CTakeDamageInfo info( GetOwner(), GetOwner(), GetDamageForActivity( nHitActivity ), DMG_CLUB );

		if( pPlayer && pHitEntity->IsNPC() )
		{
			// If bonking an NPC, adjust damage.
			info.AdjustPlayerDamageInflictedForSkillLevel();
		}

		CalculateMeleeDamageForce( &info, hitDirection, traceHit.endpos );

		pHitEntity->DispatchTraceAttack( info, hitDirection, &traceHit ); 
		ApplyMultiDamage();

		// Now hit all triggers along the ray that... 
		TraceAttackToTriggers( info, traceHit.startpos, traceHit.endpos, hitDirection );

		if ( ToBaseCombatCharacter( pHitEntity ) )
		{
			gamestats->Event_WeaponHit( pPlayer, !bIsSecondary, GetClassname(), info );
		}

		if ( m_bHasPower && pHitEntity->MyNPCPointer() && pHitEntity->MyNPCPointer()->CanBecomeRagdoll() && 
			 CBaseCombatCharacter::GetDefaultRelationshipDispositionBetweenClasses( CLASS_PLAYER, pHitEntity->Classify() ) != D_LI )
		{
			if ( (pHitEntity->m_iHealth - pHitEntity->MyNPCPointer()->SumDamage()) < 1 )
			{
				

				CBaseEntity *pRagdoll = CreateServerRagdoll( (CBaseAnimating *)pHitEntity, -1, info, COLLISION_GROUP_DEBRIS, true );
				if ( pRagdoll )
				{
					DevMsg("creating ragdoll boogie\n");
					CRagdollBoogie::Create( pRagdoll, 100, gpGlobals->curtime, 2.0f, SF_RAGDOLL_BOOGIE_ELECTRICAL );

					CTakeDamageInfo infoFinalBlow( GetOwner(), GetOwner(), GetDamageForActivity( nHitActivity ), DMG_REMOVENORAGDOLL );
					pHitEntity->AddEFlags( EFL_IS_BEING_LIFTED_BY_BARNACLE );

					if (pHitEntity->m_lifeState == LIFE_ALIVE)
					{
						pHitEntity->MyNPCPointer()->Event_Killed( infoFinalBlow );
					}

					UTIL_Remove( pHitEntity );
				}
			}
		}
		/*else
		{
			CRagdollBoogie::Create( pHitEntity, 100, gpGlobals->curtime, 2.0f, SF_RAGDOLL_BOOGIE_ELECTRICAL );

			if (pHitEntity->IsAlive())
				DevMsg("weapon_stunstick: pHitEntity IsAlive(), health left: %d\n", pHitEntity->m_iHealth);
			else
				DevMsg("weapon_stunstick: pHitEntity not dead yet, health left: %d\n", pHitEntity->m_iHealth);

			if (pHitEntity->MyNPCPointer())
			{
				DevMsg("weapon_stunstick: damagesum %f\n", pHitEntity->MyNPCPointer()->SumDamage());
			}
		}*/

		//CRagdollBoogie::Create( pHitEntity, 100, gpGlobals->curtime, 2.0f, SF_RAGDOLL_BOOGIE_ELECTRICAL );
	}

	// Apply an impact effect
	ImpactEffect( traceHit );

	//m_bHasPower = false;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : iIndex - 
// Output : const char
//-----------------------------------------------------------------------------
const char *CWeaponStunStick::GetShootSound( int iIndex ) const
{
	if (!m_bHasPower)
	{
		// We override this if we're the charged up version
		switch( iIndex )
		{

		case SINGLE:
			return "Weapon_Crowbar.Single";
			break;

		case MELEE_HIT: 
			return "Weapon_Crowbar.Melee_Hit";
			break;

		case MELEE_MISS:
			return "";
			break;

		default:
			break;
		}
	}

	return BaseClass::GetShootSound( iIndex );
}