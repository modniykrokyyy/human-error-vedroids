//=================== Half-Life 2: Short Stories Mod 2007 =====================//
//
// Purpose:	Takes away induvidual ammo from the player
//			by Au-heppa
//
//=============================================================================//


#include "cbase.h"
#include "hl2_gamerules.h"
#include "hl2_player.h"
#include "ammodef.h"
#include "hl2_shareddefs.h"
#include "globalstate.h"
#include "game.h"


#include "game.h"
#include "engine/IEngineSound.h"
#include "KeyValues.h"
#include "ai_basenpc.h"
#include "AI_Criteria.h"
#include "isaverestore.h"
#include "sceneentity.h"

#include "ai_speech.h"

#include "hlss_metrocopradio.h"

// memdbgon must be the last include file in a .cpp file!!!
#include <tier0/memdbgon.h>

// global pointer to Larson for fast lookups
CEntityClassList<CHLSS_MetrocopRadio> g_MetrocopRadioList;
template <> CHLSS_MetrocopRadio *CEntityClassList<CHLSS_MetrocopRadio>::m_pClassList = NULL;

LINK_ENTITY_TO_CLASS( hlss_metrocopradio, CHLSS_MetrocopRadio );

BEGIN_DATADESC( CHLSS_MetrocopRadio )
	
	DEFINE_KEYFIELD( m_iszSound, FIELD_SOUNDNAME, "message" ),

	DEFINE_INPUTFUNC ( FIELD_STRING, "PlaySound", InputPlaySound ),
	DEFINE_INPUTFUNC ( FIELD_STRING, "StopSound", InputStopSound ),
	DEFINE_OUTPUT( m_OnMessageEnd, "OnMessageEnd" ),

END_DATADESC()

float		CHLSS_MetrocopRadio::m_flLastSoundDuration;
string_t	CHLSS_MetrocopRadio::m_iszLastSound;
int			CHLSS_MetrocopRadio::m_iNumberOfRadios;

//=========================================================
// Returns a pointer to Eloise's entity
//=========================================================
CHLSS_MetrocopRadio *CHLSS_MetrocopRadio::GetMetrocopRadio( void )
{
	CHLSS_MetrocopRadio *pMetrocopRadio = g_MetrocopRadioList.m_pClassList;

	if (!pMetrocopRadio)
	{
		DevMsg("hlss_metrocopradio: No existing metrocop radio, creating new one\n");
		pMetrocopRadio = dynamic_cast<CHLSS_MetrocopRadio*>(CreateEntityByName("hlss_metrocopradio"));
	}

	return pMetrocopRadio;
}

CHLSS_MetrocopRadio::CHLSS_MetrocopRadio( void ) 
{
	m_iNumberOfRadios++;

	g_MetrocopRadioList.Insert(this);
}

CHLSS_MetrocopRadio::~CHLSS_MetrocopRadio( )
{
	m_iNumberOfRadios--;
	if (m_iNumberOfRadios<=0)
	{
		m_flLastSoundDuration = 0;
	}
	m_iszLastSound = NULL_STRING;

	g_MetrocopRadioList.Remove(this);
}

void CHLSS_MetrocopRadio::Precache()
{
	char *szSoundFile = (char *)STRING( m_iszSound );
	if ( m_iszSound != NULL_STRING && strlen( szSoundFile ) > 1 )
	{
		if (*szSoundFile != '!')
		{
			PrecacheScriptSound(szSoundFile);
		}
	}
}

void CHLSS_MetrocopRadio::Spawn()
{
	Precache();

	char *szSoundFile = (char *)STRING( m_iszSound );
	if ( !m_iszSound || strlen( szSoundFile ) < 1 )
	{
		Warning( "Empty %s (%s) at %.2f, %.2f, %.2f\n", GetClassname(), GetDebugName(), GetAbsOrigin().x, GetAbsOrigin().y, GetAbsOrigin().z );
		UTIL_Remove(this);
		return;
	}

    SetSolid( SOLID_NONE );
    SetMoveType( MOVETYPE_NONE );

	SetThink( NULL );
	SetNextThink( TICK_NEVER_THINK );
}


