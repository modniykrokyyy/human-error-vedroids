//=================== Half-Life 2: Short Stories Mod 2009 =====================//
//
// Purpose:	Summer aka Girl In The Summer Dress
//
//=============================================================================//


#ifndef NPC_OLIVIA
#define NPC_OLIVIA

#include	"cbase.h"
#include	"npcevent.h"
#include	"ai_basenpc.h"

#define SF_OLIVIA_TAKE_DAMAGE						(1 << 16)	
#define SF_OLIVIA_INFLICT_DAMAGE_ON_PLAYER			(1 << 17)
#define SF_OLIVIA_INVISIBLE							(1 << 18)
#define SF_OLIVIA_FLY								(1 << 19)
#define SF_OLIVIA_DANDELIONS						(1 << 20)
#define SF_OLIVIA_GLASSES							(1 << 21)
#define SF_OLIVIA_SMOKING							(1 << 22)
#define SF_OLIVIA_LIGHT								(1 << 23)
#define SF_OLIVIA_COLOR_CORRECTION					(1 << 24)

#define OLIVIA_DAMAGE_INFLIC_SCALAR 0.5f

#define OLIVIA_FACE_ENEMY_DISTANCE					512.0f
#define OLIVIA_XY_DISTANCE_NOT_FACING				8.0f
#define OLIVIA_Z_DISTANCE_NOT_FACING				4.0f
#define OLIVIA_XY_DISTANCE_FACING					4.0f
#define OLIVIA_Z_DISTANCE_FACING					2.0f

#define OLIVIA_BODY_CENTER Vector (0,0,0)//35)
#define OLIVIA_FLYSPEED_MAX 320
#define OLIVIA_FLYSPEED_MIN 100

#define OLIVIA_HULL_MINS Vector( -16, -16, 0 )
#define OLIVIA_HULL_MAXS Vector( 16, 16, 70 )

#define OLIVIA_GIVE_DAMAGE_TO_PLAYER_WITH_THINK


//used to be 64.0f
#define OLIVIA_PLAYER_CIRCLE_DISTANCE 64.0f 

//#define OLIVIA_DEBUG
//#define DEBUG_OLIVIA_BEST_APPEAR

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CNPC_Olivia : public CAI_BaseActor
{
public:
	DECLARE_CLASS( CNPC_Olivia, CAI_BaseActor );

	CNPC_Olivia();

	void	Spawn( void );
	void	Precache( void );
	Class_T Classify ( void );
	void	HandleAnimEvent( animevent_t *pEvent );
	int		GetSoundInterests( void );
	void	SetupWithoutParent( void );
	void	PrescheduleThink( void );

	int		SelectSchedule( void );

	void	NPCThink( void );

	void    TraceAttack( const CTakeDamageInfo &info, const Vector &vecDir, trace_t *ptr, CDmgAccumulator *pAccumulator );
	int		OnTakeDamage_Alive( const CTakeDamageInfo &info );

	Activity NPC_TranslateActivity( Activity eNewActivity );

	int		ObjectCaps( void ) { return UsableNPCObjectCaps(BaseClass::ObjectCaps()); }

	void	Appear();
	void	Disappear();

	// Inputs
	void	InputForceAppear( inputdata_t &inputdata );
	void	InputForceDisappear( inputdata_t &inputdata );
	void	InputAppear( inputdata_t &inputdata );
	void	InputDisappear( inputdata_t &inputdata );
	void	InputBestAppear( inputdata_t &inputdata );
	void	InputMinDisappear( inputdata_t &inputdata );
	void	InputMinAppear( inputdata_t &inputdata );
	void	InputSetDisappearTime( inputdata_t &inpudata );

	void	InputStartDandelions( inputdata_t &inputdata );
	void	InputStopDandelions( inputdata_t &inputdata );

	void	InputStartCircleAroundPlayer( inputdata_t &inputdata );
	void	InputStopCircleAroundPlayer( inputdata_t &inputdata );
	void	CircleAroundPlayer();
	void	InputAppearBehindPlayer( inputdata_t &inputdata );

	void	InputLand( inputdata_t &inpudata );

	virtual bool	OverrideMove(float flInterval);
	//virtual bool	OverrideMoveFacing( const AILocalMoveGoal_t &move, float flInterval );
	bool			Probe( const Vector &vecMoveDir, float flSpeed, Vector &vecDeflect );
	void			OliviaFly(float flInterval, Vector vMoveTargetPos, Vector vMoveGoal );
	bool			OliviaReachedPoint(Vector vecOrigin);
	bool			OliviaProgressFlyPath();

	//void			ParticleMessages(bool bStart);
	//virtual void	OnRestore();

	void			OliviaUse( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );

	enum
	{
		SCHED_OLIVIA_CIRCLE_AROUND_PLAYER = BaseClass::NEXT_SCHEDULE,
	};

	enum 
	{
		TASK_OLIVIA_CIRCLE_AROUND_PLAYER = BaseClass::NEXT_TASK,
	};

	enum
	{
		COND_OLIVIA_STOP_CIRCLE_AROUND_PLAYER = BaseClass::NEXT_CONDITION,	
	};

	void		StartTask( const Task_t *pTask );
	void		RunTask( const Task_t *pTask );
	void		GatherConditions( void );

protected:

	bool		m_bShouldAppear;
	float		m_flNextAppearCheckTime;
	float		m_flTimeToDisappear;
	float		m_flDisappearTime;
	float		m_flMinDisappearDistance;
	float		m_flMinAppearDistance;
	string_t	m_AppearDestination;

	float		m_flNextCircleAroundPlayer;
	float		m_flCircleAngle;
	int			m_iAppearBehindPlayerTries;

	CNetworkVar( bool,  m_bDandelions );
	CNetworkVar( bool,  m_bSmoking );
	CNetworkVar( bool,	m_bOliviaLight );
	CNetworkVar( bool,	m_bOliviaColorCorrection );
	CNetworkVar( int,	m_nSmokeAttachment );
	CNetworkVar( float, m_flAppearedTime );

	bool		m_bFlyBlocked;
	Vector		m_vOldGoal;

	float		m_flMaxFlySpeed;
	float		m_flCloseFlyDistance;

	bool		m_bWearingDress;

#ifdef OLIVIA_GIVE_DAMAGE_TO_PLAYER_WITH_THINK
	float		m_flDamageToGive;
#endif

	//Vector		m_vTest;	//DELETE THIS

	// Outputs
	COutputEvent	m_OnAppear;
	COutputEvent	m_OnDisappear;
	COutputEvent	m_OnPlayerUse;

	DECLARE_DATADESC();
	DECLARE_SERVERCLASS();
	DEFINE_CUSTOM_AI;
};


#endif