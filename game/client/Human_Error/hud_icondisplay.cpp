//========= Copyright � 1996-2005, Valve Corporation, All rights reserved. ============//
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

#include "ImageFX.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CHudIconDisplay::CHudIconDisplay(vgui::Panel *parent, const char *name) : BaseClass(parent, name)
{
	vgui::Panel *pParent = g_pClientMode->GetViewport();
	SetParent( pParent );

	m_iValue = 0;
	m_Icon[0] = 0;
	m_iSecondaryValue = 0;
	m_bDisplayValue = true;
	m_bDisplaySecondaryValue = false;

	/*m_iMaxValue = 100;
	m_iMaxSecondaryValue = 0;

	m_bRightSide = true;

	m_bSimple		 = false;

	m_pBar			= NULL;
	m_pBarSecondary = NULL;
	m_pBase			= NULL;
	m_pBase2		= NULL;*/
}

void CHudIconDisplay::ApplySchemeSettings(vgui::IScheme *pScheme)
{
	BaseClass::ApplySchemeSettings( pScheme );

	SetPaintBackgroundEnabled( true );
}

/*void CHudIconDisplay::ShowImages(bool show)
{
	if (m_pBar)
	{
		m_pBar->SetVisibleEx(show && m_bDisplayValue);
	}

	if (m_pBase)
	{
		m_pBase->SetVisibleEx(show && m_bDisplayValue);
	}

	if (m_pBarSecondary)
	{
		m_pBarSecondary->SetVisibleEx(show && m_bDisplaySecondaryValue);
	}

	if (m_pBase2)
	{
		m_pBase2->SetVisibleEx(show && m_bDisplaySecondaryValue);
	}

}*/

void CHudIconDisplay::SetIcon(wchar_t *icon) // int maxValue, int maxSecondaryValue )
{
	wcsncpy(m_Icon, icon, sizeof(m_Icon) / sizeof(wchar_t));
	m_Icon[(sizeof(m_Icon) / sizeof(wchar_t)) - 1] = 0;

	//m_iMaxValue			 = maxValue;
	//m_iMaxSecondaryValue = maxSecondaryValue;
}

//-----------------------------------------------------------------------------
// Purpose: Resets values on restore/new map
//-----------------------------------------------------------------------------
void CHudIconDisplay::Reset()
{
	m_flBlur = 0.0f;

	SetPaintBackgroundEnabled( true );

}

//-----------------------------------------------------------------------------
// Purpose: data accessor
//-----------------------------------------------------------------------------
void CHudIconDisplay::SetDisplayValue(int value)
{
	m_iValue = value;
}

//-----------------------------------------------------------------------------
// Purpose: data accessor
//-----------------------------------------------------------------------------
void CHudIconDisplay::SetSecondaryValue(int value)
{
	m_iSecondaryValue = value;
}

//-----------------------------------------------------------------------------
// Purpose: data accessor
//-----------------------------------------------------------------------------
void CHudIconDisplay::SetShouldDisplayValue(bool state)
{
	m_bDisplayValue = state;

	/*if (m_pBar)
	{
		m_pBar->SetVisibleEx(state);
	}
	if (m_pBase)
	{
		m_pBase->SetVisibleEx(state);
	}*/
}

//-----------------------------------------------------------------------------
// Purpose: data accessor
//-----------------------------------------------------------------------------
void CHudIconDisplay::SetShouldDisplaySecondaryValue(bool state)
{
	m_bDisplaySecondaryValue = state;

	/*if (m_pBarSecondary)
	{
		m_pBarSecondary->SetVisibleEx(state);
	}
	if (m_pBase2)
	{
		m_pBase2->SetVisibleEx(state);
	}*/
}

//-----------------------------------------------------------------------------
// Purpose: data accessor
//-----------------------------------------------------------------------------
void CHudIconDisplay::SetLabel(const wchar_t *text, bool IsAPC)
{
	wcsncpy(m_LabelText, text, sizeof(m_LabelText) / sizeof(wchar_t));
	m_LabelText[(sizeof(m_LabelText) / sizeof(wchar_t)) - 1] = 0;

	m_bIsAPC = IsAPC;
}

