//=================== Half-Life 2: Short Stories Mod 2009 =====================//
//
// Purpose:	Pull objects to us
//
//=============================================================================//

#include "cbase.h"
#include "hl2_player.h"
#include "hl2_gamerules.h"
#include "ai_utils.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

class CGravityCenterEntitiesEnum : public CFlaggedEntitiesEnum
{
public:
	CGravityCenterEntitiesEnum( CBaseEntity **pList, int listMax )
	 :	CFlaggedEntitiesEnum( pList, listMax, 0 )
	{
	}

	virtual IterationRetval_t EnumElement( IHandleEntity *pHandleEntity )
	{
		CBaseEntity *pEntity = gEntList.GetBaseEntity( pHandleEntity->GetRefEHandle() );
		if ( pEntity && 
			 pEntity->VPhysicsGetObject() && 
			 pEntity->VPhysicsGetObject()->IsMoveable() )
		{
			return CFlaggedEntitiesEnum::EnumElement( pHandleEntity );
		}
		return ITERATION_CONTINUE;
	}
	
	int m_iMaxMass;
};

class CHLSS_Gravity_Center : public CPointEntity
{
public:
	DECLARE_CLASS( CHLSS_Gravity_Center, CPointEntity );

	virtual void Activate( void );
	void GravityCenterThink( void );
	
	void	InputEnable( inputdata_t &inputdata );
	void	InputDisable( inputdata_t &inputdata );

	DECLARE_DATADESC();

private:
	inline void PushEntity( CBaseEntity *pTarget );

	bool	m_bEnabled;
	float	m_flMagnitude;
	float	m_flInnerRadius;
	float	m_flRadius;
	float	m_flZeroPull;
	float	m_flFallHeight;
};

LINK_ENTITY_TO_CLASS( hlss_gravity_center, CHLSS_Gravity_Center );

BEGIN_DATADESC( CHLSS_Gravity_Center )

	DEFINE_THINKFUNC( GravityCenterThink ),
	
	DEFINE_KEYFIELD( m_bEnabled,		FIELD_BOOLEAN,	"enabled" ),
	DEFINE_KEYFIELD( m_flMagnitude,		FIELD_FLOAT,	"magnitude" ),
	DEFINE_KEYFIELD( m_flInnerRadius,	FIELD_FLOAT,	"inner_radius" ),
	DEFINE_KEYFIELD( m_flRadius,		FIELD_FLOAT,	"radius" ),
	DEFINE_KEYFIELD( m_flZeroPull,		FIELD_FLOAT,	"zero_pull" ),
	DEFINE_KEYFIELD( m_flFallHeight,	FIELD_FLOAT,	"fall_height" ),

	DEFINE_INPUTFUNC( FIELD_VOID, "Enable", InputEnable ),
	DEFINE_INPUTFUNC( FIELD_VOID, "Disable", InputDisable ),

END_DATADESC();

#define GRAVITY_CENTER_DEPTH 256

