//========= Copyright � 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: Flaming bottle thrown from the hand
//
// $Workfile:     $
// $Date:         $
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "ai_basenpc.h"
#include "player.h"
#include "ammodef.h"
#include "gamerules.h"
#include "grenade_molotov.h"
#include "grenade_frag.h"
#include "weapon_brickbat.h"
#include "soundent.h"
#include "decals.h"
#include "fire.h"
#include "shake.h"
#include "ndebugoverlay.h"
#include "vstdlib/random.h"
#include "engine/IEngineSound.h"

#include "te_effect_dispatch.h"
#include "particle_parse.h"

#include "gib.h"
#include "fire.h"
#include "EntityFlame.h"
#include "props_shared.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

extern short	g_sModelIndexFireball;

ConVar    sk_plr_dmg_molotov		( "sk_plr_dmg_molotov","60");
ConVar    sk_npc_dmg_molotov		( "sk_npc_dmg_molotov","60");
ConVar    sk_molotov_radius			( "sk_molotov_radius","72");

#define MOLOTOV_EXPLOSION_VOLUME	1024

const float MOLOTOV_COEFFICIENT_OF_RESTITUTION = 0.2f;

#define MOLOTOV_MODEL "models/props_junk/GlassBottle01a.mdl" //"models/Weapons/w_grenade.mdl"
//"models/props_junk/GlassBottle01a.mdl"


BEGIN_DATADESC( CGrenade_Molotov )

	DEFINE_FIELD( m_pFireTrail, FIELD_CLASSPTR ),
	DEFINE_FIELD( m_inSolid,	FIELD_BOOLEAN ),

	// Function Pointers
	DEFINE_THINKFUNC( MolotovThink ),
	DEFINE_THINKFUNC( WaitThink ),
	DEFINE_THINKFUNC(  FireThink ),
	DEFINE_ENTITYFUNC( MolotovTouch ),

END_DATADESC()

LINK_ENTITY_TO_CLASS( grenade_molotov, CGrenade_Molotov );

void CGrenade_Molotov::Spawn( void )
{
	Precache();

	SetModel( MOLOTOV_MODEL );

	SetSize(Vector( -4, -4, -4), Vector(4, 4, 4));
	SetCollisionGroup( COLLISION_GROUP_WEAPON );
	CreateVPhysics();

	/*SetUse( &CBaseGrenade::DetonateUse );
	SetTouch( &CGrenade_Molotov::MolotovTouch );
	SetThink( &CGrenade_Molotov::MolotovThink );*/
	SetThink( &CGrenade_Molotov::WaitThink );

	SetNextThink( gpGlobals->curtime + 0.2f ); //TERO: this should be 0.4

	m_flDamage		= sk_plr_dmg_molotov.GetFloat();
	m_DmgRadius		= sk_molotov_radius.GetFloat();

	m_iHealth		= 1;

	AddSolidFlags( FSOLID_NOT_STANDABLE );

	//SetSequence( 1 );

	m_pFireTrail = SmokeTrail::CreateSmokeTrail();

	if( m_pFireTrail )
	{
		m_pFireTrail->m_SpawnRate			= 48;
		m_pFireTrail->m_ParticleLifetime	= 1.0f;
		
		m_pFireTrail->m_StartColor.Init( 0.2f, 0.2f, 0.2f );
		m_pFireTrail->m_EndColor.Init( 0.0, 0.0, 0.0 );
		
		m_pFireTrail->m_StartSize	= 8;
		m_pFireTrail->m_EndSize		= 32;
		m_pFireTrail->m_SpawnRadius	= 2;
		m_pFireTrail->m_MinSpeed	= 8;
		m_pFireTrail->m_MaxSpeed	= 16;
		m_pFireTrail->m_Opacity		= 0.65f;

		m_pFireTrail->SetLifetime( 20.0f );
		m_pFireTrail->FollowEntity( this, "0" );
	}

	DispatchParticleEffect( "he_molotov", PATTACH_ABSORIGIN_FOLLOW, this );

	BaseClass::Spawn();

	m_takedamage = DAMAGE_EVENTS_ONLY;

	AddSolidFlags( FSOLID_NOT_SOLID );
}

