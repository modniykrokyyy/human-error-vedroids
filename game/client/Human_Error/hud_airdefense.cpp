//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#include "cbase.h"
#include "hud_icondisplay.h"
#include "iclientmode.h"

#include <Color.h>
#include <KeyValues.h>
#include <vgui/ISurface.h>
#include <vgui/ISystem.h>
#include <vgui/IVGui.h>

#include <vgui_controls/AnimationController.h>
#include <vgui/ILocalize.h>

#include "tier1/utlflags.h"
#include "vgui/VGUI.h"
#include "vgui/Dar.h"
#include <vgui_controls/Panel.h>
#include <vgui/IScheme.h>
#include "hud.h"
#include "hudelement.h"
#include "hud_macros.h"


#include "engine/IEngineSound.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;

#define HLSS_NUMBER_OF_RECHARGE_BARS 3
#define BLUR_TIME 0.8f
#define FADE_OUT_TIME 1.2f

class CHudAirDefenseRecharge : public vgui::Panel, public CHudElement
{
	DECLARE_CLASS_SIMPLE( CHudAirDefenseRecharge, vgui::Panel );

public:
	CHudAirDefenseRecharge(const char *pElementName);
	~CHudAirDefenseRecharge();

	void MsgFunc_UpdateAirDefenseRecharge( bf_read &msg );


	virtual void ApplySchemeSettings(vgui::IScheme *pScheme);

	virtual void Reset();
	virtual void Init( void );
	virtual void VidInit( void );
	virtual void Paint();
	virtual void LevelInit( void );
	virtual void LevelShutdown( void );
	
	virtual bool ShouldDraw();
	void		 ShowRecharge(bool bShow);
	void		 PaintLabel();

protected:

	float	m_flDuration;
	float	m_flFadeOut;
	float	m_flRecharge[HLSS_NUMBER_OF_RECHARGE_BARS];
	float	m_flBlur;

	bool	m_bDisplay;
	bool	m_bRecharging;

	wchar_t m_LabelText[12];

	//CPanelAnimationVar( float, m_flBlur, "Blur", "0" );
	CPanelAnimationVar( Color, m_TextColor, "TextColor", "FgColor" );

	CPanelAnimationVar( vgui::HFont, m_hTextFont, "TextFont", "HudSelectionText" );

	CPanelAnimationVarAliasType( float, recharge_width,		"recharge_width",		"100", "proportional_float" );
	CPanelAnimationVarAliasType( float, recharge_height,	"recharge_height",		"10",  "proportional_float" );
	CPanelAnimationVarAliasType( float, recharge_gap,		"recharge_gap",			"4", "proportional_float" );
	CPanelAnimationVarAliasType( float, label_xpos,			"label_xpos",			"4", "proportional_float" );
	CPanelAnimationVarAliasType( float, label_ypos,			"label_ypos",			"4",  "proportional_float" );
};

DECLARE_HUDELEMENT( CHudAirDefenseRecharge );

DECLARE_HUD_MESSAGE( CHudAirDefenseRecharge, UpdateAirDefenseRecharge );

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------

CHudAirDefenseRecharge::CHudAirDefenseRecharge( const char *pElementName ) : BaseClass(NULL, "HudAirDefense"), CHudElement( pElementName )
{
	SetHiddenBits( HIDEHUD_HEALTH | HIDEHUD_PLAYERDEAD | HIDEHUD_NEEDSUIT );

	vgui::Panel *pParent = g_pClientMode->GetViewport();
	SetParent( pParent );

	m_flDuration = 0;

	m_bDisplay = false;

}

CHudAirDefenseRecharge::~CHudAirDefenseRecharge()
{

}

void CHudAirDefenseRecharge::ApplySchemeSettings(vgui::IScheme *pScheme)
{
	BaseClass::ApplySchemeSettings( pScheme );

	SetPaintBackgroundEnabled( true );
}

void CHudAirDefenseRecharge::ShowRecharge(bool show)
{
	SetPaintBackgroundEnabled( show );
	m_bDisplay = show;
	SetVisible(show);

	if (show)
	{
		m_flFadeOut = 0;
		SetAlpha(255); 
	}
}

//-----------------------------------------------------------------------------
// Purpose: Resets values on restore/new map
//-----------------------------------------------------------------------------
void CHudAirDefenseRecharge::Reset()
{
	m_flBlur = 0.0f;

	SetPaintBackgroundEnabled( true );

	if ( m_flDuration < gpGlobals->curtime || m_flDuration == 0 )
	{
		ShowRecharge(false);
		//g_pClientMode->GetViewportAnimationController()->StartAnimationSequence("HideRadio");
	}
	else
	{
		ShowRecharge(true);
		//g_pClientMode->GetViewportAnimationController()->StartAnimationSequence("ShowRadio");
	}

}

void CHudAirDefenseRecharge::Init()
{
	//SetLabel(L"RADIO");

	HOOK_HUD_MESSAGE( CHudAirDefenseRecharge, UpdateAirDefenseRecharge );
}


