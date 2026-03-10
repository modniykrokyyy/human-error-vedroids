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
#include "engine/ivdebugoverlay.h"

#include "iefx.h"
#include "dlight.h"
#include "c_ai_basenpc.h"

// For material proxy
#include "proxyentity.h"
#include "materialsystem/imaterial.h"
#include "materialsystem/imaterialvar.h"
#include "colorcorrectionmgr.h"

#include "ge_screeneffects.h"


// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

ConVar hlss_olivia_light("hlss_olivia_light", "20.0f");
ConVar hlss_olivia_light_front("hlss_olivia_light_front", "2.0f");
ConVar hlss_olivia_light_radius("hlss_olivia_light_radius", "100.0f");
ConVar hlss_olivia_light_radius_front("hlss_olivia_light_radius_front", "100.0f");
//ConVar hlss_olivia_light_distance("hlss_olivia_light_distance", "25");

#define DLIGHT_RADIUS (hlss_olivia_light_radius.GetFloat())
#define DLIGHT_RADIUS_FRONT (hlss_olivia_light_radius_front.GetFloat())
#define DLIGHT_MINLIGHT (hlss_olivia_light.GetFloat()/255.0f)
#define DLIGHT_MINLIGHT_FRONT (hlss_olivia_light_front.GetFloat()/255.0f)
#define DLIGHT_DISTANCE 25.0f //(hlss_olivia_light_distance.GetFloat())
#define DLIGHT_VECTOR_OFFSET Vector(0,0,20)

class C_NPC_Olivia : public C_AI_BaseNPC
{
	DECLARE_CLASS( C_NPC_Olivia, C_AI_BaseNPC );
	DECLARE_CLIENTCLASS();

public:

	C_NPC_Olivia();
	~C_NPC_Olivia();

	virtual void	ClientThink( void );
	virtual void	OnDataChanged( DataUpdateType_t updateType );
	virtual void	OnRestore( void );
	virtual void	UpdateOnRemove( void );
//	virtual void	ReceiveMessage( int classID, bf_read &msg );

	void			UpdateParticleEffects();

	bool	m_bDandelions;
	bool	m_bSmoking;
	bool	m_bOliviaLight;
	bool	m_bOliviaColorCorrection;
	int		m_nSmokeAttachment;
	int		m_nEyesAttachment;
	float	m_flAppearedTime;

	//dlight_t						*m_pDLight;
	dlight_t						*m_pDLight;
	dlight_t						*m_pELight;

private:
	
	CNewParticleEffect				*m_hDandelions;
	CNewParticleEffect				*m_hSmoke;

	ClientCCHandle_t m_CCHandle;

	CEntGlowEffect *m_pEntGlowEffect;
	bool m_bGlowEnabled;
};

IMPLEMENT_CLIENTCLASS_DT( C_NPC_Olivia, DT_NPC_Olivia, CNPC_Olivia )
	RecvPropBool( RECVINFO(m_bDandelions) ),
	RecvPropBool( RECVINFO(m_bSmoking) ),
	RecvPropBool( RECVINFO(m_bOliviaLight) ),
	RecvPropBool( RECVINFO(m_bOliviaColorCorrection) ),
	RecvPropInt( RECVINFO(m_nSmokeAttachment) ),
	RecvPropFloat( RECVINFO(m_flAppearedTime) ),
END_RECV_TABLE()

C_NPC_Olivia::C_NPC_Olivia()
{
	m_bDandelions = false;
	m_bSmoking = false;
	m_hDandelions = NULL;
	m_hSmoke = NULL;
	m_nSmokeAttachment = -1;
	m_nEyesAttachment = -1;
	m_bOliviaLight = false;
	m_bOliviaColorCorrection = false;

	m_pDLight = NULL;
	m_pELight = NULL;

	m_CCHandle = INVALID_CLIENT_CCHANDLE;

	m_pEntGlowEffect = (CEntGlowEffect*)g_pScreenSpaceEffects->GetScreenSpaceEffect("ge_entglow");
	m_bGlowEnabled = false;
}

C_NPC_Olivia::~C_NPC_Olivia()
{
	if (m_CCHandle != INVALID_CLIENT_CCHANDLE)
	{
		g_pColorCorrectionMgr->RemoveColorCorrection( m_CCHandle );
	}
}

