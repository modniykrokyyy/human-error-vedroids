//========= Copyright © 1996-2008, The New Era team, All rights reserved. ============//
//
// Purpose: TNE Render Targets
//
// $NoKeywords: $
//=============================================================================//

#ifndef TNERENDERTARGETS_H_
#define TNERENDERTARGETS_H_
#ifdef _WIN32
#pragma once
#endif
 
#include "baseclientrendertargets.h" // Base class, with interfaces called by engine and inherited members to init common render   targets
 
#define MANHACK_SCREEN_MATERIAL "vgui/screens/manhack_screen"
#define CAMERA_SCREEN_MATERIAL "vgui/screens/camera_screen"

// externs
class IMaterialSystem;
class IMaterialSystemHardwareConfig;
 
class CTNERenderTargets : public CBaseClientRenderTargets
{ 
	// no networked vars 
	DECLARE_CLASS_GAMEROOT( CTNERenderTargets, CBaseClientRenderTargets );
public: 
	virtual void InitClientRenderTargets( IMaterialSystem* pMaterialSystem, IMaterialSystemHardwareConfig* pHardwareConfig );
	virtual void ShutdownClientRenderTargets();
 
	ITexture* CreateManhackScreenTexture( IMaterialSystem* pMaterialSystem );
	ITexture* CreateCameraScreenTexture( IMaterialSystem* pMaterialSystem );

private:
	CTextureReference		m_ManhackScreenTexture; 
	CTextureReference		m_CameraScreenTexture;
};
 
extern CTNERenderTargets* TNERenderTargets;
 
#endif //TNERENDERTARGETS_H_