#include "cbase.h"
#include "player.h"
#include "gamerules.h"
#include "basehlcombatweapon.h"
#include "decals.h"
#include "soundenvelope.h"
#include "IEffects.h"
#include "engine/IEngineSound.h"
#include "hlss_minershat.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"




LINK_ENTITY_TO_CLASS( hlss_miners_hat, CHLSS_MinersHat );

BEGIN_DATADESC( CHLSS_MinersHat )

	DEFINE_FIELD( m_bLight,			FIELD_BOOLEAN ),
	DEFINE_FIELD( m_bFadeOut,		FIELD_BOOLEAN ),
	DEFINE_FIELD( m_flFadeOutSpeed,	FIELD_FLOAT ),
	DEFINE_FIELD( m_flFadeOutTime,	FIELD_FLOAT ),
	
	//Input functions

	// Function Pointers
	DEFINE_FUNCTION( HatThink ),

END_DATADESC()

//Data-tables
IMPLEMENT_SERVERCLASS_ST( CHLSS_MinersHat, DT_MinersHat )
	SendPropInt( SENDINFO( m_bLight ), 1, SPROP_UNSIGNED ),
	SendPropBool( SENDINFO( m_bFadeOut ) ),
	SendPropFloat( SENDINFO( m_flFadeOutSpeed ) ),
	SendPropFloat( SENDINFO( m_flFadeOutTime ) ),
END_SEND_TABLE()


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CHLSS_MinersHat::CHLSS_MinersHat( void )
{
	m_bLight		= true;
	m_bFadeOut		= false;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHLSS_MinersHat::Precache( void )
{
	PrecacheModel("models/props_sstories/minershat.mdl" );
	PrecacheModel("sprites/glow_test02.vmt");
	PrecacheModel("sprites/light_glow03.vmt");
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHLSS_MinersHat::Spawn( void )
{
	Precache();

	SetModel( "models/props_sstories/minershat.mdl" );

	UTIL_SetSize( this, Vector( -2, -2, -2 ), Vector( 2, 2, 2 ) );

	SetSolid( SOLID_BBOX );
	AddSolidFlags( FSOLID_NOT_SOLID );

	SetMoveType( MOVETYPE_NONE );
	SetFriction( 0.6f );
	SetGravity( UTIL_ScaleForGravity( 400 ) );

	AddEffects( EF_NOSHADOW|EF_NORECEIVESHADOW );

	AddFlag( FL_OBJECT );
}



//-----------------------------------------------------------------------------
// Purpose: 
// Input  : vecOrigin - 
//			vecAngles - 
//			*pOwner - 
// Output : CFlare
//-----------------------------------------------------------------------------
CHLSS_MinersHat *CHLSS_MinersHat::Create( Vector vecOrigin, QAngle vecAngles, CBaseEntity *pOwner)
{
	CHLSS_MinersHat *pHat = (CHLSS_MinersHat *) CreateEntityByName( "hlss_miners_hat" );

	if ( pHat == NULL )
		return NULL;

	UTIL_SetOrigin( pHat, vecOrigin );

	pHat->SetLocalAngles( vecAngles );
	pHat->Spawn();
	//pHat->SetThink( &CHLSS_MinersHat::HatThink );

	pHat->RemoveSolidFlags( FSOLID_NOT_SOLID );
	pHat->AddSolidFlags( FSOLID_NOT_STANDABLE );

	pHat->SetMoveType( MOVETYPE_FLYGRAVITY, MOVECOLLIDE_FLY_BOUNCE );

	pHat->SetOwnerEntity( pOwner );

	return pHat;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
unsigned int CHLSS_MinersHat::PhysicsSolidMaskForEntity( void ) const
{
	return MASK_NPCSOLID;
}

void CHLSS_MinersHat::Die(float flTime)
{
	m_bFadeOut = true;
	m_flFadeOutSpeed	= flTime;
	m_flFadeOutTime		= gpGlobals->curtime + m_flFadeOutSpeed;

	SetThink( &CHLSS_MinersHat::HatThink );
	SetNextThink( gpGlobals->curtime + 0.1f );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHLSS_MinersHat::HatThink( void )
{
	if (m_bFadeOut && m_flFadeOutTime + 1.0f < gpGlobals->curtime)
	{
		SetThink( &CHLSS_MinersHat::SUB_Remove );
	}

	//Next update
	SetNextThink( gpGlobals->curtime + 0.1f );
}

