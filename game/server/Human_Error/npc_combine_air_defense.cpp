//=================== Half-Life 2: Short Stories Mod 2009 =====================//
//
// Purpose:	Combine Air Defense System
//
//=============================================================================//

#include "cbase.h"
#include "ai_basenpc.h"
#include "ai_senses.h"
#include "ai_memory.h"
#include "engine/IEngineSound.h"
#include "ammodef.h"
#include "Sprite.h"
#include "hl2_player.h"
#include "soundenvelope.h"
#include "explode.h"
#include "IEffects.h"
#include "animation.h"
#include "basehlcombatweapon_shared.h"
#include "iservervehicle.h"
#include "npc_combine_air_defense.h"
#include "weapon_rpg.h"
#include "rope_shared.h"
#include "rope.h"
#include "effect_dispatch_data.h"
#include "te_effect_dispatch.h"

#include "globalstate.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//Debug visualization
ConVar	g_debug_air_defense( "g_debug_air_defense", "0" );

//Activities
int ACT_AIR_DEFENSE_OPEN;
int ACT_AIR_DEFENSE_CLOSE;
int ACT_AIR_DEFENSE_OPEN_IDLE;
int ACT_AIR_DEFENSE_CLOSED_IDLE; 

//Datatable
BEGIN_DATADESC( CNPC_CombineAirDefense )

	DEFINE_FIELD( m_iAmmoType,		FIELD_INTEGER ),
	DEFINE_KEYFIELD( m_iMinHealthDmg, FIELD_INTEGER, "minhealthdmg" ),
	DEFINE_FIELD( m_bAutoStart,		FIELD_BOOLEAN ),
	DEFINE_FIELD( m_bActive,		FIELD_BOOLEAN ),
	DEFINE_FIELD( m_bBlinkState,	FIELD_BOOLEAN ),
	DEFINE_FIELD( m_bEnabled,		FIELD_BOOLEAN ),
	DEFINE_FIELD( m_flShotTime,		FIELD_TIME ),
	DEFINE_FIELD( m_flLastSight,	FIELD_TIME ),
	DEFINE_FIELD( m_flPingTime,		FIELD_TIME ),
	DEFINE_FIELD( m_vecGoalAngles,	FIELD_VECTOR ),
	DEFINE_FIELD( m_pEyeGlow,		FIELD_CLASSPTR ),

	DEFINE_FIELD( m_iRightMuzzle,	FIELD_INTEGER ),
	DEFINE_FIELD( m_iLeftMuzzle,	FIELD_INTEGER ),

	DEFINE_FIELD( m_flShotRocketTime,	FIELD_TIME ),
	DEFINE_FIELD( m_iNumberOfRockets,	FIELD_INTEGER ),
	DEFINE_FIELD( m_flNextChargeTime,	FIELD_TIME ),
	DEFINE_FIELD( m_flRechargeSpeed,	FIELD_FLOAT ),
	DEFINE_FIELD( m_flDechargeSpeed,	FIELD_FLOAT ),
	DEFINE_FIELD( m_iRechargeLevel,		FIELD_INTEGER ),
	DEFINE_FIELD( m_iRechargeState,		FIELD_INTEGER ),

	DEFINE_FIELD( m_bShouldUpdateHud,	FIELD_BOOLEAN ),

	DEFINE_FIELD( m_iRightRocketAttachment,	FIELD_INTEGER ),
	DEFINE_FIELD( m_iLeftRocketAttachment,	FIELD_INTEGER ),
	DEFINE_FIELD( m_bRightRocket,			FIELD_BOOLEAN ),

	DEFINE_SOUNDPATCH( m_pFireSound ),
	DEFINE_SOUNDPATCH( m_pRechargeSound ),

	DEFINE_FIELD( m_hRope1, FIELD_EHANDLE ),
	DEFINE_FIELD( m_hRope2, FIELD_EHANDLE ),

#ifdef AIRDEFENSE_USE_VPHYSICS
	DEFINE_EMBEDDED( m_BoneFollowerManager ),
#endif

	DEFINE_THINKFUNC( Retire ),
	DEFINE_THINKFUNC( Deploy ),
	DEFINE_THINKFUNC( ActiveThink ),
	DEFINE_THINKFUNC( SearchThink ),
	DEFINE_THINKFUNC( AutoSearchThink ),
	DEFINE_THINKFUNC( DeathThink ),
	DEFINE_THINKFUNC( RechargeThink ),
	DEFINE_THINKFUNC( DechargeThink ),

	// Inputs
	DEFINE_INPUTFUNC( FIELD_VOID, "Toggle",						InputToggle ),
	DEFINE_INPUTFUNC( FIELD_VOID, "Enable",						InputEnable ),
	DEFINE_INPUTFUNC( FIELD_VOID, "Disable",					InputDisable ),
	DEFINE_INPUTFUNC( FIELD_VOID, "StartRecharge",				InputRecharge ),
	DEFINE_INPUTFUNC( FIELD_VOID, "StopRecharge",				InputDecharge ),
	DEFINE_INPUTFUNC( FIELD_FLOAT, "SetRechargeIncreaseTime",	InputRechargeSpeed ),
	DEFINE_INPUTFUNC( FIELD_FLOAT, "SetRechargeDecreaseTime",	InputDechargeSpeed ),

	DEFINE_OUTPUT( m_OnDeploy,				"OnDeploy" ),
	DEFINE_OUTPUT( m_OnRetire,				"OnRetire" ),
	DEFINE_OUTPUT( m_OnTipped,				"OnTipped" ),
	DEFINE_OUTPUT( m_OnRechargeComplete,	"OnRechargeComplete" ),

END_DATADESC()

LINK_ENTITY_TO_CLASS( npc_combine_air_defense, CNPC_CombineAirDefense );


//-----------------------------------------------------------------------------
// Constructor
//-----------------------------------------------------------------------------
CNPC_CombineAirDefense::CNPC_CombineAirDefense( void )
{
	m_bActive			= false;
	m_pEyeGlow			= NULL;
	m_iAmmoType			= -1;
	m_iMinHealthDmg		= 0;
	m_bAutoStart		= false;
	m_flPingTime		= 0;
	m_flShotTime		= 0;
	m_flLastSight		= 0;
	m_bBlinkState		= false;
	m_bEnabled			= false;

	m_bRightRocket		= true;

	m_pFireSound = NULL;

	m_iMinHealthDmg = 100;

	m_flRechargeSpeed = AIR_DEFENSE_DEFAULT_RECHARGE_SPEED;
	m_flDechargeSpeed = AIR_DEFENSE_DEFAULT_DECHARGE_SPEED;
	m_iRechargeState = AIR_DEFENSE_CHARGE_FULL;

	m_vecGoalAngles.Init();

	m_hRope1 = NULL;
	m_hRope2 = NULL;
}

CNPC_CombineAirDefense::~CNPC_CombineAirDefense( void )
{
}