//-----------------------------------------------------------------------------
// Purpose: paints a number at the specified position
//-----------------------------------------------------------------------------
void CHudIconDisplay::PaintIcons(HFont font, HFont font_small)
{
	float xpos, ypos;

	xpos = digi_xpos; 
	ypos = digi_ypos - (surface()->GetFontTall(font) - surface()->GetFontTall(font_small))*0.5;

	//TERO: First, lets draw the icon

	surface()->DrawSetTextFont(font);
	surface()->DrawSetTextPos((int)xpos, (int)ypos);
	surface()->DrawUnicodeString( m_Icon );

	//TERO: Lets draw the ammo name

	xpos = digi_xpos + surface()->GetCharacterWidth(font, m_Icon[0]) + surface()->GetCharacterWidth(font_small, '0');
	ypos = digi_ypos*2;

	if (m_bIsAPC)
	{
		surface()->DrawSetTextFont(m_hTextFont);
		surface()->DrawSetTextPos((int)xpos, (int)digi_ypos);
		surface()->DrawUnicodeString( L"CANNON");

		surface()->DrawSetTextFont(m_hTextFont);
		surface()->DrawSetTextPos((int)xpos, (int)(digi_ypos + surface()->GetFontTall(m_hTextFont)));
		surface()->DrawUnicodeString( L"ROCKETS");

		ypos += digi_ypos*2;
	}
	else
	{
		surface()->DrawSetTextFont(m_hTextFont);
		surface()->DrawSetTextPos((int)xpos, (int)ypos);
		surface()->DrawUnicodeString( m_LabelText );
	}

	//TERO: now lets draw the ammount

	wchar_t unicode[7];
	if (m_bDisplaySecondaryValue)
		swprintf(unicode, 7, L"%d/%d", m_iValue, m_iSecondaryValue);//swprintf(unicode, L"%d/%d", m_iValue, m_iSecondaryValue);
	else
		swprintf(unicode, 7, L"%d", m_iValue);//swprintf(unicode, L"%d", m_iValue);

	int length = 7;
	for (int i=0; i<7; i++)
	{
		if (unicode[i] == L'\0')
		{
			length = i;
			break;
		}
	}

	xpos = (GetWide() *.5f) - ((surface()->GetCharacterWidth(font, '0') * length * 0.5f));

	ypos +=(surface()->GetFontTall(m_hTextFont));

	surface()->DrawSetTextFont(font);
	surface()->DrawSetTextPos((int)xpos, (int)ypos);
	surface()->DrawUnicodeString( unicode );
}


/*void CHudIconDisplay::PaintBar(ImageFX *pBar, int value, int max, float flAlpha)
{
	if (pBar)
	{	
		float bar = (float)value/max;
		float upperbar;

		bar = clamp(bar, 0,1);

		Vertex_t *points = pBar->GetPoints();

		float width  = GetWide() * 0.64; //0.8 * 250 = 200, 0.8 times this t
		float height = GetTall() * 0.8;

		float xpos  = GetWide() * 0.08;
		float ypos  = GetTall() * 0.1;

		if (m_bRightSide)
		{
			upperbar = bar+0.1;
			upperbar = clamp(upperbar, 0,1);

			points[0].m_TexCoord = Vector2D(0,			0);
			points[1].m_TexCoord = Vector2D(upperbar,	0);
			points[2].m_TexCoord = Vector2D(bar,		1);
			points[3].m_TexCoord = Vector2D(0,			1);

			points[0].m_Position = Vector2D(xpos,					ypos);
			points[1].m_Position = Vector2D(xpos + width*upperbar,	ypos);
			points[2].m_Position = Vector2D(xpos + width*bar,		ypos + height);
			points[3].m_Position = Vector2D(xpos,					ypos + height);

		} else
		{
			bar = 1-bar;
			upperbar = bar - 0.1;
			upperbar = clamp(upperbar, 0,1);

			points[0].m_TexCoord = Vector2D(upperbar,	0);
			points[1].m_TexCoord = Vector2D(1,			0);
			points[2].m_TexCoord = Vector2D(1,			1);
			points[3].m_TexCoord = Vector2D(bar,		1);

			points[0].m_Position = Vector2D(xpos + width*upperbar,	ypos);
			points[1].m_Position = Vector2D(xpos + width,			ypos);
			points[2].m_Position = Vector2D(xpos + width,			ypos + height);
			points[3].m_Position = Vector2D(xpos + width*bar,		ypos + height);
		}
		pBar->SetPoints(points, 4);

		pBar->SetFade( flAlpha ); 
	}
}

void CHudIconDisplay::PaintSimpleBar(ImageFX *pBar, int value, int max, float ypos, float sidegap, float flAlpha)
{
	if (pBar)
	{	

		float bar = (float)value/max;

		bar = clamp(bar, 0,1);

		Vertex_t *points = pBar->GetPoints();

		float width = GetWide() * (1 - (2*sidegap));
		float tall  = width * 0.125f;
		float xpos  = GetWide() * sidegap;

		points[0].m_TexCoord = Vector2D(1-bar,			0);
		points[1].m_TexCoord = Vector2D(1,				0);
		points[2].m_TexCoord = Vector2D(1,				1);
		points[3].m_TexCoord = Vector2D(1-bar,			1);

		points[0].m_Position = Vector2D(xpos,				ypos);
		points[1].m_Position = Vector2D(xpos + width*bar,	ypos);
		points[2].m_Position = Vector2D(xpos + width*bar,	ypos + tall);
		points[3].m_Position = Vector2D(xpos,				ypos + tall);

		pBar->SetPoints(points, 4);

		if (bar < 0.2)
		{
			pBar->SetColor(Color(255,0,0));
		}
		else
		{
			pBar->SetColor(Color(255,255,255));
		}

		pBar->SetFade( flAlpha ); 
	}
}

void CHudIconDisplay::SetStandardPoints(ImageFX *pBar)
{
	Vertex_t *points = pBar->GetPoints();

	points[0].m_TexCoord = Vector2D(0,		0);
	points[1].m_TexCoord = Vector2D(1,		0);
	points[2].m_TexCoord = Vector2D(1,		1);
	points[3].m_TexCoord = Vector2D(0,		1);

	points[0].m_Position = Vector2D(GetWide()* 0.08,		GetTall() * 0.1);
	points[1].m_Position = Vector2D(GetWide()* 0.72,		GetTall() * 0.1);
	points[2].m_Position = Vector2D(GetWide()* 0.72,		GetTall() * 0.9);
	points[3].m_Position = Vector2D(GetWide()* 0.08,		GetTall() * 0.9);

	pBar->SetPoints(points, 4);
}*/

