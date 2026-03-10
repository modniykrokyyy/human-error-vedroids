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

#include "ImageFX.h"
#include "Human_Error/hud_radio.h"

#include "engine/IEngineSound.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;

DECLARE_HUDELEMENT( CHudMetrocopRadio );

DECLARE_HUD_MESSAGE( CHudMetrocopRadio, UpdateRadioDuration );

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------

CHudMetrocopRadio::CHudMetrocopRadio( const char *pElementName ) : BaseClass(NULL, "HudRadio"), CHudElement( pElementName )
{
	SetHiddenBits( HIDEHUD_HEALTH | HIDEHUD_PLAYERDEAD | HIDEHUD_NEEDSUIT );

	vgui::Panel *pParent = g_pClientMode->GetViewport();
	SetParent( pParent );

	m_flDuration = 0;

	m_bHidden = true;
	m_bDisplay = false;

	m_pBar			= NULL;
}

CHudMetrocopRadio::~CHudMetrocopRadio()
{
	if (m_pBar)
	{
		m_pBar->DeletePanel();
	}
}

void CHudMetrocopRadio::ApplySchemeSettings(vgui::IScheme *pScheme)
{
	BaseClass::ApplySchemeSettings( pScheme );

	SetPaintBackgroundEnabled( true );
}

void CHudMetrocopRadio::ShowRadio(bool show)
{
	SetPaintBackgroundEnabled( show );
	m_bDisplay = show;
	SetVisible(show);
	//m_bHidden = !show;
	if (show)
	{
		m_bHidden = false;
	}
	else
	{
		m_bHidden = true;
	}

	if (m_pBar)
	{
		m_pBar->SetVisibleEx(show);
	}
}

//-----------------------------------------------------------------------------
// Purpose: Resets values on restore/new map
//-----------------------------------------------------------------------------
void CHudMetrocopRadio::Reset()
{
	m_flBlur = 0.0f;

	SetPaintBackgroundEnabled( true );

	if ( m_flDuration < gpGlobals->curtime || m_flDuration == 0 )
	{
		ShowRadio(false);
		g_pClientMode->GetViewportAnimationController()->StartAnimationSequence("HideRadio");
	}
	else
	{
		ShowRadio(true);
		g_pClientMode->GetViewportAnimationController()->StartAnimationSequence("ShowRadio");
	}

	for (int i=0; i<VISUALIZER_BARS; i++)
	{
		visualizer_bars[i]=0;
	}

	if (!m_pBar)
		m_pBar = new ImageFX( this, "sprites/metrocop_hud/hud_base", "MetrocopHud_Radio" );

	if (m_pBar)
	{
		SetStandardPoints(m_pBar);

		m_pBar->SetZPos(6);
	}
}

void CHudMetrocopRadio::Init()
{
	//SetLabel(L"RADIO");

	HOOK_HUD_MESSAGE( CHudMetrocopRadio, UpdateRadioDuration );
}


