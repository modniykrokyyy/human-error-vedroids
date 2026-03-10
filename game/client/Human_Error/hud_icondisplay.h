//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#ifndef HUD_ICONDISPLAY_H
#define HUD_ICONDISPLAY_H
#ifdef _WIN32
#pragma once
#endif

#include "tier1/utlflags.h"
#include "vgui/VGUI.h"
#include "vgui/Dar.h"
#include <vgui_controls/Panel.h>
#include <vgui/IScheme.h>
#include "ImageFX.h"

//-----------------------------------------------------------------------------
// Purpose: Base class for all the hud elements that are just a numeric display
//			with some options for text and icons
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
// Purpose: Base class for all the hud elements that are just a numeric display
//			with some options for text and icons
//-----------------------------------------------------------------------------
class CHudIconDisplay: public vgui::Panel
{
	DECLARE_CLASS_SIMPLE( CHudIconDisplay, vgui::Panel );

public:
	CHudIconDisplay(vgui::Panel *parent, const char *name);

	void SetDisplayValue(int value);
	void SetSecondaryValue(int value);
	void SetShouldDisplayValue(bool state);
	void SetShouldDisplaySecondaryValue(bool state);

	void SetLabel(const wchar_t *text, bool IsAPC = false);

	void SetIcon(wchar_t *icon); // int m_iMaxValue, int m_iMaxSecondaryValue);

	bool ShouldDisplayValue( void ) { return m_bDisplayValue; }
	bool ShouldDisplaySecondaryValue( void ) { return m_bDisplaySecondaryValue; }

	virtual void ApplySchemeSettings(vgui::IScheme *pScheme);

	virtual void Reset();

		virtual void Paint();
protected:
	// vgui overrides
	//virtual void Paint();
	//virtual void PaintLabel();

	virtual void PaintIcons(vgui::HFont font, vgui::HFont font_small);
	/*virtual void PaintBar(ImageFX *pBar, int value, int max, float flAlha = 1.0f);
	virtual void PaintSimpleBar(ImageFX *pBar, int value, int max, float ypos, float sidegap, float flAlpha = 1.0f);

	void		 SetStandardPoints(ImageFX *pBar);

	virtual void ShowImages(bool show);*/

protected:

	/*ImageFX *m_pBar;
	ImageFX *m_pBarSecondary;
	ImageFX *m_pBase;
	ImageFX *m_pBase2;
	bool m_bRightSide;
	bool m_bSimple;*/

	int m_iValue;
	int m_iSecondaryValue;

	//int m_iMaxValue, m_iMaxSecondaryValue;
	bool m_bDisplayValue, m_bDisplaySecondaryValue;

	wchar_t m_Icon[2];
	wchar_t m_LabelText[12];
	bool	m_bIsAPC;

	CPanelAnimationVar( float, m_flBlur, "Blur", "0" );
	CPanelAnimationVar( Color, m_TextColor, "TextColor", "FgColor" );
	CPanelAnimationVar( Color, m_Ammo2Color, "Ammo2Color", "FgColor" );

	CPanelAnimationVar( vgui::HFont, m_hNumberFont,				"NumberFont",			"HE_HudFont" );
	CPanelAnimationVar( vgui::HFont, m_hNumberGlowFont,			"NumberGlowFont",		"HE_HudFontGlow" );
	CPanelAnimationVar( vgui::HFont, m_hSmallNumberFont,		"SmallNumberFont",		"HudNumbersSmall" );
	CPanelAnimationVar( vgui::HFont, m_hSmallNumberGlowFont,	"SmallNumberGlowFont",	"HudNumbersSmallGlow" );
	CPanelAnimationVar( vgui::HFont, m_hTextFont,				"TextFont",				"Default" );

	CPanelAnimationVarAliasType( float, digi_xpos, "digi_xpos", "0", "proportional_float" );
	CPanelAnimationVarAliasType( float, digi_ypos, "digi_ypos", "0", "proportional_float" );
	CPanelAnimationVarAliasType( float, icon_xpos, "icon_xpos", "0", "proportional_float" );
	CPanelAnimationVarAliasType( float, icon_ypos, "icon_ypos", "0", "proportional_float" );
	//CPanelAnimationVarAliasType( float, digit2_xpos, "digit2_xpos", "98", "proportional_float" );
	//CPanelAnimationVarAliasType( float, digit2_ypos, "digit2_ypos", "16", "proportional_float" );
};

#endif // HUD_ICONDISPLAY_H
