#include "basehlcombatweapon.h"
#include "soundenvelope.h"

#ifndef HLSS_MINERS_HAT_H
#define HLSS_MINERS_HAT_H
#ifdef _WIN32
#pragma once
#endif

//---------------------
// Flare
//---------------------

class CHLSS_MinersHat : public CBaseAnimating
{
public:
	DECLARE_CLASS( CHLSS_MinersHat, CBaseAnimating );

	CHLSS_MinersHat();

	static CHLSS_MinersHat *Create( Vector vecOrigin, QAngle vecAngles, CBaseEntity *pOwner );

	virtual unsigned int PhysicsSolidMaskForEntity( void ) const;

	void	Spawn( void );
	void	Precache( void );

	void	HatThink( void );

	void	Die(float flTime);

	DECLARE_SERVERCLASS();
	DECLARE_DATADESC();


	CNetworkVar( bool, m_bLight );
	CNetworkVar( bool, m_bFadeOut );
	CNetworkVar( float, m_flFadeOutSpeed );
	CNetworkVar( float, m_flFadeOutTime );
};


#endif // HLSS_MINERS_HAT_H