//-----------------------------------------------------------------------------
// Purpose: draws the text
//-----------------------------------------------------------------------------
/*void CHudIconDisplay::PaintLabel( void )
{
	surface()->DrawSetTextFont(m_hTextFont);
	surface()->DrawSetTextColor(GetFgColor());
	surface()->DrawSetTextPos(text_xpos, text_ypos);
	surface()->DrawUnicodeString( m_LabelText );
}*/

//-----------------------------------------------------------------------------
// Purpose: renders the vgui panel
//-----------------------------------------------------------------------------
void CHudIconDisplay::Paint()
{
	float flAlpha = GetAlpha1();
	flAlpha *= flAlpha;

	if (m_bDisplayValue)
	{

		/*if (m_bSimple)
			PaintSimpleBar(m_pBar, m_iValue, m_iMaxValue, icon_ypos + (surface()->GetFontTall(m_hSmallNumberFont)*0.6) + (GetWide()*0.1), 0.2f, flAlpha);
		else
			PaintBar(m_pBar, m_iValue, m_iMaxValue, flAlpha);*/

		// draw our numbers
		surface()->DrawSetTextColor(GetFgColor());
		PaintIcons(m_hNumberFont, m_hSmallNumberFont);

		// draw the overbright blur
		for (float fl = m_flBlur; fl > 0.0f; fl -= 1.0f)
		{
			if (fl >= 1.0f)
			{
				PaintIcons(m_hNumberGlowFont, m_hSmallNumberGlowFont);
			}
			else
			{
				// draw a percentage of the last one
				Color col = GetFgColor();
				col[3] *= fl;
				surface()->DrawSetTextColor(col);
				PaintIcons(m_hNumberGlowFont, m_hSmallNumberGlowFont);
			}
		}
	}

	// total ammo
	/*if (m_bDisplaySecondaryValue)
	{
		surface()->DrawSetTextColor(GetFgColor());
		if (m_bSimple)
			PaintSimpleBar(m_pBarSecondary, m_iSecondaryValue, m_iMaxSecondaryValue, icon_ypos + (surface()->GetFontTall(m_hSmallNumberFont)*1.4f) + (GetWide()*0.1f), 0.1f, flAlpha);
		else
			PaintBar(m_pBarSecondary, m_iSecondaryValue, m_iMaxSecondaryValue, flAlpha);
	}

	if (m_pBase)
	{
		m_pBase->SetFade( flAlpha );
	}

	if (m_pBase2)
	{
		m_pBase2->SetFade( flAlpha );
	}*/
}



