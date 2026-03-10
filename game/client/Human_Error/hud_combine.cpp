#include "cbase.h"
#include "hudelement.h"
#include "hud_macros.h"
#include "iclientmode.h"
#include "c_basehlplayer.h"

#include <vgui/IScheme.h>
#include <vgui_controls/Panel.h>

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

class CHudCombine : public vgui::Panel, public CHudElement
{
	DECLARE_CLASS_SIMPLE( CHudCombine, vgui::Panel );

public:
	CHudCombine( const char *pElementName );

	void Init();
	void MsgFunc_ShowCombineHud( bf_read &msg );

protected:
	virtual void ApplySchemeSettings(vgui::IScheme *scheme);
	virtual void Paint( void );

private:
	bool			m_bShow;
    CHudTexture*	m_pCombineHud;
};


DECLARE_HUDELEMENT( CHudCombine );
DECLARE_HUD_MESSAGE( CHudCombine, ShowCombineHud );

using namespace vgui;


/**
 * Constructor - generic HUD element initialization stuff. Make sure our 2 member variables
 * are instantiated.
 */
CHudCombine::CHudCombine( const char *pElementName ) : CHudElement(pElementName), BaseClass(NULL, "HudCombine")
{
	vgui::Panel *pParent = g_pClientMode->GetViewport();
	SetParent( pParent );

	m_bShow = false;
	m_pCombineHud = 0;

	// Scope will not show when the player is dead
	SetHiddenBits( HIDEHUD_PLAYERDEAD );
 
        // fix for users with diffrent screen ratio (Lodle)
	//int screenWide, screenTall;
	//GetHudSize(screenWide, screenTall);
	SetBounds(0, 0, 256, 256);

}


/**
 * Hook up our hud message, and make sure we are not showing the scope
 */
void CHudCombine::Init()
{
	HOOK_HUD_MESSAGE( CHudCombine, ShowCombineHud );

	m_bShow = false;
}


/**
 * Load  in the scope material here
 */
void CHudCombine::ApplySchemeSettings( vgui::IScheme *scheme )
{
	BaseClass::ApplySchemeSettings(scheme);

	SetPaintBackgroundEnabled(false);
	SetPaintBorderEnabled(false);

	if (!m_pCombineHud)
	{
		m_pCombineHud = gHUD.GetIcon("combinehud");
	}
}


/**
 * Simple - if we want to show the scope, draw it. Otherwise don't.
 */
void CHudCombine::Paint( void )
{
        C_BasePlayer* pPlayer = C_BasePlayer::GetLocalPlayer();
        if (!pPlayer)
        {
                return;
        }

	if (m_bShow)
	{
		// This will draw the scope at the origin of this HUD element, and
		// stretch it to the width and height of the element. As long as the
		// HUD element is set up to cover the entire screen, so will the scope
		Color clr(255, 255, 255, 255);
		m_pCombineHud->DrawSelf(0, 0, GetWide(), GetTall(), clr); //m_pCombineHud->DrawSelf(0, 0, GetWide(), GetTall(), Color(255,255,255,255));

		// Hide the crosshair
  	  //      pPlayer->m_Local.m_iHideHUD |= HIDEHUD_CROSSHAIR;
	}
/*	else if ((pPlayer->m_Local.m_iHideHUD & HIDEHUD_CROSSHAIR) != 0)
	{
		pPlayer->m_Local.m_iHideHUD &= ~HIDEHUD_CROSSHAIR;
	}*/
}



/**
 * Callback for our message - set the show variable to whatever
 * boolean value is received in the message
 */
void CHudCombine::MsgFunc_ShowCombineHud(bf_read &msg)
{
	DevMsg("ShowCombineHud\n");
	m_bShow = msg.ReadByte();
}