void CHudMetrocopRadio::MsgFunc_UpdateRadioDuration( bf_read &msg )
{
	float flNewDuration = msg.ReadFloat();

	if ( flNewDuration != 0 )
	{
		if ( flNewDuration > m_flDuration || m_flDuration == 0 )
		{
			m_flDuration = flNewDuration;

			if (m_bHidden || !m_bDisplay)
			{
				g_pClientMode->GetViewportAnimationController()->StartAnimationSequence("ShowRadio");
			}	
			m_iCurrentIndex = random->RandomInt(5,7);
			ShowRadio(true);
		}
	}
	else
	{
		m_flDuration = 0;
	}
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CHudMetrocopRadio::VidInit( void )
{
	Reset();
}

void CHudMetrocopRadio::LevelInit()
{
	//LevelShutdown();

	m_flDuration = 0;
	g_pClientMode->GetViewportAnimationController()->StartAnimationSequence("ShowRadio");

	ShowRadio(false);
}

void CHudMetrocopRadio::LevelShutdown()
{
	m_flDuration = 0;
	if (!m_bHidden)
	{
		g_pClientMode->GetViewportAnimationController()->StartAnimationSequence("HideRadio");
	}
	ShowRadio(false);

	//BaseClass::LevelShutDown();
}


//-----------------------------------------------------------------------------
// Purpose: data accessor
//-----------------------------------------------------------------------------
/*void CHudMetrocopRadio::SetLabel(const wchar_t *text)
{
	wcsncpy(m_LabelText, text, sizeof(m_LabelText) / sizeof(wchar_t));
	m_LabelText[(sizeof(m_LabelText) / sizeof(wchar_t)) - 1] = 0;
}*/


void CHudMetrocopRadio::SetStandardPoints(ImageFX *pBar)
{
	float width = base_width;
	float tall  = GetTall();
	float xpos  = 0;
	float ypos  = 0;

	pBar->SetPosEx(xpos, ypos);
	pBar->SetImageSize(width, tall);
	pBar->SetCustomPoints(true);
	pBar->SetVisibleEx(true);

	Vertex_t *points = pBar->GetPoints();

	points[0].m_TexCoord = Vector2D(0,		0);
	points[1].m_TexCoord = Vector2D(1,		0);
	points[2].m_TexCoord = Vector2D(1,		1);
	points[3].m_TexCoord = Vector2D(0,		1);

	points[0].m_Position = Vector2D(xpos,		ypos);
	points[1].m_Position = Vector2D(width,		ypos);
	points[2].m_Position = Vector2D(width,		tall);
	points[3].m_Position = Vector2D(xpos,		tall);

	pBar->SetPoints(points, 4);
}

//-----------------------------------------------------------------------------
// Purpose: draws the text
//-----------------------------------------------------------------------------
void CHudMetrocopRadio::PaintLabel( void )
{
	surface()->DrawSetTextFont(m_hTextFont);
	surface()->DrawSetTextColor(GetFgColor());
	surface()->DrawSetTextPos(label_xpos, label_ypos);
	surface()->DrawUnicodeString( L"CIVIL PR." );

	surface()->DrawSetTextFont(m_hTextFont);
	surface()->DrawSetTextColor(GetFgColor());
	surface()->DrawSetTextPos(label_xpos, label_ypos + surface()->GetFontTall(m_hTextFont));
	surface()->DrawUnicodeString( L"RADIO" );
}

//-----------------------------------------------------------------------------
// Purpose: renders the vgui panel
//-----------------------------------------------------------------------------
void CHudMetrocopRadio::Paint()
{
	if (m_bDisplay)
	{
		if (!m_bHidden)
		{
			if ( m_flDuration < gpGlobals->curtime)
			{
				m_bHidden = true;
				g_pClientMode->GetViewportAnimationController()->StartAnimationSequence("HideRadio");
				m_flDuration = gpGlobals->curtime + RADIO_HIDE_DELAY;
			}
		}
		else
		{
			if ( m_flDuration < gpGlobals->curtime )
			{
				ShowRadio(false);
			}
		}

		PaintVisualizer();

		PaintLabel();
	}
}

void CHudMetrocopRadio::PaintVisualizer()
{
	int iSteps = ((gpGlobals->frametime * 120.0f) + 0.4f);

	if (iSteps > VISUALIZER_STEP)
	{
		iSteps = VISUALIZER_STEP;
	}

	if (iSteps > 0)
	{
		for (int i=0; i<(VISUALIZER_BARS-iSteps); i++)
		{
			visualizer_bars[ i ] = visualizer_bars[ i + iSteps ];
		}

		for (int i=VISUALIZER_BARS-iSteps; i<VISUALIZER_BARS; i++)
		{
			if (!m_bHidden)
			{
				if (m_iCurrentIndex < 2)
				{
					visualizer_bars[i] = random->RandomInt(-20,20);
				}
				else
				{
					visualizer_bars[i] = random->RandomInt(-50,50);
				}

				m_iCurrentIndex--;
				if (m_iCurrentIndex <= 0)
				{
					m_iCurrentIndex = random->RandomInt(5,7);
				}
			}
			else
			{
				visualizer_bars[i] = 0;
			}
		}
	}

	int xpos, ypos;

	int value = 50 + (visualizer_bars[0] * (1 / 6));

	int prev_xpos = (int)visualizer_xpos;
	int prev_ypos = (int)(visualizer_ypos + (visualizer_tall * value / 100));

	int alpha;

	for (int i=1; i<VISUALIZER_BARS; i++)
	{
		alpha = i - VISUALIZER_CENTER;
		if (alpha<0)
			alpha = -alpha;
		alpha = 128 + RemapVal(alpha, 0, 50, 96,0);

		vgui::surface()->DrawSetColor(255, 255, 255, alpha);

		value = 50 + (visualizer_bars[i] * (60 - abs( 50 - i  )) / 60);

		xpos = (int)(visualizer_xpos + (visualizer_width * i / VISUALIZER_BARS));
		ypos = (int)(visualizer_ypos + (visualizer_tall * value / 100));

		vgui::surface()->DrawLine( xpos, ypos, prev_xpos, prev_ypos);

		prev_xpos = xpos;
		prev_ypos = ypos;
	}
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool CHudMetrocopRadio::ShouldDraw( void )
{
	return m_bDisplay;
}
