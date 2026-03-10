//=================== Half-Life 2: Short Stories Mod 2009 =====================//
//
// Purpose:	Combine Air Defense system, hackable
//
//=============================================================================//


#ifndef NPC_COMBINE_AIR_DEFENSE
#define NPC_COMBINE_AIR_DEFENSE

#include "cbase.h"
#include "ai_basenpc.h"
#include "rope_shared.h"
#include "rope.h"
#include "globalstate.h"
#include "physics_prop_ragdoll.h"
#include "physics_bone_follower.h"

#define	AIR_DEFENSE_MODEL			"models/props_combine/airdefense.mdl"
#define AIR_DEFENSE_GLOW_SPRITE		"sprites/glow1.vmt"
#define AIR_DEFENSE_BC_YAW			"aim_yaw"
#define AIR_DEFENSE_BC_PITCH		"aim_pitch"
#define	AIR_DEFENSE_RANGE			4096 //3072
#define AIR_DEFENSE_MIN_DISTANCE	64.0f
#define AIR_DEFENSE_SPREAD			VECTOR_CONE_2DEGREES
#define	AIR_DEFENSE_MAX_WAIT		10
#define	AIR_DEFENSE_PING_TIME		2.0f	//LPB!!

#define	AIR_DEFENSE_VOICE_PITCH_LOW		15
#define	AIR_DEFENSE_VOICE_PITCH_HIGH	45

//Aiming variables
#define	AIR_DEFENSE_MAX_NOHARM_PERIOD	0.0f
#define	AIR_DEFENSE_MAX_GRACE_PERIOD	3.0f

#define SF_AIR_DEFENSE_AUTO_ACTIVATE				( 1 << 15 )			// Auto Activate
#define SF_AIR_DEFENSE_START_ACTIVE					( 1 << 16 )			// Start Active
#define SF_AIR_DEFENSE_NO_MACHINE_GUN				( 1 << 17 )			// Can't shoot bullets
#define SF_AIR_DEFENSE_NO_ROCKETS					( 1 << 18 )			// Can't shoot rockets
#define SF_AIR_DEFENSE_NEVERRETIRE					( 1 << 19 )			// Never go inactive, once gone active
#define SF_AIR_DEFENSE_RECHARGE_HUD					( 1 << 20 )			// Before going Active or Auto Active, you need to recharge

#define AIR_DEFENSE_SOUND_SHOOT "Airboat.FireGunLoop" //"NPC_AttackHelicopter.FireGun" //"NPC_CeilingTurret.ShotSounds"
#define AIR_DEFENSE_SOUND_RECHARGE "SuitRecharge.ChargingLoop"

#define AIRDEFENSE_USE_VPHYSICS

//Turret states
enum airDefenseState_e
{
	AIR_DEFENSE_SEARCHING,
	AIR_DEFENSE_AUTO_SEARCHING,
	AIR_DEFENSE_ACTIVE,
	AIR_DEFENSE_DEPLOYING,
	AIR_DEFENSE_RETIRING,
	AIR_DEFENSE_RECHARGE,
	AIR_DEFENSE_DEAD,
};

//Eye states
enum eyeState_t
{
	AIR_DEFENSE_EYE_SEE_TARGET,			//Sees the target, bright and big
	AIR_DEFENSE_EYE_SEEKING_TARGET,		//Looking for a target, blinking (bright)
	AIR_DEFENSE_EYE_DORMANT,				//Not active
	AIR_DEFENSE_EYE_DEAD,				//Completely invisible
	AIR_DEFENSE_EYE_DISABLED,			//Turned off, must be reactivated before it'll deploy again (completely invisible)
};

enum chargeState
{
	AIR_DEFENSE_CHARGE_ZERO,
	AIR_DEFENSE_RECHARGING,
	AIR_DEFENSE_CHARGE_FULL,
	AIR_DEFENSE_DECHARGING,
};

#define AIR_DEFENSE_RECHARGE_STEPS 3

#define AIR_DEFENSE_DEFAULT_RECHARGE_SPEED 30.0f
#define AIR_DEFENSE_DEFAULT_DECHARGE_SPEED 60.0f

#define AIR_DEFENSE_ROCKETS_MINIMUN_DISTANCE 128.0f
#define AIR_DEFENSE_ROCKETS_INITIAL_DELAY 3.0f
#define AIR_DEFENSE_ROCKETS_DELAY 0.5f
#define AIR_DEFENSE_ROCKETS 3

class CNPC_CombineAirDefense : public CAI_BaseNPC
{
	DECLARE_CLASS( CNPC_CombineAirDefense, CAI_BaseNPC );
public:
	
	CNPC_CombineAirDefense( void );
	~CNPC_CombineAirDefense( void );

	void	Precache( void );
	void	Spawn( void );
	void	Activate( void );

	void	StopLoopingSounds();
	void	PlayFireSound();
	void	StopFireSound();
	void	PlayRechargeSound(bool bRecharge);
	void	StopRechargeSound();

	// Think functions
	void	Retire( void );
	void	Deploy( void );
	void	ActiveThink( void );
	void	SearchThink( void );
	void	AutoSearchThink( void );
	void	DeathThink( void );
	void	DechargeThink( void );
	void	RechargeThink( void );

