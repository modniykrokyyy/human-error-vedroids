#include "cbase.h"
#include "dlight.h"
#include "iefx.h"
#include "iviewrender_beams.h"
#include "beam_shared.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define DLIGHT_RADIUS (100.0f)
#define DLIGHT_MINLIGHT (4.0f/255.0f)

ConVar hlss_miners_hat_elight("hlss_miners_hat_elight", "1");
//ConVar hlss_miners_hat_light_color("hlss_miners_hat_light_color", "128");

#define HLSS_MINERS_HAT_LIGHT_LENGTH 8.0f

class C_MinersHat : public C_BaseAnimating
{
public:
	DECLARE_CLASS( C_MinersHat, C_BaseAnimating );
	DECLARE_CLIENTCLASS();

	C_MinersHat();

	void	OnDataChanged( DataUpdateType_t updateType );
	void	OnRestore();
	void	UpdateOnRemove();
	void	ClientThink();

	bool	m_bLight;
	bool	m_bFadeOut;
	float	m_flFadeOutSpeed;
	float	m_flFadeOutTime;

private:
	void SetupAttachments();

	int		m_iAttachment;
	dlight_t *m_pELight;
};

IMPLEMENT_CLIENTCLASS_DT( C_MinersHat, DT_MinersHat, CHLSS_MinersHat )
	RecvPropInt( RECVINFO( m_bLight ) ),
	RecvPropBool( RECVINFO( m_bFadeOut ) ),
	RecvPropFloat( RECVINFO( m_flFadeOutSpeed ) ),
	RecvPropFloat( RECVINFO( m_flFadeOutTime ) ),
END_RECV_TABLE()

//-----------------------------------------------------------------------------
// Constructor
//-----------------------------------------------------------------------------
C_MinersHat::C_MinersHat()
{
	m_bLight = false;
	m_bFadeOut = false;

	m_iAttachment = -1;

	m_pELight = NULL;
}

void C_MinersHat::OnRestore()
{
	SetupAttachments();

	BaseClass::OnRestore();
}

void C_MinersHat::SetupAttachments()
{
	if (m_iAttachment == -1)
	{
		m_iAttachment = LookupAttachment("light");
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : updateType - 
//-----------------------------------------------------------------------------
void C_MinersHat::OnDataChanged( DataUpdateType_t updateType )
{
	BaseClass::OnDataChanged( updateType );

	SetupAttachments();

	// start thinking if we need to fade.
	if ( m_bLight )
	{
		SetNextClientThink( CLIENT_THINK_ALWAYS );
	}
	else
	{
		SetNextClientThink( CLIENT_THINK_NEVER );
	}
}

void C_MinersHat::UpdateOnRemove()
{
	BaseClass::UpdateOnRemove();

	if (m_pELight)
	{
		m_pELight->die = gpGlobals->curtime;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Randomly adds extra effects
//-----------------------------------------------------------------------------
void C_MinersHat::ClientThink( void )
{
	if ( IsEffectActive( EF_NODRAW ) )
		return;

	if ( gpGlobals->frametime == 0.0f  )
		return;

	if ( m_iAttachment == -1 )
		return;

	Vector effect_origin;
	QAngle effect_angles;

	GetAttachment( m_iAttachment, effect_origin, effect_angles );

	Vector forward;
	AngleVectors(effect_angles, &forward);

	float flScale = 1.0f;

	if (m_bFadeOut)
	{
		flScale = clamp((m_flFadeOutTime - gpGlobals->curtime)/m_flFadeOutSpeed, 0.0f, 1.0f);
	}

	if ( m_bLight && hlss_miners_hat_elight.GetBool())
	{
		if (!m_pELight)
		{

			m_pELight = effects->CL_AllocElight( entindex() );

			m_pELight->minlight = DLIGHT_MINLIGHT;
			m_pELight->die		= FLT_MAX;
		}

		if (m_pELight)
		{
			m_pELight->origin	= effect_origin;
			m_pELight->radius	= 100.0f * flScale;
			m_pELight->color.r = 16.0f; //hlss_miners_hat_light_color.GetFloat();
			m_pELight->color.g = 16.0f; //hlss_miners_hat_light_color.GetFloat();
			m_pELight->color.b = 16.0f; //hlss_miners_hat_light_color.GetFloat();
		}
	}
	else if (m_pELight)
	{
		m_pELight->die = gpGlobals->curtime;
	}

	// Inner beams
	BeamInfo_t beamInfo;

	beamInfo.m_vecStart = vec3_origin;
	beamInfo.m_pStartEnt = this;
	beamInfo.m_nStartAttachment = m_iAttachment;

	beamInfo.m_vecEnd = effect_origin + (forward * HLSS_MINERS_HAT_LIGHT_LENGTH);
	beamInfo.m_pEndEnt = NULL;
	beamInfo.m_nEndAttachment = -1;

	float flColor = 128.0f;

	beamInfo.m_pszModelName = "sprites/glow_test02.vmt";
	beamInfo.m_pszHaloName = "sprites/light_glow03.vmt";
	beamInfo.m_flHaloScale = 6.0f;
	beamInfo.m_flLife = 0.01f;
	beamInfo.m_flWidth = 8.0f; //random->RandomFloat( 1.0f, 2.0f );
	beamInfo.m_flEndWidth = (24.0f * flScale) + 8.0f;
	//beamInfo.m_flFadeLength = HLSS_MINERS_HAT_LIGHT_LENGTH;
	beamInfo.m_flAmplitude = 0.0f; //random->RandomFloat( 16, 32 );
	beamInfo.m_flBrightness = flColor * flScale;
	beamInfo.m_flSpeed = 0.0;
	beamInfo.m_nStartFrame = 0.0;
	beamInfo.m_flFrameRate = 1.0f;

	beamInfo.m_flRed = flColor * flScale;
	beamInfo.m_flGreen = flColor * flScale;
	beamInfo.m_flBlue = flColor * flScale;

	beamInfo.m_nSegments = 0;
	beamInfo.m_bRenderable = true;
	beamInfo.m_nFlags = FBEAM_SHADEOUT|FBEAM_NOTILE;

	beams->CreateBeamEntPoint( beamInfo );
}