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

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//-----------------------------------------------------------------------------
//
// In-game vgui panel which shows the RPG's ammo count
//
//-----------------------------------------------------------------------------
class CManhackScreen : public CVGuiScreenPanel
{
    DECLARE_CLASS( CManhackScreen, CVGuiScreenPanel );

public:
    CManhackScreen( vgui::Panel *parent, const char *panelName );
	~CManhackScreen();

    virtual bool Init( KeyValues* pKeyValues, VGuiScreenInitData_t* pInitData );
    virtual void OnTick();
	void		 SetManhackDistance(int iDistance);
private:
    vgui::Label *m_pManhackCount;
	vgui::Label *m_pManhackDistance;
	vgui::Label *m_pManhackOnline;
	int			m_iManhackDistance;
};

extern CManhackScreen	*GetManhackScreen();
