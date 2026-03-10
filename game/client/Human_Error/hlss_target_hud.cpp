//=================== Half-Life 2: Short Stories Mod 2007 =====================//
//
// Purpose:	Manhack vehicle, see weapon_manhack
//			CONTROL. WE HAVE IT.
//
//=============================================================================//

#include "cbase.h"
#include "movevars_shared.h"			
#include "ammodef.h"
#include <vgui_controls/Controls.h>
#include <Color.h>

//hud:
#include <KeyValues.h>
#include <vgui/ISurface.h>
#include <vgui/ISystem.h>
#include <vgui/IVGui.h>
#include "view_scene.h"

//#include "Human_Error\hlss_target_hud.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

void HLSS_DrawTargetHud(Vector vecOrigin, C_BasePlayer *pPlayer, C_BaseEntity *pTarget)
{
	MDLCACHE_CRITICAL_SECTION();

	CHudTexture *pIconUpper = gHUD.GetIcon( "manhack_upper"  );
	CHudTexture *pIconLower = gHUD.GetIcon( "manhack_lower" );

	if (!pIconUpper || !pIconLower)
		return;

	if( !pPlayer || !pTarget)
		return;

	pIconUpper->EffectiveHeight(0.25f);
	pIconUpper->EffectiveWidth(0.25f);
	pIconLower->EffectiveHeight(0.25f);
	pIconLower->EffectiveWidth(0.25f);

	float flUpperX = pIconUpper->Width();
	float flUpperY = pIconUpper->Height();
	float flLowerX = pIconLower->Width();
	float flLowerY = pIconLower->Height();

	float x = ScreenWidth()/2;
	float y = ScreenHeight()/2;

	Vector vecStart = pTarget->WorldSpaceCenter();

	//Vector vecSize = m_hTarget->World //WorldAlignSize();
	Vector vecTest, screen;

	pTarget->GetRenderBounds(vecTest, screen);
 
	float height = screen.z - vecTest.z; //vecSize.z;
		
	Vector vecPlayerRight;
	pPlayer->GetVectors(NULL, &vecPlayerRight, NULL);


	//TERO: lets fix the width

	Vector vecEnemyForward, vecEnemy;

	pTarget->GetVectors(&vecEnemyForward, NULL, NULL);
	vecEnemy = (vecOrigin - vecStart);
	VectorNormalize(vecEnemy);

	float flDot = abs(DotProduct(vecEnemy, vecEnemyForward));

	float width = ((screen.y - vecTest.y) * flDot) + ((screen.x - vecTest.x) * (1.0f - flDot));
	//float width = (vecSize.y * flDot) + (vecSize.x * (1.0f - flDot));*/

	vecTest = vecStart + Vector(0, 0, height/2) - (vecPlayerRight * width * 0.5f);
	ScreenTransform( vecTest, screen );
	float x1 = x + (0.5 * screen[0] * ScreenWidth() + 0.5);
	float y1 = y - (0.5 * screen[1] * ScreenHeight() + 0.5);

	vecTest = vecStart + Vector(0, 0, height/2) + (vecPlayerRight * width * 0.5f);
	ScreenTransform( vecTest, screen );
	float x2 = x + (0.5 * screen[0] * ScreenWidth() + 0.5);
	float y2 = y - (0.5 * screen[1] * ScreenHeight() + 0.5);

	vecTest = vecStart - Vector(0, 0, height/2) - (vecPlayerRight * width * 0.5f);
	ScreenTransform( vecTest, screen );
	float x3 = x + (0.5 * screen[0] * ScreenWidth() + 0.5);
	float y3 = y - (0.5 * screen[1] * ScreenHeight() + 0.5);

	vecTest = vecStart - Vector(0, 0, height/2) + (vecPlayerRight * width * 0.5f);
	ScreenTransform( vecTest, screen );
	float x4 = x + (0.5 * screen[0] * ScreenWidth() + 0.5);
	float y4 = y - (0.5 * screen[1] * ScreenHeight() + 0.5);

	vgui::surface()->DrawSetColor(255, 0, 0, 255);

	vgui::surface()->DrawLine( x1 + flUpperX, y1, x2, y2);
	vgui::surface()->DrawLine( x1, y1 + flUpperY, x3, y3);
	vgui::surface()->DrawLine( x2, y2, x4, y4 - flLowerY);
	vgui::surface()->DrawLine( x3, y3, x4 - flLowerX, y4);

	Color color(255,255,255,255);

	pIconUpper->DrawSelf( x1, y1, color);
	pIconLower->DrawSelf( x4 - flLowerX, y4 - flLowerY, color);
}