bool CGrenade_Molotov::CreateVPhysics()
{
	// Create the object in the physics system
	VPhysicsInitNormal( SOLID_BBOX, 0, false );
	return true;
}

void CGrenade_Molotov::SetVelocity( const Vector &velocity, const AngularImpulse &angVelocity )
{
	IPhysicsObject *pPhysicsObject = VPhysicsGetObject();
	if ( pPhysicsObject )
	{
		pPhysicsObject->AddVelocity( &velocity, &angVelocity );
	}
}

//-----------------------------------------------------------------------------
// Purpose:
// Input  :
// Output :
//-----------------------------------------------------------------------------
void CGrenade_Molotov::MolotovTouch( CBaseEntity *pOther )
{
	if ( pOther->IsSolidFlagSet(FSOLID_TRIGGER | FSOLID_VOLUME_CONTENTS) )
	{
		return;
	}

	// don't hit the guy that launched this grenade
	if ( pOther == GetThrower() && GetThrower() != NULL )
		return;

	Detonate();
}

//-----------------------------------------------------------------------------
// Purpose: 
//
//
//-----------------------------------------------------------------------------
void CGrenade_Molotov::Detonate( void ) 
{
	m_takedamage = DAMAGE_NO;

	trace_t trace;
	UTIL_TraceLine ( GetAbsOrigin(), GetAbsOrigin() + Vector ( 0, 0, -128 ),  MASK_SOLID_BRUSHONLY, 
		this, COLLISION_GROUP_NONE, &trace);

	// Pull out of the wall a bit
	if ( trace.fraction != 1.0 )
	{
		SetAbsOrigin( trace.endpos + (trace.plane.normal * 10) ); //(m_flDamage - 25) * 0.2) );
	}

	int contents = UTIL_PointContents ( GetAbsOrigin() );

	EmitSound( "Grenade_Molotov.Detonate");

	//Create the gibs
	Vector velocity = vec3_origin;
	AngularImpulse	angVelocity = RandomAngularImpulse( -150, 150 );
	breakablepropparams_t params( GetAbsOrigin(), GetAbsAngles(), velocity, angVelocity ); 
	params.impactEnergyScale = 1.0f;; //m_impactEnergyScale;
	params.defCollisionGroup = COLLISION_GROUP_DEBRIS;
	
	//IPhysicsObject *pPhysics = VPhysicsGetObject();;

	//PropBreakableCreateAll( GetModelIndex(), NULL, params, this, -1, true, true );
	//end create gibs
	
	if ( (contents & MASK_WATER) )
	{
		MolotovRemove();
		return;
	}

	EmitSound( "Grenade_Molotov.Detonate" );

	// Start some fires
	int i;
	QAngle vecTraceAngles;
	Vector vecTraceDir;
	trace_t firetrace;

	int iFiresCreated = 0;

	for( i = 0 ; i < 16 ; i++ )
	{
		// build a little ray
		vecTraceAngles[PITCH]	= random->RandomFloat(45, 135);
		vecTraceAngles[YAW]		= random->RandomFloat(0, 360);
		vecTraceAngles[ROLL]	= 0.0f;

		AngleVectors( vecTraceAngles, &vecTraceDir );

		Vector vecStart, vecEnd;

		vecStart = GetAbsOrigin() + ( trace.plane.normal * 128 );
		vecEnd = vecStart + vecTraceDir * 512;

		UTIL_TraceLine( vecStart, vecEnd, MASK_SOLID_BRUSHONLY, this, COLLISION_GROUP_NONE, &firetrace );

		Vector	ofsDir = ( firetrace.endpos - GetAbsOrigin() );
		float	offset = VectorNormalize( ofsDir );

		if ( offset > 128 )
			offset = 128;

		//Get our scale based on distance
		float scale	 = (0.1f + ( 0.75f * ( 1.0f - ( offset / 128.0f ) ) )) * 64.0f;
		float growth = (0.1f + ( 0.75f * ( offset / 128.0f ) )) * 4.0f;

		if( firetrace.fraction != 1.0 )
		{
			if ( firetrace.m_pEnt && firetrace.m_pEnt->IsNPC() )
			{
				if (FireSystem_StartFire( firetrace.m_pEnt->GetAbsOrigin(), scale, growth, 6, SF_FIRE_START_ON | SF_FIRE_START_FULL | SF_FIRE_SMOKELESS, firetrace.m_pEnt, FIRE_NATURAL))
				{
					iFiresCreated++;
				}
			}
			else if ( firetrace.m_pEnt && firetrace.m_pEnt->VPhysicsGetObject() )
			{			
				if (FireSystem_StartFire( firetrace.m_pEnt->GetAbsOrigin(), scale, growth, 6, SF_FIRE_START_ON | SF_FIRE_START_FULL | SF_FIRE_SMOKELESS, firetrace.m_pEnt, FIRE_NATURAL))
				{
					iFiresCreated++;
				}
			}
			else
			{
				if (FireSystem_StartFire( firetrace.endpos, scale, growth, 6, SF_FIRE_START_ON | SF_FIRE_START_FULL | SF_FIRE_SMOKELESS, NULL, FIRE_NATURAL ))
				{
					iFiresCreated++;
				}
			}
		}
	}
	
	int npc_class = CLASS_NONE;
	if (GetOwnerEntity() && GetOwnerEntity()->MyNPCPointer())
		npc_class = GetOwnerEntity()->MyNPCPointer()->Classify();

	RadiusDamage( CTakeDamageInfo( this, GetOwnerEntity(), m_flDamage, DMG_BURN | DMG_CLUB ), GetAbsOrigin(), m_DmgRadius, npc_class, NULL );
	

	UTIL_DecalTrace( &trace, "Scorch" );

	UTIL_ScreenShake( GetAbsOrigin(), 25.0, 150.0, 1.0, 750, SHAKE_START );
	CSoundEnt::InsertSound ( SOUND_DANGER, GetAbsOrigin(), BASEGRENADE_EXPLOSION_VOLUME, 3.0 );

	

	if ( m_pFireTrail )
	{
		m_pFireTrail->SetLifetime(0.1f);
		m_pFireTrail = NULL;
	}

	if ( iFiresCreated > 0 )
	{
		SetModelName( NULL_STRING );//invisible
		AddSolidFlags( FSOLID_NOT_SOLID );

		SetTouch( NULL );
		SetSolid( SOLID_NONE );
	
		AddEffects( EF_NODRAW );
		SetAbsVelocity( vec3_origin );

		EmitSound("streetwar.fire_medium");
		SetThink( &CGrenade_Molotov::FireThink );
		SetNextThink( gpGlobals->curtime + 6.0f );
	}
	else
	{
		MolotovRemove();
	}
}

