//=============================================================================//
//
// Purpose: Ask: This is a screen we'll use for the Manhack
//
//=============================================================================//
#include "cbase.h"

#include "c_vguiscreen.h"
#include <vgui/IVGui.h>
#include <vgui_controls/Controls.h>
#include <vgui_controls/Label.h>
#include "clientmode_hlnormal.h"
#include "tne_RenderTargets.h"
#include "rendertexture.h"
#include "view_shared.h"
#include "c_manhack_screen.h"
#include "ammodef.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//-----------------------------------------------------------------------------
// Standard VGUI panel for objects
//-----------------------------------------------------------------------------
DECLARE_VGUI_SCREEN_FACTORY( CManhackScreen, "manhack_screen" );

static CManhackScreen *s_ManhackScreen = NULL;

CManhackScreen *GetManhackScreen()
{
	return s_ManhackScreen;
}


//-----------------------------------------------------------------------------
// Constructor:
//-----------------------------------------------------------------------------
CManhackScreen::CManhackScreen( vgui::Panel *parent, const char *panelName )
    : BaseClass( parent, panelName, g_hVGuiCombineScheme )
{
}

CManhackScreen::~CManhackScreen()
{
	s_ManhackScreen = NULL;
}

//-----------------------------------------------------------------------------
// Initialization
//-----------------------------------------------------------------------------
bool CManhackScreen::Init( KeyValues* pKeyValues, VGuiScreenInitData_t* pInitData )
{
	s_ManhackScreen = this;

    // Load all of the controls in
    if ( !BaseClass::Init(pKeyValues, pInitData) )
        return false;

    // Make sure we get ticked...
    vgui::ivgui()->AddTickSignal( GetVPanel() );

    // Ask: Here we find a pointer to our AmmoCountReadout Label and store it in m_pAmmoCount
    m_pManhackCount		= dynamic_cast<vgui::Label*>(FindChildByName( "ManhackCountReadout" ));
	m_pManhackDistance	= dynamic_cast<vgui::Label*>(FindChildByName( "ManhackDistanceReadout" )); 
	m_pManhackOnline	= dynamic_cast<vgui::Label*>(FindChildByName( "ManhacksOnlineLabel" ));
	m_iManhackDistance = 100;

	if (m_pManhackOnline)
	{
		m_pManhackOnline->SetFgColor(Color(255,0,0,255));
	}

	SetVisible( false );

    return true;
}

//-----------------------------------------------------------------------------
// Ask: Let's update the label, shall we?
//-----------------------------------------------------------------------------
void CManhackScreen::OnTick()
{
    BaseClass::OnTick();

    // Get our player
    CBasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();
    if ( !pPlayer )
        return;

    // Get the players active weapon
    CBaseCombatWeapon *pWeapon = pPlayer->GetActiveWeapon();

    // If pWeapon is NULL or it doesn't use primary ammo, don't update our screen
    if ( !pWeapon || pWeapon->GetSecondaryAmmoType() != GetAmmoDef()->Index( "ManhacksOnline" ) )
        return;

    // Our RPG isn't clip-based, so we need to check the player's arsenal of rockets
    int manhacks = pPlayer->GetAmmoCount( pWeapon->GetSecondaryAmmoType() );

	if (m_pManhackOnline)
	{
		if (manhacks>0)
		{
			m_pManhackOnline->SetText("ONLINE");
			m_pManhackOnline->SetFgColor(Color(0,255,0,255));
		}
		else
		{
			m_pManhackOnline->SetText("OFFLINE");
			m_pManhackOnline->SetFgColor(Color(255,0,0,255));
		}
	}

    // If our Label exist
    if ( m_pManhackCount )
    {
        char buf[32];
        Q_snprintf( buf, sizeof( buf ), "%d", manhacks );
        // Set the Labels text to the number of missiles we have left.
        m_pManhackCount->SetText( buf );
		m_pManhackCount->SetFgColor(Color(255,255,255,255));
    }

	if ( m_pManhackDistance )
	{
		char buf[32];
        Q_snprintf( buf, sizeof( buf ), "%d", m_iManhackDistance );
        // Set the Labels text to the number of missiles we have left.

		int xpos, ypos;
		m_pManhackDistance->GetPos(xpos,ypos);

		int charWidth = vgui::surface()->GetCharacterWidth(m_pManhackDistance->GetFont(),'0');

		xpos = 100;

		if (m_iManhackDistance>=10)
			xpos -= charWidth;

		if (m_iManhackDistance>=100)
			xpos -= charWidth;

		m_pManhackDistance->SetPos(xpos,ypos);

        m_pManhackDistance->SetText( buf );
		m_pManhackDistance->SetFgColor(Color(255,255,255,255));
	}
}

void CManhackScreen::SetManhackDistance(int iDistance)
{
	m_iManhackDistance = 100 - iDistance;
	m_iManhackDistance = clamp( m_iManhackDistance, 0, 100);
}




