//=================== Half-Life 2: Short Stories Mod 2007 =====================//
//
// Purpose:	Manhack vehicle, see weapon_manhack
//			CONTROL. WE HAVE IT.
//
//=============================================================================//


#ifndef VEHICLE_MANHACK_H
#define VEHICLE_MANHACK_H

#include "cbase.h"
#include "ai_basenpc.h" //AIHANDLE
#include "vehicle_base.h"
#include "hl2_player.h"
#include "npc_manhack.h"
#include "physobj.h"

class CPropVehicleManhack;

#define NUMBER_OF_MAX_CONTROLLABLE_MANHACKS 3

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CPropServerVehicleManhack : public CBaseServerVehicle
{
	typedef CBaseServerVehicle BaseClass;

// IServerVehicle
public:
	void GetVehicleViewPosition( int nRole, Vector *pAbsOrigin, QAngle *pAbsAngles, float *pFOV = NULL );

	// NPC Driving
	void	NPC_SetDriver( CNPC_VehicleDriver *pDriver ) { };
	void	NPC_DriveVehicle( void ) { } ;

	virtual bool	IsPassengerEntering( void ) { return false; }	// NOTE: This mimics the scenario HL2 would have seen
	virtual bool	IsPassengerExiting( void ) { return false; }

protected:

	CPropVehicleManhack *GetManhack( void );
};

class CPropVehicleManhack :  public CBaseProp, public IDrivableVehicle
{
	DECLARE_CLASS( CPropVehicleManhack, CBaseProp );
public:
	CPropVehicleManhack *m_pNext;

	static CPropVehicleManhack *GetManhackVehicle();


	DECLARE_DATADESC();
	DECLARE_SERVERCLASS();

	CPropVehicleManhack( void );

	~CPropVehicleManhack( void );

	CNPC_Manhack *GetManhack();

	// CBaseEntity
	void			Spawn( void ); 
	void			Precache( void );
	void			Think(void);

	virtual int		ObjectCaps( void ) { return BaseClass::ObjectCaps() | FCAP_IMPULSE_USE; };
	virtual void	TraceAttack( const CTakeDamageInfo &info, const Vector &vecDir, trace_t *ptr );
	virtual int		OnTakeDamage( const CTakeDamageInfo &info );

	void			PlayerControlInit( CBasePlayer *pPlayer );
	void			PlayerControlShutdown( void );

	virtual void	DrawDebugGeometryOverlays( void );

	//void			Activate( void );

	bool			CreateControllableManhack( const Vector &position, const QAngle &angles, CBaseEntity *pOwner );
	bool			HasNPCManhack() 
					{ 
						if (m_hManhack!=NULL) 
							return true;
						
						return false;
					}

	bool			HasDrivableManhack(CBasePlayer *pPlayer);

	// CPropVehicle

	//virtual void		DriveVehicle( float flFrameTime, CUserCmd *ucmd, int iButtonsDown, int iButtonsReleased );
	void				DriveManhack(CBasePlayer *player, int nButtons );

	void				CallManhacksBack(CBasePlayer *Player, float flComeBackTime);
	void				TellManhacksToGoThere(CBasePlayer *pPlayer, float flGoThereTime);

	// Inputs to force the player in/out of the vehicle
	void				ForcePlayerIn( CBaseEntity *pOwner );
	void				ForcePlayerOut( );

	virtual void		Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	void				ResetUseKey( CBasePlayer *pPlayer );

	virtual Vector		BodyTarget( const Vector &posSrc, bool bNoisy = true );

	void				UpdateManhackView();
	int					UpdateManhackDistance(CBaseEntity *pOrigin);
	void				FixManhackView();
	void				UpdateHead();

	void				SetLocator( CBaseEntity *pEntity );

	// IDrivableVehicle
	virtual CBaseEntity *GetDriver( void );
	virtual void		ItemPostFrame( CBasePlayer *pPlayer );