// Spawnflags
#define SF_GRAVITY_CENTER_NO_FALLOFF			0x0001
#define SF_GRAVITY_CENTER_DROP_UNDER_CENTER		0x0002

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHLSS_Gravity_Center::Activate( void )
{
	if ( m_bEnabled )
	{
		SetThink( &CHLSS_Gravity_Center::GravityCenterThink );
		SetNextThink( gpGlobals->curtime + 0.05f );
	}

	BaseClass::Activate();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pTarget - 
//-----------------------------------------------------------------------------
void CHLSS_Gravity_Center::PushEntity( CBaseEntity *pTarget )
{
	Vector vecPushDir;
	
	vecPushDir = ( pTarget->BodyTarget( GetAbsOrigin(), false ) - GetAbsOrigin() );

	float height = -vecPushDir.z;

	float dist = VectorNormalize( vecPushDir );

	//float flFalloff = ( HasSpawnFlags( SF_GRAVITY_CENTER_NO_FALLOFF ) ) ? 1.0f : RemapValClamped( dist, m_flRadius, m_flInnerRadius, 0.0f, 1.0f );
	
	float flFalloff = 1.0f;

	if ( !HasSpawnFlags( SF_GRAVITY_CENTER_NO_FALLOFF ) )
	{
		if ( dist < m_flZeroPull )
		{
			flFalloff = 0.0f;
		}
		else if ( dist > m_flZeroPull && dist < m_flInnerRadius )
		{
			flFalloff = RemapValClamped( dist, m_flZeroPull, m_flInnerRadius, 0.0f, 1.0f );
		}
		else
		{
			flFalloff = RemapValClamped( dist, m_flRadius, m_flInnerRadius, 0.0f, 1.0f );
		}
	}

	if (HasSpawnFlags( SF_GRAVITY_CENTER_DROP_UNDER_CENTER ) && height > 0 )
	{
		flFalloff *= (1.0f - clamp(height / m_flFallHeight, 0, 1.0f));
	}

	IPhysicsObject *pPhys = pTarget->VPhysicsGetObject();
	if ( pPhys )
	{
		// UNDONE: Assume the velocity is for a 100kg object, scale with mass
		bool bIsNotTouching = true;
			
		Vector vecContact;

		if (pPhys->GetContactPoint( &vecContact, NULL ))
		{
			Vector vecDir = pTarget->BodyTarget( GetAbsOrigin(), false) - vecContact;
			VectorNormalize( vecDir );

			float flDot = DotProduct( vecDir, vecPushDir );

			if (flDot > 0.5)
			{
				/*NDebugOverlay::Line(pTarget->GetAbsOrigin(), pTarget->GetAbsOrigin() + (vecDir * 12.0f), 255, 255, 255, true, 1);
				NDebugOverlay::Line(pTarget->GetAbsOrigin(), pTarget->GetAbsOrigin() + (vecPushDir * 12.0f), 255, 0, 0, true, 1);
				NDebugOverlay::Box( pTarget->BodyTarget( GetAbsOrigin(), false), Vector(10,10,10), Vector(-10,-10,-10), 255, 0, 0, 0, 1 );*/

				pPhys->EnableGravity( false );
				bIsNotTouching = false;

				float speed = 0.0f;
				pPhys->SetDamping( &speed, &speed );
						
				//vecPushDir
				Vector vecVelocity;
				AngularImpulse angImpulse;

				pPhys->GetVelocity( &vecVelocity, &angImpulse );

				float flVelocity = vecVelocity.Length() + (angImpulse.Length() * RemapValClamped( pPhys->GetMass(), 5.0f, 25.0f, 0.0f, 1.0f));

				float flTimeScale = 1.0f - clamp(gpGlobals->frametime * 5.0f, 0.0f, 1.0f);

				DevMsg("velocity %f, scale %f\n", flVelocity, flTimeScale);

				if (flVelocity < 60.0f)
				{
					vecVelocity *= flTimeScale * flTimeScale; 
					angImpulse  *= flTimeScale * flTimeScale;
							
					pPhys->SetVelocityInstantaneous( &vecVelocity, &angImpulse );
					pPhys->Sleep();
				}
				else
				{
					float flMulti = pPhys->GetMass() * 0.6f;
							
					if (flMulti > 25.0f)
					{
						flMulti = 25.0f;
					}

					pPhys->ApplyForceCenter( -m_flMagnitude * flFalloff * flMulti * vecPushDir * pPhys->GetMass() * gpGlobals->frametime );
				}
			}
		}

				
									  //TERO: either be awake or outside the radius
		if (bIsNotTouching && ( !pPhys->IsAsleep() || dist > m_flInnerRadius ))
		{
			if (!pPhys->IsGravityEnabled())
			{
				pPhys->EnableGravity( true );

				float speed = 1.0f;
				pPhys->SetDamping( &speed, &speed );
			}

			pPhys->ApplyForceCenter( -m_flMagnitude * flFalloff * 100.0f * vecPushDir * pPhys->GetMass() * gpGlobals->frametime );
		}

		return;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHLSS_Gravity_Center::GravityCenterThink( void )
{
	CBaseEntity *pEnts[GRAVITY_CENTER_DEPTH];

	CGravityCenterEntitiesEnum gravityEnum( pEnts, GRAVITY_CENTER_DEPTH );


	//CBaseEntity *pEnts[256];
	int numEnts = UTIL_EntitiesInSphere( GetAbsOrigin(), 256, &gravityEnum );

	for ( int i = 0; i < numEnts; i++ )
	{
		// Must be solid
		if ( pEnts[i]->IsSolid() == false )
			continue;

		// Cannot be parented (only push parents)
		if ( pEnts[i]->GetMoveParent() != NULL )
			continue;

		// Must be moveable
		if ( pEnts[i]->GetMoveType() != MOVETYPE_VPHYSICS )
			continue; 

		// Push it along
		PushEntity( pEnts[i] );
	}

	// Set us up for the next think
	SetNextThink( gpGlobals->curtime + 0.05f );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHLSS_Gravity_Center::InputEnable( inputdata_t &inputdata )
{
	m_bEnabled = true;
	SetThink( &CHLSS_Gravity_Center::GravityCenterThink );
	SetNextThink( gpGlobals->curtime + 0.05f );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHLSS_Gravity_Center::InputDisable( inputdata_t &inputdata )
{
	m_bEnabled = false;
	SetThink( NULL );
	SetNextThink( gpGlobals->curtime );


	CBaseEntity *pEnts[GRAVITY_CENTER_DEPTH];

	CGravityCenterEntitiesEnum gravityEnum( pEnts, GRAVITY_CENTER_DEPTH );

	//CBaseEntity *pEnts[256];
	int numEnts = UTIL_EntitiesInSphere( GetAbsOrigin(), 256, &gravityEnum );

	for ( int i = 0; i < numEnts; i++ )
	{
		// Must be solid
		if ( pEnts[i]->IsSolid() == false )
			continue;

		// Cannot be parented (only push parents)
		if ( pEnts[i]->GetMoveParent() != NULL )
			continue;

		// Must be moveable
		if ( pEnts[i]->GetMoveType() != MOVETYPE_VPHYSICS )
			continue; 

		// Push it along
		IPhysicsObject *pPhys = pEnts[i]->VPhysicsGetObject();
		if ( pPhys )
		{
			pPhys->EnableGravity( true );

			float speed = 1.0f;
			pPhys->SetDamping( &speed, &speed );
		}
	}
}

class CHLSS_Gravity_Test : public CPointEntity
{
public:
	DECLARE_CLASS( CHLSS_Gravity_Test, CPointEntity );

	virtual void Activate( void );
	void GravityTestThink( void );
	
	void	InputEnable( inputdata_t &inputdata );
	void	InputDisable( inputdata_t &inputdata );

	DECLARE_DATADESC();

private:

	bool	m_bEnabled;
};

LINK_ENTITY_TO_CLASS( hlss_gravity_test, CHLSS_Gravity_Test );

BEGIN_DATADESC( CHLSS_Gravity_Test )

	DEFINE_THINKFUNC( GravityTestThink ),
	
	DEFINE_KEYFIELD( m_bEnabled,		FIELD_BOOLEAN,	"enabled" ),

	DEFINE_INPUTFUNC( FIELD_VOID, "Enable", InputEnable ),
	DEFINE_INPUTFUNC( FIELD_VOID, "Disable", InputDisable ),

END_DATADESC();


//----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHLSS_Gravity_Test::Activate( void )
{
	if ( m_bEnabled )
	{
		SetThink( &CHLSS_Gravity_Test::GravityTestThink );
		SetNextThink( gpGlobals->curtime + 0.05f );
	}

	BaseClass::Activate();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHLSS_Gravity_Test::InputEnable( inputdata_t &inputdata )
{
	m_bEnabled = true;
	SetThink( &CHLSS_Gravity_Test::GravityTestThink );
	SetNextThink( gpGlobals->curtime + 0.05f );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHLSS_Gravity_Test::InputDisable( inputdata_t &inputdata )
{
	m_bEnabled = false;
	SetThink( NULL );
	SetNextThink( gpGlobals->curtime );

	Vector forward(0,0,-1);

	physenv->SetGravity(forward);
}

void CHLSS_Gravity_Test::GravityTestThink( void )
{
	CBasePlayer *pPlayer = AI_GetSinglePlayer();

	Vector forward;
	if (pPlayer)
	{
		pPlayer->EyeVectors( &forward, NULL, NULL );

		physenv->SetGravity(forward);
	}

	// Set us up for the next think
	SetNextThink( gpGlobals->curtime + 0.05f );
}