	void	DechargeActivity( void );

	// Updates

	void	UpdateRechargerLights();

	// Inputs
	void	InputToggle( inputdata_t &inputdata );
	void	InputEnable( inputdata_t &inputdata );
	void	InputDisable( inputdata_t &inputdata );
	void	InputRecharge( inputdata_t &inputdata );
	void	InputDecharge( inputdata_t &inputdata );
	void	InputRechargeSpeed( inputdata_t &inputdata );
	void	InputDechargeSpeed( inputdata_t &inputdata );

	void	SetLastSightTime();
	
	float	MaxYawSpeed( void );

	bool	IsUnreachable(CBaseEntity *pEntity) { return false; }

	int		GetSoundInterests( void );

	int		OnTakeDamage( const CTakeDamageInfo &inputInfo );

	Class_T	Classify( void );
	virtual Disposition_t IRelationType( CBaseEntity *pTarget );

	Vector	EyeOffset( Activity nActivity ) 
	{
		Vector vecEyeOffset(0,0,120);
		//GetEyePosition( GetModelPtr(), vecEyeOffset );
		return vecEyeOffset;
	}

	Vector	EyePosition( void )
	{
		return GetAbsOrigin() + Vector(0,0,120); //EyeOffset(GetActivity());
	}

	Vector	GetAttackSpread( CBaseCombatWeapon *pWeapon, CBaseEntity *pTarget ) 
	{
		return VECTOR_CONE_3DEGREES * ((CBaseHLCombatWeapon::GetDefaultProficiencyValues())[ WEAPON_PROFICIENCY_PERFECT ].spreadscale);
	}

	//TERO: durrrr
	bool		FInViewCone( CBaseEntity *pEntity );
	bool		FInViewCone( const Vector &vecSpot );
	bool		IsValidEnemy( CBaseEntity *pTarget );
	bool		FVisible( CBaseEntity *pEntity, int traceMask = MASK_OPAQUE, CBaseEntity **ppBlocker = NULL );

	virtual QAngle	BodyAngles()
	{
		Vector	vecMuzzle;
		QAngle	vecMuzzleAng;

		GetAttachment( "eyes", vecMuzzle, vecMuzzleAng );
		vecMuzzleAng.z = vecMuzzleAng.x = 0.0f;
		return vecMuzzleAng;
	}

	virtual void	UpdateOnRemove( void );

#ifdef AIRDEFENSE_USE_VPHYSICS
	virtual bool	CreateVPhysics( void );

private:
	CBoneFollowerManager	m_BoneFollowerManager;
	void			InitBoneFollowers( void );
#endif

protected:
	
	bool	PreThink( airDefenseState_e state );
	void	Shoot();
	void	SetEyeState( eyeState_t state );
	void	Ping( void );	
	void	Toggle( void );
	void	Enable( void );
	void	Disable( void );
	void	SpinUp( void );
	void	SpinDown( void );

	bool	UpdateFacing( void );

	int		m_iAmmoType;
	int		m_iMinHealthDmg;

	bool	m_bAutoStart;
	bool	m_bActive;		//Denotes the turret is deployed and looking for targets
	bool	m_bBlinkState;
	bool	m_bEnabled;		//Denotes whether the turret is able to deploy or not
	
	float	m_flShotTime;
	float	m_flLastSight;
	float	m_flPingTime;

	int		m_iRightMuzzle;
	int		m_iLeftMuzzle;

	float	m_flShotRocketTime;
	float	m_iNumberOfRockets;

	int		m_iRightRocketAttachment;
	int		m_iLeftRocketAttachment;
	bool	m_bRightRocket;

	float	m_flNextChargeTime;
	float	m_flRechargeSpeed;
	float	m_flDechargeSpeed;
	int		m_iRechargeLevel;
	int		m_iRechargeState;

	bool	m_bShouldUpdateHud;

	bool	m_bIsPlayingShootingSound;

	bool	CanShootBullets() { return (!HasSpawnFlags(SF_AIR_DEFENSE_NO_MACHINE_GUN)); } 
	bool	CanShootRockets() { return (!HasSpawnFlags(SF_AIR_DEFENSE_NO_ROCKETS)); }
	//bool	ShouldRecharge()  { return HasSpawnFlags(SF_AIR_DEFENSE_RECHARGE); }
	bool	ChargeIsDone();

	bool	ShootRockets();

	EHANDLE	m_hLaserDot;

	void	CreateAPCLaserDot();;

	QAngle	m_vecGoalAngles;

	CSprite	*m_pEyeGlow;

	CHandle<CRopeKeyframe>	m_hRope1;
	CHandle<CRopeKeyframe>	m_hRope2;

	COutputEvent m_OnDeploy;
	COutputEvent m_OnRetire;
	COutputEvent m_OnTipped;
	COutputEvent m_OnRechargeComplete;

		// Sounds
	CSoundPatch	*m_pFireSound;
	CSoundPatch *m_pRechargeSound;

	DECLARE_DATADESC();
};

#endif