//-----------------------------------------------------------------------------
// Purpose: Precache
//-----------------------------------------------------------------------------
void CNPC_CombineAirDefense::Precache( void )
{
	PrecacheModel( AIR_DEFENSE_MODEL );	
	PrecacheModel( AIR_DEFENSE_GLOW_SPRITE );

	// Activities
	ADD_CUSTOM_ACTIVITY( CNPC_CombineAirDefense, ACT_AIR_DEFENSE_OPEN );
	ADD_CUSTOM_ACTIVITY( CNPC_CombineAirDefense, ACT_AIR_DEFENSE_CLOSE );
	ADD_CUSTOM_ACTIVITY( CNPC_CombineAirDefense, ACT_AIR_DEFENSE_OPEN_IDLE );
	ADD_CUSTOM_ACTIVITY( CNPC_CombineAirDefense, ACT_AIR_DEFENSE_CLOSED_IDLE ); 
	//ADD_CUSTOM_ACTIVITY( CNPC_CombineAirDefense, ACT_AIR_DEFENSE_FIRE );

	PrecacheScriptSound( "NPC_CeilingTurret.Retire" );
	PrecacheScriptSound( "NPC_CeilingTurret.Deploy" );
	PrecacheScriptSound( "NPC_CeilingTurret.Move" );
	PrecacheScriptSound( "NPC_CeilingTurret.Active" );
	PrecacheScriptSound( "NPC_CeilingTurret.Alert" );
	PrecacheScriptSound( AIR_DEFENSE_SOUND_SHOOT );
	PrecacheScriptSound( AIR_DEFENSE_SOUND_RECHARGE );
	PrecacheScriptSound( "NPC_CeilingTurret.Ping" );
	PrecacheScriptSound( "NPC_CeilingTurret.Die" );

	PrecacheScriptSound( "NPC_FloorTurret.DryFire" );

	PrecacheScriptSound( "PropAPC.FireRocket" );
	
	BaseClass::Precache();
}

#ifdef AIRDEFENSE_USE_VPHYSICS