void CGrenade_Molotov::MolotovRemove( void )
{
	SetModelName( NULL_STRING );//invisible
	AddSolidFlags( FSOLID_NOT_SOLID );

	SetThink( &CBaseGrenade::SUB_Remove );
	SetTouch( NULL );
	SetSolid( SOLID_NONE );
	SetNextThink( gpGlobals->curtime + 0.1 );
	
	AddEffects( EF_NODRAW );
	SetAbsVelocity( vec3_origin );
}

void CGrenade_Molotov::FireThink( void )
{
	StopSound("streetwar.fire_medium");

	MolotovRemove();
}

void CGrenade_Molotov::WaitThink( void )
{
	RemoveSolidFlags( FSOLID_NOT_SOLID );

	SetUse( &CBaseGrenade::DetonateUse );
	SetTouch( &CGrenade_Molotov::MolotovTouch );
	SetThink( &CGrenade_Molotov::MolotovThink );

	SetNextThink( gpGlobals->curtime + 0.1f );
}

//------------------------------------------------------------------------------
// Purpose :
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CGrenade_Molotov::MolotovThink( void )
{
	// See if I can lose my owner (has dropper moved out of way?)
	// Want do this so owner can throw the brickbat
	if (GetOwnerEntity())
	{
		trace_t tr;
		Vector	vUpABit = GetAbsOrigin();
		vUpABit.z += 5.0;

		CBaseEntity* saveOwner	= GetOwnerEntity();
		SetOwnerEntity( NULL );
		UTIL_TraceEntity( this, GetAbsOrigin(), vUpABit, MASK_SOLID, &tr );
		if ( tr.startsolid || tr.fraction != 1.0 )
		{
			SetOwnerEntity( saveOwner );
		}
	}

	IPhysicsObject *pPhysicsObject = VPhysicsGetObject();
	if ( pPhysicsObject )
	{
		Vector vecVelocity, angImpulse;
		pPhysicsObject->GetVelocity( &vecVelocity, &angImpulse);

		if (vecVelocity.Length() < 120 || vecVelocity == vec3_origin)
		{
				Detonate();
		}
	}

	trace_t trace;
	UTIL_TraceLine ( GetAbsOrigin(), GetAbsOrigin() + Vector ( 0, 0, -3 ),  MASK_SOLID, this, COLLISION_GROUP_NONE, &trace);
	if (trace.fraction != 1.0)
	{
		Detonate();
	}

	SetNextThink( gpGlobals->curtime + 0.1f );
}

