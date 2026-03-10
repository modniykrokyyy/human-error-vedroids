//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=====================================================================================//

#include "cbase.h"
#include "particles_simple.h"
#include "citadel_effects_shared.h"
#include "particles_attractor.h"
#include "iefx.h"
#include "dlight.h"
#include "clienteffectprecachesystem.h"
#include "c_te_effect_dispatch.h"
#include "fx_quad.h"

#include "c_ai_basenpc.h"

// For material proxy
#include "proxyentity.h"
#include "materialsystem/imaterial.h"
#include "materialsystem/imaterialvar.h"

#include "iviewrender_beams.h"
#include "beam_shared.h"
#include "beamdraw.h"
#include "IEffects.h"
#include "model_types.h"
#include "c_te_effect_dispatch.h"


#include "mantaray_shared.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define NUM_INTERIOR_PARTICLES	8

#define DLIGHT_RADIUS (150.0f)
#define DLIGHT_MINLIGHT (20.0f/255.0f)

extern void DrawHalo( IMaterial* pMaterial, const Vector &source, float scale, float const *color, float flHDRColorScale );

class C_NPC_MantaRay : public C_AI_BaseNPC
{
public:
	DECLARE_CLASS( C_NPC_MantaRay, C_AI_BaseNPC );
	DECLARE_CLIENTCLASS();

	C_NPC_MantaRay();

	virtual void	UpdateOnRemove( void );
	virtual void	OnDataChanged( DataUpdateType_t updateType );
	//virtual void	ReceiveMessage( int classID, bf_read &msg );
	virtual void	ClientThink( void );
	virtual int		DrawModel( int flags );
	virtual RenderGroup_t	GetRenderGroup( void );

public:
	
	//CNewParticleEffect				*m_hEffect;
	//CNetworkVar( int,	m_nGunAttachment );
	//CNetworkVar( int,	m_iLaserTarget );

	int m_nGunAttachment;
	int m_iLaserTarget;

	//CNetworkVar( float, m_flAttackStartedTime );
	//CNetworkVector( m_vecLaserEndPos );

	dlight_t						*m_pDLight;
	dlight_t						*m_pELight;

	float m_flAttackStartedTime;
	Vector m_vecLaserEndPos;

	bool m_bTeleporting;
};

IMPLEMENT_CLIENTCLASS_DT( C_NPC_MantaRay, DT_NPC_MantaRay, CNPC_MantaRay )
	RecvPropFloat( RECVINFO(m_flAttackStartedTime) ),
	RecvPropVector( RECVINFO( m_vecLaserEndPos ) ),
	RecvPropInt( RECVINFO( m_nGunAttachment ) ),
	RecvPropInt( RECVINFO( m_iLaserTarget ) ),
	RecvPropBool( RECVINFO( m_bTeleporting  ) ),
END_RECV_TABLE()


