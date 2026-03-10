//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#ifndef HUD_RADIO_H
#define HUD_RADIO_H
#ifdef _WIN32
#pragma once
#endif

#include "tier1/utlflags.h"
#include "vgui/VGUI.h"
#include "vgui/Dar.h"
#include <vgui_controls/Panel.h>
#include <vgui/IScheme.h>
#include "hud.h"
#include "hudelement.h"
#include "hud_macros.h"
#include "ImageFX.h"

#define RADIO_HIDE_DELAY 1.2f

#define VISUALIZER_BARS		100
#define VISUALIZER_CENTER	50
#define VISUALIZER_STEP		10

class CHudMetrocopRadio : public vgui::Panel, public CHudElement
{
	DECLARE_CLASS_SIMPLE( CHudMetrocopRadio, vgui::Panel );

public:
	CHudMetrocopRadio(const char *pElementName);
	~CHudMetrocopRadio();

	void MsgFunc_UpdateRadioDuration( bf_read &msg );


	virtual void ApplySchemeSettings(vgui::IScheme *pScheme);

	virtual void Reset();
	virtual void Init( void );
	virtual void VidInit( void );
	virtual void Paint();
	virtual void LevelInit( void );
	virtual void LevelShutdown( void );
	
	virtual bool ShouldDraw();

	void PaintLabel();
	void PaintVisualizer();
protected:


	void		 SetStandardPoints(ImageFX *pBar);

	virtual void ShowRadio(bool show);

protected:

	ImageFX *m_pBar;

	float	m_flDuration;

	int		m_iCurrentIndex;

	int		visualizer_bars[VISUALIZER_BARS];

	bool	m_bDisplay;
	bool	m_bHidden;

	wchar_t m_LabelText[12];

	CPanelAnimationVar( float, m_flBlur, "Blur", "0" );
	CPanelAnimationVar( Color, m_TextColor, "TextColor", "FgColor" );
	/*CPanelAnimationVar( Color, m_Ammo2Color, "Ammo2Color", "FgColor" );

	CPanelAnimationVar( vgui::HFont, m_hNumberFont, "NumberFont", "HE_HudFont" );
	CPanelAnimationVar( vgui::HFont, m_hNumberGlowFont, "NumberGlowFont", "HE_HudFontGlow" );
	CPanelAnimationVar( vgui::HFont, m_hSmallNumberFont, "SmallNumberFont", "HudNumbersSmall" );*/
	CPanelAnimationVar( vgui::HFont, m_hTextFont, "TextFont", "Default" );

	CPanelAnimationVarAliasType( float, visualizer_xpos,	"visualizer_xpos",		"10", "proportional_float" );
	CPanelAnimationVarAliasType( float, visualizer_ypos,	"visualizer_ypos",		"2",  "proportional_float" );
	CPanelAnimationVarAliasType( float, visualizer_width,	"visualizer_width",		"80", "proportional_float" );
	CPanelAnimationVarAliasType( float, visualizer_tall,	"visualizer_tall",		"16", "proportional_float" );
	CPanelAnimationVarAliasType( float, base_width,			"base_width",			"100", "proportional_float" );
	CPanelAnimationVarAliasType( float, label_xpos,			"label_xpos",			"10", "proportional_float" );
	CPanelAnimationVarAliasType( float, label_ypos,			"label_ypos",			"4",  "proportional_float" );
};

#endif // HUD_ICONDISPLAY_H
