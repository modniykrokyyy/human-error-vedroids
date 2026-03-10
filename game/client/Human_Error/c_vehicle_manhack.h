//=================== Half-Life 2: Short Stories Mod 2007 =====================//
//
// Purpose:	Manhack vehicle, see weapon_manhack
//			CONTROL. WE HAVE IT.
//
//=============================================================================//

#ifndef C_VEHICLE_MANHACK_H
#define C_VEHICLE_MANHACK_H

#include "cbase.h"
#include "c_prop_vehicle.h"
#include "hud.h"		
#include "c_physicsprop.h"		
#include "iclientvehicle.h"
#include "ammodef.h"
#include <vgui_controls/Controls.h>
#include "colorcorrectionmgr.h"


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class C_PropVehicleManhack : public C_BaseAnimating, public IClientVehicle
{
	DECLARE_CLASS( C_PropVehicleManhack, C_BaseAnimating );

public:

	DECLARE_CLIENTCLASS();
	DECLARE_DATADESC();

	C_PropVehicleManhack();
	~C_PropVehicleManhack();

	void ClientThink();
	void OnDataChanged(DataUpdateType_t updateType);
	
	void PreDataUpdate( DataUpdateType_t updateType );
	void PostDataUpdate( DataUpdateType_t updateType );

	void UpdateOnRemove( void );

public:

	// IClientVehicle overrides.
	virtual void GetVehicleViewPosition( int nRole, Vector *pOrigin, QAngle *pAngles, float *pFOV = NULL );
	virtual void GetVehicleFOV( float &flFOV ) { flFOV = 0.0f; }
	virtual void DrawHudElements();
	virtual bool IsPassengerUsingStandardWeapons( int nRole = VEHICLE_ROLE_DRIVER ) { return false; }

	virtual void UpdateViewAngles( C_BasePlayer *pLocalPlayer, CUserCmd *pCmd );

	void DampenEyePosition( Vector &vecVehicleEyePos, QAngle &vecVehicleEyeAngles );
	void OnEnteredVehicle( C_BasePlayer *pPlayer );

private:

	void DampenForwardMotion( Vector &vecVehicleEyePos, QAngle &vecVehicleEyeAngles, float flFrameTime );
	void ComputePDControllerCoefficients( float *pCoefficientsOut, float flFrequency, float flDampening, float flDeltaTime );

public:
	
	virtual C_BaseCombatCharacter* GetPassenger( int nRole );
	virtual int	GetPassengerRole( C_BaseCombatCharacter *pEnt );

	virtual void GetVehicleClipPlanes( float &flZNear, float &flZFar ) const;

	virtual int GetPrimaryAmmoType() const;
	virtual int GetPrimaryAmmoCount() const;
	virtual int GetPrimaryAmmoClip() const;
	virtual bool PrimaryAmmoUsesClips() const { return false; }

	virtual int GetJoystickResponseCurve() const { return 0; }

public:

	// C_BaseEntity overrides.
	virtual IClientVehicle*	GetClientVehicle() { return this; }
	virtual C_BaseEntity	*GetVehicleEnt() { return this; }
	virtual void SetupMove( C_BasePlayer *player, CUserCmd *ucmd, IMoveHelper *pHelper, CMoveData *move ) {}
	virtual void ProcessMovement( C_BasePlayer *pPlayer, CMoveData *pMoveData ) {}
	virtual void FinishMove( C_BasePlayer *player, CUserCmd *ucmd, CMoveData *move ) {}
	virtual bool IsPredicted() const { return false; }
	virtual void ItemPostFrame( C_BasePlayer *pPlayer ) {}
	virtual bool IsSelfAnimating() { return false; };

	//virtual void GetRenderBounds( Vector& theMins, Vector& theMaxs );

private:

	CHandle<C_BasePlayer>	m_hPlayer;
	CHandle<C_BasePlayer>	m_hPrevPlayer;
	CHandle<C_BaseEntity>	m_hTarget;

	int						m_iTargetType;

	bool					m_bEnterAnimOn;
	bool					m_bExitAnimOn;
	Vector					m_vecEyeExitEndpoint;
	Vector					m_vecManhackEye;
	Vector					m_vecFlyingDirection;
	QAngle					m_angManhackEye;

	ViewSmoothingData_t		m_ViewSmoothingData;

	Vector		m_vecLastEyePos;
	Vector		m_vecLastEyeTarget;
	Vector		m_vecEyeSpeed;

	int m_iManhackHealth;
	int	m_iManhackDistance;

	float m_flStopDamageFov;
	int m_iLastManhackHealth;

	ClientCCHandle_t m_CCHandle;
};


#endif