C_NPC_MantaRay::C_NPC_MantaRay()
{
	m_nGunAttachment = -1;
	m_iLaserTarget = -1;
	m_flAttackStartedTime = -1;

	m_pDLight = NULL;
	m_pELight = NULL;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : updateType - 
//-----------------------------------------------------------------------------
void C_NPC_MantaRay::OnDataChanged( DataUpdateType_t updateType )
{
	BaseClass::OnDataChanged( updateType );

	// start thinking if we need to fade.
	if ( m_flAttackStartedTime != -1 )
	{
		SetNextClientThink( CLIENT_THINK_ALWAYS );
	}
	else
	{
		SetNextClientThink( CLIENT_THINK_NEVER );
	}
}

void C_NPC_MantaRay::UpdateOnRemove()
{
	BaseClass::UpdateOnRemove();

	if (m_pDLight)
	{
		m_pDLight->die = gpGlobals->curtime;
		m_pDLight = NULL;
	}

	if (m_pELight)
	{
		m_pELight->die = gpGlobals->curtime;
		m_pELight = NULL;
	}
}


//-----------------------------------------------------------------------------
// Purpose: Randomly adds extra effects
//-----------------------------------------------------------------------------
void C_NPC_MantaRay::ClientThink( void )
{
	//BaseClass::ClientThink();

	if ( IsEffectActive( EF_NODRAW ) )
		return;

	if ( m_nGunAttachment != -1)
	{
		Vector effect_origin;
		QAngle effect_angles;
				
		GetAttachment( m_nGunAttachment, effect_origin, effect_angles );

		if (!m_pDLight)
		{
			m_pDLight = effects->CL_AllocDlight( entindex() );
			m_pDLight->minlight = DLIGHT_MINLIGHT;
			m_pDLight->die		= FLT_MAX;
		}

		if (m_pDLight)
		{
			m_pDLight->origin	= effect_origin;
			m_pDLight->radius	= random->RandomFloat( 245.0f, 256.0f );
		}

		if (!m_pELight)
		{
			m_pELight = effects->CL_AllocElight( entindex() );

			m_pELight->minlight = DLIGHT_MINLIGHT;
			m_pELight->die		= FLT_MAX;
		}

		if (m_pELight)
		{
			m_pELight->origin	= effect_origin;
			m_pELight->radius	= random->RandomFloat( 245.0f, 256.0f );
		}
	}

	// Update our effects
	if ( gpGlobals->frametime != 0.0f && m_nGunAttachment != -1 && 
		m_flAttackStartedTime != 0 && m_flAttackStartedTime + HLSS_MANTARAY_SHOOT_TIME > gpGlobals->curtime )
	{		

		// Inner beams
		BeamInfo_t beamInfo;

		beamInfo.m_vecStart = vec3_origin;
		beamInfo.m_pStartEnt= this;
		beamInfo.m_nStartAttachment = m_nGunAttachment;

		C_BaseEntity *m_pEnt = NULL;
		if (m_iLaserTarget != -1)
		{
			m_pEnt = ClientEntityList().GetBaseEntityFromHandle( ClientEntityList().EntIndexToHandle( m_iLaserTarget ) ); //C_BaseEntity *
		}

		beamInfo.m_pEndEnt	= NULL; //m_pEnt;
		beamInfo.m_nEndAttachment = -1;

		if (m_pEnt)
		{
			m_vecLaserEndPos = m_pEnt->WorldSpaceCenter();
		}

		beamInfo.m_vecEnd = m_vecLaserEndPos;

		float flAimScale = 1.0f - clamp(((gpGlobals->curtime - m_flAttackStartedTime) / HLSS_MANTARAY_AIM_TIME), 0.0f, 1.0f);

		beamInfo.m_pszModelName = HLSS_MANTARAY_LASER_BEAM;
		beamInfo.m_flHaloScale = 0.0f;
		beamInfo.m_flLife = 0.00005f;
		beamInfo.m_flWidth = random->RandomFloat( 4.0f, 6.0f );
		beamInfo.m_flEndWidth = 0;
		beamInfo.m_flFadeLength = 0.0f;
		beamInfo.m_flAmplitude = random->RandomFloat( 4, 6 ) * flAimScale;
		beamInfo.m_flBrightness = 255.0;
		beamInfo.m_flSpeed = 0.0;
		beamInfo.m_nStartFrame = 0.0;
		beamInfo.m_flFrameRate = 1.0f;

		if (m_bTeleporting)
		{
			beamInfo.m_flRed	= 0.0f;;
			beamInfo.m_flGreen	= 255.0f;
			beamInfo.m_flBlue	= 255.0f;
		}
		else
		{
			beamInfo.m_flRed	= 255.0f;;
			beamInfo.m_flGreen	= 128.0f;
			beamInfo.m_flBlue	= 0.0f;
		}

		beamInfo.m_nSegments = 16;
		beamInfo.m_bRenderable = true;
		beamInfo.m_nFlags = 0;
		
		for (int i=0; i<3; i++)
		{
			//DevMsg("creating a beam\n");
			beams->CreateBeamEntPoint( beamInfo );
		}

		if ( m_flAttackStartedTime + HLSS_MANTARAY_AIM_TIME < gpGlobals->curtime )
		{
			Vector	vecOrigin;
			QAngle	vecAngles;

			GetAttachment( m_nGunAttachment, vecOrigin, vecAngles );

			float flShootScale = clamp(((gpGlobals->curtime - HLSS_MANTARAY_AIM_TIME - m_flAttackStartedTime) / HLSS_MANTARAY_TIME_DIFFERENCE), 0.0f, 1.0f);

			Vector vecLight = vecOrigin + ((m_vecLaserEndPos - vecOrigin) * flShootScale);

			BeamInfo_t beamInfo2;

			beamInfo2.m_vecStart = vecLight;
			beamInfo2.m_pStartEnt= NULL;
			beamInfo2.m_nStartAttachment = -1;

			beamInfo2.m_pszModelName = HLSS_MANTARAY_LASER_LIGHT_BEAM;
			beamInfo2.m_flHaloScale = 0.0f;
			beamInfo2.m_flLife = 0.001f;
			beamInfo2.m_flWidth = random->RandomFloat( 2.0f, 4.0f );
			beamInfo2.m_flEndWidth = 0;
			beamInfo2.m_flFadeLength = 0.0f;
			beamInfo2.m_flAmplitude = random->RandomFloat( 16, 32 );
			beamInfo2.m_flBrightness = 255.0;
			beamInfo2.m_flSpeed = 0.0;
			beamInfo2.m_nStartFrame = 0.0;
			beamInfo2.m_flFrameRate = 1.0f;

			if (m_bTeleporting)
			{
				beamInfo2.m_flRed	= 0.0f;;
				beamInfo2.m_flGreen	= 255.0f;
				beamInfo2.m_flBlue	= 255.0f;
			}
			else
			{
				beamInfo2.m_flRed	= 255.0f;;
				beamInfo2.m_flGreen	= 128.0f;
				beamInfo2.m_flBlue	= 0.0f;
			}

			beamInfo2.m_nSegments = 10;
			beamInfo2.m_bRenderable = true;
			beamInfo2.m_nFlags = 0;

			for (int i=0; i<3; i++)
			{
				beamInfo2.m_pEndEnt	= NULL;
				beamInfo2.m_nEndAttachment = -1;
				beamInfo2.m_vecEnd = vecLight + RandomVector( -HLSS_MANTARAY_LIGHTNING, HLSS_MANTARAY_LIGHTNING );

				beams->CreateBeamEntPoint( beamInfo2 );
			}
		}

		//DLIGHT

		if (m_bTeleporting)
		{
			m_pDLight->color.r = 0;
			m_pDLight->color.g = 255;
			m_pDLight->color.b = 255;
		}
		else
		{
			m_pDLight->color.r = 255;
			m_pDLight->color.g = 128;
			m_pDLight->color.b = 0;
		}

		if (m_bTeleporting)
		{
			m_pELight->color.r = 0;
			m_pELight->color.g = 255;
			m_pELight->color.b = 255;
		}
		else
		{
			m_pELight->color.r = 255;
			m_pELight->color.g = 128;
			m_pELight->color.b = 0;
		}
	}
	else if (gpGlobals->frametime != 0)
	{
		m_pDLight->color.r = 64;
		m_pDLight->color.g = 64;
		m_pDLight->color.b = 64;
		
		m_pELight->color.r = 64;
		m_pELight->color.g = 64;
		m_pELight->color.b = 64;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Tells us we're always a translucent entity
//-----------------------------------------------------------------------------
RenderGroup_t C_NPC_MantaRay::GetRenderGroup( void )
{
	return RENDER_GROUP_TWOPASS;
}

//-----------------------------------------------------------------------------
// Purpose: Draws the stunstick model (with extra effects)
//-----------------------------------------------------------------------------
int C_NPC_MantaRay::DrawModel( int flags )
{
	if ( ShouldDraw() == false )
		return 0;

	// Only render these on the transparent pass
	if ( flags & STUDIO_TRANSPARENCY )
	{
		if ( m_nGunAttachment != -1 && m_flAttackStartedTime != 0 && m_flAttackStartedTime + HLSS_MANTARAY_AIM_TIME < gpGlobals->curtime )
		{
			Vector	vecOrigin;
			QAngle	vecAngles;

			GetAttachment( m_nGunAttachment, vecOrigin, vecAngles );

			IMaterial *pMaterial = materials->FindMaterial( HLSS_MANTARAY_LASER_LIGHT, NULL, false );
			CMatRenderContextPtr pRenderContext( materials );
			pRenderContext->Bind( pMaterial );

			Vector vecLight = m_vecLaserEndPos - vecOrigin;

			float flShootScale = clamp(((gpGlobals->curtime - HLSS_MANTARAY_AIM_TIME - m_flAttackStartedTime) / HLSS_MANTARAY_TIME_DIFFERENCE), 0.0f, 1.0f);

			float scale = HLSS_MANTARAY_LIGHT_RADIUS + (flShootScale * HLSS_MANTARAY_LIGHT_RADIUS_EXTRA);
			float color[3];

			if (m_bTeleporting)
			{
				color[0] = 0.0f;
				color[1] = 0.8f;
				color[2] = 0.8f;

				scale *= 1.4f;
			}
			else
			{
				color[0] = 0.8f;
				color[1] = 0.4f;
				color[2] = 0.0f;
			}
		
			DrawHalo( pMaterial, vecOrigin + (vecLight * flShootScale), scale, color );
		}
		return 1;
	}

	return BaseClass::DrawModel( flags );
}