void CGrenade_Molotov::Precache( void )
{
	BaseClass::Precache();

	PrecacheModel( MOLOTOV_MODEL );

	PrecacheParticleSystem( "he_molotov" );

	PrecacheScriptSound( "Grenade_Molotov.Detonate" );
	//PrecacheScriptSound( "d1_town.FlameTrapIgnite" );
	PrecacheScriptSound( "streetwar.fire_medium" );
}

int CGrenade_Molotov::OnTakeDamage( const CTakeDamageInfo &inputInfo )
{
	// Manually apply vphysics because BaseCombatCharacter takedamage doesn't call back to CBaseEntity OnTakeDamage
	VPhysicsTakeDamage( inputInfo );

	// Grenades only suffer blast damage and burn damage.
	if( !(inputInfo.GetDamageType() & (DMG_BLAST|DMG_BURN) ) )
		return 0;

	return BaseClass::OnTakeDamage( inputInfo );
}

void CGrenade_Molotov::VPhysicsUpdate( IPhysicsObject *pPhysics )
{
	BaseClass::VPhysicsUpdate( pPhysics );
	Vector vel;
	AngularImpulse angVel;
	pPhysics->GetVelocity( &vel, &angVel );
	
	Vector start = GetAbsOrigin();
	// find all entities that my collision group wouldn't hit, but COLLISION_GROUP_NONE would and bounce off of them as a ray cast
	CTraceFilterCollisionGroupDelta filter( this, GetCollisionGroup(), COLLISION_GROUP_NONE );
	trace_t tr;

	// UNDONE: Hull won't work with hitboxes - hits outer hull.  But the whole point of this test is to hit hitboxes.
	UTIL_TraceLine( start, start + vel * gpGlobals->frametime, CONTENTS_HITBOX|CONTENTS_MONSTER|CONTENTS_SOLID, &filter, &tr );
	if ( tr.startsolid )
	{
		if ( !m_inSolid )
		{
			// UNDONE: Do a better contact solution that uses relative velocity?
			vel *= -MOLOTOV_COEFFICIENT_OF_RESTITUTION; // bounce backwards
			pPhysics->SetVelocity( &vel, NULL );
		}
		m_inSolid = true;
		return;
	}
	m_inSolid = false;
	if ( tr.DidHit() )
	{
		Vector dir = vel;
		VectorNormalize(dir);
		// send a tiny amount of damage so the character will react to getting bonked
		CTakeDamageInfo info( this, GetThrower(), pPhysics->GetMass() * vel, GetAbsOrigin(), 0.1f, DMG_CRUSH );
		tr.m_pEnt->TakeDamage( info );

		// reflect velocity around normal
		vel = -2.0f * tr.plane.normal * DotProduct(vel,tr.plane.normal) + vel;
		
		// absorb 80% in impact
		vel *= MOLOTOV_COEFFICIENT_OF_RESTITUTION;
		angVel *= -0.5f;
		pPhysics->SetVelocity( &vel, &angVel );
	}
}