static const char *pFollowerBoneNames[] =
{
	"Combine.Body",
	"Combine.Gun",
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CNPC_CombineAirDefense::CreateVPhysics( void )
{
	InitBoneFollowers();
	return BaseClass::CreateVPhysics();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_CombineAirDefense::InitBoneFollowers( void )
{
	// Don't do this if we're already loaded
	if ( m_BoneFollowerManager.GetNumBoneFollowers() != 0 )
		return;

	// Init our followers
	m_BoneFollowerManager.InitBoneFollowers( this, ARRAYSIZE(pFollowerBoneNames), pFollowerBoneNames );
}
#endif

//-----------------------------------------------------------------------------
// Purpose: Spawn the entity
//-----------------------------------------------------------------------------
void CNPC_CombineAirDefense::Spawn( void )
{ 
	Precache();

	SetModel( AIR_DEFENSE_MODEL );
	
	BaseClass::Spawn();

	//m_HackedGunPos	= Vector( 0, 0, 12.75 );

	SetHullType( HULL_HUMAN );
	SetHullSizeNormal();
	UTIL_SetSize( this, Vector( -18, -18 , 0 ), Vector( 18, 18, 80 ) );

#ifdef AIRDEFENSE_USE_VPHYSICS
	CreateVPhysics();
	InitBoneControllers();
#endif

	AddSpawnFlags( SF_NPC_LONG_RANGE );

	m_flFieldOfView		= VIEW_FIELD_FULL;
	//m_flFieldOfView	= 0.0f;
	m_takedamage	= DAMAGE_YES;
	m_iHealth		= 1000;
	m_bloodColor	= BLOOD_COLOR_MECH;
	
	SetSolid( SOLID_BBOX );
	AddSolidFlags( FSOLID_NOT_STANDABLE );

	AddFlag( FL_AIMTARGET );
	AddEFlags( EFL_NO_DISSOLVE );

	SetPoseParameter( AIR_DEFENSE_BC_YAW, 0 );
	SetPoseParameter( AIR_DEFENSE_BC_PITCH, 0 );

	m_iAmmoType = GetAmmoDef()->Index( "HelicopterGun" ); //"AR2" );

	//Create our eye sprite
	m_pEyeGlow = CSprite::SpriteCreate( AIR_DEFENSE_GLOW_SPRITE, GetLocalOrigin(), false );
	m_pEyeGlow->SetTransparency( kRenderTransAdd, 255, 0, 0, 128, kRenderFxNoDissipation );
	m_pEyeGlow->SetAttachment( this, LookupAttachment("Eye") );

	m_iRightRocketAttachment = LookupAttachment("RightRocket");
	m_iLeftRocketAttachment  = LookupAttachment("LeftRocket");
	m_iRightMuzzle			 = LookupAttachment("RightMuzzle");
	m_iLeftMuzzle			 = LookupAttachment("LeftMuzzle");

	//Set our autostart state
	m_bAutoStart = !!( m_spawnflags & SF_AIR_DEFENSE_AUTO_ACTIVATE );
	m_bEnabled	 = ( ( m_spawnflags & SF_AIR_DEFENSE_START_ACTIVE ) == false );

	//Do we start active?
	if ( m_bAutoStart && m_bEnabled )
	{
		m_iRechargeState = AIR_DEFENSE_CHARGE_FULL;
		m_iRechargeLevel = AIR_DEFENSE_RECHARGE_STEPS;

		SetThink( &CNPC_CombineAirDefense::AutoSearchThink );
		SetEyeState( AIR_DEFENSE_EYE_DORMANT );

		SetActivity( (Activity) ACT_AIR_DEFENSE_OPEN_IDLE );
	}
	else
	{
		m_iRechargeLevel = 0;
		m_iRechargeState = AIR_DEFENSE_CHARGE_ZERO;

		SetEyeState( AIR_DEFENSE_EYE_DISABLED );

		SetActivity( (Activity) ACT_AIR_DEFENSE_CLOSED_IDLE );
	}

	UpdateRechargerLights();

	m_flShotRocketTime = gpGlobals->curtime + AIR_DEFENSE_ROCKETS_INITIAL_DELAY;
	m_iNumberOfRockets = AIR_DEFENSE_ROCKETS;

	if (m_flRechargeSpeed <= 0.0f)
	{
		m_flRechargeSpeed = AIR_DEFENSE_DEFAULT_RECHARGE_SPEED;
	}
	if (m_flDechargeSpeed <= 0.0f)
	{
		m_flDechargeSpeed = AIR_DEFENSE_DEFAULT_DECHARGE_SPEED;
	}

	//Stagger our starting times
	SetNextThink( gpGlobals->curtime + random->RandomFloat( 0.1f, 0.3f ) );

	// Don't allow us to skip animation setup because our attachments are critical to us!
	SetBoneCacheFlags( BCF_NO_ANIMATION_SKIP );
}

void CNPC_CombineAirDefense::Activate()
{
	BaseClass::Activate();

	int iStart1	= LookupAttachment("Cable1a");
	int	iEnd1	= LookupAttachment("Cable1b");

	int iStart2	= LookupAttachment("Cable2a");
	int	iEnd2	= LookupAttachment("Cable2b");

	if (!m_hRope1)
	{ 
		m_hRope1 = CRopeKeyframe::Create( this, this, iStart1, iEnd1 );
	}

	if ( m_hRope1 )
	{
		m_hRope1->m_Width = 2;
		m_hRope1->m_nSegments = 8; //ROPE_MAX_SEGMENTS / 2;
		m_hRope1->EnableWind( false );
		m_hRope1->SetupHangDistance( 0 );
		m_hRope1->m_RopeLength = 64; //(m_hCraneMagnet->GetAbsOrigin() - m_hCraneTip->GetAbsOrigin()).Length() * 1.1;
		m_hRope1->m_Slack = 32;
	}

	if (!m_hRope2)
	{
		m_hRope2 = CRopeKeyframe::Create( this, this, iStart2, iEnd2 );
	}

	if ( m_hRope2 )
	{
		m_hRope2->m_Width = 2;
		m_hRope2->m_nSegments = 8; //ROPE_MAX_SEGMENTS / 2;
		m_hRope2->EnableWind( false );
		m_hRope2->SetupHangDistance( 0 );
		m_hRope2->m_RopeLength = 64; //(m_hCraneMagnet->GetAbsOrigin() - m_hCraneTip->GetAbsOrigin()).Length() * 1.1;
		m_hRope2->m_Slack = 32;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CNPC_CombineAirDefense::GetSoundInterests( void )
{
	return SOUND_WORLD | SOUND_COMBAT | SOUND_PLAYER | SOUND_PLAYER_VEHICLE | SOUND_DANGER | 
		SOUND_PHYSICS_DANGER | SOUND_BULLET_IMPACT | SOUND_MOVE_AWAY;
}


Class_T	CNPC_CombineAirDefense::Classify( void ) 
{
	if( !m_bEnabled || !m_bActive ) 
	{
		// NPC's should disregard me if I'm closed.
		return CLASS_NONE;
	}
	else
	{
		if (GlobalEntity_GetState("combine_base_hacked") == GLOBAL_ON)
		{
			return CLASS_AIR_DEFENSE_HACKED;
		}
		else
		{
			return CLASS_AIR_DEFENSE;
		}
	}
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
Disposition_t CNPC_CombineAirDefense::IRelationType( CBaseEntity *pTarget )
{
	if ( !m_bActive )
	{
		m_bActive = true;
		Disposition_t result = BaseClass::IRelationType( pTarget );
		m_bActive = false;
		return result;
	}

	return BaseClass::IRelationType( pTarget );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CNPC_CombineAirDefense::OnTakeDamage( const CTakeDamageInfo &inputInfo )
{
	if ( !m_takedamage )
		return 0;

	CTakeDamageInfo info = inputInfo;

	if ( m_bActive == false )
		info.ScaleDamage( 0.1f );

	// If attacker can't do at least the min required damage to us, don't take any damage from them
	if ( info.GetDamage() < m_iMinHealthDmg )
		return 0;

	m_iHealth -= info.GetDamage();

	if ( m_iHealth <= 0 )
	{
		m_iHealth = 0;
		m_takedamage = DAMAGE_NO;

		RemoveFlag( FL_NPC ); // why are they set in the first place???

		//FIXME: This needs to throw a ragdoll gib or something other than animating the retraction -- jdw

		ExplosionCreate( GetAbsOrigin(), GetLocalAngles(), this, 100, 100, false );
		SetThink( &CNPC_CombineAirDefense::DeathThink );

		StopSound( "NPC_CeilingTurret.Alert" );

		m_OnDamaged.FireOutput( info.GetInflictor(), this );

		SetNextThink( gpGlobals->curtime + 0.1f );

		return 0;
	}

	return 1;
}

void CNPC_CombineAirDefense::DechargeActivity( void )
{
		//Set ourselves to close
	if ( GetActivity() != ACT_AIR_DEFENSE_CLOSED_IDLE )
	{
		if ( GetActivity() != ACT_AIR_DEFENSE_CLOSE )
		{
			int iPose = LookupPoseParameter( AIR_DEFENSE_BC_PITCH );
			float flPose = GetPoseParameter( iPose ); 

			if (fabs(flPose) > 0.1f)
			{
				flPose = UTIL_Approach( 0.0f, flPose, 0.1f * MaxYawSpeed() ); //ApproachAngle
				SetPoseParameter( iPose, flPose );
			}
			else
			{
				//Set our visible state to dormant
				SetEyeState( AIR_DEFENSE_EYE_DORMANT );
		
				SetActivity( (Activity) ACT_AIR_DEFENSE_CLOSE );
				EmitSound( "NPC_CeilingTurret.Retire" );
			}
		}
		else if ( IsActivityFinished() )
		{	
			SetActivity( (Activity) ACT_AIR_DEFENSE_CLOSED_IDLE );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Retract and stop attacking
//-----------------------------------------------------------------------------
void CNPC_CombineAirDefense::Retire( void )
{
	if ( PreThink( AIR_DEFENSE_RETIRING ) )
		return;

	UpdateRechargerLights();

	//Level out the turret
	//m_vecGoalAngles = GetAbsAngles(); //TERO: if you leave this one out, the turret will point yaw to direction you left it
	SetNextThink( gpGlobals->curtime );

	bool bClosed = (GetActivity() == ACT_AIR_DEFENSE_CLOSED_IDLE);

	//Set ourselves to close
	if ( GetActivity() != ACT_AIR_DEFENSE_CLOSE && !bClosed)
	{
		int iPose = LookupPoseParameter( AIR_DEFENSE_BC_PITCH );
		float flPose = GetPoseParameter( iPose ); 

		if (fabs(flPose) > 0.1f)
		{
			flPose = UTIL_Approach( 0.0f, flPose, 0.1f * MaxYawSpeed() ); //ApproachAngle
			SetPoseParameter( iPose, flPose );
		}
		else
		{
			//Set our visible state to dormant
			SetEyeState( AIR_DEFENSE_EYE_DORMANT );

			//SetActivity( (Activity) ACT_AIR_DEFENSE_OPEN_IDLE );
		
			SetActivity( (Activity) ACT_AIR_DEFENSE_CLOSE );
			EmitSound( "NPC_CeilingTurret.Retire" );

			//Notify of the retraction
			m_OnRetire.FireOutput( NULL, this );
		}
	}
	else if ( bClosed || IsActivityFinished() )
	{	
		m_bActive		= false;
		m_flLastSight	= 0;

		SetActivity( (Activity) ACT_AIR_DEFENSE_CLOSED_IDLE );

		//Go back to auto searching
		if ( m_bAutoStart && ChargeIsDone())
		{
			SetThink( &CNPC_CombineAirDefense::AutoSearchThink );
			SetNextThink( gpGlobals->curtime + 0.05f );
		}
		else
		{
			//Set our visible state to dormant
			SetEyeState( AIR_DEFENSE_EYE_DISABLED );
			SetThink( &CNPC_CombineAirDefense::SUB_DoNothing );
		}
	}

	//TERO: added by me
	StopSound( "NPC_CeilingTurret.Alert" );
}

//-----------------------------------------------------------------------------
// Purpose: Deploy and start attacking
//-----------------------------------------------------------------------------
void CNPC_CombineAirDefense::Deploy( void )
{
	if ( PreThink( AIR_DEFENSE_DEPLOYING ) )
		return;

	m_vecGoalAngles = GetAbsAngles();

	SetNextThink( gpGlobals->curtime );

	//TERO: hm
	UpdateRechargerLights();

	//Show we've seen a target
	SetEyeState( AIR_DEFENSE_EYE_SEE_TARGET );

	bool bOpen = (GetActivity() == ACT_AIR_DEFENSE_OPEN_IDLE);

	//Open if we're not already
	if ( GetActivity() != ACT_AIR_DEFENSE_OPEN && !bOpen)
	{
		m_bActive = true;
		SetActivity( (Activity) ACT_AIR_DEFENSE_OPEN );
		EmitSound( "NPC_CeilingTurret.Deploy" );

		//Notify we're deploying
		m_OnDeploy.FireOutput( NULL, this );
	}

	//If we're done, then start searching
	else if ( bOpen || IsActivityFinished() )
	{
		SetActivity( (Activity) ACT_AIR_DEFENSE_OPEN_IDLE );

		m_flShotTime  = gpGlobals->curtime + 1.0f;
		m_flShotRocketTime = gpGlobals->curtime + AIR_DEFENSE_ROCKETS_INITIAL_DELAY;

		m_flPlaybackRate = 0;
		SetThink( &CNPC_CombineAirDefense::SearchThink );

		EmitSound( "NPC_CeilingTurret.Move" );
	}

	SetLastSightTime();
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CNPC_CombineAirDefense::SetLastSightTime()
{
	if( HasSpawnFlags( SF_AIR_DEFENSE_NEVERRETIRE ) )
	{
		m_flLastSight = FLT_MAX;
	}
	else
	{
		m_flLastSight = gpGlobals->curtime + AIR_DEFENSE_MAX_WAIT;	
	}
}

//-----------------------------------------------------------------------------
// Purpose: Returns the speed at which the turret can face a target
//-----------------------------------------------------------------------------
float CNPC_CombineAirDefense::MaxYawSpeed( void )
{
	//TODO: Scale by difficulty?
	if (!GetEnemy())
	{
		float flSpeed = (clamp(((gpGlobals->curtime - m_flShotTime + 1.0f)  / 6.0f), 0.0f, 1.0f) * 180.0f);
		//DevMsg("diff %f, max yaw speed: %f\n", (m_flLastSight - gpGlobals->curtime), flSpeed);
		return flSpeed;
	}

	return 180.0f;
}

//-----------------------------------------------------------------------------
// Purpose: Causes the turret to face its desired angles
//-----------------------------------------------------------------------------
bool CNPC_CombineAirDefense::UpdateFacing( void )
{
	bool  bMoved = false;
	matrix3x4_t localToWorld;
	
	GetAttachment( LookupAttachment( "eyes" ), localToWorld );

	Vector vecGoalDir;
	AngleVectors( m_vecGoalAngles, &vecGoalDir );

	Vector vecGoalLocalDir;
	VectorIRotate( vecGoalDir, localToWorld, vecGoalLocalDir );

	if ( g_debug_air_defense.GetBool() )
	{
		Vector	vecMuzzle, vecMuzzleDir;
		QAngle	vecMuzzleAng;

		GetAttachment( "eyes", vecMuzzle, vecMuzzleAng );
		AngleVectors( vecMuzzleAng, &vecMuzzleDir );

		NDebugOverlay::Cross3D( vecMuzzle, -Vector(2,2,2), Vector(2,2,2), 255, 255, 0, false, 0.05 );
		NDebugOverlay::Cross3D( vecMuzzle+(vecMuzzleDir*256), -Vector(2,2,2), Vector(2,2,2), 255, 255, 0, false, 0.05 );
		NDebugOverlay::Line( vecMuzzle, vecMuzzle+(vecMuzzleDir*256), 255, 255, 0, false, 0.05 );
		
		NDebugOverlay::Cross3D( vecMuzzle, -Vector(2,2,2), Vector(2,2,2), 255, 0, 0, false, 0.05 );
		NDebugOverlay::Cross3D( vecMuzzle+(vecGoalDir*256), -Vector(2,2,2), Vector(2,2,2), 255, 0, 0, false, 0.05 );
		NDebugOverlay::Line( vecMuzzle, vecMuzzle+(vecGoalDir*256), 255, 0, 0, false, 0.05 );
	}

	QAngle vecGoalLocalAngles;
	VectorAngles( vecGoalLocalDir, vecGoalLocalAngles );

	// Update pitch
	float flDiff = AngleNormalize( UTIL_ApproachAngle(  vecGoalLocalAngles.x, 0.0, 0.1f * MaxYawSpeed() ) );
	
	int iPose = LookupPoseParameter( AIR_DEFENSE_BC_PITCH );

	float flPose = GetPoseParameter( iPose ) + ( flDiff / 1.5f );
	if (flPose > 40.0f)
	{
		flPose = 40.0f;
	}

	SetPoseParameter( iPose, flPose);

	if ( fabs( flDiff ) > 0.1f )
	{
		bMoved = true;
	}

	//float desYaw = vecGoal//UTIL_AngleDiff(VecToYaw(MoveTarget - GetLocalOrigin()), 0 );

	// Update yaw
	flDiff = AngleNormalize( UTIL_ApproachAngle(  vecGoalLocalAngles.y, 0.0, 0.1f * MaxYawSpeed() ) );

	iPose = LookupPoseParameter( AIR_DEFENSE_BC_YAW );
	SetPoseParameter( iPose, GetPoseParameter( iPose ) + ( flDiff / 1.5f ) );

	if ( fabs( flDiff ) > 0.1f )
	{
		bMoved = true;
	}

	InvalidateBoneCache();

	return bMoved;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pEntity - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CNPC_CombineAirDefense::FVisible( CBaseEntity *pEntity, int traceMask, CBaseEntity **ppBlocker )
{
	CBaseEntity	*pHitEntity = NULL;
	if ( BaseClass::FVisible( pEntity, traceMask, &pHitEntity ) )
		return true;

	// If we hit something that's okay to hit anyway, still fire
	if ( pHitEntity && pHitEntity->MyCombatCharacterPointer() )
	{
		if (IRelationType(pHitEntity) == D_HT)
			return true;
	}

	if (ppBlocker)
	{
		*ppBlocker = pHitEntity;
	}
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Allows the turret to fire on targets if they're visible
//-----------------------------------------------------------------------------
void CNPC_CombineAirDefense::ActiveThink( void )
{
	//Allow descended classes a chance to do something before the think function
	if ( PreThink( AIR_DEFENSE_ACTIVE ) )
		return;

	//DevMsg("Active think\n");

	UpdateRechargerLights();

	//Update our think time
	SetNextThink( gpGlobals->curtime + 0.1f );

	//If we've become inactive, go back to searching
	if ( ( m_bActive == false ) || ( GetEnemy() == NULL ) )
	{
		SetEnemy( NULL );
		SetLastSightTime();
		SetThink( &CNPC_CombineAirDefense::SearchThink );
		//m_vecGoalAngles = GetAbsAngles();
		return;
	}
	
	//Get our shot positions
	Vector vecMid = EyePosition();
	Vector vecMidEnemy = GetEnemy()->GetAbsOrigin();

	//Store off our last seen location
	UpdateEnemyMemory( GetEnemy(), vecMidEnemy );

	//Look for our current enemy
	bool bEnemyVisible = true; //FInViewCone( GetEnemy() ) && FVisible( GetEnemy() ) && GetEnemy()->IsAlive();

	//Calculate dir and dist to enemy
	Vector	vecDirToEnemy = vecMidEnemy - vecMid;	
	float	flDistToEnemy = VectorNormalize( vecDirToEnemy );

	//We want to look at the enemy's eyes so we don't jitter
	Vector	vecDirToEnemyEyes = GetEnemy()->WorldSpaceCenter() - vecMid;
	VectorNormalize( vecDirToEnemyEyes );

	QAngle vecAnglesToEnemy;
	VectorAngles( vecDirToEnemyEyes, vecAnglesToEnemy );

	//Draw debug info
	if ( g_debug_air_defense.GetBool() )
	{
		NDebugOverlay::Cross3D( vecMid, -Vector(2,2,2), Vector(2,2,2), 0, 255, 0, false, 0.05 );
		NDebugOverlay::Cross3D( GetEnemy()->WorldSpaceCenter(), -Vector(2,2,2), Vector(2,2,2), 0, 255, 0, false, 0.05 );
		NDebugOverlay::Line( vecMid, GetEnemy()->WorldSpaceCenter(), 0, 255, 0, false, 0.05 );

		NDebugOverlay::Cross3D( vecMid, -Vector(2,2,2), Vector(2,2,2), 0, 255, 0, false, 0.05 );
		NDebugOverlay::Cross3D( vecMidEnemy, -Vector(2,2,2), Vector(2,2,2), 0, 255, 0, false, 0.05 );
		NDebugOverlay::Line( vecMid, vecMidEnemy, 0, 255, 0, false, 0.05f );
	}

	//Current enemy is not visible
	if ( ( bEnemyVisible == false ) || ( flDistToEnemy > AIR_DEFENSE_RANGE ))
	{
		if ( m_flLastSight )
		{
			m_flLastSight = gpGlobals->curtime + 0.5f;
		}
		else if ( gpGlobals->curtime > m_flLastSight )
		{
			// Should we look for a new target?
			ClearEnemyMemory();
			SetEnemy( NULL );
			SetLastSightTime();
			SetThink( &CNPC_CombineAirDefense::SearchThink );
			//m_vecGoalAngles = GetAbsAngles();
			
			SpinDown();

			return;
		}

		bEnemyVisible = false;
	}

	Vector vecMuzzle, vecMuzzleDir;
	QAngle vecMuzzleAng;
	
	GetAttachment( "eyes", vecMuzzle, vecMuzzleAng );
	AngleVectors( vecMuzzleAng, &vecMuzzleDir );
	
	//Fire the gun
	if ( DotProduct( vecDirToEnemy, vecMuzzleDir ) >= 0.9848 ) // 10 degree slop
	{

		//DevMsg("enemy dist: %f/%f, time %f\n", flDistToEnemy, AIR_DEFENSE_ROCKETS_MINIMUN_DISTANCE, gpGlobals->curtime - m_flShotRocketTime);

		if ( flDistToEnemy > AIR_DEFENSE_ROCKETS_MINIMUN_DISTANCE && 
			 CanShootRockets() && 
			 m_flShotRocketTime < gpGlobals->curtime )
		{
	
			//Fire the weapon
			if (ShootRockets())
			{
				m_iNumberOfRockets--;

				if (m_iNumberOfRockets <= 0)
				{
					m_iNumberOfRockets = AIR_DEFENSE_ROCKETS;
					m_flShotRocketTime = gpGlobals->curtime + AIR_DEFENSE_ROCKETS_INITIAL_DELAY;
				}
				else
				{
					m_flShotRocketTime = gpGlobals->curtime + AIR_DEFENSE_ROCKETS_DELAY;
				}
			}
			else
			{
				m_flShotRocketTime = gpGlobals->curtime + AIR_DEFENSE_ROCKETS_DELAY;
			}
		}
		if ( CanShootBullets() && m_flShotTime < gpGlobals->curtime ) // && (m_flShotRocketTime - AIR_DEFENSE_ROCKETS_DELAY) > gpGlobals->curtime )
		{
			m_flShotTime  = gpGlobals->curtime + 0.05f;
			
			//Fire the weapon
			Shoot();
		}
	}
	else 
	{
		StopFireSound();
	}

	//If we can see our enemy, face it
	if ( bEnemyVisible )
	{
		m_vecGoalAngles.y = vecAnglesToEnemy.y;
		m_vecGoalAngles.x = vecAnglesToEnemy.x;
	}

	//Turn to face
	UpdateFacing();
}

bool CNPC_CombineAirDefense::ShootRockets()
{
	Vector vecMuzzle; //, vecMuzzleDir;
	QAngle vecMuzzleAng;
	
	if (m_bRightRocket)
	{
		m_bRightRocket = false;
		GetAttachment( m_iRightRocketAttachment, vecMuzzle, vecMuzzleAng );
	}
	else
	{
		m_bRightRocket = true;
		GetAttachment( m_iLeftRocketAttachment, vecMuzzle, vecMuzzleAng );
	}

	Vector vecVelocity;
	
	if (GetEnemy())
	{
		trace_t tr;
		AI_TraceHull( vecMuzzle, GetEnemy()->WorldSpaceCenter(), -Vector(4,4,4), Vector(4,4,4), MASK_NPCSOLID, this, COLLISION_GROUP_PROJECTILE, &tr );
	
		/*if ((vecMuzzle - tr.endpos).Length() < AIR_DEFENSE_ROCKETS_MINIMUN_DISTANCE)
		{
			DevMsg("too close: %f\n", (vecMuzzle - tr.endpos).Length());
		}

		if (tr.m_pEnt != NULL && (IRelationType(tr.m_pEnt) != D_HT))
		{
			DevMsg("relation not correct: %d.\n", IRelationType(tr.m_pEnt));	
		}*/

		if ((vecMuzzle - tr.endpos).Length() < AIR_DEFENSE_ROCKETS_MINIMUN_DISTANCE ||
			(tr.m_pEnt != NULL && (IRelationType(tr.m_pEnt) != D_HT))) // || tr.fraction != 1))
		{
			return false;
		}

		vecVelocity = GetActualShootTrajectory( vecMuzzle );
	}
	else
	{
		AngleVectors( vecMuzzleAng, &vecVelocity );
	}
	
	VectorNormalize( vecVelocity );
	vecVelocity *= 1500;

	CMissile *pRocket = (CMissile *)CMissile::Create( vecMuzzle, vecMuzzleAng, edict() );
	pRocket->SetGracePeriod( 0.1f );
	//pRocket->SetAbsVelocity( vecVelocity );

	EmitSound( "PropAPC.FireRocket" );

	return true;
}

bool CNPC_CombineAirDefense::FInViewCone( CBaseEntity *pEntity )
{
	return true; //FInViewCone( pEntity->WorldSpaceCenter() );
}

bool CNPC_CombineAirDefense::FInViewCone( const Vector &vecSpot )
{
	return true;
}

//------------------------------------------------------------------------------
// Purpose : Override base class to check range and visibility
//------------------------------------------------------------------------------
bool CNPC_CombineAirDefense::IsValidEnemy( CBaseEntity *pTarget )
{
	// ---------------------------------
	//  Check range
	// ---------------------------------
	float flTargetDist = (GetAbsOrigin() - pTarget->GetAbsOrigin()).Length();
	if (flTargetDist < AIR_DEFENSE_MIN_DISTANCE)
	{
		return false;
	}
	if (flTargetDist > AIR_DEFENSE_RANGE)
	{
		return false;
	}
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Target doesn't exist or has eluded us, so search for one
//-----------------------------------------------------------------------------
void CNPC_CombineAirDefense::SearchThink( void )
{
	//Allow descended classes a chance to do something before the think function
	if ( PreThink( AIR_DEFENSE_SEARCHING ) )
		return;

	SetNextThink( gpGlobals->curtime + 0.05f );

	SetActivity( (Activity) ACT_AIR_DEFENSE_OPEN_IDLE );

	//If our enemy has died, pick a new enemy
	if ( ( GetEnemy() != NULL ) && ( GetEnemy()->IsAlive() == false ) )
	{
		SetEnemy( NULL );
	}

	//Acquire the target
 	if ( GetEnemy() == NULL )
	{
		m_bIgnoreUnseenEnemies = false;
		GetSenses()->Look( AIR_DEFENSE_RANGE );
		GetEnemies()->RefreshMemories();
		CBaseEntity *pEnemy = BestEnemy();
		if ( pEnemy )
		{
			SetEnemy( pEnemy );
		}
	}

	//If we've found a target, spin up the barrel and start to attack
	if ( GetEnemy() != NULL )
	{
		//Give players a grace period
		if ( GetEnemy()->IsPlayer() )
		{
			m_flShotTime  = gpGlobals->curtime + 0.5f;
			m_flShotRocketTime = gpGlobals->curtime + AIR_DEFENSE_ROCKETS_INITIAL_DELAY;
		}
		else
		{
			m_flShotTime  = gpGlobals->curtime + 0.1f;
			m_flShotRocketTime = gpGlobals->curtime + AIR_DEFENSE_ROCKETS_INITIAL_DELAY;
		}

		//DevMsg("Going to ActiveThink\n");

		m_flLastSight = 0;
		SetThink( &CNPC_CombineAirDefense::ActiveThink );
		SetEyeState( AIR_DEFENSE_EYE_SEE_TARGET );

		SpinUp();
		EmitSound( "NPC_CeilingTurret.Active" );
		return;
	}

	//Are we out of time and need to retract?
 	if ( gpGlobals->curtime > m_flLastSight )
	{
		//Before we retrace, make sure that we are spun down.
		m_flLastSight = 0;
		SetThink( &CNPC_CombineAirDefense::Retire );
		return;
	}
	
	//Display that we're scanning
	m_vecGoalAngles.x = GetAbsAngles().x + ( sin( gpGlobals->curtime * 2.0f ) * 40.0f ) - 10.0f;
	m_vecGoalAngles.y = GetAbsAngles().y + ( gpGlobals->curtime * 40.0f );

	//Turn and ping
	UpdateFacing();
	Ping();
}

//-----------------------------------------------------------------------------
// Purpose: Watch for a target to wander into our view
//-----------------------------------------------------------------------------
void CNPC_CombineAirDefense::AutoSearchThink( void )
{
	//Allow descended classes a chance to do something before the think function
	if ( PreThink( AIR_DEFENSE_AUTO_SEARCHING ) )
		return;

	//Spread out our thinking
	SetNextThink( gpGlobals->curtime + random->RandomFloat( 0.2f, 0.4f ) );

	//If the enemy is dead, find a new one
	if ( ( GetEnemy() != NULL ) && ( GetEnemy()->IsAlive() == false ) )
	{
		SetEnemy( NULL );
	}

	//Acquire Target
	if ( GetEnemy() == NULL )
	{
		GetSenses()->Look( AIR_DEFENSE_RANGE );
		GetEnemies()->RefreshMemories();
		SetEnemy( BestEnemy() );
	}

	//Deploy if we've got an active target
	if ( GetEnemy() != NULL )
	{
		SetThink( &CNPC_CombineAirDefense::Deploy );
		EmitSound( "NPC_CeilingTurret.Alert" );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Fire!
//-----------------------------------------------------------------------------
void CNPC_CombineAirDefense::Shoot()
{
	Vector vecMuzzle, vecMuzzleDir;
	QAngle vecMuzzleAng;

	int iAttachment;
	if (m_bRightRocket)
	{
		m_bRightRocket = false;
		iAttachment = m_iRightMuzzle;
	}
	else
	{
		m_bRightRocket = true;
		iAttachment = m_iLeftMuzzle;
	}

	GetAttachment( iAttachment, vecMuzzle, vecMuzzleAng );

	FireBulletsInfo_t info;

	if ( GetEnemy() != NULL )
	{
		Vector vecDir = GetActualShootTrajectory( vecMuzzle );

		info.m_vecSrc = vecMuzzle;
		info.m_vecDirShooting = vecDir;
		info.m_iTracerFreq = 1;
		info.m_iShots = 1;
		info.m_pAttacker = this;
		info.m_vecSpread = VECTOR_CONE_PRECALCULATED;
		info.m_flDistance = MAX_COORD_RANGE;
		info.m_iAmmoType = m_iAmmoType;
	}
	else
	{
		// Just shoot where you're facing!
		AngleVectors( vecMuzzleAng, &vecMuzzleDir );
		
		info.m_vecSrc = vecMuzzle;
		info.m_vecDirShooting = vecMuzzleDir;
		info.m_iTracerFreq = 1;
		info.m_iShots = 1;
		info.m_pAttacker = this;
		info.m_vecSpread = GetAttackSpread( NULL, NULL );
		info.m_flDistance = MAX_COORD_RANGE;
		info.m_iAmmoType = m_iAmmoType;
	}

	FireBullets( info );
	PlayFireSound();

	BaseClass::DoMuzzleFlash();
	
	CEffectData data;

	data.m_nAttachmentIndex = iAttachment;
	data.m_nEntIndex = entindex();
	//data.m_fFlags = MUZZLEFLASH_COMBINE;

	//DispatchEffect( "MuzzleFlash", data );
	DispatchEffect( "AirboatMuzzleFlash", data );
}

//-----------------------------------------------------------------------------
// Purpose: Shuts down looping sounds when we are killed in combat or deleted.
//-----------------------------------------------------------------------------
void CNPC_CombineAirDefense::StopLoopingSounds()
{
	BaseClass::StopLoopingSounds();

	StopFireSound();
	StopRechargeSound();
}

void CNPC_CombineAirDefense::PlayFireSound()
{
	if ( !m_pFireSound )
	{
		CSoundEnvelopeController &controller = CSoundEnvelopeController::GetController();
		CPASAttenuationFilter filter( this );

		m_pFireSound = controller.SoundCreate( filter, entindex(), AIR_DEFENSE_SOUND_SHOOT );
		controller.Play( m_pFireSound, 1.0f, 120 );
	}
}

void CNPC_CombineAirDefense::StopFireSound()
{
	if ( m_pFireSound )
	{
		CSoundEnvelopeController &controller = CSoundEnvelopeController::GetController();
		controller.SoundChangeVolume( m_pFireSound, 0.0f, 0.0f );
		controller.SoundDestroy( m_pFireSound );
		m_pFireSound = NULL;
	}
}

void CNPC_CombineAirDefense::PlayRechargeSound(bool bRecharge)
{
	CSoundEnvelopeController &controller = CSoundEnvelopeController::GetController();
	CPASAttenuationFilter filter( this );

	if ( !m_pRechargeSound )
	{
		m_pRechargeSound = controller.SoundCreate( filter, entindex(), AIR_DEFENSE_SOUND_RECHARGE );


		if (bRecharge)
		{
			controller.Play( m_pRechargeSound, 0.5f, 120 );
		}
		else
		{
			controller.Play( m_pRechargeSound, 0.5f, 80 );
		}
	}
	else
	{
		if (bRecharge)
		{
			controller.SoundChangePitch( m_pRechargeSound, 120, 0.1 );
		}
		else
		{
			controller.SoundChangePitch( m_pRechargeSound, 80, 0.1 );
		}
	}
}

void CNPC_CombineAirDefense::StopRechargeSound()
{
	if ( m_pRechargeSound )
	{
		CSoundEnvelopeController &controller = CSoundEnvelopeController::GetController();
		controller.SoundChangeVolume( m_pRechargeSound, 0.0f, 0.0f );
		controller.SoundDestroy( m_pRechargeSound );
		m_pRechargeSound = NULL;
	}
}


//-----------------------------------------------------------------------------
// Purpose: Allows a generic think function before the others are called
// Input  : state - which state the turret is currently in
//-----------------------------------------------------------------------------
bool CNPC_CombineAirDefense::PreThink( airDefenseState_e state )
{
	if ( state != AIR_DEFENSE_ACTIVE )
	{
		StopFireSound();

		if (m_hLaserDot)
		{
			EnableLaserDot( m_hLaserDot, false );
		}
	}
	else
	{
		CreateAPCLaserDot();

		if ( GetEnemy() )
		{
			m_hLaserDot->SetAbsOrigin( GetEnemy()->BodyTarget( WorldSpaceCenter(), false ) );
		}
		if (m_hLaserDot)
		{
			SetLaserDotTarget( m_hLaserDot, GetEnemy() );
			EnableLaserDot( m_hLaserDot, GetEnemy() != NULL );
		}
	}

	CheckPVSCondition();

	//Animate
	StudioFrameAdvance();

#ifdef AIRDEFENSE_USE_VPHYSICS
	m_BoneFollowerManager.UpdateBoneFollowers(this);
#endif

	//Do not interrupt current think function
	return false;
}

//------------------------------------------------------------------------------
// On Remove
//------------------------------------------------------------------------------
void CNPC_CombineAirDefense::UpdateOnRemove()
{
	BaseClass::UpdateOnRemove();

#ifdef AIRDEFENSE_USE_VPHYSICS
	m_BoneFollowerManager.DestroyBoneFollowers();
#endif

	if ( m_hLaserDot )
	{
		UTIL_Remove( m_hLaserDot );
		m_hLaserDot = NULL;
	}

	StopLoopingSounds();
}


//-----------------------------------------------------------------------------
// Purpose: Sets the state of the glowing eye attached to the turret
// Input  : state - state the eye should be in
//-----------------------------------------------------------------------------
void CNPC_CombineAirDefense::SetEyeState( eyeState_t state )
{
	//Must have a valid eye to affect
	if ( m_pEyeGlow == NULL )
		return;

	//Set the state
	switch( state )
	{
	default:
	case AIR_DEFENSE_EYE_SEE_TARGET: //Fade in and scale up
		m_pEyeGlow->SetColor( 255, 0, 0 );
		m_pEyeGlow->SetBrightness( 164, 0.1f );
		m_pEyeGlow->SetScale( 0.4f, 0.1f );
		break;

	case AIR_DEFENSE_EYE_SEEKING_TARGET: //Ping-pongs
		
		//Toggle our state
		m_bBlinkState = !m_bBlinkState;
		m_pEyeGlow->SetColor( 255, 128, 0 );

		if ( m_bBlinkState )
		{
			//Fade up and scale up
			m_pEyeGlow->SetScale( 0.25f, 0.1f );
			m_pEyeGlow->SetBrightness( 164, 0.1f );
		}
		else
		{
			//Fade down and scale down
			m_pEyeGlow->SetScale( 0.2f, 0.1f );
			m_pEyeGlow->SetBrightness( 64, 0.1f );
		}

		break;

	case AIR_DEFENSE_EYE_DORMANT: //Fade out and scale down
		m_pEyeGlow->SetColor( 0, 255, 0 );
		m_pEyeGlow->SetScale( 0.1f, 0.5f );
		m_pEyeGlow->SetBrightness( 64, 0.5f );
		break;

	case AIR_DEFENSE_EYE_DEAD: //Fade out slowly
		m_pEyeGlow->SetColor( 255, 0, 0 );
		m_pEyeGlow->SetScale( 0.1f, 3.0f );
		m_pEyeGlow->SetBrightness( 0, 3.0f );
		break;

	case AIR_DEFENSE_EYE_DISABLED:
		m_pEyeGlow->SetColor( 0, 255, 0 );
		m_pEyeGlow->SetScale( 0.1f, 1.0f );
		m_pEyeGlow->SetBrightness( 0, 1.0f );
		break;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Make a pinging noise so the player knows where we are
//-----------------------------------------------------------------------------
void CNPC_CombineAirDefense::Ping( void )
{
	//See if it's time to ping again
	if ( m_flPingTime > gpGlobals->curtime )
		return;

	//Ping!
	EmitSound( "NPC_CeilingTurret.Ping" );

	SetEyeState( AIR_DEFENSE_EYE_SEEKING_TARGET );

	m_flPingTime = gpGlobals->curtime + AIR_DEFENSE_PING_TIME;
}

//-----------------------------------------------------------------------------
// Purpose: Toggle the turret's state
//-----------------------------------------------------------------------------
void CNPC_CombineAirDefense::Toggle( void )
{
	//Toggle the state
	if ( m_bEnabled )
	{
		Disable();
	}
	else 
	{
		Enable();
	}
}

void CNPC_CombineAirDefense::InputRechargeSpeed( inputdata_t &inputdata )
{
	float flRechargeSpeed = inputdata.value.Float();

	if (flRechargeSpeed <= 0.0f)
	{
		flRechargeSpeed = AIR_DEFENSE_DEFAULT_RECHARGE_SPEED;
	}

	//TERO: let's update diz shit
	if (m_iRechargeState == AIR_DEFENSE_RECHARGING)
	{
		float flDiff = (m_flNextChargeTime - gpGlobals->curtime) / m_flRechargeSpeed;
		flDiff = clamp(flDiff, 0.0f, 1.0f);
		m_flNextChargeTime = gpGlobals->curtime + (flDiff * flRechargeSpeed);

		m_flRechargeSpeed = flRechargeSpeed;
	}
}

void CNPC_CombineAirDefense::InputDechargeSpeed( inputdata_t &inputdata )
{
	float flDechargeSpeed = inputdata.value.Float();

	if (flDechargeSpeed <= 0.0f)
	{
		flDechargeSpeed = AIR_DEFENSE_DEFAULT_DECHARGE_SPEED;
	}

	
	//TERO: let's update diz shit
	if (m_iRechargeState == AIR_DEFENSE_DECHARGING)
	{
		float flDiff = (m_flNextChargeTime - gpGlobals->curtime) / m_flDechargeSpeed;
		flDiff = clamp(flDiff, 0.0f, 1.0f);
		m_flNextChargeTime = gpGlobals->curtime + (flDiff * flDechargeSpeed);

		m_flDechargeSpeed = flDechargeSpeed;
	}
}

void CNPC_CombineAirDefense::InputRecharge( inputdata_t &inputdata )
{
	if (m_iRechargeState == AIR_DEFENSE_CHARGE_ZERO ||
		m_iRechargeState == AIR_DEFENSE_DECHARGING )
	{
		if (m_iRechargeState == AIR_DEFENSE_DECHARGING)
		{
			float flDiff = (m_flNextChargeTime - gpGlobals->curtime) / m_flDechargeSpeed;
			flDiff = 1.0f - clamp(flDiff, 0.0f, 1.0f);
			m_flNextChargeTime = gpGlobals->curtime + (flDiff * m_flRechargeSpeed);
		}
		else
		{
			EmitSound( "NPC_CeilingTurret.Ping" );
			m_flNextChargeTime = gpGlobals->curtime + m_flRechargeSpeed;
		}

		m_iRechargeState = AIR_DEFENSE_RECHARGING;

		SetThink( &CNPC_CombineAirDefense::RechargeThink );
		SetNextThink( gpGlobals->curtime );

		PlayRechargeSound(true);
	}
}

void CNPC_CombineAirDefense::InputDecharge( inputdata_t &inputdata )
{
	SetNextThink( gpGlobals->curtime + 0.1f );

	if (m_iRechargeState == AIR_DEFENSE_CHARGE_FULL ||
		m_iRechargeState == AIR_DEFENSE_RECHARGING )
	{
		if (m_iRechargeState == AIR_DEFENSE_RECHARGING)
		{
			float flDiff = (m_flNextChargeTime - gpGlobals->curtime) / m_flRechargeSpeed;
			flDiff = 1.0f - clamp(flDiff, 0.0f, 1.0f);
			m_flNextChargeTime = gpGlobals->curtime + (flDiff * m_flDechargeSpeed);
		}
		else
		{
			m_flNextChargeTime = gpGlobals->curtime + m_flDechargeSpeed;
		}

		m_iRechargeState = AIR_DEFENSE_DECHARGING;

		SetThink( &CNPC_CombineAirDefense::DechargeThink );
		SetNextThink( gpGlobals->curtime + 0.05f );

		StopFireSound();

		PlayRechargeSound(false);
	}
}

void CNPC_CombineAirDefense::RechargeThink( void )
{
	//Allow descended classes a chance to do something before the think function
	if ( PreThink( AIR_DEFENSE_RECHARGE ) )
		return;

	//Update our think time
	SetNextThink( gpGlobals->curtime + 0.1f );
		
	//m_vecGoalAngles = GetAbsAngles();
	//UpdateFacing();
	//DechargeActivity();

	if (m_flNextChargeTime < gpGlobals->curtime)
	{
		m_iRechargeLevel++;

		if (m_iRechargeLevel >= AIR_DEFENSE_RECHARGE_STEPS)
		{
			m_OnRechargeComplete.FireOutput( this, this );
			Enable();
		}
		else
		{
			EmitSound( "NPC_CeilingTurret.Ping" );
			m_flNextChargeTime = gpGlobals->curtime + m_flRechargeSpeed;
		}
	}

	UpdateRechargerLights();
}

bool CNPC_CombineAirDefense::ChargeIsDone()
{
	if (m_iRechargeState == AIR_DEFENSE_CHARGE_FULL)
		return true;

	return false;
}

void CNPC_CombineAirDefense::DechargeThink( void )
{
	//Allow descended classes a chance to do something before the think function
	if ( PreThink( AIR_DEFENSE_RECHARGE ) )
		return;

	//Update our think time
	SetNextThink( gpGlobals->curtime + 0.1f );
		
	//m_vecGoalAngles = GetAbsAngles();
	//UpdateFacing();
	DechargeActivity();

	if (m_flNextChargeTime < gpGlobals->curtime)
	{
		m_iRechargeLevel--;

		if (m_iRechargeLevel <= 0)
		{
			m_iRechargeState = AIR_DEFENSE_CHARGE_ZERO;
			Disable();
		}
		else
		{
			m_flNextChargeTime = gpGlobals->curtime + m_flDechargeSpeed;
		}
	}

	UpdateRechargerLights();
}

//-----------------------------------------------------------------------------
// Purpose: Enable the turret and deploy
//-----------------------------------------------------------------------------
void CNPC_CombineAirDefense::Enable( void )
{
	m_iRechargeState = AIR_DEFENSE_CHARGE_FULL;
	m_iRechargeLevel = AIR_DEFENSE_RECHARGE_STEPS;

	m_flNextChargeTime = 0.0f;

	UpdateRechargerLights();
	StudioFrameAdvance();

	StopFireSound();
	StopRechargeSound();

	// if the turret is flagged as an autoactivate turret, re-enable its ability open self.
	if ( m_spawnflags & SF_AIR_DEFENSE_AUTO_ACTIVATE )
	{
		m_bAutoStart = true;
	}

	m_bEnabled = true;

	SetThink( &CNPC_CombineAirDefense::Deploy );
	SetNextThink( gpGlobals->curtime + 0.05f );
}

//-----------------------------------------------------------------------------
// Purpose: Retire the turret until enabled again
//-----------------------------------------------------------------------------
void CNPC_CombineAirDefense::Disable( void )
{
	m_iRechargeState = AIR_DEFENSE_CHARGE_ZERO;
	m_iRechargeLevel = 0;

	m_flNextChargeTime = 0.0f;

	UpdateRechargerLights();
	StudioFrameAdvance();

	StopFireSound();
	StopRechargeSound();


	SetEnemy( NULL );
	SetThink( &CNPC_CombineAirDefense::Retire );
	SetNextThink( gpGlobals->curtime + 0.1f );

	m_bAutoStart = false;
	m_bEnabled = false;
}

//-----------------------------------------------------------------------------
// Purpose: Toggle the turret's state via input function
//-----------------------------------------------------------------------------
void CNPC_CombineAirDefense::InputToggle( inputdata_t &inputdata )
{
	Toggle();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_CombineAirDefense::InputEnable( inputdata_t &inputdata )
{
	Enable();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_CombineAirDefense::InputDisable( inputdata_t &inputdata )
{
	Disable();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_CombineAirDefense::SpinUp( void )
{
}

#define	AIR_DEFENSE_MIN_SPIN_DOWN	1.0f

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_CombineAirDefense::SpinDown( void )
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_CombineAirDefense::DeathThink( void )
{
	if ( PreThink( AIR_DEFENSE_DEAD ) )
		return;

	//Level out our angles
	m_vecGoalAngles = GetAbsAngles();
	SetNextThink( gpGlobals->curtime );

	if ( m_lifeState != LIFE_DEAD )
	{
		m_lifeState = LIFE_DEAD;

		EmitSound( "NPC_CeilingTurret.Die" );

		SetActivity( (Activity) ACT_AIR_DEFENSE_CLOSE );
	}

	// lots of smoke
	Vector pos;
	CollisionProp()->RandomPointInBounds( vec3_origin, Vector( 1, 1, 1 ), &pos );
	
	CBroadcastRecipientFilter filter;
	
	te->Smoke( filter, 0.0, &pos, g_sModelIndexSmoke, 2.5, 10 );
	
	g_pEffects->Sparks( pos );

	if ( IsActivityFinished() && ( UpdateFacing() == false ) )
	{
		m_flPlaybackRate = 0;
		SetThink( NULL );
	}
}

char *RechargeParameters[AIR_DEFENSE_RECHARGE_STEPS] = 
{
	"recharge1",
	"recharge2",
	"recharge3",
};

void CNPC_CombineAirDefense::UpdateRechargerLights()
{
	//m_iRechargeLevel

	bool bRecharging = true;

	if (m_iRechargeState == AIR_DEFENSE_CHARGE_ZERO ||
		m_iRechargeState == AIR_DEFENSE_DECHARGING)
	{
		bRecharging = false;
		m_nSkin = 1;
	}
	else
	{
		m_nSkin = 0;
	}

	//DevMsg("Setting poses: ");

	for (int i=0; i<m_iRechargeLevel; i++)
	{
		SetPoseParameter( RechargeParameters[i], 1.0 );

		//DevMsg("[%d] = 1.0f ", i);
	}

	float flDiff = 1.0f;

	if ( m_iRechargeLevel < AIR_DEFENSE_RECHARGE_STEPS)
	{
		if (bRecharging)
		{
			flDiff = clamp( 1.0f - ((m_flNextChargeTime - gpGlobals->curtime)/m_flRechargeSpeed), 0.0f, 1.0f);
		}
		else
		{
			flDiff = clamp(((m_flNextChargeTime - gpGlobals->curtime)/m_flDechargeSpeed), 0.0f, 1.0f);
		}

		SetPoseParameter( RechargeParameters[ m_iRechargeLevel ], flDiff );

		//DevMsg("[%d] = %f ", m_iRechargeLevel, flDiff);

		for (int i=m_iRechargeLevel+1; i < AIR_DEFENSE_RECHARGE_STEPS; i++)
		{
			SetPoseParameter( RechargeParameters[i], 0.0 );

			//DevMsg("[%d] = 0.0f ", i);
		}
	}

	if (HasSpawnFlags(SF_AIR_DEFENSE_RECHARGE_HUD))
	{
		CBasePlayer *pPlayer  = AI_GetSinglePlayer();
		if (pPlayer && pPlayer->IsSuitEquipped())
		{
			if (m_bShouldUpdateHud)
			{	
				CSingleUserRecipientFilter filter(pPlayer);
				UserMessageBegin(filter, "UpdateAirDefenseRecharge");
					WRITE_BOOL(true);
					WRITE_BOOL(bRecharging);
					WRITE_FLOAT(gpGlobals->curtime + 1.0f);

					for (int i=0; i<AIR_DEFENSE_RECHARGE_STEPS; i++)
					{
						if (i == m_iRechargeLevel)
						{
							WRITE_FLOAT(flDiff);
						}
						else if (i < m_iRechargeLevel)
						{	
							WRITE_FLOAT(1.0f);
						}	
						else
						{
							WRITE_FLOAT(0.0f);
						}
					}
				MessageEnd();
			}

			if ( ( bRecharging && m_iRechargeLevel == AIR_DEFENSE_RECHARGE_STEPS) ||
				 (!bRecharging && m_iRechargeLevel == 0 && flDiff <= 0.0f))
			{
				m_bShouldUpdateHud = false;
			}
			else
			{
				m_bShouldUpdateHud = true;
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Create a laser
//-----------------------------------------------------------------------------
void CNPC_CombineAirDefense::CreateAPCLaserDot( void )
{
	// Create a laser if we don't have one
	if ( m_hLaserDot == NULL )
	{
		m_hLaserDot = CreateLaserDot( GetAbsOrigin(), this, false );
	}
}


