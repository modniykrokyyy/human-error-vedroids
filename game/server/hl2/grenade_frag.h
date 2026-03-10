//========= Copyright � 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef GRENADE_FRAG_H
#define GRENADE_FRAG_H
#pragma once

class CBaseGrenade;
struct edict_t;

CBaseGrenade *Fraggrenade_Create( const Vector &position, const QAngle &angles, const Vector &velocity, const AngularImpulse &angVelocity, CBaseEntity *pOwner, float timer, bool combineSpawned );
bool	Fraggrenade_WasPunted( const CBaseEntity *pEntity );
bool	Fraggrenade_WasCreatedByCombine( const CBaseEntity *pEntity );


// this will hit only things that are in newCollisionGroup, but NOT in collisionGroupAlreadyChecked
class CTraceFilterCollisionGroupDelta : public CTraceFilterEntitiesOnly
{
public:
	// It does have a base, but we'll never network anything below here..
	DECLARE_CLASS_NOBASE( CTraceFilterCollisionGroupDelta );
	
	CTraceFilterCollisionGroupDelta( const IHandleEntity *passentity, int collisionGroupAlreadyChecked, int newCollisionGroup )
		: m_pPassEnt(passentity), m_collisionGroupAlreadyChecked( collisionGroupAlreadyChecked ), m_newCollisionGroup( newCollisionGroup )
	{
	}
	
	virtual bool ShouldHitEntity( IHandleEntity *pHandleEntity, int contentsMask )
	{
		if ( !PassServerEntityFilter( pHandleEntity, m_pPassEnt ) )
			return false;
		CBaseEntity *pEntity = EntityFromEntityHandle( pHandleEntity );

		if ( pEntity )
		{
			if ( g_pGameRules->ShouldCollide( m_collisionGroupAlreadyChecked, pEntity->GetCollisionGroup() ) )
				return false;
			if ( g_pGameRules->ShouldCollide( m_newCollisionGroup, pEntity->GetCollisionGroup() ) )
				return true;
		}

		return false;
	}

protected:
	const IHandleEntity *m_pPassEnt;
	int		m_collisionGroupAlreadyChecked;
	int		m_newCollisionGroup;
};

#endif // GRENADE_FRAG_H
