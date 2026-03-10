#ifndef HLSS_MANTARAY_H
#define HLSS_MANTARAY_H

#include	"cbase.h"
#include	"ai_basenpc.h"
#include	"pathtrack.h"

#define SF_RAY_FLY_ESCAPE_PATH_WHEN_NO_ENEMY	( 1 << 15 )			// 

#define RAY_FACE_ENEMY_DISTANCE					512.0f
#define RAY_XY_DISTANCE_NOT_FACING				32.0f
#define RAY_Z_DISTANCE_NOT_FACING				8.0f
#define RAY_XY_DISTANCE_FACING					32.0f
#define RAY_Z_DISTANCE_FACING					4.0f

#define RAY_ANGULAR_SPEED						90.0f
#define RAY_ANGLE_X_LIMIT						35.0f

#define RAY_MAX_SPEED							mantaray_max_speed.GetFloat() //500.0f
#define RAY_MAX_SPEED_DISTANCE					64.0f
#define RAY_MIN_TURNING_SPEED					40.0f //TERO: used to be 80.0f
#define RAY_TURNING_TO_SPEED_DIFFERENCE			45.0f

#define RAY_BEAM_SOUND "NPC_Vortigaunt.ZapPowerup"
#define RAY_LIGHT_SOUND "NPC_Hunter.FlechetteExplode"
#define RAY_LIGHT_START_SOUND "NPC_Hunter.FlechettePreExplode"

#define RAY_SOUND_PAIN "NPC_FastZombie.Pain"
#define RAY_SOUND_IDLE "NPC_PoisonZombie.Moan1"
#define RAY_SOUND_DEATH "NPC_Hunter.Death"
#define RAY_SOUND_EXPLODE "NPC_Antlion.PoisonBurstExplode"
#define RAY_SOUND_TELEPORT "k_lab.mini_teleport_arcs"

#define RAY_ATTACK_DELAY 1.0f

#define RAY_ESCAPE_DAMAGE_DIFFERENCE random->RandomInt(180,240)
#define RAY_ESCAPE_TIME				 5.0f

#define HLSS_MANTARAY_MAX_NPCS 3

class CHLSS_Mantaray_Teleport_Target : public CPointEntity
{
public:
	DECLARE_CLASS( CHLSS_Mantaray_Teleport_Target, CPointEntity );
	DECLARE_DATADESC();

	CHLSS_Mantaray_Teleport_Target();

	bool m_bIsBeingUsed;

	COutputEvent m_OnSpawnedNPC;
	COutputEvent m_OnTeleported;
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CNPC_MantaRay : public CAI_BaseNPC
{
public:
	DECLARE_CLASS( CNPC_MantaRay, CAI_BaseNPC );
	DECLARE_DATADESC();
	DECLARE_SERVERCLASS();

	CNPC_MantaRay();

	void			Spawn( void );
	void			Precache( void );
	Class_T			Classify ( void );
	void			OnRestore( void );

	bool			CreateVPhysics( void );
	void			UpdateOnRemove();
	virtual void	StopLoopingSounds();

	virtual bool	OverrideMove(float flInterval);
	virtual bool	OverrideMoveFacing( const AILocalMoveGoal_t &move, float flInterval ) { return true; }
	virtual float	MaxYawSpeed( void ) { return 0.0f; }
	bool			Probe( const Vector &vecMoveDir, float flSpeed, Vector &vecDeflect );
	void			RayFly(float flInterval, Vector vMoveTargetPos, Vector vMoveGoal );
	bool			RayProgressFlyPath();
	bool			RayReachedPoint(Vector vecOrigin);
	void			RayCheckBlocked(CBaseEntity *pMoveTarget, Vector vMoveTargetPos);

	virtual void	PainSound( const CTakeDamageInfo &info );
	virtual void	IdleSound( void );

	virtual Activity GetFlinchActivity( bool bHeavyDamage, bool bGesture );

	bool			CanBecomeRagdoll() { return false; }

	//virtual bool	IsHeavyDamage( const CTakeDamageInfo &info );

	virtual int		OnTakeDamage_Alive( const CTakeDamageInfo &info );

	CBaseEntity		*GetEnemyVehicle();

	void			NPCThink();
	void			Event_Killed( const CTakeDamageInfo &info );

	void			CreateBeam(CBaseEntity *pTarget, bool bCreate);

	//TERO: from ai_trackpather.cpp
	CPathTrack		*BestPointOnPath( CPathTrack *pPath, const Vector &targetPos, float avoidRadius, bool facing );

	bool			FlyToDeath();

	void			InputTeleporter( inputdata_t &inputdata );
	void			InputHostile( inputdata_t &inputdata );
	void			InputTeleportTarget( inputdata_t &inpudata );
	void			InputTeleportTargetName( inputdata_t &inpudata );

	void			FindTeleportTarget( string_t iszTargetName );
	bool			ShouldLookForTeleporTarget();

	bool			SpawnNPCs(Vector vecOrigin, float flRadius);
	bool			PlaceNPCInRadius( CAI_BaseNPC *pNPC, Vector vecOrigin, float flRadius );
		
	virtual void	DeathNotice( CBaseEntity *pChild );// NPC maker children use this to tell the NPC maker that they have died.

	void			InputEscapePath( inputdata_t &inpudata );

private:
	virtual void	Explode(bool bFire = false );
	virtual void	LightExplode( Vector vecOrigin );

	CBoneFollowerManager	m_BoneFollowerManager;
	void			InitBoneFollowers( void );

	bool										m_bTeleporter;
	CHandle<CHLSS_Mantaray_Teleport_Target>		m_hTeleportTarget;
	string_t									m_iszTeleportTargetName;
	CNetworkVar( bool,	m_bTeleporting );

	bool			m_bFlyBlocked;

	CNetworkVar( int, m_nGunAttachment );
	CNetworkVar( int, m_iLaserTarget );
	CBeam*			m_pBeam;
	CSprite*		m_pLightGlow;

	float			m_flEscapeTimeEnds;
	CHandle<CPathTrack> m_pCurrentEscapePath;
	int				m_iDamageToEscape;

	//float			m_flAttackStartedTime;
	CNetworkVar( float, m_flAttackStartedTime );
	bool			m_bBallyStarted;
	CNetworkVector( m_vecLaserEndPos );

	CHandle<CPathTrack> m_pCurrentPathTarget;
	string_t			m_strCurrentPathName;

	float			m_flNextPainSoundTime;

	float			m_flDeathTime;

	// Sounds
	CSoundPatch	*m_pIdleSound;

	//NPCS!!

	string_t	m_iszTemplateName[HLSS_MANTARAY_MAX_NPCS];		// The name of the NPC that will be used as the template.
	string_t	m_iszTemplateData[HLSS_MANTARAY_MAX_NPCS];
	bool		m_bNPC_Alive[HLSS_MANTARAY_MAX_NPCS];
	bool		m_bMustBeDead[HLSS_MANTARAY_MAX_NPCS];

	COutputEvent m_OnSpawnedNPC;
	COutputEvent m_OnTeleported;
};

#endif