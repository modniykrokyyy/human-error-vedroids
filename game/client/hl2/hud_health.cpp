//========= Copyright � 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//
//
// Health.cpp
//
// implementation of CHudHealth class
//
#include "cbase.h"
#include "hud.h"
#include "hud_macros.h"
#include "view.h"

#include "iclientmode.h"

#include <KeyValues.h>
#include <vgui/ISurface.h>
#include <vgui/ISystem.h>
#include <vgui_controls/AnimationController.h>

#include <vgui/ILocalize.h>

using namespace vgui;

#include "hudelement.h"
#include "Human_Error/hud_icondisplay.h"

#include "convar.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define INIT_HEALTH -1

//-----------------------------------------------------------------------------
// Purpose: Health panel
//-----------------------------------------------------------------------------
class CHudHealth : public CHudElement, public CHudIconDisplay
{
	DECLARE_CLASS_SIMPLE( CHudHealth, CHudIconDisplay );

	/*~CHudHealth()
	{
		if (m_pBar)
		{
			m_pBar->DeletePanel();
		}
		if (m_pBase)
		{
			m_pBase->DeletePanel();
		}
	}*/

public:
	CHudHealth( const char *pElementName );
	virtual void Init( void );
	virtual void VidInit( void );
	virtual void Reset( void );
	virtual void OnThink();
			void MsgFunc_Damage( bf_read &msg );

private:
	// old variables
	int		m_iHealth;
	
	int		m_bitsDamage;
};	

DECLARE_HUDELEMENT( CHudHealth );
DECLARE_HUD_MESSAGE( CHudHealth, Damage );

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CHudHealth::CHudHealth( const char *pElementName ) : CHudElement( pElementName ), CHudIconDisplay(NULL, "HudHealth")
{
	SetHiddenBits( HIDEHUD_HEALTH | HIDEHUD_PLAYERDEAD | HIDEHUD_NEEDSUIT );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudHealth::Init()
{
	HOOK_HUD_MESSAGE( CHudHealth, Damage );
	Reset();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudHealth::Reset()
{
	BaseClass::Reset();

	m_iHealth		= INIT_HEALTH;
	m_bitsDamage	= 0;

	SetIcon(L"+");
	SetLabel(L"Health");

	/*m_bSimple = true;

	if (!m_pBase)
		m_pBase = new ImageFX( this, "sprites/metrocop_hud/hud_base", "MetrocopHud_Left" );	//hud_right

	if (m_pBase)
	{
		m_pBase->SetCustomPoints(true);
		m_pBase->SetVisibleEx(true);

		m_pBase->SetZPos(5);	
		PaintSimpleBar(m_pBase, 100, 100, icon_ypos + (surface()->GetFontTall(m_hSmallNumberFont)*0.6) + (GetWide()*0.1), 0.2f);
	}

	if (!m_pBar)
		m_pBar = new ImageFX( this, "sprites/metrocop_hud/hud_health", "MetrocopHud_Health" ); //"sprites/metrocop_hud/hud_bigbar"

	if (m_pBar)
	{
		m_pBar->SetPosEx(GetWide()/2,GetTall()/2);
		m_pBar->SetImageSize(GetWide(), GetTall());
		m_pBar->SetCustomPoints(true);
		m_pBar->SetVisibleEx(true);

		m_pBar->SetZPos(6);
	}

	DevMsg("Health hud reset\n");*/

	SetDisplayValue(m_iHealth);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudHealth::VidInit()
{
	Reset();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudHealth::OnThink()
{
	int newHealth = 0;
	C_BasePlayer *local = C_BasePlayer::GetLocalPlayer();
	if ( local )
	{
		// Never below zero
		newHealth = MAX( local->GetHealth(), 0 );
	}

	// Only update the fade if we've changed health
	if ( newHealth == m_iHealth )
	{
		return;
	}

	m_iHealth = newHealth;

	if ( m_iHealth >= 20 )
	{
		g_pClientMode->GetViewportAnimationController()->StartAnimationSequence("HealthIncreasedAbove20");
	}
	else if ( m_iHealth > 0 )
	{
		g_pClientMode->GetViewportAnimationController()->StartAnimationSequence("HealthIncreasedBelow20");
		g_pClientMode->GetViewportAnimationController()->StartAnimationSequence("HealthLow");
	}

	SetDisplayValue(m_iHealth);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudHealth::MsgFunc_Damage( bf_read &msg )
{

	int armor = msg.ReadByte();	// armor
	int damageTaken = msg.ReadByte();	// health
	long bitsDamage = msg.ReadLong(); // damage bits
	bitsDamage; // variable still sent but not used

	Vector vecFrom;

	vecFrom.x = msg.ReadBitCoord();
	vecFrom.y = msg.ReadBitCoord();
	vecFrom.z = msg.ReadBitCoord();

	// Actually took damage?
	if ( damageTaken > 0 || armor > 0 )
	{
		if ( damageTaken > 0 )
		{
			// start the animation
			g_pClientMode->GetViewportAnimationController()->StartAnimationSequence("HealthDamageTaken");
		}
	}
}