void CHudAirDefenseRecharge::MsgFunc_UpdateAirDefenseRecharge( bf_read &msg )
{
	bool bBlur = false;

	bool bDisplay = msg.ReadOneBit();

	if (bDisplay != m_bDisplay)
	{
		bBlur = true;
	}

	ShowRecharge(bDisplay);

	bool bRecharging = msg.ReadOneBit();

	if (bRecharging != m_bRecharging)
	{
		bBlur = true;
	}

	m_bRecharging = bRecharging;

	float flNewDuration = msg.ReadFloat();

	if ( flNewDuration != 0 )
	{
		if ( flNewDuration > m_flDuration || m_flDuration == 0 )
		{
			m_flDuration = flNewDuration;

			ShowRecharge(true);
		}
	}
	else
	{
		m_flDuration = 0;
	}

	float flRecharge;

	for (int i=0; i<HLSS_NUMBER_OF_RECHARGE_BARS; i++)
	{
		flRecharge = msg.ReadFloat();

		if (m_bRecharging)
		{
			if (flRecharge >= 1.0f && m_flRecharge[i] < 1.0f)
			{
				bBlur = true;
			}
		}
		else
		{
			if (flRecharge <= 0.0f && m_flRecharge[i] > 0.0f)
			{
				bBlur = true;
			}
		}

		m_flRecharge[i] = flRecharge;

		//DevMsg("Air Defense hud: bar %d is %f\n", i+1, m_flRecharge[i]);
	}

	if (bBlur)
	{
		m_flBlur = gpGlobals->curtime + BLUR_TIME;
	}
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CHudAirDefenseRecharge::VidInit( void )
{
	Reset();
}

void CHudAirDefenseRecharge::LevelInit()
{
	//LevelShutdown();

	m_flDuration = 0;

	ShowRecharge(false);
}

void CHudAirDefenseRecharge::LevelShutdown()
{
	m_flDuration = 0;

	ShowRecharge(false);

	//BaseClass::LevelShutDown();
}




//-----------------------------------------------------------------------------
// Purpose: draws the text
//-----------------------------------------------------------------------------
void CHudAirDefenseRecharge::PaintLabel( void )
{
	int alpha = 100;

	if (m_flBlur != 0 && m_flBlur > gpGlobals->curtime)
	{
		alpha = (int)RemapValClamped(m_flBlur - gpGlobals->curtime, 0.0f, BLUR_TIME, 100, 255);
	}

	surface()->DrawSetTextColor( 0, 220, 255, alpha );

	surface()->DrawSetTextFont(m_hTextFont);
	surface()->DrawSetTextPos(label_xpos, label_ypos);
	surface()->DrawUnicodeString( L"AIR DEFENSE" );

	surface()->DrawSetTextFont(m_hTextFont);
	surface()->DrawSetTextPos(label_xpos, label_ypos + surface()->GetFontTall(m_hTextFont));
	
	if (m_bRecharging)
	{
		surface()->DrawUnicodeString( L"RECHARGING" );
	}
	else
	{
		surface()->DrawUnicodeString( L"DECHARGING" );
	}
}

//-----------------------------------------------------------------------------
// Purpose: renders the vgui panel
//-----------------------------------------------------------------------------
void CHudAirDefenseRecharge::Paint()
{
	if (m_bDisplay)
	{
		if (m_flDuration < gpGlobals->curtime )
		{
			//m_bDisplay = false;

			if (m_flFadeOut == 0)
			{
				m_flFadeOut = gpGlobals->curtime + FADE_OUT_TIME;
			}
			else if ( m_flFadeOut > gpGlobals->curtime )
			{
				int alpha = (int)RemapValClamped(m_flFadeOut - gpGlobals->curtime, 0.0, FADE_OUT_TIME, 0, 255 );

				SetAlpha(alpha); 
			}
			else
			{
				ShowRecharge(false);
				return;
			}
		}
	
		PaintLabel();

		float y = label_ypos + (surface()->GetFontTall(m_hTextFont) * 2) + recharge_gap;
		float h = recharge_gap + recharge_height;

		float x = (GetWide() - recharge_width) * 0.5f;

		//TERO: first draw black background
		
		for (int i=0; i<HLSS_NUMBER_OF_RECHARGE_BARS; i++)
		{
			vgui::surface()->DrawSetColor(0, 0, 0, 128);
			vgui::surface()->DrawFilledRect(x, y + (i*h), x + recharge_width, y  + (i*h) + recharge_height);

			vgui::surface()->DrawSetColor( 0, 220, 255, 100 ); 
			vgui::surface()->DrawOutlinedRect(x, y + (i*h), x + recharge_width, y  + (i*h) + recharge_height);
		}

		//TERO: now draw the actual bars

		for (int i=0; i<HLSS_NUMBER_OF_RECHARGE_BARS; i++)
		{
			int alpha = (int)RemapValClamped(m_flRecharge[i], 0.0f, 0.2f, 100, 255);


			if (m_bRecharging)
			{
				vgui::surface()->DrawSetColor( 0, 220, 255, alpha ); 
			}
			else
			{
				vgui::surface()->DrawSetColor(255, 0, 0, alpha);
			}

			vgui::surface()->DrawFilledRect(x, y + (i*h), x + (recharge_width * m_flRecharge[i]), y  + (i*h) + recharge_height);
		}
	}
}



//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool CHudAirDefenseRecharge::ShouldDraw( void )
{
	return m_bDisplay;
}
