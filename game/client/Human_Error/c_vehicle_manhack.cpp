//=================== Half-Life 2: Short Stories Mod 2007 =====================//
//
// Purpose:	Manhack vehicle, see weapon_manhack
//			CONTROL. WE HAVE IT.
//
//=============================================================================//

#include "cbase.h"
#include "movevars_shared.h"
#include "c_prop_vehicle.h"
#include "c_vehicle_manhack.h"
#include "hud.h"		
#include "c_physicsprop.h"		
#include "iclientvehicle.h"
#include "ammodef.h"
#include <vgui_controls/Controls.h>
#include <Color.h>

//hud:
#include <KeyValues.h>
#include <vgui/ISurface.h>
#include <vgui/ISystem.h>
#include <vgui/IVGui.h>
#include "view_scene.h"

#include "Human_Error/hud_radio.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

extern float RemapAngleRange( float startInterval, float endInterval, float value );

void HLSS_DrawTargetHud(Vector vecOrigin, C_BasePlayer *pPlayer, C_BaseEntity *pTarget, int iEnemyType);

//extern static ConVar mat_colcorrection_disableentities;

#define MANHACK_DELTA_LENGTH_MAX	24.0f			// 1 foot
#define MANHACK_FRAMETIME_MIN		1e-6

#define MANHACK_FOV_NORMAL 50
#define MANHACK_FOV_DAMAGE 60
#define MANHACK_DAMAGE_FOV_TIME 0.2f

IMPLEMENT_CLIENTCLASS_DT(C_PropVehicleManhack, DT_PropVehicleManhack, CPropVehicleManhack)
	RecvPropEHandle( RECVINFO(m_hPlayer) ),
	RecvPropEHandle( RECVINFO(m_hTarget) ),
	RecvPropInt( RECVINFO(m_iTargetType) ),
	RecvPropQAngles( RECVINFO( m_angManhackEye ) ),
	RecvPropVector( RECVINFO( m_vecManhackEye ) ),
	RecvPropVector( RECVINFO( m_vecFlyingDirection ) ),
	RecvPropInt( RECVINFO( m_iManhackHealth ) ),
	RecvPropInt( RECVINFO( m_iManhackDistance ) ),
END_RECV_TABLE()

BEGIN_DATADESC( C_PropVehicleManhack )
	DEFINE_EMBEDDED( m_ViewSmoothingData ),