void C_NPC_Olivia::ClientThink()
{
	// Don't update if our frame hasn't moved forward (paused)
	if ( gpGlobals->frametime <= 0.0f )
		return;

	C_BasePlayer *pPlayer = UTIL_PlayerByIndex(1);

	//bool m_bOliviaColorCorrection = true;

	float flScale = 0.0f;

	if ( pPlayer && (m_bOliviaColorCorrection || m_bOliviaLight))
	{
		Vector vecForward;
		pPlayer->EyeVectors(&vecForward, NULL, NULL);

		Vector vecPlayer = (WorldSpaceCenter() - pPlayer->WorldSpaceCenter());
		float flDist = VectorNormalize(vecPlayer);
		float flDot = DotProduct(vecPlayer, vecForward);

		flScale = RemapValClamped(flDot, 0.1, 0.9, 0.0f, 1.0f) * RemapValClamped(flDist, 128.0f, 512.0f, 1.0f, 0.0f);
		//flScale *= clamp(gpGlobals->curtime - m_flAppearedTime, 0.0f, 1.0f);
	}

	if (pPlayer && m_bOliviaColorCorrection)
	{
		if ( m_CCHandle == INVALID_CLIENT_CCHANDLE )
		{

			m_CCHandle = g_pColorCorrectionMgr->AddColorCorrection( "correction/olivia.raw" );
			SetNextClientThink( ( m_CCHandle != INVALID_CLIENT_CCHANDLE ) ? CLIENT_THINK_ALWAYS : CLIENT_THINK_NEVER );
		}

		g_pColorCorrectionMgr->SetColorCorrectionWeight( m_CCHandle, flScale );
	}
	else
	{
		if (m_CCHandle != INVALID_CLIENT_CCHANDLE)
		{
			g_pColorCorrectionMgr->RemoveColorCorrection( m_CCHandle );
			m_CCHandle = INVALID_CLIENT_CCHANDLE;
		}
	}

	if (pPlayer && m_bOliviaLight)
	{
		

		Vector vecPlayer = EyePosition() - pPlayer->EyePosition();
		VectorNormalize(vecPlayer);
		vecPlayer.z *= 0.5f;

		Vector effect_origin = DLIGHT_VECTOR_OFFSET + WorldSpaceCenter() + (vecPlayer * DLIGHT_DISTANCE); 

		//debugoverlay->AddBoxOverlay( effect_origin, -Vector(1,1,1), Vector(1,1,1), vec3_angle, 255,0,0, 0, 0.1 );

		/*if (!m_pDLight)
		{
			m_pDLight = effects->CL_AllocDlight( entindex() );
			m_pDLight->die		= FLT_MAX;
		}

		if (m_pDLight)
		{
			m_pDLight->origin	= effect_origin;
			m_pDLight->radius	= DLIGHT_RADIUS; //random->RandomFloat( 245.0f, 256.0f );
			m_pDLight->minlight = DLIGHT_MINLIGHT * flScale;
			m_pDLight->color.r = 255 * flScale;
			m_pDLight->color.g = 255 * flScale;
			m_pDLight->color.b = 255 * flScale;
		}*/

		if (!m_pDLight)
		{
			m_pDLight = effects->CL_AllocDlight( entindex() );
			m_pDLight->die		= FLT_MAX;
		}

		if (m_pDLight)
		{
			m_pDLight->origin	= effect_origin;
			m_pDLight->radius	= DLIGHT_RADIUS;
			m_pDLight->minlight = DLIGHT_MINLIGHT * flScale;
			m_pDLight->color.r = 255 * flScale;
			m_pDLight->color.g = 255 * flScale;
			m_pDLight->color.b = 255 * flScale;
		}

		if (m_nEyesAttachment != -1)
		{
			Vector vecEyes;
			QAngle angEyes;
			GetAttachment(m_nEyesAttachment, vecEyes, angEyes);

			effect_origin = vecEyes - Vector(0,0,4) - (vecPlayer * DLIGHT_DISTANCE); 
		}

		if (!m_pELight)
		{
			m_pELight = effects->CL_AllocElight( entindex() );
			m_pELight->die		= FLT_MAX;
		}

		//debugoverlay->AddBoxOverlay( effect_origin, -Vector(1,1,1), Vector(1,1,1), vec3_angle, 0,255,0, 0, 0.1 );

		if (m_pELight)
		{
			m_pELight->origin	= effect_origin;
			m_pELight->radius	= DLIGHT_RADIUS_FRONT;
			m_pELight->minlight = DLIGHT_MINLIGHT_FRONT * flScale;
			m_pELight->color.r = 255 * flScale;
			m_pELight->color.g = 255 * flScale;
			m_pELight->color.b = 255 * flScale;
		}
	}
	else
	{
		/*if (m_pDLight)
		{
			m_pDLight->die = gpGlobals->curtime;
		}*/

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
	

	if (!m_bOliviaLight && !m_bOliviaColorCorrection)
	{
		SetNextClientThink( CLIENT_THINK_NEVER );
	}

	if (m_bGlowEnabled != m_bOliviaLight)
	{
		if (m_bOliviaLight)
		{
			m_pEntGlowEffect->RegisterEnt( this, Color(255, 255, 196, 64) );
		}
		else
		{
			m_pEntGlowEffect->DeregisterEnt( this );
		}

		m_bGlowEnabled = m_bOliviaLight;
	}

	if (m_bGlowEnabled)
	{
		m_pEntGlowEffect->SetEntGlowScale( this, flScale * 0.1f );
	}

	UpdateParticleEffects();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : updateType - 
//-----------------------------------------------------------------------------
void C_NPC_Olivia::OnDataChanged( DataUpdateType_t updateType )
{
	BaseClass::OnDataChanged( updateType );

	//UpdateParticleEffects();

	if ( updateType == DATA_UPDATE_CREATED )
	{
		m_nEyesAttachment = LookupAttachment("eyes");
	}

	SetNextClientThink( CLIENT_THINK_ALWAYS );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : updateType - 
//-----------------------------------------------------------------------------
void C_NPC_Olivia::OnRestore( void )
{
	BaseClass::OnRestore();

	m_nEyesAttachment = LookupAttachment("eyes");
	
	SetNextClientThink( CLIENT_THINK_ALWAYS );
}


void C_NPC_Olivia::UpdateParticleEffects()
{
	if (m_bDandelions)
	{
		if (!m_hDandelions)
		{

			// Place a beam between the two points //m_pEnt->
			CNewParticleEffect *pDandelions = ParticleProp()->Create( "dandelions_olivia", PATTACH_ABSORIGIN );
			if ( pDandelions )
			{
				m_hDandelions = pDandelions;
			}
		}
	}
	else
	{
		if ( m_hDandelions )
		{
			m_hDandelions->StopEmission();
			m_hDandelions = NULL;
		}
	}

	if (m_bSmoking)
	{
		if (!m_hSmoke)
		{
			if (m_nSmokeAttachment > 0)
			{
				/*Vector	vecOrigin;
				QAngle	vecAngles;

				GetAttachment( m_nSmokeAttachment, vecOrigin, vecAngles );*/

				// Place a beam between the two points //m_pEnt->
				CNewParticleEffect *pSmoke = ParticleProp()->Create( "he_cigarette", PATTACH_POINT_FOLLOW, m_nSmokeAttachment );
				if ( pSmoke )
				{
					//pSmoke->SetControlPoint( 0, vecOrigin );
					m_hSmoke = pSmoke;
				}
			}
		}
	}
	else
	{
		if ( m_hSmoke )
		{
			m_hSmoke->StopEmission();
			m_hSmoke = NULL;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Receive messages from the server
//-----------------------------------------------------------------------------
/*void C_NPC_Olivia::ReceiveMessage( int classID, bf_read &msg )
{
	// Is the message for a sub-class?
	if ( classID != GetClientClass()->m_ClassID )
	{
		BaseClass::ReceiveMessage( classID, msg );
		return;
	}


	int messageType = msg.ReadByte();

	switch( messageType)
	{
	case 0:
		{
			if (!m_hEffect)
			{
				// Find our attachment point
				unsigned char nAttachment = msg.ReadByte();
			
				// Get our attachment position
				Vector vecStart;
				QAngle vecAngles;
				GetAttachment( nAttachment, vecStart, vecAngles );

				// Place a beam between the two points //m_pEnt->
				CNewParticleEffect *pEffect = ParticleProp()->Create( "dandelions_olivia", PATTACH_ABSORIGIN, nAttachment );
				if ( pEffect )
				{
					pEffect->SetControlPoint( 0, vecStart );

					m_hEffect = pEffect;
				}
			}
		}
		break;
	default:
		{
			if ( m_hEffect )
			{
				m_hEffect->StopEmission();
				m_hEffect = NULL;
			}
		}
		break;
	}
}*/


void C_NPC_Olivia::UpdateOnRemove( void )
{
	if ( m_hDandelions )
	{
		m_hDandelions->StopEmission();
		m_hDandelions = NULL;
	}
	if ( m_hSmoke )
	{
		m_hSmoke->StopEmission();
		m_hSmoke = NULL;
	}

	/*if (m_pDLight)
	{
		m_pDLight->die = gpGlobals->curtime;
	}*/

	if (m_pDLight)
	{
		m_pDLight->die = gpGlobals->curtime;
		m_pDLight= NULL;
	}

	if (m_pELight)
	{
		m_pELight->die = gpGlobals->curtime;
		m_pELight = NULL;
	}

	if (m_CCHandle != INVALID_CLIENT_CCHANDLE)
	{
		g_pColorCorrectionMgr->RemoveColorCorrection( m_CCHandle );
		m_CCHandle = INVALID_CLIENT_CCHANDLE;
	}

	BaseClass::UpdateOnRemove();
}