	virtual void		SetupMove( CBasePlayer *player, CUserCmd *ucmd, IMoveHelper *pHelper, CMoveData *move );
	virtual void		ProcessMovement( CBasePlayer *pPlayer, CMoveData *pMoveData ) { return; }
	virtual void		FinishMove( CBasePlayer *player, CUserCmd *ucmd, CMoveData *move ) { return; }

	virtual void	DampenEyePosition( Vector &vecVehicleEyePos, QAngle &vecVehicleEyeAngles );

	virtual bool		CanEnterVehicle( CBaseEntity *pEntity );
	virtual bool		CanExitVehicle( CBaseEntity *pEntity );
	virtual void		SetVehicleEntryAnim( bool bOn ) {  }
	virtual void		SetVehicleExitAnim( bool bOn, Vector vecEyeExitEndpoint ) { }

	virtual bool		AllowBlockedExit( CBaseCombatCharacter *pPassenger, int nRole ) { return true; }
	virtual bool		AllowMidairExit( CBaseCombatCharacter *pPassenger, int nRole ) { return false; }
	virtual void		PreExitVehicle( CBaseCombatCharacter *pPassenger, int nRole );

	virtual void		EnterVehicle( CBaseCombatCharacter *pPassenger );
	virtual void		ExitVehicle( int nRole );

	void				GetVectors(Vector* pForward, Vector* pRight, Vector* pUp) const;

	virtual string_t	GetVehicleScriptName() { return m_vehicleScript; }

	virtual bool		PassengerShouldReceiveDamage( CTakeDamageInfo &info ) { return true; }

	int					GetNumberOfHacks(bool bReleaseSlot);

	// If this is a vehicle, returns the vehicle interface
	virtual IServerVehicle *GetServerVehicle() { return &m_ServerVehicle; }

	void				SetNPCDriver( CNPC_VehicleDriver *pDriver ) { };

private:

	void		ComputePDControllerCoefficients( float *pCoefficientsOut, float flFrequency, float flDampening, float flDeltaTime );
	void		DampenForwardMotion( Vector &vecVehicleEyePos, QAngle &vecVehicleEyeAngles, float flFrameTime );
	void		DampenUpMotion( Vector &vecVehicleEyePos, QAngle &vecVehicleEyeAngles, float flFrameTime );

protected:

	// Contained IServerVehicle
	CPropServerVehicleManhack  m_ServerVehicle;

	

	Vector			m_vecLastEyePos;
	Vector			m_vecLastEyeTarget;
	Vector			m_vecEyeSpeed;
	Vector			m_vecTargetSpeed;

public:

	Vector			GetManhackEyePosition() { return m_vecManhackEye; }

private:

	CNetworkHandle( CBasePlayer, m_hPlayer );

	CNetworkHandle( CBaseEntity,	m_hTarget );
	CNetworkVar( int, m_iTargetType );
	
	CNetworkQAngle(		m_angManhackEye );
	CNetworkVector(		m_vecManhackEye );
	CNetworkVector(		m_vecFlyingDirection );

	CHandle<CNPC_Manhack>		m_hManhack;

	CHandle<CNPC_Manhack>		m_hSetOfManhacks[NUMBER_OF_MAX_CONTROLLABLE_MANHACKS];
	int				m_iNumberOfManhacks;

	int				m_iCurrentManhackIndex;
	float			m_fTimeToChangeManhack;

	bool			FindNextManhack(bool bRemoveIfNone = false);
	void			DestroyAllManhacks();

	int				m_iHintTimesShown;
	int				m_iHintNoSwapTimesShown;

	//int				m_iManhackHealth;
	CNetworkVar( int, m_iManhackHealth );
	CNetworkVar( int, m_iManhackDistance );
	//CNetworkVar( int, m_iManhackDistances, NUMBER_OF_MAX_CONTROLLABLE_MANHACKS );

	float			m_flLastWarningSound;

	string_t			m_vehicleScript;


	bool m_bDriverDucked;
	bool m_bHadDriver;

};

CPropVehicleManhack *VehicleManhack_Create( const Vector &position, const QAngle &angles, CBaseEntity *pOwner  );

#endif //VEHICLE_MANHACK_H