END_DATADESC()


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
C_PropVehicleManhack::C_PropVehicleManhack( void )
{
	memset( &m_ViewSmoothingData, 0, sizeof( m_ViewSmoothingData ) );

	m_ViewSmoothingData.pVehicle = this;
	m_ViewSmoothingData.bClampEyeAngles = false;
	m_ViewSmoothingData.bDampenEyePosition = true;

	m_ViewSmoothingData.flFOV = MANHACK_FOV_NORMAL;

	m_iManhackHealth = 0;
	m_iManhackDistance = 100;

	m_flStopDamageFov = 0;
	m_iLastManhackHealth = 0;

	m_CCHandle = INVALID_CLIENT_CCHANDLE;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
C_PropVehicleManhack::~C_PropVehicleManhack( void )
{
	g_pColorCorrectionMgr->RemoveColorCorrection( m_CCHandle );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : updateType - 
//-----------------------------------------------------------------------------
void C_PropVehicleManhack::PreDataUpdate( DataUpdateType_t updateType )
{
	BaseClass::PreDataUpdate( updateType );

	m_hPrevPlayer = m_hPlayer;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_PropVehicleManhack::PostDataUpdate( DataUpdateType_t updateType )
{
	BaseClass::PostDataUpdate( updateType );

	if ( !m_hPlayer && m_hPrevPlayer )
	{
		// They have just exited the vehicle.
		// Sometimes we never reach the end of our exit anim, such as if the
		// animation doesn't have fadeout 0 specified in the QC, so we fail to
		// catch it in VehicleViewSmoothing. Catch it here instead.
		//m_ViewSmoothingData.bWasRunningAnim = false;

		if ( m_CCHandle != INVALID_CLIENT_CCHANDLE )
		{
			g_pColorCorrectionMgr->SetColorCorrectionWeight( m_CCHandle, 0.0f );
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
C_BaseCombatCharacter *C_PropVehicleManhack::GetPassenger( int nRole )
{
	if ( nRole == VEHICLE_ROLE_DRIVER )
		return m_hPlayer.Get();

	return NULL;
}


//-----------------------------------------------------------------------------
// Returns the role of the passenger
//-----------------------------------------------------------------------------
int	C_PropVehicleManhack::GetPassengerRole( C_BaseCombatCharacter *pPassenger )
{
	if ( m_hPlayer.Get() == pPassenger )
		return VEHICLE_ROLE_DRIVER;

	return VEHICLE_ROLE_NONE;
}


//-----------------------------------------------------------------------------
// Purpose: Modify the player view/camera while in a vehicle
//-----------------------------------------------------------------------------
void C_PropVehicleManhack::GetVehicleViewPosition( int nRole, Vector *pAbsOrigin, QAngle *pAbsAngles, float *pFOV )
{
	//c_prop_vehicle.cpp
	//ManhackVehicleViewSmoothing(m_hPlayer, pAbsOrigin, pAbsAngles, m_angManhackEye, m_vecManhackEye, &m_ViewSmoothingData, pFOV );

	matrix3x4_t vehicleEyePosToWorld;

	AngleMatrix( m_angManhackEye, vehicleEyePosToWorld );

	// Dampen the eye positional change as we drive around.
	*pAbsAngles = m_hPlayer->EyeAngles();

	DampenEyePosition( m_vecManhackEye, m_angManhackEye );
	
	// Compute the relative rotation between the unperturbed eye attachment + the eye angles
	matrix3x4_t cameraToWorld;
	AngleMatrix( *pAbsAngles, cameraToWorld );

	matrix3x4_t worldToEyePos;
	MatrixInvert( vehicleEyePosToWorld, worldToEyePos );

	matrix3x4_t vehicleCameraToEyePos;
	ConcatTransforms( worldToEyePos, cameraToWorld, vehicleCameraToEyePos );

	AngleMatrix( m_angManhackEye, m_vecManhackEye, vehicleEyePosToWorld );

	// Now treat the relative eye angles as being relative to this new, perturbed view position...
	matrix3x4_t newCameraToWorld;
	ConcatTransforms( vehicleEyePosToWorld, vehicleCameraToEyePos, newCameraToWorld );

	// output new view abs angles
	MatrixAngles( newCameraToWorld, *pAbsAngles );

	// UNDONE: *pOrigin would already be correct in single player if the HandleView() on the server ran after vphysics
	MatrixGetColumn( newCameraToWorld, 3, *pAbsOrigin );

	*pFOV = m_ViewSmoothingData.flFOV;
}


//-----------------------------------------------------------------------------
void C_PropVehicleManhack::DampenEyePosition( Vector &vecVehicleEyePos, QAngle &vecVehicleEyeAngles )
{
#ifdef HL2_CLIENT_DLL
	// Get the frametime. (Check to see if enough time has passed to warrent dampening).
	float flFrameTime = gpGlobals->frametime;

	if ( flFrameTime < 1e-6 ) //FRAMETIME MIN FOR JEEP = 1e-6
	{
		vecVehicleEyePos = m_vecLastEyePos;
		return;
	}

	// Keep static the sideways motion.
	// Dampen forward/backward motion.
	DampenForwardMotion( vecVehicleEyePos, vecVehicleEyeAngles, flFrameTime );

	// Blend up/down motion.
#endif
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void C_PropVehicleManhack::DampenForwardMotion( Vector &vecVehicleEyePos, QAngle &vecVehicleEyeAngles, float flFrameTime )
{
	// vecVehicleEyePos = real eye position this frame

	// m_vecLastEyePos = eye position last frame
	// m_vecEyeSpeed = eye speed last frame
	// vecPredEyePos = predicted eye position this frame (assuming no acceleration - it will get that from the pd controller).
	// vecPredEyeSpeed = predicted eye speed

	Vector vecForward = m_vecFlyingDirection;
	VectorNormalize(vecForward);

	Vector vecPredEyePos = m_vecLastEyePos + m_vecEyeSpeed * flFrameTime;
	Vector vecPredEyeSpeed = m_vecEyeSpeed;

	// m_vecLastEyeTarget = real eye position last frame (used for speed calculation).
	// Calculate the approximate speed based on the current vehicle eye position and the eye position last frame.
	Vector vecVehicleEyeSpeed = ( vecVehicleEyePos - m_vecLastEyeTarget ) / flFrameTime;
	m_vecLastEyeTarget = vecVehicleEyePos;
	if (vecVehicleEyeSpeed.Length() == 0.0)
		return;

	// Calculate the delta between the predicted eye position and speed and the current eye position and speed.
	Vector vecDeltaSpeed = vecVehicleEyeSpeed - vecPredEyeSpeed;
	Vector vecDeltaPos = vecVehicleEyePos - vecPredEyePos;

	float flDeltaLength = vecDeltaPos.Length();
	if ( flDeltaLength > MANHACK_DELTA_LENGTH_MAX )
	{
		// Clamp.
		float flDelta = flDeltaLength - MANHACK_DELTA_LENGTH_MAX;
		if ( flDelta > 40.0f )
		{
			// This part is a bit of a hack to get rid of large deltas (at level load, etc.).
			m_vecLastEyePos = vecVehicleEyePos;
			m_vecEyeSpeed = vecVehicleEyeSpeed;
		}
		else
		{
			// Position clamp.
			float flRatio = MANHACK_DELTA_LENGTH_MAX / flDeltaLength;
			vecDeltaPos *= flRatio;
			Vector vecForwardOffset = vecForward* ( vecForward.Dot( vecDeltaPos ) );
			vecVehicleEyePos -= vecForwardOffset;
			m_vecLastEyePos = vecVehicleEyePos;

			// Speed clamp.
			vecDeltaSpeed *= flRatio;
			float flCoefficients[2];
			ComputePDControllerCoefficients( flCoefficients, r_ManhackViewDampenFreq.GetFloat(), r_ManhackViewDampenDamp.GetFloat(), flFrameTime );
			m_vecEyeSpeed += ( ( flCoefficients[0] * vecDeltaPos + flCoefficients[1] * vecDeltaSpeed ) * flFrameTime );
		}
	}
	else
	{
		// Generate an updated (dampening) speed for use in next frames position prediction.
		float flCoefficients[2];
		ComputePDControllerCoefficients( flCoefficients, r_ManhackViewDampenFreq.GetFloat(), r_ManhackViewDampenDamp.GetFloat(), flFrameTime );
		m_vecEyeSpeed += ( ( flCoefficients[0] * vecDeltaPos + flCoefficients[1] * vecDeltaSpeed ) * flFrameTime );
		
		// Save off data for next frame.
		m_vecLastEyePos = vecPredEyePos;
		
		// Move eye forward/backward.
		Vector vecForwardOffset = vecForward * ( vecForward.Dot( vecDeltaPos ) );
		vecVehicleEyePos -= vecForwardOffset;
	}
}


//-----------------------------------------------------------------------------
// Use the controller as follows:
// speed += ( pCoefficientsOut[0] * ( targetPos - currentPos ) + pCoefficientsOut[1] * ( targetSpeed - currentSpeed ) ) * flDeltaTime;
//-----------------------------------------------------------------------------
void C_PropVehicleManhack::ComputePDControllerCoefficients( float *pCoefficientsOut,
												  float flFrequency, float flDampening,
												  float flDeltaTime )
{
	float flKs = 9.0f * flFrequency * flFrequency;
	float flKd = 4.5f * flFrequency * flDampening;

	float flScale = 1.0f / ( 1.0f + flKd * flDeltaTime + flKs * flDeltaTime * flDeltaTime );

	pCoefficientsOut[0] = flKs * flScale;
	pCoefficientsOut[1] = ( flKd + flKs * flDeltaTime ) * flScale;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_PropVehicleManhack::OnEnteredVehicle( C_BasePlayer *pPlayer )
{
	m_vecLastEyeTarget = m_vecManhackEye;
	Vector vecEyeAngles;
	AngleVectors(m_angManhackEye,&vecEyeAngles);
	m_vecLastEyePos = vecEyeAngles;
	m_vecEyeSpeed = vec3_origin;
}



//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pLocalPlayer - 
//			pCmd - 
//-----------------------------------------------------------------------------
void C_PropVehicleManhack::UpdateViewAngles( C_BasePlayer *pLocalPlayer, CUserCmd *pCmd )
{
	/*
	//int eyeAttachmentIndex = LookupAttachment( "Eyes" );
	Vector vehicleEyeOrigin;
	QAngle vehicleEyeAngles;
	//GetAttachmentLocal( eyeAttachmentIndex, vehicleEyeOrigin, vehicleEyeAngles );

	// Limit the yaw.
	//float flAngleDiff = AngleDiff( pCmd->viewangles.y, vehicleEyeAngles.y );
	//flAngleDiff = clamp( flAngleDiff, POD_VIEW_YAW_MIN, POD_VIEW_YAW_MAX );
	//pCmd->viewangles.y = m_angManhackEye.y; // + flAngleDiff;
	//pCmd->mousedx = 0;
	/*Vector *pAbsOrigin;
	QAngle *pAbsAngles;
	ManhackVehicleViewSmoothing( m_hPlayer, pAbsOrigin, pAbsAngles, m_angManhackEye, m_vecManhackEye, &m_ViewSmoothingData, &m_flFOV );
	pCmd->viewangles.y = pAbsAngles->y;*/

	// Limit the pitch -- don't let them look down into the empty pod!
	/*float flAngleDiff = AngleDiff( pCmd->viewangles.x, 0 );
	flAngleDiff = clamp( flAngleDiff, -10, 10 );
	pCmd->viewangles.x = 0 + flAngleDiff;
	//*/
} 


//-----------------------------------------------------------------------------
// Futzes with the clip planes
//-----------------------------------------------------------------------------
void C_PropVehicleManhack::GetVehicleClipPlanes( float &flZNear, float &flZFar ) const
{
	// Pod doesn't need to adjust the clip planes.
	flZNear = 6;
}

	
//-----------------------------------------------------------------------------
// Renders hud elements
//-----------------------------------------------------------------------------
void C_PropVehicleManhack::DrawHudElements( )
{
	HLSS_DrawTargetHud(m_vecManhackEye, m_hPlayer, m_hTarget, m_iTargetType);
}

/*void C_PropVehicleManhack::GetRenderBounds( Vector &theMins, Vector &theMaxs )
{
	// This is kind of hacky:( Add 660.0 to the y coordinate of the bounding box to
	// allow for the full extension of the crane arm.
	BaseClass::GetRenderBounds( theMins, theMaxs );
}*/

int C_PropVehicleManhack::GetPrimaryAmmoType() const
{
	if ( m_iManhackHealth < 0 )
		return -1;

	int nAmmoType = GetAmmoDef()->Index( "Manhack" );
	return nAmmoType; 
}

int C_PropVehicleManhack::GetPrimaryAmmoCount() const
{ 
	return m_iManhackHealth; 
}

int C_PropVehicleManhack::GetPrimaryAmmoClip() const
{ 
	return m_iManhackDistance; 
}

void C_PropVehicleManhack::ClientThink()
{
	if ( m_CCHandle == INVALID_CLIENT_CCHANDLE )
		return;

	/*if ( mat_colcorrection_disableentities.GetInt() )
	{
		// Allow the colorcorrectionui panel (or user) to turn off color-correction entities
		g_pColorCorrectionMgr->SetColorCorrectionWeight( m_CCHandle, 0.0f );
		return;
	}*/

	if( !m_hPlayer )
	{
		g_pColorCorrectionMgr->SetColorCorrectionWeight( m_CCHandle, 0.0f );
		return;
	}

	if ( m_iLastManhackHealth > m_iManhackHealth )
	{
		m_flStopDamageFov = gpGlobals->curtime + MANHACK_DAMAGE_FOV_TIME;
	}
	else if ( m_iLastManhackHealth < m_iManhackHealth )
	{
		m_flStopDamageFov = 0;
	}

	m_iLastManhackHealth = m_iManhackHealth;

	float flScale = clamp( fabsf(m_flStopDamageFov - gpGlobals->curtime) / MANHACK_DAMAGE_FOV_TIME, 0.0f, 1.0f);

	m_ViewSmoothingData.flFOV = (flScale * MANHACK_FOV_DAMAGE) + ((1.0f - flScale) * MANHACK_FOV_NORMAL);

	g_pColorCorrectionMgr->SetColorCorrectionWeight( m_CCHandle, 1.0f );

	BaseClass::ClientThink();
}

//------------------------------------------------------------------------------
// Purpose :
// Input   :
// Output  :
//------------------------------------------------------------------------------
void C_PropVehicleManhack::OnDataChanged(DataUpdateType_t updateType)
{
	BaseClass::OnDataChanged( updateType );

	if ( updateType == DATA_UPDATE_CREATED )
	{
		if ( m_CCHandle == INVALID_CLIENT_CCHANDLE )
		{

			m_CCHandle = g_pColorCorrectionMgr->AddColorCorrection( "correction/manhack.raw" );
			SetNextClientThink( ( m_CCHandle != INVALID_CLIENT_CCHANDLE ) ? CLIENT_THINK_ALWAYS : CLIENT_THINK_NEVER );
		}
	}
}

void C_PropVehicleManhack::UpdateOnRemove( void )
{
	if (m_CCHandle != INVALID_CLIENT_CCHANDLE)
	{
		g_pColorCorrectionMgr->SetColorCorrectionWeight( m_CCHandle, 0.0f );
		g_pColorCorrectionMgr->RemoveColorCorrection( m_CCHandle );
		m_CCHandle = INVALID_CLIENT_CCHANDLE;
	}

	BaseClass::UpdateOnRemove();
}


void HLSS_DrawTargetHud(Vector vecOrigin, C_BasePlayer *pPlayer, C_BaseEntity *pTarget, int iEnemyType)
{
	//MDLCACHE_CRITICAL_SECTION();

	CHudTexture *pIconUpper = gHUD.GetIcon( "manhack_upper"  );
	CHudTexture *pIconLower = gHUD.GetIcon( "manhack_lower" );

	if (!pIconUpper || !pIconLower)
		return;

	if( !pPlayer || !pTarget)
		return;

	float flBaseScale = 0.25f;

	pIconUpper->EffectiveHeight(flBaseScale);
	pIconUpper->EffectiveWidth(flBaseScale);
	pIconLower->EffectiveHeight(flBaseScale);
	pIconLower->EffectiveWidth(flBaseScale);

	float flUpperX = pIconUpper->Width();
	float flUpperY = pIconUpper->Height();
	float flLowerX = pIconLower->Width();
	float flLowerY = pIconLower->Height();

	float x = ScreenWidth()/2;
	float y = ScreenHeight()/2;

	Vector vecStart = pTarget->GetAbsOrigin(); //WorldSpaceCenter();

	//Vector vecSize = m_hTarget->World //WorldAlignSize();
	Vector vecTest, screen;

	pTarget->GetRenderBounds(vecTest, screen);
 
	float height = screen.z - vecTest.z; //vecSize.z;
		
	Vector vecPlayerRight;
	pPlayer->GetVectors(NULL, &vecPlayerRight, NULL);

	vecPlayerRight.z = 0;

	//TERO: lets fix the width

	Vector vecEnemyForward, vecEnemy;

	pTarget->GetVectors(&vecEnemyForward, NULL, NULL);
	vecEnemy = (vecOrigin - vecStart);
	VectorNormalize(vecEnemy);

	float flDot = fabsf(DotProduct(vecEnemy, vecEnemyForward));

	float width = ((screen.y - vecTest.y) * flDot) + ((screen.x - vecTest.x) * (1.0f - flDot)) + 2.0f; //TERO: add 2.0f to make it slighlty large than the NPC
	//float width = (vecSize.y * flDot) + (vecSize.x * (1.0f - flDot));*/

	vecTest = vecStart + Vector(0, 0, height + 1.0f) - (vecPlayerRight * width  * 0.5f);
	ScreenTransform( vecTest, screen );
	float x1 = x + (0.5 * screen[0] * ScreenWidth() + 0.5);
	float y1 = y - (0.5 * screen[1] * ScreenHeight() + 0.5);

	/*vecTest = vecStart + Vector(0, 0, height/2) + (vecPlayerRight * width * 0.5f);
	ScreenTransform( vecTest, screen );
	float x2 = x + (0.5 * screen[0] * ScreenWidth() + 0.5);
	float y2 = y - (0.5 * screen[1] * ScreenHeight() + 0.5);

	vecTest = vecStart - Vector(0, 0, height/2) - (vecPlayerRight * width * 0.5f);
	ScreenTransform( vecTest, screen );
	float x3 = x + (0.5 * screen[0] * ScreenWidth() + 0.5);
	float y3 = y - (0.5 * screen[1] * ScreenHeight() + 0.5);*/

	vecTest = vecStart - Vector(0,0,1) + (vecPlayerRight * width * 0.5f); //Vector(0,0,1) to make it slighlty larger than the NPC
	ScreenTransform( vecTest, screen );
	float x4 = x + (0.5 * screen[0] * ScreenWidth() + 0.5);
	float y4 = y - (0.5 * screen[1] * ScreenHeight() + 0.5);

	float flScaleUpper = min( (x4 - x1) / flUpperX, (y4 - y1) / flUpperY );
	float flScaleLower = min( (x4 - x1) / flLowerX, (y4 - y1) / flLowerY );

	//TERO: make sure the icons are not clipping
	/*if ((x2 - x1) >= flUpperX &&
		(y3 - y1) >= flUpperY &&
		(x4 - x3) >= flLowerX &&
		(y4 - y2) >= flLowerY)*/
	
	//DevMsg("flScaleUpper %f, flScaleLower %f\n", flScaleUpper, flScaleLower);

	if (flScaleUpper >= 0.16f && flScaleLower >= 0.16f)
	{
		if (flScaleUpper > 1.0f)
			flScaleUpper = 1.0f;

		if (flScaleLower > 1.0f)
			flScaleLower = 1.0f;

		Color color(0,0,0,64);

		vgui::surface()->DrawSetColor(color);
		vgui::surface()->DrawFilledRect(x1, y1, x4, y4);

		/*int px[4] = { x1, x4, x4, x1 };
		int py[4] = { y1, y1, y4, y4 };

		vgui::surface()->DrawPolyLine( px, py, 4);*/

		Color color2(255,255,255,196);

		pIconUpper->DrawSelf( x1, y1, (flUpperX * flScaleUpper), flUpperY * flScaleUpper, color2);
		pIconLower->DrawSelf( x4 - (flLowerX  * flScaleLower), y4 - (flLowerY * flScaleLower), (flLowerX  * flScaleLower), (flLowerY * flScaleLower), color2);

		vgui::HScheme scheme = vgui::scheme()->GetScheme( "ClientScheme" );
		vgui::HFont m_hFont = vgui::scheme()->GetIScheme(scheme)->GetFont( MAKE_STRING("Default") );

		wchar_t *unicode = L" "; //[14];

		int length = 8;

		switch ( iEnemyType )
		{
			case 1:
				unicode = L"ANTI-CITIZEN.";
				length = 13;
				//swprintf(unicode, L"ANTI-CITIZE");	
				break;
			case 2:
				unicode = L"XENOFORM.";
				length = 10;
				break;
			case 3:
				unicode = L"INFECTION.";
				length = 10;
				break;	
			case 4:
				unicode = L"HACKED.";
				length = 7;
				break;	
			default:
				length = 0;	
				break;	
		}

		/*int length = 14;
		for (int i=0; i<14; i++)
		{
			if (unicode[i] == L'')
			{
				length = i;
				break;
			}
		}*/

		//TERO: 8 is the length of "HOSTILE." 
		length = max(length, 8) + 1;

		float ypos = vgui::surface()->GetFontTall( m_hFont ) * 1.1f;
		float xpos = (surface()->GetCharacterWidth(m_hFont, '0') * length) + (flLowerX  * flScaleLower);

		if ((x4 - x1) >= xpos && (y4 - y1) >= ((2.0f * ypos) + (flUpperY * flScaleUpper)))
		{
			vgui::surface()->DrawSetTextFont( m_hFont );
			vgui::surface()->DrawSetTextColor(0, 220, 255, 196);

			x1 += surface()->GetCharacterWidth(m_hFont, '0');

			if (unicode[0] != L'\0')
			{
				vgui::surface()->DrawSetTextPos(x1, y4 - (ypos * 2.0f)); 
				vgui::surface()->DrawUnicodeString( unicode );
			}

			vgui::surface()->DrawSetTextPos(x1, y4 - ypos); 
			vgui::surface()->DrawUnicodeString( L"HOSTILE." );
		}
	}
}
