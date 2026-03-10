//========= Copyright © 1996-2008, The New Era team, All rights reserved. ============//
//
// Purpose: TNE Render Targets
//
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "tne_RenderTargets.h"
#include "materialsystem/imaterialsystem.h"
#include "rendertexture.h"
 
ITexture* CTNERenderTargets::CreateManhackScreenTexture( IMaterialSystem* pMaterialSystem )
{
//	DevMsg("Creating Scope Render Target: _rt_Scope\n");
	return pMaterialSystem->CreateNamedRenderTargetTextureEx2(
		MANHACK_SCREEN_MATERIAL,
		128, 64, RT_SIZE_OFFSCREEN,
		pMaterialSystem->GetBackBufferFormat(),
		MATERIAL_RT_DEPTH_SHARED, 
		TEXTUREFLAGS_CLAMPS | TEXTUREFLAGS_CLAMPT,
		CREATERENDERTARGETFLAGS_HDR );
  
}

ITexture* CTNERenderTargets::CreateCameraScreenTexture( IMaterialSystem* pMaterialSystem )
{
	//DevMsg("Creating Camera Screen Render Target: _rt_Scope\n");
	return pMaterialSystem->CreateNamedRenderTargetTextureEx2(
		CAMERA_SCREEN_MATERIAL,
		128, 64, RT_SIZE_OFFSCREEN,
		pMaterialSystem->GetBackBufferFormat(),
		MATERIAL_RT_DEPTH_SHARED, 
		TEXTUREFLAGS_CLAMPS | TEXTUREFLAGS_CLAMPT,
		CREATERENDERTARGETFLAGS_HDR );
  
}

//-----------------------------------------------------------------------------
// Purpose: Called by the engine in material system init and shutdown.
//			Clients should override this in their inherited version, but the base
//			is to init all standard render targets for use.
// Input  : pMaterialSystem - the engine's material system (our singleton is not yet inited at the time this is called)
//			pHardwareConfig - the user hardware config, useful for conditional render target setup
//-----------------------------------------------------------------------------
void CTNERenderTargets::InitClientRenderTargets( IMaterialSystem* pMaterialSystem, IMaterialSystemHardwareConfig* pHardwareConfig )
{ 
	m_ManhackScreenTexture.Init( CreateManhackScreenTexture( pMaterialSystem ) ); 
	m_CameraScreenTexture.Init( CreateCameraScreenTexture( pMaterialSystem ) ); 

	// Water effects & camera from the base class (standard HL2 targets) 
	BaseClass::InitClientRenderTargets( pMaterialSystem, pHardwareConfig );	//TERO: not sure if we need this
}
  
//-----------------------------------------------------------------------------
// Purpose: Shut down each CTextureReference we created in InitClientRenderTargets.
//			Called by the engine in material system shutdown.
// Input  :  - 
//-----------------------------------------------------------------------------
void CTNERenderTargets::ShutdownClientRenderTargets()
{ 
	m_ManhackScreenTexture.Shutdown();
	m_CameraScreenTexture.Shutdown();
  
	// Clean up standard HL2 RTs (camera and water) 
	BaseClass::ShutdownClientRenderTargets();
}
 
//add the interface!
static CTNERenderTargets g_TNERenderTargets;
EXPOSE_SINGLE_INTERFACE_GLOBALVAR( CTNERenderTargets, IClientRenderTargets, CLIENTRENDERTARGETS_INTERFACE_VERSION, g_TNERenderTargets  );
CTNERenderTargets* TNERenderTargets = &g_TNERenderTargets;
