//========= Copyright � 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose:		Molotov grenades
//
// $Workfile:     $
// $Date:         $
//
//-----------------------------------------------------------------------------
// $Log: $
//
// $NoKeywords: $
//=============================================================================//

#ifndef	GRENADEMOLOTOV_H
#define	GRENADEMOLOTOV_H

#include "basegrenade_shared.h"
#include "smoke_trail.h"

class CGrenade_Molotov : public CBaseGrenade
{
public:
	DECLARE_CLASS( CGrenade_Molotov, CBaseGrenade );

	virtual void	Spawn( void );
	virtual void	Precache( void );
	virtual void	Detonate( void );
	//void			CreateFireBlast( void );
	bool			CreateVPhysics();
	void			SetVelocity( const Vector &velocity, const AngularImpulse &angVelocity );
	int				OnTakeDamage( const CTakeDamageInfo &inputInfo );
	void			VPhysicsUpdate( IPhysicsObject *pPhysics );
	void			MolotovTouch( CBaseEntity *pOther );
	void			WaitThink( void );
	void			MolotovThink( void );
	void			FireThink( void );
	void			MolotovRemove( void );

//	virtual	unsigned int	PhysicsSolidMaskForEntity( void ) const { return ( BaseClass::PhysicsSolidMaskForEntity() | CONTENTS_WATER ); }

protected:

	SmokeTrail		*m_pFireTrail;
	bool			m_inSolid;

private:

	DECLARE_DATADESC();
};

#endif	//GRENADEMOLOTOV_H