void CHLSS_MetrocopRadio::InputPlaySound( inputdata_t &inputdata )
{
	PlayRadio( m_iszSound, false );
}

void CHLSS_MetrocopRadio::PlayRadio( string_t sample, bool playedByNPC)
{
	CBasePlayer *pPlayer  = AI_GetSinglePlayer();

	float fvol;
	int pitch = PITCH_NORM;

	fvol = 1.0f; //suitvolume.GetFloat();

//	CBaseEntity *pSource = pPlayer;

	if (pPlayer && pPlayer->GetWeapon(0) && pPlayer->IsSuitEquipped()  && 
		(!playedByNPC || m_flLastSoundDuration < gpGlobals->curtime) ) //&& ( force || ( fvol > 0.005 && m_flLastSoundDuration < gpGlobals->curtime ) ) )
	{
		// If friendlies are talking, reduce the volume of the radio
		if ( playedByNPC && !g_AIFriendliesTalkSemaphore.IsAvailable( pPlayer ) )
		{
			fvol *= 0.3;
		}

		if (m_iszLastSound != NULL_STRING )
		{	
			pPlayer->StopSound( (char *)STRING(m_iszLastSound) );
			m_iszLastSound = NULL_STRING;
		}

		SetAbsOrigin( pPlayer->GetAbsOrigin() );

		CPASAttenuationFilter filter( pPlayer );
		filter.MakeReliable();
		filter.UsePredictionRules();

		EmitSound_t ep;
		ep.m_nChannel = CHAN_VOICE; //CHAN_VOICE;
		ep.m_pSoundName = (char *)STRING( m_iszSound );;
		ep.m_flVolume = fvol;
		ep.m_SoundLevel = SNDLVL_180dB; //NORM;
		ep.m_nFlags = SND_SHOULDPAUSE;
		ep.m_nPitch = pitch;
		ep.m_bEmitCloseCaption = true;
		ep.m_nSpeakerEntity = ENTINDEX(pPlayer);

		CBaseEntity::EmitSound( filter, ENTINDEX(pPlayer->GetWeapon(0)), ep );

		m_flLastSoundDuration = pPlayer->GetSoundDuration( (char *)STRING( m_iszSound ), (char *)STRING(pPlayer->GetModelName()) ) + gpGlobals->curtime;
		m_iszLastSound = sample;

		UpdateRadioHud();
	}
}

void CHLSS_MetrocopRadio::InputStopSound( inputdata_t &inputdata )
{
	CBasePlayer *pPlayer  = AI_GetSinglePlayer();
	if (pPlayer && pPlayer->GetWeapon(0) && IDENT_STRINGS(m_iszLastSound, m_iszSound))  //&& ( force || ( fvol > 0.005 && m_flLastSoundDuration < gpGlobals->curtime ) ) )
	{
		CBaseEntity::StopSound( ENTINDEX(pPlayer->GetWeapon(0)), STRING( m_iszLastSound ) );	
		//StopRadio( m_iszLastSound );

		m_flLastSoundDuration = 0;
		UpdateRadioHud();
	}

	m_iszLastSound = NULL_STRING;
}

void CHLSS_MetrocopRadio::OnRestore()
{
	UpdateRadioHud();
}

void CHLSS_MetrocopRadio::FakeRadio(float flTime)
{
	if (m_flLastSoundDuration < flTime)
	{
		m_flLastSoundDuration = flTime;
		UpdateRadioHud();
	}
}

void CHLSS_MetrocopRadio::UpdateRadioHud()
{
	CBasePlayer *pPlayer  = AI_GetSinglePlayer();
	if (pPlayer && pPlayer->IsSuitEquipped())
	{
		CSingleUserRecipientFilter filter(pPlayer);
		UserMessageBegin(filter, "UpdateRadioDuration");
			WRITE_FLOAT(m_flLastSoundDuration);
		MessageEnd();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
/*void CHLSS_MetrocopRadio::UpdateOnRemove( void)
{
	m_flLastSoundDuration = 0;
	UpdateRadioHud();

	// Chain at end to mimic destructor unwind order
	//BaseClass::UpdateOnRemove();
}*/


