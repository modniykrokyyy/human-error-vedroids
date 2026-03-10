//========= Copyright � 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#include "cbase.h"
#include "soundent.h"
#include "npcevent.h"
#include "globalstate.h"
#include "ai_squad.h"
#include "ai_tacticalservices.h"
#include "npc_manhack.h"
#include "npc_metropolice.h"
#include "weapon_stunstick.h"
#include "basegrenade_shared.h"
#include "ai_route.h"
#include "hl2_player.h"
#include "iservervehicle.h"
#include "items.h"
#include "hl2_gamerules.h"

#include "ai_squad.h"
#include "ai_pathfinder.h"
#include "ai_route.h"
#include "ai_hint.h"
#include "ammodef.h"

#include "npc_headcrab.h"

#include "Human_Error/hlss_metrocopradio.h"
#include "Human_Error/npc_unique_metropolice.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define METROPOLICE_MID_RANGE_ATTACK_RANGE	3500.0f

#define METROPOLICE_SQUAD_STITCH_MIN_INTERVAL	1.0f
#define METROPOLICE_SQUAD_STITCH_MAX_INTERVAL	1.2f

#define AIM_ALONG_SIDE_LINE_OF_DEATH_DISTANCE		300.0f
#define AIM_ALONG_SIDE_STEER_DISTANCE				200.0f
#define AIM_ALONG_SIDE_DEFAULT_STITCH_LENGTH		750.0f
#define AIM_ALONG_SIDE_LINE_OF_DEATH_LEAD_TIME		0.0f
#define AIM_ALONG_SIDE_LINE_INITIAL_DRAW_FRACTION	0.2f

#define AIM_BEHIND_DEFAULT_STITCH_LENGTH		1000.0f
#define AIM_BEHIND_MINIMUM_DISTANCE				650.0f
#define AIM_BEHIND_STEER_DISTANCE				150.0f

#define RECENT_DAMAGE_INTERVAL		3.0f
#define RECENT_DAMAGE_THRESHOLD		0.2f

#define VEHICLE_PREDICT_ACCELERATION		333.0f
#define VEHICLE_PREDICT_MAX_SPEED			600.0f

#define	METROPOLICE_MAX_WARNINGS	3

#define	METROPOLICE_BODYGROUP_MANHACK	1
#define METROPOLICE_BODYGROUP_HEALTHKIT 2


const float HEAL_MOVE_RANGE = 30*12;
const float HEAL_TARGET_RANGE = 120; // 10 feet
const float HEAL_TOSS_TARGET_RANGE = 480; // 40 feet when we are throwing medkits 
const float HEAL_TARGET_RANGE_Z = 72; // a second check that Gordon isn't too far above us -- 6 feet

enum
{
	// NOTE: Exact #s are important, since they are referred to by number in schedules below

	METROPOLICE_SENTENCE_FREEZE				= 0,
	METROPOLICE_SENTENCE_HES_OVER_HERE		= 1,
	METROPOLICE_SENTENCE_HES_RUNNING		= 2,
	METROPOLICE_SENTENCE_TAKE_HIM_DOWN		= 3,	
	METROPOLICE_SENTENCE_ARREST_IN_POSITION	= 4,
	METROPOLICE_SENTENCE_DEPLOY_MANHACK		= 5,
	METROPOLICE_SENTENCE_MOVE_INTO_POSITION	= 6,
	METROPOLICE_SENTENCE_HEARD_SOMETHING	= 7,
	METROPOLICE_SENTENCE_AFFIRMATIVE		= 8,
	METROPOLICE_SENTENCE_CANT_MOVE			= 9,
};

enum
{
	METROPOLICE_ANNOUNCE_ATTACK_PRIMARY = 1,
	METROPOLICE_ANNOUNCE_ATTACK_SECONDARY,
	METROPOLICE_ANNOUNCE_ATTACK_HARASS,
};

enum
{
	METROPOLICE_CHATTER_WAIT_FOR_RESPONSE = 0,
	METROPOLICE_CHATTER_ASK_QUESTION = 1,
	METROPOLICE_CHATTER_RESPONSE = 2,

	METROPOLICE_CHATTER_RESPONSE_TYPE_COUNT = 2,
};


enum SpeechMemory_t
{
	bits_MEMORY_PAIN_LIGHT_SOUND	= bits_MEMORY_CUSTOM1,
	bits_MEMORY_PAIN_HEAVY_SOUND	= bits_MEMORY_CUSTOM2,
	bits_MEMORY_PLAYER_HURT			= bits_MEMORY_CUSTOM3,
	bits_MEMORY_PLAYER_HARASSED		= bits_MEMORY_CUSTOM4,
};

//Metrocop
int g_interactionMetrocopIdleChatter = 0;
int g_interactionMetrocopClearSentenceQueues = 0;

extern int g_interactionHitByPlayerThrownPhysObj;

ConVar	sk_metropolice_health( "sk_metropolice_health","0");
ConVar	sk_metropolice_simple_health( "sk_metropolice_simple_health","26");
ConVar  sk_metropolice_unique_health( "sk_metropolice_unique_health","0");

ConVar	metropolice_chase_use_follow( "metropolice_chase_use_follow", "0" );
ConVar  metropolice_move_and_melee("metropolice_move_and_melee", "1" );
ConVar  metropolice_charge("metropolice_charge", "1" );



//TERO: COMMANDABLE SHIT

const int MAX_PLAYER_SQUAD = 4;

extern ConVar sk_healthkit;
extern ConVar sk_healthvial;

ConVar	sk_metropolice_heal_player			( "sk_metropolice_heal_player",				"25");
ConVar	sk_metropolice_heal_player_delay	( "sk_metropolice_heal_player_delay",		"25");
ConVar	sk_metropolice_giveammo_player_delay( "sk_metropolice_giveammo_player_delay",	"10");
ConVar	sk_metropolice_heal_player_min_pct	( "sk_metropolice_heal_player_min_pct",		"0.60");
ConVar	sk_metropolice_heal_player_min_forced( "sk_metropolice_heal_player_min_forced",		"10.0");
ConVar	sk_metropolice_heal_ally			( "sk_metropolice_heal_ally",				"40");
ConVar	sk_metropolice_heal_ally_delay		( "sk_metropolice_heal_ally_delay",			"10");
ConVar	sk_metropolice_heal_ally_min_pct	( "sk_metropolice_heal_ally_min_pct",		"0.90");
ConVar	sk_metropolice_player_stare_time	( "sk_metropolice_player_stare_time",		"1.0" );
ConVar  sk_metropolice_player_stare_dist	( "sk_metropolice_player_stare_dist",		"72" );
ConVar	sk_metropolice_stare_heal_time		( "sk_metropolice_stare_heal_time",			"5" );

ConVar	g_ai_metropolice_show_enemy( "g_ai_metropolice_show_enemy", "0" );

ConVar	npc_metropolice_insignia( "npc_metropolice_insignia", "0" );
ConVar	npc_metropolice_squad_marker( "npc_metropolice_squad_marker", "0" );
ConVar	npc_metropolice_explosive_resist( "npc_metropolice_explosive_resist", "0" );
ConVar	npc_metropolice_auto_player_squad( "npc_metropolice_auto_player_squad", "1" );
ConVar	npc_metropolice_auto_player_squad_allow_use( "npc_metropolice_auto_player_squad_allow_use", "0" );

ConVar  npc_metropolice_medic_emit_sound("npc_citizen_medic_emit_sound", "1" );

// todo: bake these into pound constants (for now they're not just for tuning purposes)
ConVar  npc_metropolice_heal_chuck_medkit("npc_metropolice_heal_chuck_medkit" , "1" , FCVAR_ARCHIVE, "Set to 1 to use new experimental healthkit-throwing medic.");
ConVar  npc_metropolice_medic_throw_style( "npc_metropolice_medic_throw_style", "1", FCVAR_ARCHIVE, "Set to 0 for a lobbier trajectory" );
ConVar  npc_metropolice_medic_throw_speed( "npc_metropolice_medic_throw_speed", "650" );
ConVar	sk_metropolice_heal_toss_player_delay("sk_metropolice_heal_toss_player_delay", "26", FCVAR_NONE, "how long between throwing healthkits" );


#define MEDIC_THROW_SPEED npc_metropolice_medic_throw_speed.GetFloat()
#define USE_EXPERIMENTAL_MEDIC_CODE() ( npc_metropolice_heal_chuck_medkit.GetBool() && NameMatches("griggs") ) //TERO: disabled this crap again

ConVar player_squad_autosummon_time( "player_squad_autosummon_time", "5" );
ConVar player_squad_autosummon_move_tolerance( "player_squad_autosummon_move_tolerance", "20" );
ConVar player_squad_autosummon_player_tolerance( "player_squad_autosummon_player_tolerance", "10" );
ConVar player_squad_autosummon_time_after_combat( "player_squad_autosummon_time_after_combat", "8" );
ConVar player_squad_autosummon_debug( "player_squad_autosummon_debug", "0" );

#define ShouldAutosquad() (npc_metropolice_auto_player_squad.GetBool())


ConVar	ai_follow_move_commands( "ai_follow_move_commands", "1" );
ConVar	ai_metropolice_debug_commander( "ai_metropolice_debug_commander", "1" );
#define DebuggingCommanderMode() (ai_metropolice_debug_commander.GetBool() && (m_debugOverlays & OVERLAY_NPC_SELECTED_BIT))


// How many clips of pistol ammo a metropolice carries.
#define METROPOLICE_NUM_CLIPS			5

int AE_METROPOLICE_BATON_ON;
int	AE_METROPOLICE_BATON_OFF;
int AE_METROPOLICE_SHOVE;
int AE_METROPOLICE_START_DEPLOY;
int AE_METROPOLICE_DRAW_PISTOL;		// was	50
int AE_METROPOLICE_DEPLOY_MANHACK;	// was	51

int AE_METROPOLICE_HEAL_SELF;
int AE_METROPOLICE_HEAL_TARGET;
int AE_METROPOLICE_TAKE_HEALTHKIT;

#ifdef ELOISE_KICK_BALLS

int AE_METROPOLICE_KICK_BALLS;

#endif

//HLSS, 357 activities
Activity ACT_IDLE_ANGRY_357;
Activity ACT_RANGE_ATTACK_357;
Activity ACT_GESTURE_RANGE_ATTACK_357;
Activity ACT_METROPOLICE_DRAW_357;
Activity ACT_RELOAD_357;
Activity ACT_GESTURE_RELOAD_357;

#define COMMAND_POINT_CLASSNAME "info_target_command_point"

class CCommandPoint : public CPointEntity
{
	DECLARE_CLASS( CCommandPoint, CPointEntity );
public:
	CCommandPoint()
		: m_bNotInTransition(false)
	{
		if ( ++gm_nCommandPoints > 1 )
			DevMsg( "WARNING: More than one citizen command point present\n" );
	}

	~CCommandPoint()
	{
		--gm_nCommandPoints;
	}

	int ObjectCaps()
	{
		int caps = ( BaseClass::ObjectCaps() | FCAP_NOTIFY_ON_TRANSITION );

		if ( m_bNotInTransition )
			caps |= FCAP_DONT_SAVE;

		return caps;
	}

	void InputOutsideTransition( inputdata_t &inputdata )
	{
		if ( !AI_IsSinglePlayer() )
			return;

		m_bNotInTransition = true;

		CAI_Squad *pPlayerAISquad = g_AI_SquadManager.FindSquad(AllocPooledString(PLAYER_SQUADNAME));

		if ( pPlayerAISquad )
		{
			AISquadIter_t iter;
			for ( CAI_BaseNPC *pAllyNpc = pPlayerAISquad->GetFirstMember(&iter); pAllyNpc; pAllyNpc = pPlayerAISquad->GetNextMember(&iter) )
			{
				if ( pAllyNpc->GetCommandGoal() != vec3_invalid )
				{
					bool bHadGag = pAllyNpc->HasSpawnFlags(SF_NPC_GAG);

					pAllyNpc->AddSpawnFlags(SF_NPC_GAG);
					pAllyNpc->TargetOrder( UTIL_GetLocalPlayer(), &pAllyNpc, 1 );
					if ( !bHadGag )
						pAllyNpc->RemoveSpawnFlags(SF_NPC_GAG);
				}
			}
		}
	}
	DECLARE_DATADESC();

private:
	bool m_bNotInTransition; // does not need to be saved. If this is ever not default, the object is not being saved.
	static int gm_nCommandPoints;
};

int CCommandPoint::gm_nCommandPoints;

LINK_ENTITY_TO_CLASS( info_target_command_point, CCommandPoint );
BEGIN_DATADESC( CCommandPoint )
	
//	DEFINE_FIELD( m_bNotInTransition,	FIELD_BOOLEAN ),
	DEFINE_INPUTFUNC( FIELD_VOID,	"OutsideTransition",	InputOutsideTransition ),

END_DATADESC()

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------


// -----------------------------------------------
//	> Squad slots
// -----------------------------------------------
enum SquadSlot_T
{
	SQUAD_SLOT_POLICE_CHARGE_ENEMY = LAST_SHARED_SQUADSLOT,
	SQUAD_SLOT_POLICE_HARASS, // Yell at the player with a megaphone, etc.
	SQUAD_SLOT_POLICE_DEPLOY_MANHACK,
	SQUAD_SLOT_POLICE_ADVANCE,
	SQUAD_SLOT_POLICE_ATTACK_OCCLUDER1,
	SQUAD_SLOT_POLICE_ATTACK_OCCLUDER2,
	SQUAD_SLOT_POLICE_COVERING_FIRE1,
	SQUAD_SLOT_POLICE_COVERING_FIRE2,
//	SQUAD_SLOT_POLICE_ARREST_ENEMY,
};

//=========================================================
// Metro Police  Activities
//=========================================================
int ACT_METROPOLICE_DRAW_PISTOL;
int ACT_METROPOLICE_DEPLOY_MANHACK;
int ACT_METROPOLICE_FLINCH_BEHIND;

int	ACT_WALK_BATON;
int	ACT_IDLE_ANGRY_BATON;
int	ACT_PUSH_PLAYER;
int ACT_MELEE_ATTACK_THRUST;
int ACT_ACTIVATE_BATON;
int ACT_DEACTIVATE_BATON;

int ACT_METROPOLICE_HEAL_SELF;
int ACT_METROPOLICE_HEAL_TARGET;
 
LINK_ENTITY_TO_CLASS( npc_metropolice, CNPC_MetroPolice );

BEGIN_DATADESC( CNPC_MetroPolice )

	DEFINE_EMBEDDED( m_BatonSwingTimer ),	
	DEFINE_EMBEDDED( m_NextChargeTimer ),
	DEFINE_FIELD( m_flBatonDebounceTime, FIELD_FLOAT ),
	DEFINE_FIELD( m_bShouldActivateBaton, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_iPistolClips, FIELD_INTEGER ),
	DEFINE_KEYFIELD( m_fWeaponDrawn, FIELD_BOOLEAN, "weapondrawn" ),
	DEFINE_FIELD( m_LastShootSlot, FIELD_INTEGER ),
	DEFINE_EMBEDDED( m_TimeYieldShootSlot ),
	DEFINE_EMBEDDED( m_Sentences ),
	DEFINE_FIELD( m_bPlayerIsNear, FIELD_BOOLEAN ),

	DEFINE_FIELD( m_flTaskCompletionTime, FIELD_TIME ),
	DEFINE_FIELD( m_flLastPhysicsFlinchTime, FIELD_TIME ),
	DEFINE_FIELD( m_flLastDamageFlinchTime, FIELD_TIME ),

	DEFINE_FIELD( m_bMedkitHidden, FIELD_BOOLEAN ),

	DEFINE_FIELD( m_hManhack, FIELD_EHANDLE ),
	DEFINE_FIELD( m_hBlockingProp, FIELD_EHANDLE ),

	DEFINE_FIELD( m_nRecentDamage, FIELD_INTEGER ),
	DEFINE_FIELD( m_flRecentDamageTime, FIELD_TIME ),

	DEFINE_FIELD( m_flNextPainSoundTime, FIELD_TIME ),
	DEFINE_FIELD( m_flNextLostSoundTime, FIELD_TIME ),
	DEFINE_FIELD( m_nIdleChatterType, FIELD_INTEGER ),

	DEFINE_FIELD( m_bSimpleCops, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_flLastHitYaw, FIELD_FLOAT ),

/*	DEFINE_FIELD( m_bPlayerTooClose,	FIELD_BOOLEAN ),
	DEFINE_FIELD( m_bKeepFacingPlayer,	FIELD_BOOLEAN ),
	DEFINE_FIELD( m_flChasePlayerTime,	FIELD_TIME ),
	DEFINE_FIELD( m_vecPreChaseOrigin,	FIELD_VECTOR ),
	DEFINE_FIELD( m_flPreChaseYaw,		FIELD_FLOAT ),*/
//	DEFINE_FIELD( m_nNumWarnings,		FIELD_INTEGER ),
//	DEFINE_FIELD( m_iNumPlayerHits,		FIELD_INTEGER ),

	//								m_ActBusyBehavior (auto saved by AI)
	//								m_StandoffBehavior (auto saved by AI)
	//								m_AssaultBehavior (auto saved by AI)
	//								m_FuncTankBehavior (auto saved by AI)
	//								m_RappelBehavior (auto saved by AI)
	//								m_PolicingBehavior (auto saved by AI)
	//								m_FollowBehavior (auto saved by AI)

	DEFINE_KEYFIELD( m_iManhacks, FIELD_INTEGER, "manhacks" ),
	DEFINE_INPUTFUNC( FIELD_VOID, "EnableManhackToss", InputEnableManhackToss ),
	DEFINE_INPUTFUNC( FIELD_STRING, "SetPoliceGoal", InputSetPoliceGoal ),
	DEFINE_INPUTFUNC( FIELD_VOID, "ActivateBaton", InputActivateBaton ),
	DEFINE_INPUTFUNC( FIELD_VOID, "SpeakRecharge", InputSpeakRecharge ),
	DEFINE_INPUTFUNC( FIELD_VOID, "SpeakGeneratorOffline", InputSpeakGeneratorOffline ),

	DEFINE_FIELD(		m_flNextHealthSearchTime ,	FIELD_TIME ),

	DEFINE_FIELD( 		m_flPlayerGiveAmmoTime, 	FIELD_TIME ),
	DEFINE_KEYFIELD(	m_iszAmmoSupply, 			FIELD_STRING,	"ammosupply" ),
	DEFINE_KEYFIELD(	m_iAmmoAmount, 				FIELD_INTEGER,	"ammoamount" ),
	DEFINE_FIELD( 		m_iszOriginalSquad, 		FIELD_STRING ),
	DEFINE_FIELD( 		m_flTimeJoinedPlayerSquad,	FIELD_TIME ),
	DEFINE_FIELD( 		m_bWasInPlayerSquad, FIELD_BOOLEAN ),
	DEFINE_FIELD( 		m_flTimeLastCloseToPlayer,	FIELD_TIME ),
	DEFINE_EMBEDDED(	m_AutoSummonTimer ),
	DEFINE_FIELD(		m_vAutoSummonAnchor, FIELD_POSITION_VECTOR ),
	DEFINE_FIELD(		m_flTimePlayerStare,		FIELD_TIME ),
	DEFINE_FIELD(		m_flTimeNextHealStare,		FIELD_TIME ),
	DEFINE_FIELD( 		m_hSavedFollowGoalEnt,		FIELD_EHANDLE ),
	DEFINE_KEYFIELD(	m_bNotifyNavFailBlocked,	FIELD_BOOLEAN, "notifynavfailblocked" ),
	DEFINE_KEYFIELD(	m_bNeverLeavePlayerSquad,	FIELD_BOOLEAN, "neverleaveplayersquad" ),
	DEFINE_KEYFIELD(	m_iszDenyCommandConcept,	FIELD_STRING, "denycommandconcept" ),

	DEFINE_FIELD(		m_iUniqueMetropolice,		FIELD_INTEGER ),

	DEFINE_KEYFIELD(	m_bCanRecharge,				FIELD_BOOLEAN,	"can_recharge" ),

	DEFINE_OUTPUT(		m_OnJoinedPlayerSquad,	"OnJoinedPlayerSquad" ),
	DEFINE_OUTPUT(		m_OnLeftPlayerSquad,	"OnLeftPlayerSquad" ),
	DEFINE_OUTPUT(		m_OnFollowOrder,		"OnFollowOrder" ),
	DEFINE_OUTPUT(		m_OnStationOrder,		"OnStationOrder" ),
	DEFINE_OUTPUT(		m_OnPlayerUse,			"OnPlayerUse" ),
	DEFINE_OUTPUT(		m_OnNavFailBlocked,		"OnNavFailBlocked" ),

	DEFINE_INPUTFUNC( FIELD_VOID,	"RemoveFromPlayerSquad", InputRemoveFromPlayerSquad ),
	DEFINE_INPUTFUNC( FIELD_VOID,	"SetCommandable",		 InputSetCommandable ),
	DEFINE_INPUTFUNC( FIELD_VOID,	"SetNotCommandable",	 InputSetNotCommandable ),
	DEFINE_INPUTFUNC( FIELD_VOID,	"SetMedicOn",			 InputSetMedicOn ),
	DEFINE_INPUTFUNC( FIELD_VOID,	"SetMedicOff",			 InputSetMedicOff ),
	DEFINE_INPUTFUNC( FIELD_VOID,	"SetAmmoResupplierOn",	 InputSetAmmoResupplierOn ),
	DEFINE_INPUTFUNC( FIELD_VOID,	"SetAmmoResupplierOff",	 InputSetAmmoResupplierOff ),
	DEFINE_INPUTFUNC( FIELD_VOID,	"SetRechargerOn",		 InputSetRechargerOn ),
	DEFINE_INPUTFUNC( FIELD_VOID,	"SetRechargerOff",		 InputSetRechargerOff ),
//	DEFINE_INPUTFUNC( FIELD_VOID,	"SpeakIdleResponse", InputSpeakIdleResponse ),

	DEFINE_INPUTFUNC( FIELD_VOID,   "ThrowHealthKit", InputForceHealthKitToss ),

	DEFINE_USEFUNC( SimpleUse ),
	DEFINE_USEFUNC( CommanderUse ),

	DEFINE_OUTPUT( m_OnCupCopped, "OnCupCopped" ),

END_DATADESC()

//------------------------------------------------------------------------------

float CNPC_MetroPolice::gm_flTimeLastSpokePeek;

CSimpleSimTimer CNPC_MetroPolice::gm_PlayerSquadEvaluateTimer;

//------------------------------------------------------------------------------
// Purpose 
//------------------------------------------------------------------------------
/*CBaseEntity *CNPC_MetroPolice::CheckTraceHullAttack( float flDist, const Vector &mins, const Vector &maxs, int iDamage, int iDmgType, float forceScale, bool bDamageAnyNPC )
{
	// If only a length is given assume we want to trace in our facing direction
	Vector forward;
	AngleVectors( GetAbsAngles(), &forward );
	Vector vStart = GetAbsOrigin();

	// The ideal place to start the trace is in the center of the attacker's bounding box.
	// however, we need to make sure there's enough clearance. Some of the smaller monsters aren't 
	// as big as the hull we try to trace with. (SJB)
	float flVerticalOffset = WorldAlignSize().z * 0.5;

	if( flVerticalOffset < maxs.z )
	{
		// There isn't enough room to trace this hull, it's going to drag the ground.
		// so make the vertical offset just enough to clear the ground.
		flVerticalOffset = maxs.z + 1.0;
	}

	vStart.z += flVerticalOffset;
	Vector vEnd = vStart + (forward * flDist );
	return CheckTraceHullAttack( vStart, vEnd, mins, maxs, iDamage, iDmgType, forceScale, bDamageAnyNPC );
}

//------------------------------------------------------------------------------
// Melee filter for police
//------------------------------------------------------------------------------
class CTraceFilterMetroPolice : public CTraceFilterEntitiesOnly
{
public:
	// It does have a base, but we'll never network anything below here..
	DECLARE_CLASS_NOBASE( CTraceFilterMetroPolice );
	
	CTraceFilterMetroPolice( const IHandleEntity *passentity, int collisionGroup, CTakeDamageInfo *dmgInfo, float flForceScale, bool bDamageAnyNPC )
		: m_pPassEnt(passentity), m_collisionGroup(collisionGroup), m_dmgInfo(dmgInfo), m_pHit(NULL), m_flForceScale(flForceScale), m_bDamageAnyNPC(bDamageAnyNPC)
	{
	}
	
	virtual bool ShouldHitEntity( IHandleEntity *pHandleEntity, int contentsMask )
	{
		if ( !StandardFilterRules( pHandleEntity, contentsMask ) )
			return false;

		if ( !PassServerEntityFilter( pHandleEntity, m_pPassEnt ) )
			return false;

		// Don't test if the game code tells us we should ignore this collision...
		CBaseEntity *pEntity = EntityFromEntityHandle( pHandleEntity );
		
		if ( pEntity )
		{
			if ( !pEntity->ShouldCollide( m_collisionGroup, contentsMask ) )
				return false;
			
			if ( !g_pGameRules->ShouldCollide( m_collisionGroup, pEntity->GetCollisionGroup() ) )
				return false;

			if ( pEntity->m_takedamage == DAMAGE_NO )
				return false;

			// Translate the vehicle into its driver for damage
			if ( pEntity->GetServerVehicle() != NULL )
			{
				CBaseEntity *pDriver = pEntity->GetServerVehicle()->GetPassenger();

				if ( pDriver != NULL )
				{
					pEntity = pDriver;
				}
			}
	
			Vector	attackDir = pEntity->WorldSpaceCenter() - m_dmgInfo->GetAttacker()->WorldSpaceCenter();
			VectorNormalize( attackDir );

			CTakeDamageInfo info = (*m_dmgInfo);				
			CalculateMeleeDamageForce( &info, attackDir, info.GetAttacker()->WorldSpaceCenter(), m_flForceScale );

			if( !(pEntity->GetFlags() & FL_ONGROUND) )
			{
				// Don't hit airborne entities so hard. They fly farther since
				// there's no friction with the ground.
				info.ScaleDamageForce( 0.001 );
			}

			CBaseCombatCharacter *pBCC = info.GetAttacker()->MyCombatCharacterPointer();
			CBaseCombatCharacter *pVictimBCC = pEntity->MyCombatCharacterPointer();

			// Only do these comparisons between NPCs
			if ( pBCC && pVictimBCC )
			{
				// Can only damage other NPCs that we hate
				if ( m_bDamageAnyNPC || pBCC->IRelationType( pEntity ) == D_HT || pEntity->IsPlayer() )
				{
					if ( info.GetDamage() )
					{
						// If gordon's a criminal, do damage now
						if ( !pEntity->IsPlayer() || GlobalEntity_GetState( "gordon_precriminal" ) == GLOBAL_OFF )
						{
							if ( pEntity->IsPlayer() && ((CBasePlayer *)pEntity)->IsSuitEquipped() )
							{
								info.ScaleDamage( .25 );
								info.ScaleDamageForce( .25 );
							}

							pEntity->TakeDamage( info );
						}
					}
					
					m_pHit = pEntity;
					return true;
				}
			}
			else
			{
				// Make sure if the player is holding this, he drops it
				Pickup_ForcePlayerToDropThisObject( pEntity );

				// Otherwise just damage passive objects in our way
				if ( info.GetDamage() )
				{
					pEntity->TakeDamage( info );
				}
			}
		}

		return false;
	}

public:
	const IHandleEntity *m_pPassEnt;
	int					m_collisionGroup;
	CTakeDamageInfo		*m_dmgInfo;
	CBaseEntity			*m_pHit;
	float				m_flForceScale;
	bool				m_bDamageAnyNPC;
};

//------------------------------------------------------------------------------
// Purpose :	start and end trace position, amount 
//				of damage to do, and damage type. Returns a pointer to
//				the damaged entity in case the NPC wishes to do
//				other stuff to the victim (punchangle, etc)
//
//				Used for many contact-range melee attacks. Bites, claws, etc.
// Input   :
// Output  :
//------------------------------------------------------------------------------
CBaseEntity *CNPC_MetroPolice::CheckTraceHullAttack( const Vector &vStart, const Vector &vEnd, const Vector &mins, const Vector &maxs, int iDamage, int iDmgType, float flForceScale, bool bDamageAnyNPC )
{

	CTakeDamageInfo	dmgInfo( this, this, iDamage, DMG_SLASH );
	
	CTraceFilterMetroPolice traceFilter( this, COLLISION_GROUP_NONE, &dmgInfo, flForceScale, bDamageAnyNPC );

	Ray_t ray;
	ray.Init( vStart, vEnd, mins, maxs );

	trace_t tr;
	enginetrace->TraceRay( ray, MASK_SHOT, &traceFilter, &tr );

	CBaseEntity *pEntity = traceFilter.m_pHit;
	
	if ( pEntity == NULL )
	{
		// See if perhaps I'm trying to claw/bash someone who is standing on my head.
		Vector vecTopCenter;
		Vector vecEnd;
		Vector vecMins, vecMaxs;

		// Do a tracehull from the top center of my bounding box.
		vecTopCenter = GetAbsOrigin();
		CollisionProp()->WorldSpaceAABB( &vecMins, &vecMaxs );
		vecTopCenter.z = vecMaxs.z + 1.0f;
		vecEnd = vecTopCenter;
		vecEnd.z += 2.0f;
		
		ray.Init( vecTopCenter, vEnd, mins, maxs );
		enginetrace->TraceRay( ray, MASK_SHOT_HULL, &traceFilter, &tr );

		pEntity = traceFilter.m_pHit;
	}

	return pEntity;
}*/

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
#define SHOTGUN_DEFER_SEARCH_TIME	20.0f
#define OTHER_DEFER_SEARCH_TIME		FLT_MAX
bool CNPC_MetroPolice::ShouldLookForBetterWeapon()
{
	if (!IsUnique() && GetActiveWeapon())
		return false;


	if ( BaseClass::ShouldLookForBetterWeapon() )
	{
		if ( IsInPlayerSquad() && (GetActiveWeapon()&& IsMoving()) && ( m_FollowBehavior.GetFollowTarget() && m_FollowBehavior.GetFollowTarget()->IsPlayer() ) )
		{
			// For citizens in the player squad, you must be unarmed, or standing still (if armed) in order to 
			// divert attention to looking for a new weapon.
			return false;
		}

		if ( GetActiveWeapon() && IsMoving() )
			return false;

		if ( GlobalEntity_GetState("gordon_precriminal") == GLOBAL_ON )
		{
			// This stops the NPC looking altogether.
			m_flNextWeaponSearchTime = FLT_MAX;
			return false;
		}

#ifdef DEBUG
		// Cached off to make sure you change this if you ask the code to defer.
		float flOldWeaponSearchTime = m_flNextWeaponSearchTime;
#endif

		CBaseCombatWeapon *pWeapon = GetActiveWeapon();
		if( pWeapon )
		{
			bool bDefer = false;

			if( FClassnameIs( pWeapon, "weapon_ar2" ) )
			{
				// Content to keep this weapon forever
				m_flNextWeaponSearchTime = OTHER_DEFER_SEARCH_TIME;
				bDefer = true;
			}
			else if( FClassnameIs( pWeapon, "weapon_rpg" ) )
			{
				// Content to keep this weapon forever
				m_flNextWeaponSearchTime = OTHER_DEFER_SEARCH_TIME;
				bDefer = true;
			}
			else if( FClassnameIs( pWeapon, "weapon_357" ) )
			{
				// Content to keep this weapon forever
				m_flNextWeaponSearchTime = OTHER_DEFER_SEARCH_TIME;
				bDefer = true;
			}
			else if( FClassnameIs( pWeapon, "weapon_shotgun" ) )
			{
				// Shotgunners do not defer their weapon search indefinitely.
				// If more than one citizen in the squad has a shotgun, we force
				// some of them to trade for another weapon.
				if( NumWeaponsInSquad("weapon_shotgun") > 1 )
				{
					// Check for another weapon now. If I don't find one, this code will
					// retry in 2 seconds or so.
					bDefer = false;
				}
				else
				{
					// I'm the only shotgunner in the group right now, so I'll check
					// again in 3 0seconds or so. This code attempts to distribute
					// the desire to reduce shotguns amongst squadmates so that all 
					// shotgunners do not discard their weapons when they suddenly realize
					// the squad has too many.
					if( random->RandomInt( 0, 1 ) == 0 )
					{
						m_flNextWeaponSearchTime = gpGlobals->curtime + SHOTGUN_DEFER_SEARCH_TIME;
					}
					else
					{
						m_flNextWeaponSearchTime = gpGlobals->curtime + SHOTGUN_DEFER_SEARCH_TIME + 10.0f;
					}

					bDefer = true;
				}
			}

			if( bDefer )
			{
				// I'm happy with my current weapon. Don't search now.
				// If you ask the code to defer, you must have set m_flNextWeaponSearchTime to when
				// you next want to try to search.
				Assert( m_flNextWeaponSearchTime != flOldWeaponSearchTime );
				return false;
			}
		}

		return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CNPC_MetroPolice::OnChangeActiveWeapon( CBaseCombatWeapon *pOldWeapon, CBaseCombatWeapon *pNewWeapon )
{
	if ( pNewWeapon )
	{
		GetShotRegulator()->SetParameters( pNewWeapon->GetMinBurst(), pNewWeapon->GetMaxBurst(), pNewWeapon->GetMinRestTime(), pNewWeapon->GetMaxRestTime() );
	}
	BaseClass::OnChangeActiveWeapon( pOldWeapon, pNewWeapon );
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
void CNPC_MetroPolice::PickupItem( CBaseEntity *pItem )
{
	Assert( pItem != NULL );
	if( FClassnameIs( pItem, "item_healthkit" ) )
	{
		if ( TakeHealth( sk_healthkit.GetFloat(), DMG_GENERIC ) )
		{
			RemoveAllDecals();
			UTIL_Remove( pItem );
		}
	}
	else if( FClassnameIs( pItem, "item_healthvial" ) )
	{
		if ( TakeHealth( sk_healthvial.GetFloat(), DMG_GENERIC ) )
		{
			RemoveAllDecals();
			UTIL_Remove( pItem );
		}
	}
	else
	{
		DevMsg("Metrocop doesn't know how to pick up %s!\n", pItem->GetClassname() );
	}
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
void CNPC_MetroPolice::GiveWeapon( string_t iszWeaponName )
{
	CBaseCombatWeapon *pWeapon = Weapon_Create( STRING(iszWeaponName) );
	if ( !pWeapon )
	{
		Warning( "Couldn't create weapon %s to give NPC %s.\n", STRING(iszWeaponName), STRING(GetEntityName()) );
		return;
	}

	// If I have a name, make my weapon match it with "_weapon" appended
	if ( GetEntityName() != NULL_STRING )
	{
		pWeapon->SetName( AllocPooledString(UTIL_VarArgs("%s_weapon", GetEntityName())) );
	}

	Weapon_Equip( pWeapon );

	// Handle this case
	OnGivenWeapon( pWeapon );

	// If I have a weapon already, drop it
	if ( GetActiveWeapon() )
	{
		//Weapon_Drop( GetActiveWeapon() );
		Weapon_Switch(pWeapon);
	}
}

//-----------------------------------------------------------------------------
// My buddies got killed!
//-----------------------------------------------------------------------------
void CNPC_MetroPolice::NotifyDeadFriend( CBaseEntity* pFriend )
{
	BaseClass::NotifyDeadFriend(pFriend);

	if ( pFriend == m_hManhack )
	{
		if (!IsUnique())
		{
			m_Sentences.Speak( "METROPOLICE_MANHACK_KILLED", SENTENCE_PRIORITY_NORMAL, SENTENCE_CRITERIA_NORMAL );
		}
		m_hManhack = NULL;
		return;
	}

	// No notifications for squadmates' dead manhacks
	if ( FClassnameIs( pFriend, "npc_manhack" ) )
		return;

	// Reset idle chatter, we may never get a response back
	if ( m_nIdleChatterType == METROPOLICE_CHATTER_WAIT_FOR_RESPONSE )
	{
		m_nIdleChatterType = METROPOLICE_CHATTER_ASK_QUESTION;
	}

	if (!IsUnique())
	{

		if ( GetSquad()->NumMembers() < 2 )
		{
			m_Sentences.Speak( "METROPOLICE_LAST_OF_SQUAD", SENTENCE_PRIORITY_MEDIUM, SENTENCE_CRITERIA_NORMAL );
			return;
		}

		m_Sentences.Speak( "METROPOLICE_MAN_DOWN", SENTENCE_PRIORITY_MEDIUM );
	}
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
CNPC_MetroPolice::CNPC_MetroPolice()
{
	m_iUniqueMetropolice = 0;
	m_bCanRecharge = false;

	m_iAmmoAmount = 30;

	m_iszAmmoSupply = MAKE_STRING("Current");
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_MetroPolice::OnScheduleChange()
{
	BaseClass::OnScheduleChange();

	if ( GetEnemy() && HasCondition( COND_ENEMY_DEAD ) )
	{
		AnnounceEnemyKill( GetEnemy() );
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_MetroPolice::PrescheduleThink( void )
{
	BaseClass::PrescheduleThink();

	UpdatePlayerSquad();
	UpdateFollowCommandPoint();

	// Speak any queued sentences
	m_Sentences.UpdateSentenceQueue();

	if ( !npc_metropolice_insignia.GetBool() && npc_metropolice_squad_marker.GetBool() && IsInPlayerSquad() )
	{
		Vector mins = WorldAlignMins() * .5 + GetAbsOrigin();
		Vector maxs = WorldAlignMaxs() * .5 + GetAbsOrigin();
		
		float rMax = 255;
		float gMax = 255;
		float bMax = 255;

		float rMin = 255;
		float gMin = 128;
		float bMin = 0;

		const float TIME_FADE = 1.0;
		float timeInSquad = gpGlobals->curtime - m_flTimeJoinedPlayerSquad;
		timeInSquad = min( static_cast<int>(TIME_FADE), max( static_cast<int>(timeInSquad), static_cast<int>(0) ) ); //timeInSquad = min( TIME_FADE, max( timeInSquad, 0 ) );

		float fade = ( 1.0 - timeInSquad / TIME_FADE );

		float r = rMin + ( rMax - rMin ) * fade;
		float g = gMin + ( gMax - gMin ) * fade;
		float b = bMin + ( bMax - bMin ) * fade;

		// THIS IS A PLACEHOLDER UNTIL WE HAVE A REAL DESIGN & ART -- DO NOT REMOVE
		NDebugOverlay::Line( Vector( mins.x, GetAbsOrigin().y, GetAbsOrigin().z+1 ), Vector( maxs.x, GetAbsOrigin().y, GetAbsOrigin().z+1 ), r, g, b, false, .11 );
		NDebugOverlay::Line( Vector( GetAbsOrigin().x, mins.y, GetAbsOrigin().z+1 ), Vector( GetAbsOrigin().x, maxs.y, GetAbsOrigin().z+1 ), r, g, b, false, .11 );
	}
	if( GetEnemy() && g_ai_metropolice_show_enemy.GetBool() )
	{
		NDebugOverlay::Line( EyePosition(), GetEnemy()->EyePosition(), 255, 0, 0, false, .1 );
	}
	
	if ( DebuggingCommanderMode() )
	{
		if ( HaveCommandGoal() )
		{
			CBaseEntity *pCommandPoint = gEntList.FindEntityByClassname( NULL, COMMAND_POINT_CLASSNAME );
			
			if ( pCommandPoint )
			{
				NDebugOverlay::Cross3D(pCommandPoint->GetAbsOrigin(), 16, 0, 255, 255, false, 0.1 );
			}
		}
	}

	if( IsOnFire() )
	{
		SetCondition( COND_METROPOLICE_ON_FIRE );
	}
	else
	{
		ClearCondition( COND_METROPOLICE_ON_FIRE );
	}

	if (gpGlobals->curtime > m_flRecentDamageTime + RECENT_DAMAGE_INTERVAL)
	{
		m_nRecentDamage = 0;
		m_flRecentDamageTime = 0;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &move - 
//			flInterval - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CNPC_MetroPolice::OverrideMoveFacing( const AILocalMoveGoal_t &move, float flInterval )
{
	// Don't do this if we're scripted
	if ( IsInAScript() )
		return BaseClass::OverrideMoveFacing( move, flInterval );
  	
	// ROBIN: Disabled at request of mapmakers for now
	/*
  	// If we're moving during a police sequence, always face our target
	if ( m_PolicingBehavior.IsEnabled() )
  	{	
		CBaseEntity *pTarget = m_PolicingBehavior.GetGoalTarget();

		if ( pTarget )
		{
			AddFacingTarget( pTarget, pTarget->WorldSpaceCenter(), 1.0f, 0.2f );
		}
	}
	*/

	return BaseClass::OverrideMoveFacing( move, flInterval );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_MetroPolice::Precache( void )
{
	if(!IsUnique())
	{
		if ( HasSpawnFlags( SF_NPC_START_EFFICIENT ) )
		{
			SetModelName( AllocPooledString("models/police_cheaple.mdl" ) );
		}
		else
		{
			SetModelName( AllocPooledString("models/police.mdl") );
		}
	}

	PrecacheModel( STRING( GetModelName() ) );

	UTIL_PrecacheOther( "npc_manhack" );

	PrecacheScriptSound( "NPC_Metropolice.Shove" );
	PrecacheScriptSound( "NPC_MetroPolice.WaterSpeech" );
	PrecacheScriptSound( "NPC_MetroPolice.HidingSpeech" );
	enginesound->PrecacheSentenceGroup( "METROPOLICE" );

	BaseClass::Precache();
}


//-----------------------------------------------------------------------------
// Create components
//-----------------------------------------------------------------------------
bool CNPC_MetroPolice::CreateComponents()
{
	if ( !BaseClass::CreateComponents() )
		return false;

	m_Sentences.Init( this, "NPC_Metropolice.SentenceParameters" );
	return true;
}


//-----------------------------------------------------------------------------
// Purpose: 
//
//
//-----------------------------------------------------------------------------
void CNPC_MetroPolice::Spawn( void )
{	
	m_iUniqueMetropolice = METROPOLICE_NORMAL;

	BaseClass::Spawn();

#ifdef _XBOX
	// Always fade the corpse
	AddSpawnFlags( SF_NPC_FADE_CORPSE );
#endif // _XBOX

	m_nIdleChatterType = METROPOLICE_CHATTER_ASK_QUESTION; 
	m_bSimpleCops = HasSpawnFlags( SF_METROPOLICE_SIMPLE_VERSION );

	if ( HasSpawnFlags( SF_METROPOLICE_NOCHATTER ) )
	{
		AddSpawnFlags( SF_NPC_GAG );
	}

	if (IsUnique())
	{
		m_iMaxHealth = m_iHealth = sk_metropolice_unique_health.GetFloat();

		m_flFieldOfView	= VIEW_FIELD_FULL;
	}
	else if (!m_bSimpleCops )
	{
		m_iMaxHealth = m_iHealth = sk_metropolice_health.GetFloat();
	}
	else
	{
		m_iMaxHealth = m_iHealth = sk_metropolice_simple_health.GetFloat();
	}

	m_HackedGunPos = Vector ( 0, 0, 55 );

	m_iPistolClips = METROPOLICE_NUM_CLIPS;

	// NOTE: This must occur *after* init, since init sets default dist look
	if ( HasSpawnFlags( SF_METROPOLICE_MID_RANGE_ATTACK ) )
	{
		m_flDistTooFar = METROPOLICE_MID_RANGE_ATTACK_RANGE;
		SetDistLook( METROPOLICE_MID_RANGE_ATTACK_RANGE );
	}

	m_hManhack = NULL;

	m_fWeaponDrawn = false;

	if ( GetActiveWeapon() )
	{
		CBaseCombatWeapon *pWeapon;

		pWeapon = GetActiveWeapon();

		if( !FClassnameIs( pWeapon, "weapon_pistol" ) && !FClassnameIs( pWeapon, "weapon_357") )
		{
			m_fWeaponDrawn = true;
		}
		
		if( !m_fWeaponDrawn ) 
		{
			GetActiveWeapon()->AddEffects( EF_NODRAW );
		}
	}

	if (IsEloise())
	{
		CapabilitiesAdd	( bits_CAP_INNATE_MELEE_ATTACK1 );
	}


	m_TimeYieldShootSlot.Set( 2, 6 );

	m_bShouldActivateBaton = false;

	m_bMedkitHidden = true;

	// Clear out spawnflag if we're missing the smg1
	if( HasSpawnFlags( SF_METROPOLICE_ALWAYS_STITCH ) )
	{
		if ( !Weapon_OwnsThisType( "weapon_smg1" ) )
		{
			Warning( "Warning! Metrocop is trying to use the stitch behavior but he has no smg1!\n" );
			RemoveSpawnFlags( SF_METROPOLICE_ALWAYS_STITCH );
		}
	}

	if ( !m_bSimpleCops && ShouldAutosquad() )
	{
		if ( m_SquadName == GetPlayerSquadName() )
		{
			CAI_Squad *pPlayerSquad = g_AI_SquadManager.FindSquad( GetPlayerSquadName() );
			if ( pPlayerSquad && pPlayerSquad->NumMembers() >= MAX_PLAYER_SQUAD )
				m_SquadName = NULL_STRING;
		}
		gm_PlayerSquadEvaluateTimer.Force();
	}

	m_iszOriginalSquad = m_SquadName;

	SetUse( &CNPC_MetroPolice::SimpleUse );

	m_flTimePlayerStare = FLT_MAX;

	AddEFlags( EFL_NO_DISSOLVE | EFL_NO_MEGAPHYSCANNON_RAGDOLL | EFL_NO_PHYSCANNON_INTERACTION );

	NPCInit();

	SetUse( &CNPC_MetroPolice::CommanderUse );
	Assert( !ShouldAutosquad() || !IsInPlayerSquad() );

	m_bWasInPlayerSquad = IsInPlayerSquad();

	m_flNextHealthSearchTime = gpGlobals->curtime;

	// Use render bounds instead of human hull for guys sitting in chairs, etc.
	m_ActBusyBehavior.SetUseRenderBounds( HasSpawnFlags( SF_METROPOLICE_USE_RENDER_BOUNDS ) );

	// Start us with a visible manhack if we have one
	if ( m_iManhacks )
	{
		SetBodygroup( METROPOLICE_BODYGROUP_MANHACK, true );
	}
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CNPC_MetroPolice::PostNPCInit()
{
	if ( !gEntList.FindEntityByClassname( NULL, COMMAND_POINT_CLASSNAME ) )
	{
		CreateEntityByName( COMMAND_POINT_CLASSNAME );
	}
	
	if ( IsInPlayerSquad() )
	{
		if ( m_pSquad->NumMembers() > MAX_PLAYER_SQUAD )
			DevMsg( "Error: Spawning metropolice in player squad but exceeds squad limit of %d members\n", MAX_PLAYER_SQUAD );

		FixupPlayerSquad();
	}
	else
	{
		if ( ( m_spawnflags & SF_METROPOLICE_FOLLOW ) && AI_IsSinglePlayer() )
		{
			m_FollowBehavior.SetFollowTarget( UTIL_GetLocalPlayer() );
			m_FollowBehavior.SetParameters( AIF_SIMPLE );
		}
	}

	//TERO:
	m_RechargeBehavior.SetCanRecharge( m_bCanRecharge );

	BaseClass::PostNPCInit();
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CNPC_MetroPolice::OnRestore()
{
	gm_PlayerSquadEvaluateTimer.Force();

	BaseClass::OnRestore();

	if ( !gEntList.FindEntityByClassname( NULL, COMMAND_POINT_CLASSNAME ) )
	{
		CreateEntityByName( COMMAND_POINT_CLASSNAME );
	}
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool CNPC_MetroPolice::ShouldAlwaysThink() 
{ 
	return ( BaseClass::ShouldAlwaysThink() || IsInPlayerSquad() ); 
}

//-----------------------------------------------------------------------------
// Update weapon ranges
//-----------------------------------------------------------------------------
void CNPC_MetroPolice::Weapon_Equip( CBaseCombatWeapon *pWeapon )
{
	BaseClass::Weapon_Equip( pWeapon );

	if ( HasSpawnFlags(SF_METROPOLICE_MID_RANGE_ATTACK) && GetActiveWeapon() )
	{
		GetActiveWeapon()->m_fMaxRange1 = METROPOLICE_MID_RANGE_ATTACK_RANGE;
		GetActiveWeapon()->m_fMaxRange2 = METROPOLICE_MID_RANGE_ATTACK_RANGE;
	}
}


//-----------------------------------------------------------------------------
// FuncTankBehavior-related sentences
//-----------------------------------------------------------------------------
void CNPC_MetroPolice::SpeakFuncTankSentence( int nSentenceType )
{
	switch ( nSentenceType )
	{
	case FUNCTANK_SENTENCE_MOVE_TO_MOUNT:
		m_Sentences.Speak( "METROPOLICE_FT_APPROACH", SENTENCE_PRIORITY_MEDIUM );
		break;

	case FUNCTANK_SENTENCE_JUST_MOUNTED:
		m_Sentences.Speak( "METROPOLICE_FT_MOUNT", SENTENCE_PRIORITY_HIGH );
		break;

	case FUNCTANK_SENTENCE_SCAN_FOR_ENEMIES:
		m_Sentences.Speak( "METROPOLICE_FT_SCAN", SENTENCE_PRIORITY_NORMAL );
		break;

	case FUNCTANK_SENTENCE_DISMOUNTING:
		m_Sentences.Speak( "METROPOLICE_FT_DISMOUNT", SENTENCE_PRIORITY_HIGH );
		break;
	}
}


//-----------------------------------------------------------------------------
// Standoff Behavior-related sentences
//-----------------------------------------------------------------------------
void CNPC_MetroPolice::SpeakStandoffSentence( int nSentenceType )
{
	switch ( nSentenceType )
	{
	case STANDOFF_SENTENCE_BEGIN_STANDOFF:
		m_Sentences.Speak( "METROPOLICE_SO_BEGIN", SENTENCE_PRIORITY_HIGH, SENTENCE_CRITERIA_SQUAD_LEADER );
		break;

	case STANDOFF_SENTENCE_END_STANDOFF:
		m_Sentences.Speak( "METROPOLICE_SO_END", SENTENCE_PRIORITY_HIGH, SENTENCE_CRITERIA_SQUAD_LEADER );
		break;

	case STANDOFF_SENTENCE_OUT_OF_AMMO:
		AnnounceOutOfAmmo( );
		break;

	case STANDOFF_SENTENCE_FORCED_TAKE_COVER:
		m_Sentences.Speak( "METROPOLICE_SO_FORCE_COVER" );
		break;

	case STANDOFF_SENTENCE_STAND_CHECK_TARGET:
		if ( gm_flTimeLastSpokePeek != 0 && gpGlobals->curtime - gm_flTimeLastSpokePeek > 20 )
		{
			m_Sentences.Speak( "METROPOLICE_SO_PEEK" );
			gm_flTimeLastSpokePeek = gpGlobals->curtime;
		}
		break;
	}
}

//-----------------------------------------------------------------------------
// Assault Behavior-related sentences
//-----------------------------------------------------------------------------
void CNPC_MetroPolice::SpeakAssaultSentence( int nSentenceType )
{
	switch ( nSentenceType )
	{
	case ASSAULT_SENTENCE_HIT_RALLY_POINT:
		m_Sentences.SpeakQueued( "METROPOLICE_AS_HIT_RALLY", SENTENCE_PRIORITY_NORMAL );
		break;

	case ASSAULT_SENTENCE_HIT_ASSAULT_POINT:
		m_Sentences.SpeakQueued( "METROPOLICE_AS_HIT_ASSAULT", SENTENCE_PRIORITY_NORMAL );
		break;

	case ASSAULT_SENTENCE_SQUAD_ADVANCE_TO_RALLY:
		if ( m_Sentences.Speak( "METROPOLICE_AS_ADV_RALLY", SENTENCE_PRIORITY_MEDIUM, SENTENCE_CRITERIA_SQUAD_LEADER ) >= 0 )
		{
			GetSquad()->BroadcastInteraction( g_interactionMetrocopClearSentenceQueues, NULL );
		}
		break;

	case ASSAULT_SENTENCE_SQUAD_ADVANCE_TO_ASSAULT:
		if ( m_Sentences.Speak( "METROPOLICE_AS_ADV_ASSAULT", SENTENCE_PRIORITY_MEDIUM, SENTENCE_CRITERIA_SQUAD_LEADER ) >= 0 )
		{
			GetSquad()->BroadcastInteraction( g_interactionMetrocopClearSentenceQueues, NULL );
		}
		break;

	case ASSAULT_SENTENCE_COVER_NO_AMMO:
		AnnounceOutOfAmmo( );
		break;

	case ASSAULT_SENTENCE_UNDER_ATTACK:
		m_Sentences.Speak( "METROPOLICE_GO_ALERT" );
		break;
	}
}


//-----------------------------------------------------------------------------
// Speaking while using TASK_SPEAK_SENTENCE
//-----------------------------------------------------------------------------
void CNPC_MetroPolice::SpeakSentence( int nSentenceType )
{
	//TERO: If we are special, then don't use this normal CP stuff
	if (IsUnique())
	{
		if (GetRunningBehavior() == &m_RechargeBehavior)
		{
			if (nSentenceType == RECHARGE_SENTENCE_STARTING_RECHARGE)
			{
				Speak( TLK_CP_RECHARGING ); //IfAllowed
			}

			return;
		}
	}

	if ( nSentenceType >= SENTENCE_BASE_BEHAVIOR_INDEX )
	{
		if ( GetRunningBehavior() == &m_FuncTankBehavior )
		{
			SpeakFuncTankSentence( nSentenceType );
			return;
		}

		if ( GetRunningBehavior() == &m_StandoffBehavior )
		{
			SpeakStandoffSentence( nSentenceType );
			return;
		}

		if ( GetRunningBehavior() == &m_AssaultBehavior )
		{
			SpeakAssaultSentence( nSentenceType );
			return;
		}
	}

	switch ( nSentenceType )
	{
	case METROPOLICE_SENTENCE_FREEZE:
		m_Sentences.Speak( "METROPOLICE_FREEZE", SENTENCE_PRIORITY_MEDIUM, SENTENCE_CRITERIA_NORMAL );
		break;

	case METROPOLICE_SENTENCE_HES_OVER_HERE:
		m_Sentences.Speak( "METROPOLICE_OVER_HERE", SENTENCE_PRIORITY_MEDIUM, SENTENCE_CRITERIA_NORMAL );
		break;

	case METROPOLICE_SENTENCE_HES_RUNNING:
		m_Sentences.Speak( "METROPOLICE_HES_RUNNING", SENTENCE_PRIORITY_HIGH, SENTENCE_CRITERIA_NORMAL );
		break;

	case METROPOLICE_SENTENCE_TAKE_HIM_DOWN:
		m_Sentences.Speak( "METROPOLICE_TAKE_HIM_DOWN", SENTENCE_PRIORITY_HIGH, SENTENCE_CRITERIA_NORMAL );
		break;

/*	case METROPOLICE_SENTENCE_ARREST_IN_POSITION:
		m_Sentences.Speak( "METROPOLICE_ARREST_IN_POS", SENTENCE_PRIORITY_MEDIUM, SENTENCE_CRITERIA_NORMAL );
		break;*/

	case METROPOLICE_SENTENCE_DEPLOY_MANHACK:
		m_Sentences.Speak( "METROPOLICE_DEPLOY_MANHACK" );
		break;

	case METROPOLICE_SENTENCE_MOVE_INTO_POSITION:
		{
			CBaseEntity *pEntity = GetEnemy();
			
			// NOTE: This is a good time to check to see if the player is hurt.
			// Have the cops notice this and call out
			if ( pEntity  ) //&& !HasSpawnFlags( SF_METROPOLICE_ARREST_ENEMY ) )
			{
				if ( pEntity->IsPlayer() && (pEntity->GetHealth() <= 20) )
				{
					if ( !HasMemory(bits_MEMORY_PLAYER_HURT) ) 
					{
						if ( m_Sentences.Speak( "METROPOLICE_PLAYERHIT", SENTENCE_PRIORITY_HIGH ) >= 0 )
						{
							m_pSquad->SquadRemember(bits_MEMORY_PLAYER_HURT);
						}
					}
				}

				if ( GetNavigator()->GetPath()->GetPathLength() > 20 * 12.0f )
				{
					m_Sentences.Speak( "METROPOLICE_FLANK" );
				}
			}
		}
		break;

	case METROPOLICE_SENTENCE_HEARD_SOMETHING:
		if ( ( GetState() == NPC_STATE_ALERT ) || ( GetState() == NPC_STATE_IDLE ) )
		{
			m_Sentences.Speak( "METROPOLICE_HEARD_SOMETHING", SENTENCE_PRIORITY_MEDIUM );
		}
		break;
	case METROPOLICE_SENTENCE_AFFIRMATIVE:
		m_Sentences.Speak( "METROPOLICE_IDLE_ANSWER_CR", SENTENCE_PRIORITY_HIGH );
		break;
	case METROPOLICE_SENTENCE_CANT_MOVE:
		m_Sentences.Speak( "METROPOLICE_CANT_MOVE", SENTENCE_PRIORITY_HIGH );
		break;
	}
}

/*void CNPC_MetroPolice::InputAnswerQuestion( inputdata_t &inputdata )
{

}*/

void CNPC_MetroPolice::AnswerQuestion( CAI_PlayerAlly *pQuestioner, int iQARandomNum, bool bAnsweringHello )
{
	if (!IsUnique())
	{
		m_Sentences.Speak( "METROPOLICE_IDLE_ANSWER", SENTENCE_PRIORITY_HIGH, SENTENCE_CRITERIA_NORMAL );
	}
	else
	{
		BaseClass::AnswerQuestion( pQuestioner, iQARandomNum, bAnsweringHello );
	}
}

//-----------------------------------------------------------------------------
// Speaking
//-----------------------------------------------------------------------------
void CNPC_MetroPolice::AnnounceEnemyType( CBaseEntity *pEnemy )
{
	if ( !pEnemy || !m_pSquad )
		return;

	// Don't announce enemies when I'm in arrest behavior
	//if ( HasSpawnFlags( SF_METROPOLICE_ARREST_ENEMY ) )
	//	return;

	if ( m_pSquad->IsLeader( this ) || ( m_pSquad->GetLeader() && m_pSquad->GetLeader()->GetEnemy() != GetEnemy() ) )
	{
		// First contact, and I'm the squad leader.
		const char *pSentenceName = "METROPOLICE_MONST";
		switch ( pEnemy->Classify() )
		{
		case CLASS_PLAYER:
			{
				CBasePlayer *pPlayer = assert_cast<CBasePlayer*>( pEnemy );
				if ( pPlayer && pPlayer->IsInAVehicle() )
				{
					pSentenceName = "METROPOLICE_MONST_PLAYER_VEHICLE";
				}
				else
				{
					pSentenceName = "METROPOLICE_MONST_PLAYER";
				}
			}
			break;

		case CLASS_PLAYER_ALLY:
		case CLASS_CITIZEN_REBEL:
		case CLASS_CITIZEN_PASSIVE:
		case CLASS_VORTIGAUNT:
			pSentenceName = "METROPOLICE_MONST_CITIZENS";
			break;

		case CLASS_PLAYER_ALLY_VITAL:
			pSentenceName = "METROPOLICE_MONST_CHARACTER";
			break;

		case CLASS_ANTLION:
			pSentenceName = "METROPOLICE_MONST_BUGS";
			break;

		case CLASS_ZOMBIE:
			pSentenceName = "METROPOLICE_MONST_ZOMBIES";
			break;

		case CLASS_HEADCRAB:
		case CLASS_BARNACLE:
			pSentenceName = "METROPOLICE_MONST_PARASITES";
			break;
		}

		m_Sentences.Speak( pSentenceName, SENTENCE_PRIORITY_HIGH );
	}
	else
	{
		if ( m_pSquad->GetLeader() && FOkToMakeSound( SENTENCE_PRIORITY_MEDIUM ) )
		{
			// squelch anything that isn't high priority so the leader can speak
			JustMadeSound( SENTENCE_PRIORITY_MEDIUM );	
		}
	}

}


//-----------------------------------------------------------------------------
// Speaking
//-----------------------------------------------------------------------------
void CNPC_MetroPolice::AnnounceEnemyKill( CBaseEntity *pEnemy )
{
	if ( !pEnemy )
		return;

	const char *pSentenceName = "METROPOLICE_KILL_MONST";
	switch ( pEnemy->Classify() )
	{
	case CLASS_PLAYER:
		pSentenceName = "METROPOLICE_KILL_PLAYER";
		break;

	// no sentences for these guys yet
	case CLASS_PLAYER_ALLY:
	case CLASS_CITIZEN_REBEL:
	case CLASS_CITIZEN_PASSIVE:
	case CLASS_VORTIGAUNT:
		pSentenceName = "METROPOLICE_KILL_CITIZENS";
		break;

	case CLASS_PLAYER_ALLY_VITAL:
		pSentenceName = "METROPOLICE_KILL_CHARACTER";
		break;

	case CLASS_ANTLION:
		pSentenceName = "METROPOLICE_KILL_BUGS";
		break;

	case CLASS_ZOMBIE:
		pSentenceName = "METROPOLICE_KILL_ZOMBIES";
		break;

	case CLASS_HEADCRAB:
	case CLASS_BARNACLE:
		pSentenceName = "METROPOLICE_KILL_PARASITES";
		break;
	}

	m_Sentences.Speak( pSentenceName, SENTENCE_PRIORITY_HIGH );
}


//-----------------------------------------------------------------------------
// Announce out of ammo
//-----------------------------------------------------------------------------
void CNPC_MetroPolice::AnnounceOutOfAmmo( )
{
	if ( HasCondition( COND_NO_PRIMARY_AMMO ) )
	{
		m_Sentences.Speak( "METROPOLICE_COVER_NO_AMMO" );
	}
	else
	{
		m_Sentences.Speak( "METROPOLICE_COVER_LOW_AMMO" );
	}
}

//-----------------------------------------------------------------------------
// We're taking cover from danger
//-----------------------------------------------------------------------------
void CNPC_MetroPolice::AnnounceTakeCoverFromDanger( CSound *pSound )
{
	CBaseEntity *pSoundOwner = pSound->m_hOwner;
	if ( pSoundOwner )
	{
		CBaseGrenade *pGrenade = dynamic_cast<CBaseGrenade *>(pSoundOwner);
		if ( pGrenade )
		{
			if ( IRelationType( pGrenade->GetThrower() ) != D_LI )
			{
				// special case call out for enemy grenades
				m_Sentences.Speak( "METROPOLICE_DANGER_GREN", SENTENCE_PRIORITY_HIGH, SENTENCE_CRITERIA_NORMAL );
			}
			return;
		}

		if ( pSoundOwner->GetServerVehicle() )
		{
			m_Sentences.Speak( "METROPOLICE_DANGER_VEHICLE", SENTENCE_PRIORITY_HIGH, SENTENCE_CRITERIA_NORMAL );
			return;
		}

		if ( FClassnameIs( pSoundOwner, "npc_manhack" ) )
		{
			if ( pSoundOwner->HasPhysicsAttacker( 1.0f ) )
			{
				m_Sentences.Speak( "METROPOLICE_DANGER_MANHACK", SENTENCE_PRIORITY_HIGH, SENTENCE_CRITERIA_NORMAL );
			}
			return;
		}
	}

	// I hear something dangerous, probably need to take cover.
	// dangerous sound nearby!, call it out
	const char *pSentenceName = "METROPOLICE_DANGER";
	m_Sentences.Speak( pSentenceName, SENTENCE_PRIORITY_HIGH, SENTENCE_CRITERIA_NORMAL );
}


				
//-----------------------------------------------------------------------------
// Which entity are we actually trying to shoot at?
//-----------------------------------------------------------------------------
/*CBaseEntity *CNPC_MetroPolice::GetShootTarget()
{
	// Should this be a condition??
	CBaseEntity *pEnemy = GetEnemy();
	if ( !pEnemy || !pEnemy->IsPlayer() )
		return pEnemy;

	CBaseEntity *pVehicle = static_cast<CBasePlayer*>( pEnemy )->GetVehicleEntity(); 
	return pVehicle ? pVehicle : pEnemy;
}*/


//-----------------------------------------------------------------------------
// Set up the shot regulator based on the equipped weapon
//-----------------------------------------------------------------------------

// Ranges across which to tune fire rates
const float MIN_PISTOL_MODIFY_DIST = 15 * 12;
const float MAX_PISTOL_MODIFY_DIST = 150 * 12;

// Range for rest period minimums
const float MIN_MIN_PISTOL_REST_INTERVAL = 0.6;
const float MAX_MIN_PISTOL_REST_INTERVAL = 1.2;

// Range for rest period maximums
const float MIN_MAX_PISTOL_REST_INTERVAL = 1.2;
const float MAX_MAX_PISTOL_REST_INTERVAL = 2.0;

// Range for burst minimums
const int 	MIN_MIN_PISTOL_BURST = 2;
const int 	MAX_MIN_PISTOL_BURST = 4;

// Range for burst maximums
const int 	MIN_MAX_PISTOL_BURST = 5;
const int 	MAX_MAX_PISTOL_BURST = 8;

void CNPC_MetroPolice::OnUpdateShotRegulator( )
{
	BaseClass::OnUpdateShotRegulator();

	// FIXME: This code (except the burst interval) could be used for all weapon types 
	if( Weapon_OwnsThisType( "weapon_pistol" ) ) //|| Weapon_OwnsThisType( "weapon_357") )
	{
		if ( GetEnemy() )
		{
			float dist = WorldSpaceCenter().DistTo( GetEnemy()->WorldSpaceCenter() );
				
			dist = clamp( dist, MIN_PISTOL_MODIFY_DIST, MAX_PISTOL_MODIFY_DIST );
				
			float factor = (dist - MIN_PISTOL_MODIFY_DIST) / (MAX_PISTOL_MODIFY_DIST - MIN_PISTOL_MODIFY_DIST);
				
			int		nMinBurst			= MIN_MIN_PISTOL_BURST + ( MAX_MIN_PISTOL_BURST - MIN_MIN_PISTOL_BURST ) * (1.0 - factor);
			int		nMaxBurst			= MIN_MAX_PISTOL_BURST + ( MAX_MAX_PISTOL_BURST - MIN_MAX_PISTOL_BURST ) * (1.0 - factor);
			float	flMinRestInterval	= MIN_MIN_PISTOL_REST_INTERVAL + ( MAX_MIN_PISTOL_REST_INTERVAL - MIN_MIN_PISTOL_REST_INTERVAL ) * factor;
			float	flMaxRestInterval	= MIN_MAX_PISTOL_REST_INTERVAL + ( MAX_MAX_PISTOL_REST_INTERVAL - MIN_MAX_PISTOL_REST_INTERVAL ) * factor;
				
			GetShotRegulator()->SetRestInterval( flMinRestInterval, flMaxRestInterval );
			GetShotRegulator()->SetBurstShotCountRange( nMinBurst, nMaxBurst );
		}
		else
		{
			GetShotRegulator()->SetBurstShotCountRange(GetActiveWeapon()->GetMinBurst(), GetActiveWeapon()->GetMaxBurst() );
			GetShotRegulator()->SetRestInterval( 0.6, 1.4 );
		}

		// Add some noise into the pistol
		GetShotRegulator()->SetBurstInterval( 0.2f, 0.5f );
	}
}




//-----------------------------------------------------------------------------
// Choose a random vector somewhere between the two specified vectors
//-----------------------------------------------------------------------------
/*void CNPC_MetroPolice::RandomDirectionBetweenVectors( const Vector &vecStart, const Vector &vecEnd, Vector *pResult )
{
	Assert( fabs( vecStart.Length() - 1.0f ) < 1e-3 );
	Assert( fabs( vecEnd.Length() - 1.0f ) < 1e-3 );

	float flCosAngle = DotProduct( vecStart, vecEnd );
	if ( fabs( flCosAngle - 1.0f ) < 1e-3 )
	{
		*pResult = vecStart;
		return;
	}

	Vector vecNormal;
	CrossProduct( vecStart, vecEnd, vecNormal );
	float flLength = VectorNormalize( vecNormal );
	if ( flLength < 1e-3 )
	{
		// This is wrong for anti-parallel vectors. so what?
		*pResult = vecStart;
		return;
	}

	// Rotate the starting angle the specified amount
	float flAngle = acos(flCosAngle) * random->RandomFloat( 0.0f, 1.0f );
	VMatrix rotationMatrix;
	MatrixBuildRotationAboutAxis( rotationMatrix, vecNormal, flAngle );
	Vector3DMultiply( rotationMatrix, vecStart, *pResult );
}*/


//-----------------------------------------------------------------------------
// Compute a predicted shoot target position n seconds into the future
//-----------------------------------------------------------------------------
/*void CNPC_MetroPolice::PredictShootTargetPosition( float flDeltaTime, float flMinLeadDist, float flAddVelocity, Vector *pVecTarget, Vector *pVecTargetVelocity )
{
	CBaseEntity *pShootTarget = GetShootTarget();
	*pVecTarget = StitchAimTarget( GetAbsOrigin(), true );

	Vector vecSmoothedVel = pShootTarget->GetSmoothedVelocity();

	// When we're in the air, don't predict vertical motion
	if( (pShootTarget->GetFlags() & FL_ONGROUND) == 0 )
	{
		vecSmoothedVel.z = 0.0f;
	}

	Vector vecVelocity;
	AngularImpulse angImpulse;
	GetShootTarget()->GetVelocity( &vecVelocity, &angImpulse );

	Vector vecLeadVector;
	VMatrix rotationMatrix;
	float flAngVel = VectorNormalize( angImpulse );
	flAngVel -= 30.0f;
	if ( flAngVel > 0.0f )
	{
		MatrixBuildRotationAboutAxis( rotationMatrix, angImpulse, flAngVel * flDeltaTime * 0.333f );
		Vector3DMultiply( rotationMatrix, vecSmoothedVel, vecLeadVector );
	}
	else
	{
		vecLeadVector = vecSmoothedVel;
	}

	if ( flAddVelocity != 0.0f )
	{
		Vector vecForward;
		pShootTarget->GetVectors( &vecForward, NULL, NULL );
		VectorMA( vecLeadVector, flAddVelocity, vecForward,	vecLeadVector );
	}

	*pVecTargetVelocity = vecLeadVector;
	
	if ( (vecLeadVector.LengthSqr() * flDeltaTime * flDeltaTime) < flMinLeadDist * flMinLeadDist )
	{
		VectorNormalize( vecLeadVector );
		vecLeadVector *= flMinLeadDist;
	}
	else
	{
		vecLeadVector *= flDeltaTime;
	}

	*pVecTarget += vecLeadVector;
}


//-----------------------------------------------------------------------------
// Compute a predicted velocity n seconds into the future (given a known acceleration rate)
//-----------------------------------------------------------------------------
void CNPC_MetroPolice::PredictShootTargetVelocity( float flDeltaTime, Vector *pVecTargetVel )
{
	*pVecTargetVel = GetShootTarget()->GetSmoothedVelocity();

	// Unless there's a big angular velocity, we can assume he accelerates
	// along the forward direction. Predict acceleration for
	Vector vecForward;
	GetShootTarget()->GetVectors( &vecForward, NULL, NULL );

//	float flBlendFactor = 1.0f;
//	VectorMA( *pVecTargetVel, flBlendFactor * VEHICLE_PREDICT_ACCELERATION, vecForward, *pVecTargetVel );
//	if ( pVecTargetVel->LengthSqr() > (VEHICLE_PREDICT_MAX_SPEED * VEHICLE_PREDICT_MAX_SPEED) )
//	{
//		VectorNormalize( *pVecTargetVel );
//		*pVecTargetVel *= VEHICLE_PREDICT_MAX_SPEED;
//	}
}*/


//-----------------------------------------------------------------------------
// How many shots will I fire in a particular amount of time?
//-----------------------------------------------------------------------------
int CNPC_MetroPolice::CountShotsInTime( float flDeltaTime ) const
{
	return (int)(flDeltaTime / GetActiveWeapon()->GetFireRate() + 0.5f);
}

float CNPC_MetroPolice::GetTimeForShots( int nShotCount ) const
{
	return nShotCount * GetActiveWeapon()->GetFireRate();
}




//-----------------------------------------------------------------------------
// Behaviors! Lovely behaviors
//-----------------------------------------------------------------------------
bool CNPC_MetroPolice::CreateBehaviors()
{
	AddBehavior( &m_RappelBehavior );
	//AddBehavior( &m_FollowBehavior );
	AddBehavior( &m_PolicingBehavior );
	//AddBehavior( &m_ActBusyBehavior );
	//AddBehavior( &m_AssaultBehavior );
	//AddBehavior( &m_StandoffBehavior );
	AddBehavior( &m_FuncTankBehavior );

	//HLSS: TERO
	AddBehavior( &m_RechargeBehavior );
	
	return BaseClass::CreateBehaviors();
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
#define METROPOLICE_FOLLOWER_DESERT_FUNCTANK_DIST	45.0f*12.0f
bool CNPC_MetroPolice::ShouldBehaviorSelectSchedule( CAI_BehaviorBase *pBehavior )
{
	if( pBehavior == &m_FollowBehavior )
	{
		// Suppress follow behavior if I have a func_tank and the func tank is near
		// what I'm supposed to be following.
		if( m_FuncTankBehavior.CanSelectSchedule() )
		{
			// Is the tank close to the follow target?
			Vector vecTank = m_FuncTankBehavior.GetFuncTank()->WorldSpaceCenter();
			Vector vecFollowGoal = m_FollowBehavior.GetFollowGoalInfo().position;

			float flTankDistSqr = (vecTank - vecFollowGoal).LengthSqr();
			float flAllowDist = m_FollowBehavior.GetFollowGoalInfo().followPointTolerance * 2.0f;
			float flAllowDistSqr = flAllowDist * flAllowDist;
			if( flTankDistSqr < flAllowDistSqr )
			{
				// Deny follow behavior so the tank can go.
				return false;
			}
		}
	}
	else if( IsInPlayerSquad() && pBehavior == &m_FuncTankBehavior && m_FuncTankBehavior.IsMounted() )
	{
		if( m_FollowBehavior.GetFollowTarget() )
		{
			Vector vecFollowGoal = m_FollowBehavior.GetFollowTarget()->GetAbsOrigin();
			if( vecFollowGoal.DistToSqr( GetAbsOrigin() ) > Square(METROPOLICE_FOLLOWER_DESERT_FUNCTANK_DIST) )
			{
				return false;
			}
		}
	}

	return BaseClass::ShouldBehaviorSelectSchedule( pBehavior );
}

void CNPC_MetroPolice::InputEnableManhackToss( inputdata_t &inputdata )
{
	if ( HasSpawnFlags( SF_METROPOLICE_NO_MANHACK_DEPLOY ) )
	{
		RemoveSpawnFlags( SF_METROPOLICE_NO_MANHACK_DEPLOY );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &inputdata - 
//-----------------------------------------------------------------------------
void CNPC_MetroPolice::InputSetPoliceGoal( inputdata_t &inputdata )
{
	CBaseEntity *pGoal = gEntList.FindEntityByName( NULL, inputdata.value.String() );

	if ( pGoal == NULL )
	{
		DevMsg( "SetPoliceGoal: %s (%s) unable to find ai_goal_police: %s\n", GetClassname(), GetDebugName(), inputdata.value.String() );
		return;
	}

	CAI_PoliceGoal *pPoliceGoal = dynamic_cast<CAI_PoliceGoal *>(pGoal);

	if ( pPoliceGoal == NULL )
	{
		DevMsg( "SetPoliceGoal: %s (%s)'s target %s is not an ai_goal_police entity!\n", GetClassname(), GetDebugName(), inputdata.value.String() );
		return;
	}

	m_PolicingBehavior.Enable( pPoliceGoal );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &inputdata - 
//-----------------------------------------------------------------------------
void CNPC_MetroPolice::InputActivateBaton( inputdata_t &inputdata )
{
	SetBatonState( inputdata.value.Bool() );
}


//-----------------------------------------------------------------------------
// Purpose: 
//
//-----------------------------------------------------------------------------
void CNPC_MetroPolice::AlertSound( void )
{
	if (!IsUnique())
	{
		m_Sentences.Speak( "METROPOLICE_GO_ALERT" );
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//
//-----------------------------------------------------------------------------
void CNPC_MetroPolice::DeathSound( const CTakeDamageInfo &info )
{
	if ( IsOnFire() )
		return;

	m_Sentences.Speak( "METROPOLICE_DIE", SENTENCE_PRIORITY_INVALID, SENTENCE_CRITERIA_ALWAYS );
}


//-----------------------------------------------------------------------------
// Purpose: implemented by subclasses to give them an opportunity to make
//			a sound when they lose their enemy
// Input  :
// Output :
//-----------------------------------------------------------------------------
void CNPC_MetroPolice::LostEnemySound( void)
{
	if (IsUnique())
	{
		return;
	}

	if ( gpGlobals->curtime <= m_flNextLostSoundTime )
		return;

	const char *pSentence;
	if (!(CBaseEntity*)GetEnemy() || gpGlobals->curtime - GetEnemyLastTimeSeen() > 10)
	{
		pSentence = "METROPOLICE_LOST_LONG"; 
	}
	else
	{
		pSentence = "METROPOLICE_LOST_SHORT";
	}

	if ( m_Sentences.Speak( pSentence ) >= 0 )
	{
		m_flNextLostSoundTime = gpGlobals->curtime + random->RandomFloat(5.0,15.0);
	}
}


//-----------------------------------------------------------------------------
// Purpose: implemented by subclasses to give them an opportunity to make
//			a sound when they lose their enemy
// Input  :
// Output :
//-----------------------------------------------------------------------------
void CNPC_MetroPolice::FoundEnemySound( void)
{
	if (IsUnique())
	{
		return;
	}

	// Don't announce enemies when I'm in arrest behavior
	//if ( HasSpawnFlags( SF_METROPOLICE_ARREST_ENEMY ) )
	//	return;

	m_Sentences.Speak( "METROPOLICE_REFIND_ENEMY", SENTENCE_PRIORITY_HIGH );
}


//-----------------------------------------------------------------------------
// Purpose: Indicates whether or not this npc should play an idle sound now.
//-----------------------------------------------------------------------------
bool CNPC_MetroPolice::ShouldPlayIdleSound( void )
{
	// If someone is waiting for a response, then respond!
	if ( ( m_NPCState == NPC_STATE_IDLE ) || ( m_NPCState == NPC_STATE_ALERT ) )
	{
		if ( m_nIdleChatterType >= METROPOLICE_CHATTER_RESPONSE )
			return FOkToMakeSound();
	}

	return BaseClass::ShouldPlayIdleSound();
}


//-----------------------------------------------------------------------------
// IdleSound 
//-----------------------------------------------------------------------------
void CNPC_MetroPolice::IdleSound( void )
{
	if (IsUnique())
	{
		return;
	}

	bool bIsCriminal = (GlobalEntity_GetState("gordon_precriminal") == GLOBAL_ON);

	// This happens when the NPC is waiting for his buddies to respond to him
	switch( m_nIdleChatterType )
	{
	case METROPOLICE_CHATTER_WAIT_FOR_RESPONSE:
		break;

	case METROPOLICE_CHATTER_ASK_QUESTION:
		{
			if ( m_bPlayerIsNear && !HasMemory(bits_MEMORY_PLAYER_HARASSED) )
			{
				if ( m_Sentences.Speak( "METROPOLICE_IDLE_HARASS_PLAYER", SENTENCE_PRIORITY_NORMAL, SENTENCE_CRITERIA_NORMAL ) >= 0 )
				{
					Remember( bits_MEMORY_PLAYER_HARASSED );
					if ( GetSquad() )
					{
						GetSquad()->SquadRemember(bits_MEMORY_PLAYER_HARASSED);
					}
				}
				return;
			}

			if ( !random->RandomInt(0,1) )
				break;

			int nQuestionType = random->RandomInt( 0, METROPOLICE_CHATTER_RESPONSE_TYPE_COUNT );
			if ( !IsInSquad() || ( nQuestionType == METROPOLICE_CHATTER_RESPONSE_TYPE_COUNT ) )
			{
				m_Sentences.Speak( bIsCriminal ? "METROPOLICE_IDLE_CR" : "METROPOLICE_IDLE" );
				break;
			}

			static const char *pQuestion[2][METROPOLICE_CHATTER_RESPONSE_TYPE_COUNT] = 
			{
				{ "METROPOLICE_IDLE_CHECK",		"METROPOLICE_IDLE_QUEST" },
				{ "METROPOLICE_IDLE_CHECK_CR",	"METROPOLICE_IDLE_QUEST_CR" },
			};

			if ( m_Sentences.Speak( pQuestion[bIsCriminal][nQuestionType] ) >= 0 )
			{
				GetSquad()->BroadcastInteraction( g_interactionMetrocopIdleChatter, (void*)(METROPOLICE_CHATTER_RESPONSE + nQuestionType), this );
				m_nIdleChatterType = METROPOLICE_CHATTER_WAIT_FOR_RESPONSE;
			}
		}
		break;

	default:
		{
			int nResponseType = m_nIdleChatterType - METROPOLICE_CHATTER_RESPONSE;

			static const char *pResponse[2][METROPOLICE_CHATTER_RESPONSE_TYPE_COUNT] = 
			{
				{ "METROPOLICE_IDLE_CLEAR",		"METROPOLICE_IDLE_ANSWER" },
				{ "METROPOLICE_IDLE_CLEAR_CR",	"METROPOLICE_IDLE_ANSWER_CR" },
			};

			if ( m_Sentences.Speak( pResponse[bIsCriminal][nResponseType] ) >= 0 )
			{
				GetSquad()->BroadcastInteraction( g_interactionMetrocopIdleChatter, (void*)(METROPOLICE_CHATTER_ASK_QUESTION), this );
				m_nIdleChatterType = METROPOLICE_CHATTER_ASK_QUESTION;
			}
		}
		break;
	}
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
void CNPC_MetroPolice::InputSpeakRecharge( inputdata_t &inputdata )
{
	Speak( TLK_CP_RECHARGE ); //IfAllowed
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
void CNPC_MetroPolice::InputSpeakGeneratorOffline( inputdata_t &inputdata )
{
	Speak( TLK_CP_GENERATOR_OFFLINE ); //IfAllowed
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_MetroPolice::PainSound( const CTakeDamageInfo &info )
{
	if ( gpGlobals->curtime < m_flNextPainSoundTime )
		return;

	if (IsUnique())
	{	
		if (!IsMedic() && m_iHealth < (m_iMaxHealth / 2))
		{
			//DevMsg("Metrocop health %d / %d\n", m_iHealth, m_iMaxHealth);
			SpeakIfAllowed( TLK_CP_MEDIC );
		}
		else
		{
			SpeakIfAllowed( TLK_WOUND );
		}

		/*if (random->RandomInt(0,4) == 0	&& info.GetAttacker())//FClassnameIs(info.GetAttacker(), "bee_missile"))
		{
			SpeakIfAllowed( TLK_CP_BEES );
		}
		else
		{*/
			
		//}

		m_flNextPainSoundTime = gpGlobals->curtime + 1;
		return;
	}

	// Don't make pain sounds if I'm on fire. The looping sound will take care of that for us.
	if ( IsOnFire() )
		return;

	float healthRatio = (float)GetHealth() / (float)GetMaxHealth();
	if ( healthRatio > 0.0f )
	{
		const char *pSentenceName = "METROPOLICE_PAIN";
		if ( !HasMemory(bits_MEMORY_PAIN_HEAVY_SOUND) && (healthRatio < 0.25f) )
		{
			Remember( bits_MEMORY_PAIN_HEAVY_SOUND | bits_MEMORY_PAIN_LIGHT_SOUND );
			pSentenceName = "METROPOLICE_PAIN_HEAVY";
		}
		else if ( !HasMemory(bits_MEMORY_PAIN_LIGHT_SOUND) && healthRatio > 0.8f )
		{
			Remember( bits_MEMORY_PAIN_LIGHT_SOUND );
			pSentenceName = "METROPOLICE_PAIN_LIGHT";
		}
		
		// This causes it to speak it no matter what; doesn't bother with setting sounds.
		m_Sentences.Speak( pSentenceName, SENTENCE_PRIORITY_INVALID, SENTENCE_CRITERIA_ALWAYS );
		m_flNextPainSoundTime = gpGlobals->curtime + 1;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CNPC_MetroPolice::GetSoundInterests( void )
{
	return SOUND_WORLD | SOUND_COMBAT | SOUND_PLAYER | SOUND_PLAYER_VEHICLE | SOUND_DANGER | 
		SOUND_PHYSICS_DANGER | SOUND_BULLET_IMPACT | SOUND_MOVE_AWAY;
}

bool CNPC_MetroPolice::IsAllowedToSpeak( AIConcept_t concept, bool bRespondingToPlayer )
{
	return (IsUnique() && BaseClass::IsAllowedToSpeak( concept, bRespondingToPlayer ));
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
float CNPC_MetroPolice::MaxYawSpeed( void )
{
	switch( GetActivity() )
	{
	case ACT_TURN_LEFT:
	case ACT_TURN_RIGHT:
		return 120;

	case ACT_RUN:
	case ACT_RUN_HURT:
		return 15;

	case ACT_WALK:
	case ACT_WALK_CROUCH:
	case ACT_RUN_CROUCH:
		return 25;

	default:
		return 45;
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//
//
//-----------------------------------------------------------------------------
Class_T	CNPC_MetroPolice::Classify ( void )
{
	if (IsUnique())
	{
		return CLASS_PLAYER_ALLY_VITAL;
	}
	
	return CLASS_METROPOLICE;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pEvent - 
//-----------------------------------------------------------------------------
void CNPC_MetroPolice::OnAnimEventStartDeployManhack( void )
{
	Assert( m_iManhacks );
	
	if ( m_iManhacks <= 0 )
	{
		DevMsg( "Error: Throwing manhack but out of manhacks!\n" );
		return;
	}

	m_iManhacks--;

	// Turn off the manhack on our body
	if ( m_iManhacks <= 0 )
	{
		SetBodygroup( METROPOLICE_BODYGROUP_MANHACK, false );
	}

	// Create the manhack to throw
	CNPC_Manhack *pManhack = (CNPC_Manhack *)CreateEntityByName( "npc_manhack" );
	
	Vector	vecOrigin;
	QAngle	vecAngles;

	int handAttachment = LookupAttachment( "LHand" );
	GetAttachment( handAttachment, vecOrigin, vecAngles );

	pManhack->SetLocalOrigin( vecOrigin );
	pManhack->SetLocalAngles( vecAngles );
	pManhack->AddSpawnFlags( (SF_MANHACK_PACKED_UP|SF_MANHACK_CARRIED|SF_NPC_WAIT_FOR_SCRIPT) );
	
	// Also fade if our parent is marked to do it
	if ( HasSpawnFlags( SF_NPC_FADE_CORPSE ) )
	{
		pManhack->AddSpawnFlags( SF_NPC_FADE_CORPSE );
	}

	pManhack->Spawn();

	// Make us move with his hand until we're deployed
	pManhack->SetParent( this, handAttachment );

	m_hManhack = pManhack;
}

//-----------------------------------------------------------------------------
// Anim event handlers
//-----------------------------------------------------------------------------
void CNPC_MetroPolice::OnAnimEventDeployManhack( animevent_t *pEvent )
{
	// Let it go
	ReleaseManhack();

	Vector forward, right;
	GetVectors( &forward, &right, NULL );

	IPhysicsObject *pPhysObj = m_hManhack->VPhysicsGetObject();

	if ( pPhysObj )
	{
		Vector	yawOff = right * random->RandomFloat( -1.0f, 1.0f );

		Vector	forceVel = ( forward + yawOff * 16.0f ) + Vector( 0, 0, 250 );
		Vector	forceAng = vec3_origin;

		// Give us velocity
		pPhysObj->AddVelocity( &forceVel, &forceAng );
	}

	// Stop dealing with this manhack
	m_hManhack = NULL;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_MetroPolice::OnAnimEventShove( void )
{
	CBaseEntity *pHurt = CheckTraceHullAttack( 16, Vector(-16,-16,-16), Vector(16,16,16), 15, DMG_CLUB, 1.0f, false );

	if ( pHurt )
	{
		Vector vecForceDir = ( pHurt->WorldSpaceCenter() - WorldSpaceCenter() );

		CBasePlayer *pPlayer = ToBasePlayer( pHurt );

		if ( pPlayer != NULL )
		{
			//Kick the player angles
			pPlayer->ViewPunch( QAngle( 8, 14, 0 ) );

			Vector	dir = pHurt->GetAbsOrigin() - GetAbsOrigin();
			VectorNormalize(dir);

			QAngle angles;
			VectorAngles( dir, angles );
			Vector forward, right;
			AngleVectors( angles, &forward, &right, NULL );

			//If not on ground, then don't make them fly!
			if ( !(pHurt->GetFlags() & FL_ONGROUND ) )
				  forward.z = 0.0f;

			//Push the target back
			pHurt->ApplyAbsVelocityImpulse( forward * 250.0f );

			// Force the player to drop anyting they were holding
			pPlayer->ForceDropOfCarriedPhysObjects();
		}

		// Play a random attack hit sound
		EmitSound( "NPC_Metropolice.Shove" );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_MetroPolice::OnAnimEventBatonOn( void )
{
#ifndef HL2MP

	CWeaponStunStick *pStick = dynamic_cast<CWeaponStunStick *>(GetActiveWeapon());

	if ( pStick )
	{
		pStick->SetStunState( true );
	}
#endif

}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_MetroPolice::OnAnimEventBatonOff( void )
{
#ifndef HL2MP

	CWeaponStunStick *pStick = dynamic_cast<CWeaponStunStick *>(GetActiveWeapon());
	
	if ( pStick )
	{
		pStick->SetStunState( false );
	}
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//
// Input  : *pEvent - 
//
//-----------------------------------------------------------------------------
void CNPC_MetroPolice::HandleAnimEvent( animevent_t *pEvent )
{
	bool isHealSchedule = !IsCurSchedule( SCHED_METROPOLICE_DEPLOY_MANHACK );
		//(IsCurSchedule( SCHED_METROPOLICE_HEAL ) || IsCurSchedule( SCHED_METROPOLICE_HEAL_TOSS ) );

	// Shove!
	if ( pEvent->event == AE_METROPOLICE_SHOVE )
	{
		OnAnimEventShove();
		return;
	}

	if ( pEvent->event == AE_METROPOLICE_BATON_ON )
	{
		OnAnimEventBatonOn();
		return;
	}

	if ( pEvent->event == AE_METROPOLICE_BATON_OFF )
	{
		OnAnimEventBatonOff();
		return;
	}

	if ( pEvent->event == AE_METROPOLICE_START_DEPLOY )
	{
		if (!isHealSchedule)
			OnAnimEventStartDeployManhack();
		return;
	}

	if ( pEvent->event == AE_METROPOLICE_DRAW_PISTOL )
	{
		m_fWeaponDrawn = true;
		if( GetActiveWeapon() )
		{
			GetActiveWeapon()->RemoveEffects( EF_NODRAW );
		}
		return;
	}

	if ( pEvent->event == AE_METROPOLICE_DEPLOY_MANHACK )
	{
		if (!isHealSchedule)
			OnAnimEventDeployManhack( pEvent );
		else
			OnAnimEventHeal();
		return;
	}

	if ( pEvent->event == AE_METROPOLICE_TAKE_HEALTHKIT )
	{
		SetBodygroup( METROPOLICE_BODYGROUP_HEALTHKIT, true );
		m_bMedkitHidden = false;
		return;
	}

	if (pEvent->event == AE_METROPOLICE_HEAL_TARGET || 
		pEvent->event == AE_METROPOLICE_HEAL_SELF )
	{
		SetBodygroup( METROPOLICE_BODYGROUP_HEALTHKIT, false );
		m_bMedkitHidden = true;
		OnAnimEventHeal();
		return;
	}

#ifdef ELOISE_KICK_BALLS

	if (pEvent->event == AE_METROPOLICE_KICK_BALLS )
	{
		CBaseEntity *pHurt = CheckTraceHullAttack( 70, -Vector(16,16,18), Vector(16,16,18), 36, DMG_CLUB, 4.0f );
		if (pHurt)
		{
			EmitSound( "NPC_Metropolice.Shove"  );
		}
	}

#endif 

	BaseClass::HandleAnimEvent( pEvent );
}

void CNPC_MetroPolice::OnAnimEventHeal()
{
	CBaseCombatCharacter *pTarget = dynamic_cast<CBaseCombatCharacter *>( GetTarget() );
	Assert(pTarget);
	if ( USE_EXPERIMENTAL_MEDIC_CODE() && IsMedic() && pTarget && pTarget->IsPlayer() )	//TERO: only use this stuff for the player because the NPCs don't seem to be able to pick them up
	{
		m_flPlayerHealTime 	= gpGlobals->curtime + sk_metropolice_heal_toss_player_delay.GetFloat();;
			TossHealthKit( pTarget, Vector(48.0f, 0.0f, 0.0f)  );
	}
	else
	{
		Heal();
	}
}


//-----------------------------------------------------------------------------
// Purpose:  This is a generic function (to be implemented by sub-classes) to
//			 handle specific interactions between different types of characters
//			 (For example the barnacle grabbing an NPC)
// Input  :  Constant for the type of interaction
// Output :	 true  - if sub-class has a response for the interaction
//			 false - if sub-class has no response
//-----------------------------------------------------------------------------
bool CNPC_MetroPolice::HandleInteraction(int interactionType, void *data, CBaseCombatCharacter* sourceEnt)
{
	if ( interactionType == g_interactionMetrocopIdleChatter )
	{
		m_nIdleChatterType = (int)(size_t)data;
		return true;
	}

	if ( interactionType == g_interactionMetrocopClearSentenceQueues )
	{
			m_Sentences.ClearQueue();
			return true;
	}

	// React to being hit by physics objects
	if ( interactionType == g_interactionHitByPlayerThrownPhysObj )
	{
		// Ignore if I'm in scripted state
		if ( !IsInAScript() && (m_NPCState != NPC_STATE_SCRIPT) )
		{
			SetCondition( COND_METROPOLICE_PHYSOBJECT_ASSAULT );
		}
		else
		{
			//TERO: Say something like "Fuck off"
		}

		return true;
	}

	return BaseClass::HandleInteraction( interactionType, data, sourceEnt );
}

#ifdef ELOISE_KICK_BALLS

//-----------------------------------------------------------------------------
// Purpose: For combine melee attack (kick/hit)
// Input  :
// Output :
//-----------------------------------------------------------------------------
int CNPC_MetroPolice::MeleeAttack1Conditions ( float flDot, float flDist )
{
	if (!IsEloise())
	{
		return COND_NONE;
	}

	bool bComplain = ((GetActiveWeapon() == NULL)); // && !(GetActiveWeapon()->CapabilitiesGet() & bits_CAP_WEAPON_RANGE_ATTACK1));

	if (flDist > 80)
	{
		if (bComplain)
		{
			return COND_TOO_FAR_TO_ATTACK;
		}
		else
		{
			return COND_NONE; 
		}
	}
	else if (flDot < 0.7)
	{
		//return COND_NONE; // COND_NOT_FACING_ATTACK;
		if (bComplain)
		{
			return COND_NOT_FACING_ATTACK;
		}
		else
		{
			return COND_NONE; 
		}
	}

	// Check Z
	if ( GetEnemy() && fabs(GetEnemy()->GetAbsOrigin().z - GetAbsOrigin().z) > 64 )
	{
		return COND_NONE;
	}

	if ( dynamic_cast<CBaseHeadcrab *>(GetEnemy()) != NULL )
	{
		return COND_NONE;
	}

	// Make sure not trying to kick through a window or something. 
	trace_t tr;
	Vector vecSrc, vecEnd;

	vecSrc = WorldSpaceCenter();
	vecEnd = GetEnemy()->WorldSpaceCenter();

	AI_TraceLine(vecSrc, vecEnd, MASK_SHOT, this, COLLISION_GROUP_NONE, &tr);
	if( tr.m_pEnt != GetEnemy() )
	{
		return COND_NONE;
	}

	return COND_CAN_MELEE_ATTACK1;
}

#endif


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
Activity CNPC_MetroPolice::NPC_TranslateActivity( Activity newActivity )
{
	if( IsOnFire() && newActivity == ACT_RUN )
	{
		return ACT_RUN_ON_FIRE;
	}

	newActivity = BaseClass::NPC_TranslateActivity( newActivity );

	// This will put him into an angry idle, which will then be translated
	// by the weapon to the appropriate type. 
	if ( m_fWeaponDrawn && newActivity == ACT_IDLE && ( GetState() == NPC_STATE_COMBAT || BatonActive() ) )
	{
		newActivity = ACT_IDLE_ANGRY;
	}

	if (IsEloise())
	{
		//DevMsg("Is Eloise, activity id: %d\n", newActivity );

		if ( newActivity == ACT_IDLE_ANGRY_PISTOL )
			return ACT_IDLE_ANGRY_357;
		if ( newActivity == ACT_RANGE_ATTACK_PISTOL )
			return ACT_RANGE_ATTACK_357;
		if ( newActivity == ACT_GESTURE_RANGE_ATTACK_PISTOL )
			return ACT_GESTURE_RANGE_ATTACK_357;
		if ( newActivity == ACT_METROPOLICE_DRAW_PISTOL )
			return ACT_METROPOLICE_DRAW_357;
		if ( newActivity == ACT_RELOAD_PISTOL )
			return ACT_RELOAD_357;
		if ( newActivity == ACT_GESTURE_RELOAD_PISTOL )
			return ACT_GESTURE_RELOAD_357;
	}

	switch ( newActivity )
	{
	case ACT_RUN_AIM_SHOTGUN:
		return ACT_RUN_AIM_RIFLE;
		break;
	case ACT_WALK_AIM_SHOTGUN:
		return ACT_WALK_AIM_RIFLE;
		break;
	case ACT_IDLE_ANGRY_SHOTGUN:
		return ACT_IDLE_ANGRY_SMG1;
		break;
	case ACT_RANGE_ATTACK_SHOTGUN_LOW:
		return ACT_RANGE_ATTACK_SMG1_LOW;
		break;

	/*case ACT_GESTURE_RANGE_ATTACK1:
		{
		if (SelectWeightedSequence( ACT_GESTURE_RANGE_ATTACK1 ) == ACTIVITY_NOT_AVAILABLE )
			return ACT_GESTURE_RANGE_ATTACK_SMG1;
		}
		break;*/
	case ACT_PICKUP_RACK:
		{
		if (SelectWeightedSequence( ACT_PICKUP_RACK ) == ACTIVITY_NOT_AVAILABLE )
			return ACT_PICKUP_GROUND;
		}
		break;
	}


	//THIS STUFF IS PRETTY AWFUL BUT HEY YOU GOTTA HAVE A SHOTGUN, RIGHT???
	if (!IsUnique())
	{
		switch ( newActivity )
		{
			case ACT_GESTURE_RANGE_ATTACK_SHOTGUN:
			{
				//if (SelectWeightedSequence( ACT_GESTURE_RANGE_ATTACK_SHOTGUN ) == ACTIVITY_NOT_AVAILABLE )
				return ACT_GESTURE_RANGE_ATTACK_SMG1;
			}
			break;

			case ACT_GESTURE_RANGE_ATTACK_AR2:
			{
				//if (SelectWeightedSequence( ACT_GESTURE_RANGE_ATTACK_AR2 ) == ACTIVITY_NOT_AVAILABLE )
				return ACT_GESTURE_RANGE_ATTACK_SMG1;
			}
			break;

			case ACT_RANGE_ATTACK_SHOTGUN:
			{
				//if (SelectWeightedSequence( ACT_RANGE_ATTACK_SHOTGUN ) == ACTIVITY_NOT_AVAILABLE )
				return ACT_RANGE_ATTACK_SMG1;
			}
			break;

			case ACT_RANGE_ATTACK_AR2:
			{
				//if (SelectWeightedSequence( ACT_RANGE_ATTACK_AR2) == ACTIVITY_NOT_AVAILABLE )
				return ACT_RANGE_ATTACK_SMG1;
			}
			break;

			case ACT_IDLE_SHOTGUN_RELAXED:
			case ACT_IDLE_SHOTGUN_STIMULATED:
			case ACT_IDLE_SHOTGUN_AGITATED:
			{
				return ACT_IDLE_ANGRY_SMG1;
			}
			break;

			//case ACT_RELOAD_AR2:
			case ACT_RELOAD_SHOTGUN:
			{
				return ACT_RELOAD_SMG1;
			}
			break;

			case ACT_RELOAD_SHOTGUN_LOW:
			{
				return ACT_RELOAD_SMG1_LOW;
			}
			break;

			/*default:
			{
				DevMsg("Activity id, %d\n", newActivity);
			}
			break;*/
		}
	}

	return newActivity;
}

//-----------------------------------------------------------------------------
// Purpose: Makes the held manhack solid
//-----------------------------------------------------------------------------
void CNPC_MetroPolice::ReleaseManhack( void )
{
	Assert( m_hManhack );

	// Make us physical
	m_hManhack->RemoveSpawnFlags( SF_MANHACK_CARRIED );
	m_hManhack->CreateVPhysics();

	// Release us
	m_hManhack->RemoveSolidFlags( FSOLID_NOT_SOLID );
	m_hManhack->SetMoveType( MOVETYPE_VPHYSICS );
	m_hManhack->SetParent( NULL );

	// Make us active
	m_hManhack->RemoveSpawnFlags( SF_NPC_WAIT_FOR_SCRIPT );
	m_hManhack->ClearSchedule( "Manhack released by metropolice" );
	
	// Start him with knowledge of our current enemy
	if ( GetEnemy() )
	{
		m_hManhack->SetEnemy( GetEnemy() );
		m_hManhack->SetState( NPC_STATE_COMBAT );

		m_hManhack->UpdateEnemyMemory( GetEnemy(), GetEnemy()->GetAbsOrigin() );
	}

	// Place him into our squad so we can communicate
	if ( m_pSquad )
	{
		m_pSquad->AddToSquad( m_hManhack );
	}
}

//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
void CNPC_MetroPolice::Event_Killed( const CTakeDamageInfo &info )
{
	// Release the manhack if we're in the middle of deploying him
	if ( m_hManhack && m_hManhack->IsAlive() )
	{
		ReleaseManhack();
		m_hManhack = NULL;
	}

	CBasePlayer *pPlayer = ToBasePlayer( info.GetAttacker() );

	if ( pPlayer != NULL )
	{
		CHalfLife2 *pHL2GameRules = static_cast<CHalfLife2 *>(g_pGameRules);

		// Attempt to drop health
		if ( pHL2GameRules->NPC_ShouldDropHealth( pPlayer ) )
		{
			DropItem( "item_healthvial", WorldSpaceCenter()+RandomVector(-4,4), RandomAngle(0,360) );
			pHL2GameRules->NPC_DroppedHealth();
		}
	}

	/*if ( IsInPlayerSquad() || Classify() == CLASS_PLAYER_ALLY_VITAL )
	{
		bool forceSound = ( Classify() == CLASS_PLAYER_ALLY_VITAL && IsGameEndAlly() );

		CHLSS_MetrocopRadio * pMetrocopRadio = CHLSS_MetrocopRadio::GetMetrocopRadio();

		if (pMetrocopRadio)
		{
			string_t scriptname_die = MAKE_STRING( "NPC_MetroPolice.Die" );
			pMetrocopRadio->PlayRadio( scriptname_die, forceSound); 
		}
	}*/

	BaseClass::Event_Killed( info );
}

//-----------------------------------------------------------------------------
// Try to enter a slot where we shoot a pistol 
//-----------------------------------------------------------------------------
bool CNPC_MetroPolice::TryToEnterPistolSlot( int nSquadSlot )
{
	// This logic here will not allow us to occupy the a squad slot
	// too soon after we already were in it.
	if ( ( m_LastShootSlot != nSquadSlot || !m_TimeYieldShootSlot.Expired() ) &&
			OccupyStrategySlot( nSquadSlot ) )
	{
		if ( m_LastShootSlot != nSquadSlot )
		{
			m_TimeYieldShootSlot.Reset();
			m_LastShootSlot = nSquadSlot;
		}
		return true;
	}

	return false;
}


//-----------------------------------------------------------------------------
// Combat schedule selection 
//-----------------------------------------------------------------------------
int CNPC_MetroPolice::SelectRangeAttackSchedule()
{
	// Range attack if we're able
	if( TryToEnterPistolSlot( SQUAD_SLOT_ATTACK1 ) || TryToEnterPistolSlot( SQUAD_SLOT_ATTACK2 ))
		return SCHED_RANGE_ATTACK1;
	
	// We're not in a shoot slot... so we've allowed someone else to grab it
	m_LastShootSlot = SQUAD_SLOT_NONE;

	if( CanDeployManhack() && OccupyStrategySlot( SQUAD_SLOT_POLICE_DEPLOY_MANHACK ) )
	{
		return SCHED_METROPOLICE_DEPLOY_MANHACK;
	}

	return SCHED_METROPOLICE_ADVANCE;
}


//-----------------------------------------------------------------------------
// How many squad members are trying to arrest the player?
//-----------------------------------------------------------------------------
/*int CNPC_MetroPolice::SquadArrestCount()
{
	int nCount = 0;

	AISquadIter_t iter;
	CAI_BaseNPC *pSquadmate = m_pSquad->GetFirstMember( &iter );
	while ( pSquadmate )
	{
		if ( pSquadmate->IsCurSchedule(	SCHED_METROPOLICE_ARREST_ENEMY ) ||
			pSquadmate->IsCurSchedule( SCHED_METROPOLICE_WARN_AND_ARREST_ENEMY ) )
		{
			++nCount;
		}

		pSquadmate = m_pSquad->GetNextMember( &iter );
	}

	return nCount;
}*/


//-----------------------------------------------------------------------------
// Arrest schedule selection 
//-----------------------------------------------------------------------------
/*int CNPC_MetroPolice::SelectScheduleArrestEnemy()
{
	if ( !HasSpawnFlags( SF_METROPOLICE_ARREST_ENEMY ) || !IsInSquad() )
		return SCHED_NONE;

	if ( !HasCondition( COND_SEE_ENEMY ) )
		return SCHED_NONE;

	if ( !m_fWeaponDrawn )
		return SCHED_METROPOLICE_DRAW_PISTOL;

	// First guy that sees the enemy will tell him to freeze
	if ( OccupyStrategySlot( SQUAD_SLOT_POLICE_ARREST_ENEMY ) )
		return SCHED_METROPOLICE_WARN_AND_ARREST_ENEMY;

	// Squad members 1 -> n will simply gain a line of sight
	return SCHED_METROPOLICE_ARREST_ENEMY;
}*/


//-----------------------------------------------------------------------------
// Combat schedule selection 
//-----------------------------------------------------------------------------
int CNPC_MetroPolice::SelectScheduleNewEnemy()
{
	/*int nSched = SelectScheduleArrestEnemy();
	if ( nSched != SCHED_NONE )
		return nSched;*/

	if ( HasCondition( COND_NEW_ENEMY ) )
	{
//		m_flNextLedgeCheckTime = gpGlobals->curtime;

		if( CanDeployManhack() && OccupyStrategySlot( SQUAD_SLOT_POLICE_DEPLOY_MANHACK ) )
			return SCHED_METROPOLICE_DEPLOY_MANHACK;
	}

	if ( !m_fWeaponDrawn )
		return SCHED_METROPOLICE_DRAW_PISTOL;

	// Switch our baton on, if it's not already
	if ( HasBaton() && BatonActive() == false && IsCurSchedule( SCHED_METROPOLICE_ACTIVATE_BATON ) == false )
	{
		SetTarget( GetEnemy() );
		SetBatonState( true );
		m_flBatonDebounceTime = gpGlobals->curtime + random->RandomFloat( 2.5f, 4.0f );
		return SCHED_METROPOLICE_ACTIVATE_BATON;
	}

	return SCHED_NONE;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CNPC_MetroPolice::OnObstructionPreSteer( AILocalMoveGoal_t *pMoveGoal, float distClear, AIMoveResult_t *pResult )
{
	if ( pMoveGoal->directTrace.pObstruction )
	{
		// Is it a physics prop? Store it off as the last thing to block me
		CPhysicsProp *pProp = dynamic_cast<CPhysicsProp*>( pMoveGoal->directTrace.pObstruction );
		if ( pProp && pProp->GetHealth() )
		{
			m_hBlockingProp = pProp;
		}
		else
		{
			m_hBlockingProp = NULL;
		}
	}

	return BaseClass::OnObstructionPreSteer( pMoveGoal, distClear, pResult );
}

//-----------------------------------------------------------------------------
// Combat schedule selection 
//-----------------------------------------------------------------------------
int CNPC_MetroPolice::SelectScheduleNoDirectEnemy()
{
	// If you can't attack, but you can deploy a manhack, do it!
	if( CanDeployManhack() && OccupyStrategySlot( SQUAD_SLOT_POLICE_DEPLOY_MANHACK ) )
		return SCHED_METROPOLICE_DEPLOY_MANHACK;

	// If you can't attack, but you have a baton & there's a physics object in front of you, swat it
	if ( m_hBlockingProp && HasBaton() )
	{
		SetTarget( m_hBlockingProp );
		m_hBlockingProp = NULL;
		return SCHED_METROPOLICE_SMASH_PROP;
	}

	//TERO: don't get too close to these guys
	if ( HasCondition( COND_SEE_ENEMY ) && GetEnemy() && 
		 ( GetEnemy()->Classify() == CLASS_ALIENGRUNT || GetEnemy()->Classify() == CLASS_ALIENCONTROLLER ))
	{
		DevMsg("npc_metropolice: selecting SCHED_BACK_AWAY_FROM_ENEMY over SCHED_METROPOLICE_CHASE_ENEMY\n");
	}

	return SCHED_METROPOLICE_CHASE_ENEMY;
}


//-----------------------------------------------------------------------------
// Combat schedule selection 
//-----------------------------------------------------------------------------
int CNPC_MetroPolice::SelectCombatSchedule()
{
	// Announce a new enemy
	if ( HasCondition( COND_NEW_ENEMY ) && !IsUnique() )
	{
		AnnounceEnemyType( GetEnemy() );
	}

	int nResult = SelectScheduleNewEnemy();
	if ( nResult != SCHED_NONE )
		return nResult;

	if( !m_fWeaponDrawn )
	{
		return SCHED_METROPOLICE_DRAW_PISTOL;
	}

	if (!HasBaton() && ((float)m_nRecentDamage / (float)GetMaxHealth()) > RECENT_DAMAGE_THRESHOLD)
	{
		m_nRecentDamage = 0;
		m_flRecentDamageTime = 0;
		if (!IsUnique())
		{
			m_Sentences.Speak( "METROPOLICE_COVER_HEAVY_DAMAGE", SENTENCE_PRIORITY_MEDIUM, SENTENCE_CRITERIA_NORMAL );
		}

		return SCHED_TAKE_COVER_FROM_ENEMY;
	}

	if ( HasCondition( COND_CAN_RANGE_ATTACK1 ) )
	{
		if ( !GetShotRegulator()->IsInRestInterval() )
			return SelectRangeAttackSchedule();
		else
			return SCHED_METROPOLICE_ADVANCE;
	}

	if ( HasCondition( COND_CAN_MELEE_ATTACK1 ) )
	{
		if ( m_BatonSwingTimer.Expired() )
		{
			// Stop chasing the player now that we've taken a swing at them
			m_BatonSwingTimer.Set( 1.0, 1.75 );
			return SCHED_MELEE_ATTACK1;
		}
		else
			return SCHED_COMBAT_FACE;
	}

	if ( HasCondition( COND_TOO_CLOSE_TO_ATTACK ) )
	{
		return SCHED_BACK_AWAY_FROM_ENEMY;
	}
	
	if ( HasCondition( COND_LOW_PRIMARY_AMMO ) || HasCondition( COND_NO_PRIMARY_AMMO ) )
	{
		if (!IsUnique())
		{
			AnnounceOutOfAmmo( );
		}
		return SCHED_HIDE_AND_RELOAD;
	}

	if ( HasCondition(COND_WEAPON_SIGHT_OCCLUDED) && !HasBaton() )
	{
		// If they are hiding behind something that we can destroy, start shooting at it.
		CBaseEntity *pBlocker = GetEnemyOccluder();
		if ( pBlocker && pBlocker->GetHealth() > 0 && OccupyStrategySlotRange( SQUAD_SLOT_POLICE_ATTACK_OCCLUDER1, SQUAD_SLOT_POLICE_ATTACK_OCCLUDER2 ) )
		{
			if (!IsUnique())
			{
				m_Sentences.Speak( "METROPOLICE_SHOOT_COVER" );
			}
			return SCHED_SHOOT_ENEMY_COVER;
		}
	}

	if (HasCondition(COND_ENEMY_OCCLUDED))
	{
		if ( GetEnemy() && !(GetEnemy()->GetFlags() & FL_NOTARGET) )
		{
			// Charge in and break the enemy's cover!
			return SCHED_ESTABLISH_LINE_OF_FIRE;
		}
	}

	nResult = SelectScheduleNoDirectEnemy();
	if ( nResult != SCHED_NONE )
		return nResult;

	return SCHED_NONE;
}

//-----------------------------------------------------------------------------
// Purpose: This is a bridge between stunstick, NPC and its behavior
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CNPC_MetroPolice::ShouldKnockOutTarget( CBaseEntity *pTarget )
{
	if ( m_PolicingBehavior.IsEnabled() && m_PolicingBehavior.ShouldKnockOutTarget( pTarget ) )
		return true;

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: This is a bridge between stunstick, NPC and its behavior
//-----------------------------------------------------------------------------
void CNPC_MetroPolice::KnockOutTarget( CBaseEntity *pTarget )
{
	if ( m_PolicingBehavior.IsEnabled() )
	{
		m_PolicingBehavior.KnockOutTarget( pTarget );
	}
}

//-----------------------------------------------------------------------------
// Can me enemy see me? 
//-----------------------------------------------------------------------------
bool CNPC_MetroPolice::CanEnemySeeMe( )
{
	if ( GetEnemy()->IsPlayer() )
	{
		if ( static_cast<CBasePlayer*>(GetEnemy())->FInViewCone( this ) )
		{
			return true;
		}
	}
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &info - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CNPC_MetroPolice::IsHeavyDamage( const CTakeDamageInfo &info )
{
	// Metropolice considers bullet fire heavy damage
	if ( info.GetDamageType() & DMG_BULLET )
		return true;

	return BaseClass::IsHeavyDamage( info );
}

//-----------------------------------------------------------------------------
// TraceAttack
//-----------------------------------------------------------------------------
void CNPC_MetroPolice::TraceAttack( const CTakeDamageInfo &info, const Vector &vecDir, trace_t *ptr, CDmgAccumulator *pAccumulator )
{
	// This is needed so we can keep track of the direction of the shot
	// because we're going to use it to choose the flinch animation
	if ( m_bSimpleCops )
	{
		if ( m_takedamage == DAMAGE_YES )
		{
			Vector vecLastHitDirection;
			VectorIRotate( vecDir, EntityToWorldTransform(), vecLastHitDirection );

			// Point *at* the shooter
			vecLastHitDirection *= -1.0f;

			QAngle lastHitAngles;
			VectorAngles( vecLastHitDirection, lastHitAngles );
			m_flLastHitYaw	= lastHitAngles.y;
		}
	}

	BaseClass::TraceAttack( info, vecDir, ptr, pAccumulator );
}

//-----------------------------------------------------------------------------
// Determines the best type of flinch anim to play.
//-----------------------------------------------------------------------------
Activity CNPC_MetroPolice::GetFlinchActivity( bool bHeavyDamage, bool bGesture )
{
	if ( !bGesture && m_bSimpleCops )
	{
		// Version for getting shot from behind
		if ( ( m_flLastHitYaw > 90 ) && ( m_flLastHitYaw < 270 ) )
		{
			Activity flinchActivity = (Activity)ACT_METROPOLICE_FLINCH_BEHIND;
			if ( SelectWeightedSequence ( flinchActivity ) != ACTIVITY_NOT_AVAILABLE )
				return flinchActivity;
		}

		if ( ( LastHitGroup() == HITGROUP_CHEST ) ||
			( LastHitGroup() == HITGROUP_LEFTLEG ) ||
			( LastHitGroup() == HITGROUP_RIGHTLEG ) )
		{
			Activity flinchActivity = ACT_FLINCH_STOMACH;
			if ( SelectWeightedSequence ( ACT_FLINCH_STOMACH ) != ACTIVITY_NOT_AVAILABLE )
				return flinchActivity;
		}
	}

	return BaseClass::GetFlinchActivity( bHeavyDamage, bGesture );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_MetroPolice::PlayFlinchGesture( void )
{
	BaseClass::PlayFlinchGesture();

	// To ensure old playtested difficulty stays the same, stop cops shooting for a bit after gesture flinches
	GetShotRegulator()->FireNoEarlierThan( gpGlobals->curtime + 0.5 );
}

//-----------------------------------------------------------------------------
// We're taking cover from danger
//-----------------------------------------------------------------------------
void CNPC_MetroPolice::AnnounceHarrassment( void )
{
	if (IsUnique())
	{
		return;
	}

	static const char *pWarnings[3] = 
	{
		"METROPOLICE_BACK_UP_A",
		"METROPOLICE_BACK_UP_B",
		"METROPOLICE_BACK_UP_C",
	};

	m_Sentences.Speak( pWarnings[ random->RandomInt( 0, ARRAYSIZE(pWarnings)-1 ) ], SENTENCE_PRIORITY_MEDIUM, SENTENCE_CRITERIA_NORMAL );
}


//-----------------------------------------------------------------------------
// Purpose: 
// Output : float
//-----------------------------------------------------------------------------
float CNPC_MetroPolice::GetIdealAccel( void ) const
{
	return GetIdealSpeed() * 2.0f;
}

//-----------------------------------------------------------------------------
// Purpose: Chase after a player who's just pissed us off, and hit him
//-----------------------------------------------------------------------------
/*void CNPC_MetroPolice::AdministerJustice( void )
{
	if ( !AI_IsSinglePlayer() )
		return;

	// If we're allowed to chase the player, do so. Otherwise, just threaten.
	if ( !IsInAScript() && (m_NPCState != NPC_STATE_SCRIPT) && HasSpawnFlags( SF_METROPOLICE_ALLOWED_TO_RESPOND ) )
	{
		if ( m_vecPreChaseOrigin == vec3_origin )
		{
			m_vecPreChaseOrigin = GetAbsOrigin();
			m_flPreChaseYaw = GetAbsAngles().y;
		}
		m_flChasePlayerTime = gpGlobals->curtime + RandomFloat( 3, 7 );

		// Attack the target
		CBasePlayer *pPlayer = UTIL_PlayerByIndex(1);
		SetEnemy( pPlayer );
		SetState( NPC_STATE_COMBAT );
		UpdateEnemyMemory( pPlayer, pPlayer->GetAbsOrigin() );
	}
	else
	{
		// Watch the player for a time.
		m_bKeepFacingPlayer = true;

		// Try and find a nearby cop to administer justice
		CAI_BaseNPC **ppAIs = g_AI_Manager.AccessAIs();
		int nAIs = g_AI_Manager.NumAIs();
		for ( int i = 0; i < nAIs; i++ )
		{
			if ( ppAIs[i] == this )
				continue;

			if ( ppAIs[i]->Classify() == CLASS_METROPOLICE && FClassnameIs( ppAIs[i], "npc_metropolice" ) )
			{
				CNPC_MetroPolice *pNPC = assert_cast<CNPC_MetroPolice*>(ppAIs[i]);
				if ( pNPC->HasSpawnFlags( SF_METROPOLICE_ALLOWED_TO_RESPOND ) )
				{
					// Is he within site & range?
					if ( FVisible(pNPC) && pNPC->FVisible( UTIL_PlayerByIndex(1) ) && 
						UTIL_DistApprox( WorldSpaceCenter(), pNPC->WorldSpaceCenter() ) < 512 )
					{
						pNPC->AdministerJustice();
						break;
					}
				}
			}
		}
	}
}*/

//-----------------------------------------------------------------------------
// Schedule selection 
//-----------------------------------------------------------------------------
int CNPC_MetroPolice::SelectSchedule( void )
{
	m_bMovingAwayFromPlayer = false;

	if ( IsInAScript() )
		return BaseClass::SelectSchedule();


#ifdef HL2_EPISODIC
	// Always defer to passenger if it's running
	if ( ShouldDeferToPassengerBehavior() )
	{
		DeferSchedulingToBehavior( &m_PassengerBehavior );
		return BaseClass::SelectSchedule();
	}
#endif // HL2_EPISODIC

	if ( m_ActBusyBehavior.IsRunning() && m_ActBusyBehavior.NeedsToPlayExitAnim() )
	{
		trace_t tr;
		Vector	vUp = GetAbsOrigin();
		vUp.z += .25;

		AI_TraceHull( GetAbsOrigin(), vUp, GetHullMins(),
			GetHullMaxs(), MASK_SOLID, this, COLLISION_GROUP_NONE, &tr );

		if ( tr.startsolid )
		{
			if ( HasCondition( COND_HEAR_DANGER ) )
			{
				m_ActBusyBehavior.StopBusying();
			}
			DeferSchedulingToBehavior( &m_ActBusyBehavior );
			return BaseClass::SelectSchedule();
		}
	}

	int schedule = SelectScheduleDanger();
	if ( schedule != SCHED_NONE )
		return schedule;
	
	schedule = SelectSchedulePriorityAction();
	if ( schedule != SCHED_NONE )
		return schedule;

	if ( ShouldDeferToFollowBehavior() )
	{
		DeferSchedulingToBehavior( &(GetFollowBehavior()) );
	}
	else if ( !BehaviorSelectSchedule() )
	{
		if ( m_NPCState == NPC_STATE_IDLE || m_NPCState == NPC_STATE_ALERT )
		{
			schedule = SelectScheduleNonCombat();
			if ( schedule != SCHED_NONE )
				return schedule;
		}
		else if ( m_NPCState == NPC_STATE_COMBAT )
		{
			schedule = SelectScheduleCombat();
			if ( schedule != SCHED_NONE )
				return schedule;
		}
	}

	if ( !GetEnemy() && HasCondition( COND_IN_PVS ) && AI_GetSinglePlayer() && !AI_GetSinglePlayer()->IsAlive() )
	{
		return SCHED_PATROL_WALK;
	}

	if ( HasCondition(COND_METROPOLICE_ON_FIRE) )
	{
		if (!IsUnique())
		{
			m_Sentences.Speak( "METROPOLICE_ON_FIRE", SENTENCE_PRIORITY_INVALID, SENTENCE_CRITERIA_ALWAYS );
		}

		return SCHED_METROPOLICE_BURNING_STAND;
	}

	int nSched = SelectFlinchSchedule();
	if ( nSched != SCHED_NONE )
		return nSched;

	if ( HasBaton() )
	{
		// See if we're being told to activate our baton
		if ( m_bShouldActivateBaton && BatonActive() == false && IsCurSchedule( SCHED_METROPOLICE_ACTIVATE_BATON ) == false )
			return SCHED_METROPOLICE_ACTIVATE_BATON;

		if ( m_bShouldActivateBaton == false && BatonActive() && IsCurSchedule( SCHED_METROPOLICE_DEACTIVATE_BATON ) == false )
			return SCHED_METROPOLICE_DEACTIVATE_BATON;

		if( metropolice_chase_use_follow.GetBool() )
		{
			if( GetEnemy() )
			{
				AI_FollowParams_t params;
				params.formation = AIF_TIGHT;
				m_FollowBehavior.SetParameters( params );
				m_FollowBehavior.SetFollowTarget( GetEnemy() );
			}
		}
	}

	// Cower when physics objects are thrown at me
	if ( HasCondition( COND_HEAR_PHYSICS_DANGER ) )
	{
		if ( m_flLastPhysicsFlinchTime + 4.0f <= gpGlobals->curtime )
		{
			m_flLastPhysicsFlinchTime = gpGlobals->curtime;
			return SCHED_FLINCH_PHYSICS;
		}
	}

	// Always run for cover from danger sounds
	if ( HasCondition(COND_HEAR_DANGER) )
	{
		CSound *pSound;
		pSound = GetBestSound();

		Assert( pSound != NULL );
		if ( pSound )
		{
			if (pSound->m_iType & SOUND_DANGER)
			{
				if (!IsUnique())
				{
					AnnounceTakeCoverFromDanger( pSound );
				}
				return SCHED_TAKE_COVER_FROM_BEST_SOUND;
			}
			if (!HasCondition( COND_SEE_ENEMY ) && ( pSound->m_iType & (SOUND_PLAYER | SOUND_PLAYER_VEHICLE | SOUND_COMBAT) ))
			{
				GetMotor()->SetIdealYawToTarget( pSound->GetSoundReactOrigin() );
			}
		}
	}

	bool bHighHealth = ((float)GetHealth() / (float)GetMaxHealth() > 0.75f);

	// This will cause the cops to run backwards + shoot at the same time
	if ( !bHighHealth && !HasBaton() )
	{
		if ( GetActiveWeapon() && (GetActiveWeapon()->m_iClip1 <= 5) )
		{
			if (!IsUnique())
			{
				m_Sentences.Speak( "METROPOLICE_COVER_LOW_AMMO" );
			}
			return SCHED_HIDE_AND_RELOAD;
		}
	}

	if( HasCondition( COND_NO_PRIMARY_AMMO ) )
	{
		if ( bHighHealth )
			return SCHED_RELOAD;

		AnnounceOutOfAmmo( );
		return SCHED_HIDE_AND_RELOAD;
	}

	if ( !BehaviorSelectSchedule() )
	{
		if ( m_NPCState == NPC_STATE_COMBAT )
		{
			int nResult = SelectCombatSchedule();
			if ( nResult != SCHED_NONE )
				return nResult;
		}
	}

	return CAI_PlayerAlly::SelectSchedule();
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : failedSchedule - 
//			failedTask - 
//			taskFailCode - 
// Output : int
//-----------------------------------------------------------------------------
int CNPC_MetroPolice::SelectFailSchedule( int failedSchedule, int failedTask, AI_TaskFailureCode_t taskFailCode )
{
	if ( failedSchedule == SCHED_METROPOLICE_CHASE_ENEMY )
	{
		return SCHED_METROPOLICE_ESTABLISH_LINE_OF_FIRE;
	}

	return BaseClass::SelectFailSchedule( failedSchedule, failedTask, taskFailCode );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : code - 
//-----------------------------------------------------------------------------
void CNPC_MetroPolice::TaskFail( AI_TaskFailureCode_t code )
{
	// If our heal task has failed, push out the heal time
	if ( IsCurSchedule( SCHED_METROPOLICE_HEAL_TOSS ) )
	{
		m_flPlayerHealTime 	= gpGlobals->curtime + sk_metropolice_heal_ally_delay.GetFloat();
	}

	BaseClass::TaskFail( code );
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
int CNPC_MetroPolice::TranslateSchedule( int scheduleType )
{
	switch( scheduleType )
	{
	case SCHED_ALERT_FACE_BESTSOUND:
		if ( !IsCurSchedule( SCHED_METROPOLICE_ALERT_FACE_BESTSOUND, false ) )
		{
			return SCHED_METROPOLICE_ALERT_FACE_BESTSOUND;
		}
		return SCHED_ALERT_FACE_BESTSOUND;

	case SCHED_CHASE_ENEMY:
		if( HaveCommandGoal() )
		{
			return SCHED_STANDOFF;
		}

		if ( !IsRunningBehavior() )
		{
			return SCHED_METROPOLICE_CHASE_ENEMY;
		}
		
		break;
		
	case SCHED_WAKE_ANGRY:
		return SCHED_METROPOLICE_WAKE_ANGRY;

	case SCHED_FAIL_TAKE_COVER:
		
		if ( HasCondition( COND_CAN_RANGE_ATTACK1 ) )
		{
			// Must be able to shoot now
			if( TryToEnterPistolSlot( SQUAD_SLOT_ATTACK1 ) || TryToEnterPistolSlot( SQUAD_SLOT_ATTACK2 ) )
				return SCHED_RANGE_ATTACK1;
		}

		if ( HasCondition( COND_NO_PRIMARY_AMMO ) )
			return SCHED_RELOAD;
		return SCHED_RUN_RANDOM;

	case SCHED_RANGE_ATTACK1:
		Assert( !HasCondition( COND_NO_PRIMARY_AMMO ) );

		if( !m_fWeaponDrawn )
		{
			return SCHED_METROPOLICE_DRAW_PISTOL;
		}
		break;
	case SCHED_METROPOLICE_ADVANCE:
		if ( m_NextChargeTimer.Expired() && metropolice_charge.GetBool() )
		{	
			if ( Weapon_OwnsThisType( "weapon_pistol" ) || Weapon_OwnsThisType( "weapon_357" ) )
			{
				if (  GetEnemy() && GetEnemy()->GetAbsOrigin().DistToSqr( GetAbsOrigin() ) > 300*300 )
				{
					if ( OccupyStrategySlot( SQUAD_SLOT_POLICE_CHARGE_ENEMY ) )
					{
						m_NextChargeTimer.Set( 3, 7 );
						return SCHED_METROPOLICE_CHARGE;
					}
				}
			}
			else
			{
				m_NextChargeTimer.Set( 99999 );
			}
		}
		break;
	}


	return BaseClass::TranslateSchedule( scheduleType );
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool CNPC_MetroPolice::ShouldAcceptGoal( CAI_BehaviorBase *pBehavior, CAI_GoalEntity *pGoal )
{
	if ( BaseClass::ShouldAcceptGoal( pBehavior, pGoal ) )
	{
		CAI_FollowBehavior *pFollowBehavior = dynamic_cast<CAI_FollowBehavior *>(pBehavior );
		if ( pFollowBehavior )
		{
			if ( IsInPlayerSquad() )
			{
				m_hSavedFollowGoalEnt = (CAI_FollowGoal *)pGoal;
				return false;
			}
		}
		return true;
	}
	return false;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CNPC_MetroPolice::OnClearGoal( CAI_BehaviorBase *pBehavior, CAI_GoalEntity *pGoal )
{
	if ( m_hSavedFollowGoalEnt == pGoal )
		m_hSavedFollowGoalEnt = NULL;
}

//-----------------------------------------------------------------------------
// Ha
//-----------------------------------------------------------------------------
bool CNPC_MetroPolice::ShouldMoveAndShoot()
{
//	if ( HasSpawnFlags( SF_METROPOLICE_ARREST_ENEMY ) )
//		return false;

	return BaseClass::ShouldMoveAndShoot();
}


//-----------------------------------------------------------------------------
// Only move and shoot when attacking
//-----------------------------------------------------------------------------
bool CNPC_MetroPolice::OnBeginMoveAndShoot()
{
	if ( BaseClass::OnBeginMoveAndShoot() )
	{
		if( HasStrategySlotRange( SQUAD_SLOT_ATTACK1, SQUAD_SLOT_ATTACK2 ) )
			return true; // already have the slot I need

		if( OccupyStrategySlotRange( SQUAD_SLOT_ATTACK1, SQUAD_SLOT_ATTACK2 ) )
			return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// Only move and shoot when attacking
//-----------------------------------------------------------------------------
void CNPC_MetroPolice::OnEndMoveAndShoot()
{
	VacateStrategySlot();
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pTask - 
//-----------------------------------------------------------------------------
void CNPC_MetroPolice::StartTask( const Task_t *pTask )
{
	switch (pTask->iTask)
	{	
	/*case TASK_CP_HIDE_MEDKIT:
		{
			SetBodygroup( METROPOLICE_BODYGROUP_HEALTHKIT, false );
			TaskComplete();
		}
		break;*/

	case TASK_METROPOLICE_WAIT_FOR_SENTENCE:
		{
			if ( FOkToMakeSound( pTask->flTaskData ) )
			{
				TaskComplete();
			}
		}
		break;

	case TASK_CP_HEAL:
	case TASK_CP_HEAL_TOSS:
		{
			bool bIsHealing = false;
			bool bHealingItself = false;

			if ( IsMedic() && GetTarget() && GetTarget()->IsPlayer() && GetTarget()->m_iMaxHealth > GetTarget()->m_iHealth )
			{
				bIsHealing = true;
				bHealingItself = (GetTarget() == this);

				if ( !bHealingItself && IsUnique())
				{
					Speak( TLK_HEAL );
				}
			}
			else if ( IsAmmoResupplier() )
			{
				if (IsUnique())
				{
					Speak( TLK_GIVEAMMO );
				}
			}
			else
			{
				//TERO: not healing, not ammo resupplier
				TaskComplete();
				break;
			}

			if (IsUnique() && bIsHealing )
			{
				if ( bHealingItself )
				{
					SetIdealActivity( (Activity) ACT_METROPOLICE_HEAL_SELF );
				}
				else
				{
					SetIdealActivity( (Activity) ACT_METROPOLICE_HEAL_TARGET );
				}
			} 
			else
			{
				SetIdealActivity( (Activity)ACT_METROPOLICE_DEPLOY_MANHACK ); //TERO: this is translated into the correct animevents
			}
		}
		break;

	case TASK_METROPOLICE_ACTIVATE_BATON:
		{
			// Simply early out if we're in here without a baton
			if ( HasBaton() == false )
			{
				TaskComplete();
				break;
			}

			bool activate = ( pTask->flTaskData != 0 );

			if ( activate )
			{
				if ( BatonActive() || m_bShouldActivateBaton == false )
				{
					TaskComplete();
					break;
				}

				if (!IsUnique())
				{
					m_Sentences.Speak( "METROPOLICE_ACTIVATE_BATON", SENTENCE_PRIORITY_NORMAL, SENTENCE_CRITERIA_NORMAL );
				}
				SetIdealActivity( (Activity) ACT_ACTIVATE_BATON );
			}
			else
			{
				if ( BatonActive() == false || m_bShouldActivateBaton )
				{
					TaskComplete();
					break;
				}

				if (!IsUnique())
				{
					m_Sentences.Speak( "METROPOLICE_DEACTIVATE_BATON", SENTENCE_PRIORITY_NORMAL, SENTENCE_CRITERIA_NORMAL );
				}
				SetIdealActivity( (Activity) ACT_DEACTIVATE_BATON );
			}
		}
		break;

	case TASK_METROPOLICE_DIE_INSTANTLY:
		{
			if ( Classify() != CLASS_PLAYER_ALLY_VITAL )
			{

				CTakeDamageInfo info;

				info.SetAttacker( this );
				info.SetInflictor( this );
				info.SetDamage( m_iHealth );
				info.SetDamageType( pTask->flTaskData );
				info.SetDamageForce( Vector( 0.1, 0.1, 0.1 ) );

				TakeDamage( info );
			}

			TaskComplete();
		}
		break;

	/*case TASK_METROPOLICE_LEAD_ARREST_ENEMY:
	case TASK_METROPOLICE_ARREST_ENEMY:
		m_flTaskCompletionTime = gpGlobals->curtime + pTask->flTaskData;
		break;

	case TASK_METROPOLICE_SIGNAL_FIRING_TIME:
		EnemyResistingArrest();
		TaskComplete();
		break;*/

	case TASK_METROPOLICE_HARASS:
		{
			if( !( m_spawnflags & SF_METROPOLICE_NOCHATTER ) )
			{
				if( GetEnemy() && GetEnemy()->GetWaterLevel() > 0 )
				{
					EmitSound( "NPC_MetroPolice.WaterSpeech" );
				}
				else
				{
					EmitSound( "NPC_MetroPolice.HidingSpeech" );
				}
			}

			TaskComplete();
		}
		break;

	case TASK_METROPOLICE_GET_PATH_TO_BESTSOUND_LOS:
		{
		}
		break;

	default:
		BaseClass::StartTask( pTask );
		break;
	}
}


//-----------------------------------------------------------------------------
//
// Run tasks!
//
//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
// He's resisting arrest!
//-----------------------------------------------------------------------------
/*void CNPC_MetroPolice::EnemyResistingArrest()
{
	// Prevent any other arrest from being made in this squad
	// and tell them all that the player is resisting arrest!
	
	if ( m_pSquad != NULL )
	{
		AISquadIter_t iter;
		CAI_BaseNPC *pSquadmate = m_pSquad->GetFirstMember( &iter );
		while ( pSquadmate )
		{
			pSquadmate->RemoveSpawnFlags( SF_METROPOLICE_ARREST_ENEMY );
			pSquadmate->SetCondition( COND_METROPOLICE_ENEMY_RESISTING_ARREST );
			pSquadmate = m_pSquad->GetNextMember( &iter );
		}
	}
}*/


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pTask - 
//-----------------------------------------------------------------------------
#define FLEEING_DISTANCE_SQR (100 * 100)

void CNPC_MetroPolice::RunTask( const Task_t *pTask )
{
	/*if (IsLarson())
	{
		if (GetCurSchedule())
			DevMsg("Larson: running schedule with name: %s, with task id %d\n", GetCurSchedule()->GetName(), pTask->iTask );
		else
			DevMsg("Larson: running schedule with task: %d\n", pTask->iTask );
	}*/

	switch( pTask->iTask )
	{
	case TASK_WAIT_FOR_MOVEMENT:
		BaseClass::RunTask( pTask );
		break;

	case TASK_CP_HEAL:
			if ( IsActivityFinished() ) //IsSequenceFinished
			{
				TaskComplete();
			}
			else if (!GetTarget())
			{
				// Our heal target was killed or deleted somehow.
				TaskFail(FAIL_NO_TARGET);
			}
			else
			{
				if ( ( GetTarget()->GetAbsOrigin() - GetAbsOrigin() ).Length2D() > HEAL_MOVE_RANGE/2 )
					TaskComplete();

				GetMotor()->SetIdealYawToTargetAndUpdate( GetTarget()->GetAbsOrigin() );
			}
			break;

	case TASK_CP_HEAL_TOSS:
			if ( IsActivityFinished() ) //IsSequenceFinished
			{
				TaskComplete();
			}
			else if (!GetTarget())
			{
				// Our heal target was killed or deleted somehow.
				TaskFail(FAIL_NO_TARGET);
			}
			else
			{
				GetMotor()->SetIdealYawToTargetAndUpdate( GetTarget()->GetAbsOrigin() );
			}
			break;

	case TASK_METROPOLICE_WAIT_FOR_SENTENCE:
		{
			if ( FOkToMakeSound( pTask->flTaskData ) )
			{
				TaskComplete();
			}
		}
		break;

	case TASK_METROPOLICE_ACTIVATE_BATON:
		AutoMovement();
		
		if ( IsActivityFinished() )
		{
			TaskComplete();
		}	
		break;

	/*case TASK_METROPOLICE_LEAD_ARREST_ENEMY:
	case TASK_METROPOLICE_ARREST_ENEMY:
		{
			if ( !GetEnemy() )
			{
				TaskComplete();
				break;
			}

			if ( gpGlobals->curtime >= m_flTaskCompletionTime )
			{
				TaskComplete();
				break;
			}

			// Complete the arrest after the last squad member has a bead on the enemy
			// But only if you're the first guy who saw him
			if ( pTask->iTask == TASK_METROPOLICE_LEAD_ARREST_ENEMY )
			{
				int nArrestCount = SquadArrestCount();
				if ( nArrestCount == m_pSquad->NumMembers() )
				{
					TaskComplete();
					break;
				}

				// Do a distance check of the enemy from his initial position.
				// Shoot if he gets too far.
				if ( m_vSavePosition.DistToSqr( GetEnemy()->GetAbsOrigin() ) > FLEEING_DISTANCE_SQR )
				{
					SpeakSentence( METROPOLICE_SENTENCE_HES_RUNNING );
					EnemyResistingArrest();
					break;
				}
			}

			// Keep aiming at the enemy
			if ( GetEnemy() && FacingIdeal() )
			{
				float flNewIdealYaw = CalcIdealYaw( GetEnemy()->EyePosition() );
				if ( fabs(UTIL_AngleDiff( GetMotor()->GetIdealYaw(), flNewIdealYaw )) >= 45.0f )
				{
					GetMotor()->SetIdealYawToTarget( GetEnemy()->EyePosition() );
					SetTurnActivity(); 
				}
			}
			GetMotor()->UpdateYaw();
		}
		break;*/

	case TASK_METROPOLICE_GET_PATH_TO_BESTSOUND_LOS:
		{
			switch( GetTaskInterrupt() )
			{
			case 0:
				{
					CSound *pSound = GetBestSound();
					if (!pSound)
					{
						TaskFail(FAIL_NO_SOUND);
					}
					else
					{
						float flMaxRange = 2000;
						float flMinRange = 0;
						if ( GetActiveWeapon() )
						{
							flMaxRange = max( GetActiveWeapon()->m_fMaxRange1, GetActiveWeapon()->m_fMaxRange2 );
							flMinRange = min( GetActiveWeapon()->m_fMinRange1, GetActiveWeapon()->m_fMinRange2 );
						}

						// Check against NPC's max range
						if (flMaxRange > m_flDistTooFar)
						{
							flMaxRange = m_flDistTooFar;
						}

						// Why not doing lateral LOS first?

						Vector losTarget = pSound->GetSoundReactOrigin();
						if ( GetTacticalServices()->FindLos( pSound->GetSoundReactOrigin(), losTarget, flMinRange, flMaxRange, 1.0, &m_vInterruptSavePosition ) )
						{
							TaskInterrupt();
						}
						else
						{
							TaskFail(FAIL_NO_SHOOT);
						}
					}
				}
				break;

			case 1:
				{
					AI_NavGoal_t goal( m_vInterruptSavePosition, ACT_RUN, AIN_HULL_TOLERANCE );
					GetNavigator()->SetGoal( goal );
				}
				break;
			}
		}
		break;

	default:
		BaseClass::RunTask( pTask );
		break;
	}
}


		
//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pevInflictor - 
//			pAttacker - 
//			flDamage - 
//			bitsDamageType - 
// Output : int
//-----------------------------------------------------------------------------
int CNPC_MetroPolice::OnTakeDamage_Alive( const CTakeDamageInfo &inputInfo )
{
	CTakeDamageInfo info = inputInfo;

	/*if ( HasSpawnFlags( SF_METROPOLICE_ARREST_ENEMY ) )
	{
		EnemyResistingArrest();
	}*/
	if (info.GetAttacker() == GetEnemy())
	{
		// Keep track of recent damage by my attacker. If it seems like we're
		// being killed, consider running off and hiding.
		m_nRecentDamage += info.GetDamage();
		m_flRecentDamageTime = gpGlobals->curtime;
	}

	if( IsInSquad() && (info.GetDamageType() & DMG_BLAST) && info.GetInflictor() )
	{
		if( npc_metropolice_explosive_resist.GetBool() )
		{
			// Blast damage. If this kills a squad member, give the 
			// remaining citizens a resistance bonus to this inflictor
			// to try to avoid having the entire squad wiped out by a
			// single explosion.
			if( m_pSquad->IsSquadInflictor( info.GetInflictor() ) )
			{
				info.ScaleDamage( 0.5 );
			}
			else
			{
				// If this blast is going to kill me, designate the inflictor
				// so that the rest of the squad can enjoy a damage resist.
				if( info.GetDamage() >= GetHealth() )
				{
					m_pSquad->SetSquadInflictor( info.GetInflictor() );
				}
			}
		}
	}

	//DevMsg("npc_metropolice: health left %d, damage recieved %f\n", m_iHealth, info.GetDamage());

	return BaseClass::OnTakeDamage_Alive( info ); 
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool CNPC_MetroPolice::IsCommandable() 
{
	return ( !HasSpawnFlags(SF_METROPOLICE_NOT_COMMANDABLE) && IsInPlayerSquad() );
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool CNPC_MetroPolice::IsPlayerAlly( CBasePlayer *pPlayer )											
{ 
	if ( GlobalEntity_GetState("gordon_precriminal") != GLOBAL_ON )
	{
		// Robin: Citizens use friendly speech semaphore in trainstation
		return true;
	}

	return BaseClass::IsPlayerAlly( pPlayer );
}

//-----------------------------------------------------------------------------
// Purpose: I want to deploy a manhack. Can I?
//-----------------------------------------------------------------------------
bool CNPC_MetroPolice::CanDeployManhack( void )
{
	if ( HasSpawnFlags( SF_METROPOLICE_NO_MANHACK_DEPLOY ) )
		return false;

	// Nope, already have one out.
	if( m_hManhack != NULL )
		return false;

	// Nope, don't have any!
	if( m_iManhacks < 1 )
		return false;

	return true;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool CNPC_MetroPolice::CanJoinPlayerSquad()
{
	if ( !AI_IsSinglePlayer() )
		return false;

	if ( m_NPCState == NPC_STATE_SCRIPT || m_NPCState == NPC_STATE_PRONE )
		return false;

	if ( HasSpawnFlags(SF_METROPOLICE_NOT_COMMANDABLE) )
		return false;

	if ( IsInAScript() )
		return false;

	// Don't bother people who don't want to be bothered
	if ( !CanBeUsedAsAFriend() )
		return false;

	if ( IRelationType( UTIL_GetLocalPlayer() ) != D_LI )
		return false;

	return true;
}
 
//-----------------------------------------------------------------------------
// Purpose: Allows for modification of the interrupt mask for the current schedule.
//			In the most cases the base implementation should be called first.
//-----------------------------------------------------------------------------
void CNPC_MetroPolice::BuildScheduleTestBits( void )
{
	BaseClass::BuildScheduleTestBits();

	if ( IsMedic() && IsCustomInterruptConditionSet( COND_HEAR_MOVE_AWAY ) )
	{
		if( !IsCurSchedule(SCHED_RELOAD, false) )
		{
			// Since schedule selection code prioritizes reloading over requests to heal
			// the player, we must prevent this condition from breaking the reload schedule.
			SetCustomInterruptCondition( COND_CP_PLAYERHEALREQUEST );
		}

		SetCustomInterruptCondition( COND_CP_COMMANDHEAL );
	}

	if( !IsCurSchedule( SCHED_NEW_WEAPON ) )
	{
		SetCustomInterruptCondition( COND_RECEIVED_ORDERS );
	}

	if( GetCurSchedule()->HasInterrupt( COND_IDLE_INTERRUPT ) )
	{
		SetCustomInterruptCondition( COND_BETTER_WEAPON_AVAILABLE );
	}

	if( IsMedic() && m_AssaultBehavior.IsRunning() )
	{
		if( !IsCurSchedule(SCHED_RELOAD, false) )
		{
			SetCustomInterruptCondition( COND_CP_PLAYERHEALREQUEST );
		}

		SetCustomInterruptCondition( COND_CP_COMMANDHEAL );
	}

	if ( !IsCurSchedule( SCHED_METROPOLICE_BURNING_RUN ) && !IsCurSchedule( SCHED_METROPOLICE_BURNING_STAND ) && !IsMoving() )
	{
		SetCustomInterruptCondition( COND_METROPOLICE_ON_FIRE );
	}

	if (IsCurSchedule(SCHED_TAKE_COVER_FROM_ENEMY))
	{
		ClearCustomInterruptCondition( COND_LIGHT_DAMAGE );
		ClearCustomInterruptCondition( COND_HEAVY_DAMAGE );
	}

	if ( !IsCurSchedule( SCHED_CHASE_ENEMY ) &&
		 !IsCurSchedule( SCHED_METROPOLICE_ACTIVATE_BATON ) &&
		 !IsCurSchedule( SCHED_METROPOLICE_DEACTIVATE_BATON ) &&
		 !IsCurSchedule( SCHED_METROPOLICE_SHOVE ) )
	{
		SetCustomInterruptCondition( COND_METROPOLICE_CHANGE_BATON_STATE );
	}

	if ( IsCurSchedule( SCHED_MELEE_ATTACK1 ) )
	{
		if ( gpGlobals->curtime - m_flLastDamageFlinchTime < 10.0 )
		{
			ClearCustomInterruptCondition( COND_LIGHT_DAMAGE );
			ClearCustomInterruptCondition( COND_HEAVY_DAMAGE );
		}
	}
	else if ( HasBaton() && IsCurSchedule( SCHED_COMBAT_FACE ) && !m_BatonSwingTimer.Expired() )
	{
		ClearCustomInterruptCondition( COND_CAN_MELEE_ATTACK1 );
	}
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
int CNPC_MetroPolice::SelectSchedulePriorityAction()
{
	int schedule = SelectScheduleHeal();
	if ( schedule != SCHED_NONE )
		return schedule;

	schedule = BaseClass::SelectSchedulePriorityAction();
	if ( schedule != SCHED_NONE )
		return schedule;

	schedule = SelectScheduleRetrieveItem();
	if ( schedule != SCHED_NONE )
		return schedule;

	return SCHED_NONE;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
int CNPC_MetroPolice::SelectScheduleRetrieveItem()
{
	if ( HasCondition(COND_BETTER_WEAPON_AVAILABLE) )
	{
		if( m_iszPendingWeapon != NULL_STRING )
		{
			return SCHED_SWITCH_TO_PENDING_WEAPON;
		}

		CBaseHLCombatWeapon *pWeapon = dynamic_cast<CBaseHLCombatWeapon *>(Weapon_FindUsable( WEAPON_SEARCH_DELTA ));
		if ( pWeapon )
		{
			m_flNextWeaponSearchTime = gpGlobals->curtime + 10.0;
			// Now lock the weapon for several seconds while we go to pick it up.
			pWeapon->Lock( 10.0, this );
			SetTarget( pWeapon );
			return SCHED_NEW_WEAPON;
		}
	}

	if( HasCondition(COND_HEALTH_ITEM_AVAILABLE) )
	{
		if( !IsInPlayerSquad() )
		{
			// Been kicked out of the player squad since the time I located the health.
			ClearCondition( COND_HEALTH_ITEM_AVAILABLE );
		}
		else
		{
			CBaseEntity *pBase = FindHealthItem(m_FollowBehavior.GetFollowTarget()->GetAbsOrigin(), Vector( 120, 120, 120 ) );
			CItem *pItem = dynamic_cast<CItem *>(pBase);

			if( pItem )
			{
				SetTarget( pItem );
				DevMsg("Sched Get Healthkit\n");
				return SCHED_GET_HEALTHKIT;
			}
		}
	}
	return SCHED_NONE;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool CNPC_MetroPolice::ShouldLookForHealthItem()
{
	// Definitely do not take health if not in the player's squad.
	if( !IsInPlayerSquad() )
		return false;

	if( gpGlobals->curtime < m_flNextHealthSearchTime )
		return false;

	// I'm fully healthy.
	if( GetHealth() >= GetMaxHealth() )
		return false;

	// Player is hurt, don't steal his health.
	if( AI_IsSinglePlayer() && UTIL_GetLocalPlayer()->GetHealth() <= UTIL_GetLocalPlayer()->GetHealth() * 0.75f )
		return false;

	// Wait till you're standing still.
	if( IsMoving() )
		return false;

	DevMsg("Should look for health item\n");

	return true;
}

//-----------------------------------------------------------------------------
// Determine if citizen should perform heal action.
//-----------------------------------------------------------------------------
int CNPC_MetroPolice::SelectScheduleHeal()
{
	if ( CanHeal() )
	{
		CBaseEntity *pEntity = PlayerInRange( GetLocalOrigin(), HEAL_TOSS_TARGET_RANGE );
		if ( pEntity )
		{
			if ( USE_EXPERIMENTAL_MEDIC_CODE() && IsMedic() )
			{
				// use the new heal toss algorithm
				if ( ShouldHealTossTarget( pEntity, HasCondition( COND_CP_PLAYERHEALREQUEST ) ) )
				{
					SetTarget( pEntity );
					return SCHED_METROPOLICE_HEAL_TOSS;
				}
			}
			else if ( PlayerInRange( GetLocalOrigin(), HEAL_MOVE_RANGE ) )
			{
				// use old mechanism for ammo
				if ( ShouldHealTarget( pEntity, HasCondition( COND_CP_PLAYERHEALREQUEST ) ) )
				{
					SetTarget( pEntity );
					return SCHED_METROPOLICE_HEAL;
				}
			}

		}
		
		if ( m_pSquad )
		{
			pEntity = NULL;
			float distClosestSq = HEAL_MOVE_RANGE*HEAL_MOVE_RANGE;
			float distCurSq;
			
			AISquadIter_t iter;
			CAI_BaseNPC *pSquadmate = m_pSquad->GetFirstMember( &iter );
			while ( pSquadmate )
			{
				if ( pSquadmate != this )
				{
					distCurSq = ( GetAbsOrigin() - pSquadmate->GetAbsOrigin() ).LengthSqr();
					if ( distCurSq < distClosestSq && ShouldHealTarget( pSquadmate ) )
					{
						distClosestSq = distCurSq;
						pEntity = pSquadmate;
					}
				}

				pSquadmate = m_pSquad->GetNextMember( &iter );
			}
			
			if ( pEntity )
			{
				SetTarget( pEntity );
				return SCHED_METROPOLICE_HEAL;
			}
		}

		/*char *uniqueClassNames[3] = { "npc_eloise", "npc_noah", "npc_larson", };

		//TERO: this is to make sure we try to heal custom CPs even if they are not in our team, assumes that there's only one of each kind
		for (int i=0; i<3; i++)
		{
			//CN *pEntity = dynamic_cast<CBaseEntity *>
			CNPC_MetroPolice *pEntity = dynamic_cast<CNPC_MetroPolice*>(gEntList.FindEntityByClassname( NULL, uniqueClassNames[i] ));
			if (pEntity)
			{
				if ( pEntity != this )
				{
					if ( (GetAbsOrigin() - pEntity->GetAbsOrigin()).Length() < HEAL_MOVE_RANGE*HEAL_MOVE_RANGE && ShouldHealTarget( pEntity ))
					{
						DevMsg("Should heal unique cp team member, %s\n", uniqueClassNames[i]);
						SetTarget( pEntity );
						return SCHED_METROPOLICE_HEAL;
					}
				} else if ( ShouldHealItself() )
				{
					DevMsg("%s is setting himself as a heal target, what a selfish jerk!\n", GetClassname());
					SetTarget( this );
					return SCHED_METROPOLICE_HEAL;
				}
			}
		}*/

		// Iterate over all unique metrocops
		for ( CNPC_UniqueMetrocop *pUnique = CNPC_UniqueMetrocop::GetUniqueMetrocopList(); pUnique != NULL; pUnique = pUnique->m_pNext )
		{
			if ( pUnique == NULL )
				continue;

			if ( pUnique != this )
			{
				if ( (GetAbsOrigin() - pUnique->GetAbsOrigin()).Length() < HEAL_MOVE_RANGE*HEAL_MOVE_RANGE && ShouldHealTarget( pUnique ))
				{
					DevMsg("Should heal unique cp team member, %s\n", pUnique->GetDebugName() );
					SetTarget( pUnique );
					return SCHED_METROPOLICE_HEAL;
				}
			} else if ( ShouldHealItself() )
			{
				DevMsg("%s is setting himself as a heal target, what a selfish jerk!\n", GetClassname());
				SetTarget( this );
				return SCHED_METROPOLICE_HEAL;
			}
		}
	}
	else
	{
		if ( HasCondition( COND_CP_PLAYERHEALREQUEST ) )
			DevMsg( "Would say: sorry, need to recharge\n" );
	}
	
	return SCHED_NONE;
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool CNPC_MetroPolice::ShouldDeferToFollowBehavior()
{
#if 0
	if ( HaveCommandGoal() )
		return false;
#endif
		
	return BaseClass::ShouldDeferToFollowBehavior();
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
WeaponProficiency_t CNPC_MetroPolice::CalcWeaponProficiency( CBaseCombatWeapon *pWeapon )
{
	if( FClassnameIs( pWeapon, "weapon_pistol" ) )
	{
		return WEAPON_PROFICIENCY_POOR;
	}

	if( FClassnameIs( pWeapon, "weapon_smg1" ) || FClassnameIs( pWeapon, "weapon_357") )
	{
		return WEAPON_PROFICIENCY_VERY_GOOD;
	}

	return BaseClass::CalcWeaponProficiency( pWeapon );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_MetroPolice::GatherConditions( void )
{
	BaseClass::GatherConditions();

	if ( IsInAScript() || (m_NPCState == NPC_STATE_SCRIPT) )
	{
		SetCondition( COND_CP_SCRIPT_INTERRUPT );
		ClearCondition( COND_CP_PLAYERHEALREQUEST );
		ClearCondition( COND_CP_COMMANDHEAL );
	}
	else
	{
		ClearCondition( COND_CP_SCRIPT_INTERRUPT );
	}

	if ( !m_bMedkitHidden && IsCurSchedule( SCHED_METROPOLICE_HEAL ) || IsCurSchedule( SCHED_METROPOLICE_HEAL_TOSS ))
	{
		SetBodygroup( METROPOLICE_BODYGROUP_HEALTHKIT, false );
		m_bMedkitHidden = true;
	}

	if( IsInPlayerSquad() && hl2_episodic.GetBool() )
	{
		// Leave the player squad if someone has made me neutral to player.
		if( IRelationType(UTIL_GetLocalPlayer()) == D_NU )
		{
			RemoveFromPlayerSquad();
		}
	}

	if( ShouldLookForHealthItem() )
	{
		if( FindHealthItem( GetAbsOrigin(), Vector( 240, 240, 240 ) ) )
		{
			SetCondition( COND_HEALTH_ITEM_AVAILABLE );
		}
		else
			ClearCondition( COND_HEALTH_ITEM_AVAILABLE );

		m_flNextHealthSearchTime = gpGlobals->curtime + 4.0;
	}

	// If the player is standing near a medic and can see the medic, 
	// assume the player is 'staring' and wants health.
	if( !HasCondition( COND_CP_SCRIPT_INTERRUPT ) && CanHeal() )
	{
		CBasePlayer *pPlayer = AI_GetSinglePlayer();

		if ( !pPlayer )
		{
			m_flTimePlayerStare = FLT_MAX;
			return;
		}

		float flDistSqr = ( GetAbsOrigin() - pPlayer->GetAbsOrigin() ).Length2DSqr();
		float flStareDist = sk_metropolice_player_stare_dist.GetFloat();
		float flPlayerDamage = pPlayer->GetMaxHealth() - pPlayer->GetHealth();

		if( pPlayer->IsAlive() && flPlayerDamage > 0 && (flDistSqr <= flStareDist * flStareDist) && pPlayer->FInViewCone( this ) && pPlayer->FVisible( this ) )
		{
			if( m_flTimePlayerStare == FLT_MAX )
			{
				// Player wasn't looking at me at last think. He started staring now.
				m_flTimePlayerStare = gpGlobals->curtime;
			}

			// Heal if it's been long enough since last time I healed a staring player.
			if( gpGlobals->curtime - m_flTimePlayerStare >= sk_metropolice_player_stare_time.GetFloat() && gpGlobals->curtime > m_flTimeNextHealStare && !IsCurSchedule( SCHED_METROPOLICE_HEAL ) )
			{
				if ( ShouldHealTarget( pPlayer, true ) )
				{
					SetCondition( COND_CP_PLAYERHEALREQUEST );
				}
				else
				{
					m_flTimeNextHealStare = gpGlobals->curtime + sk_metropolice_stare_heal_time.GetFloat() * .5f;
					ClearCondition( COND_CP_PLAYERHEALREQUEST );
				}
			}

#ifdef HL2_EPISODIC
			// Heal if I'm on an assault. The player hasn't had time to stare at me.
			if( m_AssaultBehavior.IsRunning() && IsMoving() )
			{
				SetCondition( COND_CP_PLAYERHEALREQUEST );
			}
#endif
		}
		else
		{
			m_flTimePlayerStare = FLT_MAX;
		}
	}

}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CNPC_MetroPolice::PredictPlayerPush()
{
	if ( !AI_IsSinglePlayer() )
		return;

	if ( HasCondition( COND_CP_PLAYERHEALREQUEST ) )
		return;

	bool bHadPlayerPush = HasCondition( COND_PLAYER_PUSHING );

	BaseClass::PredictPlayerPush();

	CBasePlayer *pPlayer = UTIL_GetLocalPlayer();
	if ( !bHadPlayerPush && HasCondition( COND_PLAYER_PUSHING ) && 
		 pPlayer->FInViewCone( this ) && CanHeal() )
	{
		if ( ShouldHealTarget( pPlayer, true ) )
		{
			ClearCondition( COND_PLAYER_PUSHING );
			SetCondition( COND_CP_PLAYERHEALREQUEST );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CNPC_MetroPolice::HasBaton( void )
{
	CBaseCombatWeapon *pWeapon = GetActiveWeapon();

	if ( pWeapon )
		return FClassnameIs( pWeapon, "weapon_stunstick" );

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CNPC_MetroPolice::BatonActive( void )
{
#ifndef HL2MP

	CWeaponStunStick *pStick = dynamic_cast<CWeaponStunStick *>(GetActiveWeapon());

	if ( pStick )
		return pStick->GetStunState();
#endif

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : state - 
//-----------------------------------------------------------------------------
void CNPC_MetroPolice::SetBatonState( bool state )
{
	if ( !HasBaton() )
		return;

	if ( m_bShouldActivateBaton != state )
	{
		m_bShouldActivateBaton = state;
		SetCondition( COND_METROPOLICE_CHANGE_BATON_STATE );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pSound - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CNPC_MetroPolice::QueryHearSound( CSound *pSound )
{
	// Only behave differently if the player is pre-criminal

	// If the person making the sound was a friend, don't respond
	if ( pSound->IsSoundType( SOUND_DANGER ) && pSound->m_hOwner && IRelationType( pSound->m_hOwner ) == D_NU )
		return false;

	return BaseClass::QueryHearSound( pSound );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pTarget - 
//-----------------------------------------------------------------------------
void CNPC_MetroPolice::StunnedTarget( CBaseEntity *pTarget )
{
	SetLastAttackTime( gpGlobals->curtime );
}

//-----------------------------------------------------------------------------
// Purpose: Use response for when the player is pre-criminal
//-----------------------------------------------------------------------------
void CNPC_MetroPolice::SimpleUse( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	if ( IsInAScript() )
		return;
	// Don't respond if I'm busy hating the player

	if ( HasSpawnFlags(SF_METROPOLICE_NOT_COMMANDABLE) || IRelationType( pActivator ) == D_NU )
	{
		// If I'm denying commander mode because a level designer has made that decision,
		// then fire this output in case they've hooked it to an event.
		m_OnDenyCommanderUse.FireOutput( this, this );
	} else
	{
		//This is something we want to do every time unless the above happens

		if (GlobalEntity_GetState( "gordon_precriminal" ) == GLOBAL_ON)
		{
			SpeakIfAllowed( TLK_HELLO );
		}

	}
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool CNPC_MetroPolice::WasInPlayerSquad()
{
	return m_bWasInPlayerSquad;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool CNPC_MetroPolice::HaveCommandGoal() const			
{	
	if (GetCommandGoal() != vec3_invalid)
		return true;
	return false;
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool CNPC_MetroPolice::IsCommandMoving()
{
	if ( AI_IsSinglePlayer() && IsInPlayerSquad() )
	{
		if ( m_FollowBehavior.GetFollowTarget() == UTIL_GetLocalPlayer() ||
			 IsFollowingCommandPoint() )
		{
			return ( m_FollowBehavior.IsMovingToFollowTarget() );
		}
	}
	return false;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool CNPC_MetroPolice::ShouldAutoSummon()
{
	if ( !AI_IsSinglePlayer() || !IsFollowingCommandPoint() || !IsInPlayerSquad() )
		return false;

	CHL2_Player *pPlayer = (CHL2_Player *)UTIL_GetLocalPlayer();
	
	float distMovedSq = ( pPlayer->GetAbsOrigin() - m_vAutoSummonAnchor ).LengthSqr();
	float moveTolerance = player_squad_autosummon_move_tolerance.GetFloat() * 12;
	const Vector &vCommandGoal = GetCommandGoal();

	if ( distMovedSq < Square(moveTolerance * 10) && (GetAbsOrigin() - vCommandGoal).LengthSqr() > Square(10*12) && IsCommandMoving() )
	{
		m_AutoSummonTimer.Set( player_squad_autosummon_time.GetFloat() );
		if ( player_squad_autosummon_debug.GetBool() )
			DevMsg( "Waiting for arrival before initiating autosummon logic\n");
	}
	else if ( m_AutoSummonTimer.Expired() )
	{
		bool bSetFollow = false;
		bool bTestEnemies = true;
		
		// Auto summon unconditionally if a significant amount of time has passed
		if ( gpGlobals->curtime - m_AutoSummonTimer.GetNext() > player_squad_autosummon_time.GetFloat() * 2 )
		{
			bSetFollow = true;
			if ( player_squad_autosummon_debug.GetBool() )
				DevMsg( "Auto summoning squad: long time (%f)\n", ( gpGlobals->curtime - m_AutoSummonTimer.GetNext() ) + player_squad_autosummon_time.GetFloat() );
		}
			
		// Player must move for autosummon
		if ( distMovedSq > Square(12) )
		{
			bool bCommandPointIsVisible = pPlayer->FVisible( vCommandGoal + pPlayer->GetViewOffset() );

			// Auto summon if the player is close by the command point
			if ( !bSetFollow && bCommandPointIsVisible && distMovedSq > Square(24) )
			{
				float closenessTolerance = player_squad_autosummon_player_tolerance.GetFloat() * 12;
				if ( (pPlayer->GetAbsOrigin() - vCommandGoal).LengthSqr() < Square( closenessTolerance ) &&
					 ((m_vAutoSummonAnchor - vCommandGoal).LengthSqr() > Square( closenessTolerance )) )
				{
					bSetFollow = true;
					if ( player_squad_autosummon_debug.GetBool() )
						DevMsg( "Auto summoning squad: player close to command point (%f)\n", (GetAbsOrigin() - vCommandGoal).Length() );
				}
			}
			
			// Auto summon if moved a moderate distance and can't see command point, or moved a great distance
			if ( !bSetFollow )
			{
				if ( distMovedSq > Square( moveTolerance * 2 ) )
				{
					bSetFollow = true;
					bTestEnemies = ( distMovedSq < Square( moveTolerance * 10 ) );
					if ( player_squad_autosummon_debug.GetBool() )
						DevMsg( "Auto summoning squad: player very far from anchor (%f)\n", sqrt(distMovedSq) );
				}
				else if ( distMovedSq > Square( moveTolerance ) )
				{
					if ( !bCommandPointIsVisible )
					{
						bSetFollow = true;
						if ( player_squad_autosummon_debug.GetBool() )
							DevMsg( "Auto summoning squad: player far from anchor (%f)\n", sqrt(distMovedSq) );
					}
				}
			}
		}
		
		// Auto summon only if there are no readily apparent enemies
		if ( bSetFollow && bTestEnemies )
		{
			for ( int i = 0; i < g_AI_Manager.NumAIs(); i++ )
			{
				CAI_BaseNPC *pNpc = g_AI_Manager.AccessAIs()[i];
				float timeSinceCombatTolerance = player_squad_autosummon_time_after_combat.GetFloat();
				
				if ( pNpc->IsInPlayerSquad() )
				{
					if ( gpGlobals->curtime - pNpc->GetLastAttackTime() > timeSinceCombatTolerance || 
						 gpGlobals->curtime - pNpc->GetLastDamageTime() > timeSinceCombatTolerance )
						continue;
				}
				else if ( pNpc->GetEnemy() )
				{
					CBaseEntity *pNpcEnemy = pNpc->GetEnemy();
					if ( !IsSniper( pNpc ) && ( gpGlobals->curtime - pNpc->GetEnemyLastTimeSeen() ) > timeSinceCombatTolerance )
						continue;

					if ( pNpcEnemy == pPlayer )
					{
						if ( pNpc->CanBeAnEnemyOf( pPlayer ) )
						{
							bSetFollow = false;
							break;
						}
					}
					else if ( pNpcEnemy->IsNPC() && ( pNpcEnemy->MyNPCPointer()->GetSquad() == GetSquad() || pNpcEnemy->Classify() == CLASS_PLAYER_ALLY_VITAL ) )
					{
						if ( pNpc->CanBeAnEnemyOf( this ) )
						{
							bSetFollow = false;
							break;
						}
					}
				}
			}
			if ( !bSetFollow && player_squad_autosummon_debug.GetBool() )
				DevMsg( "Auto summon REVOKED: Combat recent \n");
		}
		
		return bSetFollow;
	}
	return false;
}

//-----------------------------------------------------------------------------
// Is this entity something that the citizen should interact with (return true)
// or something that he should try to get close to (return false)
//-----------------------------------------------------------------------------
bool CNPC_MetroPolice::IsValidCommandTarget( CBaseEntity *pTarget )
{
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: return TRUE if the commander mode should try to give this order
//			to more people. return FALSE otherwise. For instance, we don't
//			try to send all 3 selectedcitizens to pick up the same gun.
//-----------------------------------------------------------------------------
bool CNPC_MetroPolice::TargetOrder( CBaseEntity *pTarget, CAI_BaseNPC **Allies, int numAllies )
{
	if ( pTarget->IsPlayer() )
	{
		// I'm the target! Toggle follow!
		if( m_FollowBehavior.GetFollowTarget() != pTarget )
		{
			ClearFollowTarget();
			SetCommandGoal( vec3_invalid );

			// Turn follow on!
			m_AssaultBehavior.Disable();
			m_FollowBehavior.SetFollowTarget( pTarget );
			m_FollowBehavior.SetParameters( AIF_SIMPLE );			


			SpeakSentence(METROPOLICE_SENTENCE_AFFIRMATIVE);

			m_OnFollowOrder.FireOutput( this, this );
		}
		else if ( m_FollowBehavior.GetFollowTarget() == pTarget )
		{
			// Stop following.
			m_FollowBehavior.SetFollowTarget( NULL );
	
			//TERO: SAY NAY HERE
			SpeakSentence(METROPOLICE_SENTENCE_CANT_MOVE);
		}
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Turn off following before processing a move order.
//-----------------------------------------------------------------------------
void CNPC_MetroPolice::MoveOrder( const Vector &vecDest, CAI_BaseNPC **Allies, int numAllies )
{
	if ( !AI_IsSinglePlayer() )
		return;

	if( hl2_episodic.GetBool() && m_iszDenyCommandConcept != NULL_STRING )
	{
		//TERO: SAY NAY HERE
		SpeakSentence(METROPOLICE_SENTENCE_CANT_MOVE);
		return;
	}

	CHL2_Player *pPlayer = (CHL2_Player *)UTIL_GetLocalPlayer();

	m_AutoSummonTimer.Set( player_squad_autosummon_time.GetFloat() );
	m_vAutoSummonAnchor = pPlayer->GetAbsOrigin();

	if( m_StandoffBehavior.IsRunning() )
	{
		m_StandoffBehavior.SetStandoffGoalPosition( vecDest );
	}

	// If in assault, cancel and move.
	if( m_AssaultBehavior.HasHitRallyPoint() && !m_AssaultBehavior.HasHitAssaultPoint() )
	{
		m_AssaultBehavior.Disable();
		ClearSchedule( "Moving from rally point to assault point" );
	}

	bool spoke = false;

	CAI_BaseNPC *pClosest = NULL;
	float closestDistSq = FLT_MAX;

	for( int i = 0 ; i < numAllies ; i++ )
	{
		if( Allies[i]->IsInPlayerSquad() )
		{
			Assert( Allies[i]->IsCommandable() );
			float distSq = ( pPlayer->GetAbsOrigin() - Allies[i]->GetAbsOrigin() ).LengthSqr();
			if( distSq < closestDistSq )
			{
				pClosest = Allies[i];
				closestDistSq = distSq;
			}
		}
	}

	if( m_FollowBehavior.GetFollowTarget() && !IsFollowingCommandPoint() )
	{
		ClearFollowTarget();
#if 0
		if ( ( pPlayer->GetAbsOrigin() - GetAbsOrigin() ).LengthSqr() < Square( 180 ) &&
			 ( ( vecDest - pPlayer->GetAbsOrigin() ).LengthSqr() < Square( 120 ) || 
			   ( vecDest - GetAbsOrigin() ).LengthSqr() < Square( 120 ) ) )
		{
			if ( pClosest == this )
				SpeakIfAllowed( TLK_STOPFOLLOW );
			spoke = true;
		}
#endif
	}

	if ( !spoke && pClosest == this )
	{
		float destDistToPlayer = ( vecDest - pPlayer->GetAbsOrigin() ).Length();
		float destDistToClosest = ( vecDest - GetAbsOrigin() ).Length();
		CFmtStr modifiers( "commandpoint_dist_to_player:%.0f,"
						   "commandpoint_dist_to_npc:%.0f",
						   destDistToPlayer,
						   destDistToClosest );

		if (IsUnique())
		{
			SpeakIfAllowed( TLK_COMMANDED );
		}
		else
		{
			SpeakSentence(METROPOLICE_SENTENCE_AFFIRMATIVE);
		}
	}

	m_OnStationOrder.FireOutput( this, this );

	BaseClass::MoveOrder( vecDest, Allies, numAllies );
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CNPC_MetroPolice::OnMoveOrder()
{
	SetReadinessLevel( AIRL_STIMULATED, false, false );
	BaseClass::OnMoveOrder();
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CNPC_MetroPolice::CommanderUse( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	m_OnPlayerUse.FireOutput( pActivator, pCaller );

	// Under these conditions, citizens will refuse to go with the player.
	// Robin: NPCs should always respond to +USE even if someone else has the semaphore.
	if ( !AI_IsSinglePlayer() || !CanJoinPlayerSquad() ) //
	{
		SimpleUse( pActivator, pCaller, useType, value );
		return;
	}
	
	if ( pActivator == UTIL_GetLocalPlayer() )
	{
		if ( npc_metropolice_auto_player_squad_allow_use.GetBool() )
		{
			if ( !ShouldAutosquad() )
				TogglePlayerSquadState();
			else if ( !IsInPlayerSquad() && npc_metropolice_auto_player_squad_allow_use.GetBool() )
				AddToPlayerSquad();
		}
	}
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool CNPC_MetroPolice::ShouldSpeakRadio( CBaseEntity *pListener )
{
	if ( !pListener )
		return false;

	const float		radioRange = 384 * 384;
	Vector			vecDiff;

	vecDiff = WorldSpaceCenter() - pListener->WorldSpaceCenter();

	if( vecDiff.LengthSqr() > radioRange )
	{
		return true;
	}

	return false;
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CNPC_MetroPolice::OnMoveToCommandGoalFailed()
{
	// Clear the goal.
	SetCommandGoal( vec3_invalid );

	// Announce failure.
	//TERO: SAY NAY HERE
	SpeakSentence(METROPOLICE_SENTENCE_CANT_MOVE);
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CNPC_MetroPolice::AddToPlayerSquad()
{
	Assert( !IsInPlayerSquad() );

	AddToSquad( AllocPooledString(PLAYER_SQUADNAME) );
	m_hSavedFollowGoalEnt = m_FollowBehavior.GetFollowGoal();
	m_FollowBehavior.SetFollowGoalDirect( NULL );

	FixupPlayerSquad();

	SetCondition( COND_PLAYER_ADDED_TO_SQUAD );
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CNPC_MetroPolice::RemoveFromPlayerSquad()
{
	Assert( IsInPlayerSquad() );

	ClearFollowTarget();
	ClearCommandGoal();
	if ( m_iszOriginalSquad != NULL_STRING && strcmp( STRING( m_iszOriginalSquad ), PLAYER_SQUADNAME ) != 0 )
		AddToSquad( m_iszOriginalSquad );
	else
		RemoveFromSquad();
	
	if ( m_hSavedFollowGoalEnt )
		m_FollowBehavior.SetFollowGoal( m_hSavedFollowGoalEnt );

	SetCondition( COND_PLAYER_REMOVED_FROM_SQUAD );

	// Don't evaluate the player squad for 2 seconds. 
	gm_PlayerSquadEvaluateTimer.Set( 2.0 );
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CNPC_MetroPolice::TogglePlayerSquadState()
{
	if ( !AI_IsSinglePlayer() )
		return;

	if ( !IsInPlayerSquad() )
	{
		AddToPlayerSquad();

		if ( HaveCommandGoal() )
		{
			SpeakSentence(METROPOLICE_SENTENCE_AFFIRMATIVE);
		}
		else if ( m_FollowBehavior.GetFollowTarget() == UTIL_GetLocalPlayer() )
		{
			SpeakSentence(METROPOLICE_SENTENCE_AFFIRMATIVE);
		}
	}
	else
	{
		//TERO: SAY NAY HERE
		SpeakSentence(METROPOLICE_SENTENCE_CANT_MOVE);
		RemoveFromPlayerSquad();
	}
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
struct SquadCandidate_t
{
	CNPC_MetroPolice *pCitizen;
	bool		  bIsInSquad;
	float		  distSq;
	int			  iSquadIndex;
};

void CNPC_MetroPolice::UpdatePlayerSquad()
{
	if ( !AI_IsSinglePlayer() )
		return;

	CBasePlayer *pPlayer = UTIL_GetLocalPlayer();
	if ( ( pPlayer->GetAbsOrigin().AsVector2D() - GetAbsOrigin().AsVector2D() ).LengthSqr() < Square(20*12) )
		m_flTimeLastCloseToPlayer = gpGlobals->curtime;

	if ( !gm_PlayerSquadEvaluateTimer.Expired() )
		return;

	gm_PlayerSquadEvaluateTimer.Set( 2.0 );

	// Remove stragglers
	CAI_Squad *pPlayerSquad = g_AI_SquadManager.FindSquad( MAKE_STRING( PLAYER_SQUADNAME ) );
	if ( pPlayerSquad )
	{
		CUtlVectorFixed<CNPC_MetroPolice *, MAX_PLAYER_SQUAD> squadMembersToRemove;
		AISquadIter_t iter;

		for ( CAI_BaseNPC *pPlayerSquadMember = pPlayerSquad->GetFirstMember(&iter); pPlayerSquadMember; pPlayerSquadMember = pPlayerSquad->GetNextMember(&iter) )
		{
			if ( pPlayerSquadMember->GetClassname() != GetClassname() )
				continue;

			CNPC_MetroPolice *pCitizen = assert_cast<CNPC_MetroPolice *>(pPlayerSquadMember);

			if ( !pCitizen->m_bNeverLeavePlayerSquad &&
				 pCitizen->m_FollowBehavior.GetFollowTarget() &&
				 !pCitizen->m_FollowBehavior.FollowTargetVisible() && 
				 pCitizen->m_FollowBehavior.GetNumFailedFollowAttempts() > 0 && 
				 gpGlobals->curtime - pCitizen->m_FollowBehavior.GetTimeFailFollowStarted() > 20 &&
				 ( fabsf(( pCitizen->m_FollowBehavior.GetFollowTarget()->GetAbsOrigin().z - pCitizen->GetAbsOrigin().z )) > 196 ||
				   ( pCitizen->m_FollowBehavior.GetFollowTarget()->GetAbsOrigin().AsVector2D() - pCitizen->GetAbsOrigin().AsVector2D() ).LengthSqr() > Square(50*12) ) )
			{
				if ( DebuggingCommanderMode() )
				{
					DevMsg( "Player follower is lost (%d, %f, %d)\n", 
						 pCitizen->m_FollowBehavior.GetNumFailedFollowAttempts(), 
						 gpGlobals->curtime - pCitizen->m_FollowBehavior.GetTimeFailFollowStarted(), 
						 (int)((pCitizen->m_FollowBehavior.GetFollowTarget()->GetAbsOrigin().AsVector2D() - pCitizen->GetAbsOrigin().AsVector2D() ).Length()) );
				}

				squadMembersToRemove.AddToTail( pCitizen );
			}
		}

		for ( int i = 0; i < squadMembersToRemove.Count(); i++ )
		{
			squadMembersToRemove[i]->RemoveFromPlayerSquad();
		}
	}

	// Autosquadding
	const float JOIN_PLAYER_XY_TOLERANCE_SQ = Square(36*12);
	const float UNCONDITIONAL_JOIN_PLAYER_XY_TOLERANCE_SQ = Square(12*12);
	const float UNCONDITIONAL_JOIN_PLAYER_Z_TOLERANCE = 5*12;
	const float SECOND_TIER_JOIN_DIST_SQ = Square(48*12);
	if ( pPlayer && ShouldAutosquad() && !(pPlayer->GetFlags() & FL_NOTARGET ) && pPlayer->IsAlive() )
	{
		CAI_BaseNPC **ppAIs = g_AI_Manager.AccessAIs();
		CUtlVector<SquadCandidate_t> candidates;
		const Vector &vPlayerPos = pPlayer->GetAbsOrigin();
		bool bFoundNewGuy = false;
		int i;

		for ( i = 0; i < g_AI_Manager.NumAIs(); i++ )
		{
			if ( ppAIs[i]->GetState() == NPC_STATE_DEAD )
				continue;

			if ( ppAIs[i]->GetClassname() != GetClassname() )
				continue;

			CNPC_MetroPolice *pCitizen = assert_cast<CNPC_MetroPolice *>(ppAIs[i]);
			int iNew;

			if ( pCitizen->IsInPlayerSquad() )
			{
				iNew = candidates.AddToTail();
				candidates[iNew].pCitizen = pCitizen;
				candidates[iNew].bIsInSquad = true;
				candidates[iNew].distSq = 0;
				candidates[iNew].iSquadIndex = pCitizen->GetSquad()->GetSquadIndex( pCitizen );
			}
			else
			{
				float distSq = (vPlayerPos.AsVector2D() - pCitizen->GetAbsOrigin().AsVector2D()).LengthSqr(); 
				if ( distSq > JOIN_PLAYER_XY_TOLERANCE_SQ && 
					( pCitizen->m_flTimeJoinedPlayerSquad == 0 || gpGlobals->curtime - pCitizen->m_flTimeJoinedPlayerSquad > 60.0 ) && 
					( pCitizen->m_flTimeLastCloseToPlayer == 0 || gpGlobals->curtime - pCitizen->m_flTimeLastCloseToPlayer > 15.0 ) )
					continue;

				if ( !pCitizen->CanJoinPlayerSquad() )
					continue;

				bool bShouldAdd = false;

				if ( pCitizen->HasCondition( COND_SEE_PLAYER ) )
					bShouldAdd = true;
				else
				{
					bool bPlayerVisible = pCitizen->FVisible( pPlayer );
					if ( bPlayerVisible )
					{
						if ( pCitizen->HasCondition( COND_HEAR_PLAYER ) )
							bShouldAdd = true;
						else if ( distSq < UNCONDITIONAL_JOIN_PLAYER_XY_TOLERANCE_SQ && fabsf(vPlayerPos.z - pCitizen->GetAbsOrigin().z) < UNCONDITIONAL_JOIN_PLAYER_Z_TOLERANCE )
							bShouldAdd = true;
					}
				}

				if ( bShouldAdd )
				{
					// @TODO (toml 05-25-04): probably everyone in a squad should be a candidate if one of them sees the player
					AI_Waypoint_t *pPathToPlayer = pCitizen->GetPathfinder()->BuildRoute( pCitizen->GetAbsOrigin(), vPlayerPos, pPlayer, 5*12, NAV_NONE, true );
					GetPathfinder()->UnlockRouteNodes( pPathToPlayer );

					if ( !pPathToPlayer )
						continue;

					CAI_Path tempPath;
					tempPath.SetWaypoints( pPathToPlayer ); // path object will delete waypoints

					iNew = candidates.AddToTail();
					candidates[iNew].pCitizen = pCitizen;
					candidates[iNew].bIsInSquad = false;
					candidates[iNew].distSq = distSq;
					candidates[iNew].iSquadIndex = -1;
					
					bFoundNewGuy = true;
				}
			}
		}
		
		if ( bFoundNewGuy )
		{
			// Look for second order guys
			int initialCount = candidates.Count();
			for ( i = 0; i < initialCount; i++ )
				candidates[i].pCitizen->AddSpawnFlags( SF_METROPOLICE_NOT_COMMANDABLE ); // Prevents double-add
			for ( i = 0; i < initialCount; i++ )
			{
				if ( candidates[i].iSquadIndex == -1 )
				{
					for ( int j = 0; j < g_AI_Manager.NumAIs(); j++ )
					{
						if ( ppAIs[j]->GetState() == NPC_STATE_DEAD )
							continue;

						if ( ppAIs[j]->GetClassname() != GetClassname() )
							continue;

						if ( ppAIs[j]->HasSpawnFlags( SF_METROPOLICE_NOT_COMMANDABLE ) )
							continue; 

						CNPC_MetroPolice *pCitizen = assert_cast<CNPC_MetroPolice *>(ppAIs[j]);

						float distSq = (vPlayerPos - pCitizen->GetAbsOrigin()).Length2DSqr(); 
						if ( distSq > JOIN_PLAYER_XY_TOLERANCE_SQ )
							continue;

						distSq = (candidates[i].pCitizen->GetAbsOrigin() - pCitizen->GetAbsOrigin()).Length2DSqr(); 
						if ( distSq > SECOND_TIER_JOIN_DIST_SQ )
							continue;

						if ( !pCitizen->CanJoinPlayerSquad() )
							continue;

						if ( !pCitizen->FVisible( pPlayer ) )
							continue;

						int iNew = candidates.AddToTail();
						candidates[iNew].pCitizen = pCitizen;
						candidates[iNew].bIsInSquad = false;
						candidates[iNew].distSq = distSq;
						candidates[iNew].iSquadIndex = -1;
						pCitizen->AddSpawnFlags( SF_METROPOLICE_NOT_COMMANDABLE ); // Prevents double-add
					}
				}
			}
			for ( i = 0; i < candidates.Count(); i++ )
				candidates[i].pCitizen->RemoveSpawnFlags( SF_METROPOLICE_NOT_COMMANDABLE );

			if ( candidates.Count() > MAX_PLAYER_SQUAD )
			{
				candidates.Sort( PlayerSquadCandidateSortFunc );

				for ( i = MAX_PLAYER_SQUAD; i < candidates.Count(); i++ )
				{
					if ( candidates[i].pCitizen->IsInPlayerSquad() )
					{
						candidates[i].pCitizen->RemoveFromPlayerSquad();
					}
				}
			}

			if ( candidates.Count() )
			{
				CNPC_MetroPolice *pClosest = NULL;
				float closestDistSq = FLT_MAX;
				int nJoined = 0;

				for ( i = 0; i < candidates.Count() && i < MAX_PLAYER_SQUAD; i++ )
				{
					if ( !candidates[i].pCitizen->IsInPlayerSquad() )
					{
						candidates[i].pCitizen->AddToPlayerSquad();
						nJoined++;

						if ( candidates[i].distSq < closestDistSq )
						{
							pClosest = candidates[i].pCitizen;
							closestDistSq = candidates[i].distSq;
						}
					}
				}

				if ( pClosest && !IsUnique() )
				{
					pClosest->m_Sentences.Speak("METROPOLICE_IDLE_CLEAR");

					/*if ( !pClosest->SpokeConcept( TLK_JOINPLAYER ) )
					{
						pClosest->SpeakSentence(METROPOLICE_SENTENCE_AFFIRMATIVE);
					}
					else
					{
						pClosest->SpeakSentence(METROPOLICE_SENTENCE_AFFIRMATIVE);
					}

					for ( i = 0; i < candidates.Count() && i < MAX_PLAYER_SQUAD; i++ )
					{
						candidates[i].pCitizen->SetSpokeConcept( TLK_JOINPLAYER, NULL ); 
					}*/
				}
			}
		}
	}
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
int CNPC_MetroPolice::PlayerSquadCandidateSortFunc( const SquadCandidate_t *pLeft, const SquadCandidate_t *pRight )
{
	// "Bigger" means less approprate 
	CNPC_MetroPolice *pLeftCitizen = pLeft->pCitizen;
	CNPC_MetroPolice *pRightCitizen = pRight->pCitizen;

	// Medics are better than anyone
	if ( pLeftCitizen->IsMedic() && !pRightCitizen->IsMedic() )
		return -1;

	if ( !pLeftCitizen->IsMedic() && pRightCitizen->IsMedic() )
		return 1;

	CBaseCombatWeapon *pLeftWeapon = pLeftCitizen->GetActiveWeapon();
	CBaseCombatWeapon *pRightWeapon = pRightCitizen->GetActiveWeapon();
	
	// People with weapons are better than those without
	if ( pLeftWeapon && !pRightWeapon )
		return -1;
		
	if ( !pLeftWeapon && pRightWeapon )
		return 1;
	
	// Existing squad members are better than non-members
	if ( pLeft->bIsInSquad && !pRight->bIsInSquad )
		return -1;

	if ( !pLeft->bIsInSquad && pRight->bIsInSquad )
		return 1;

	// New squad members are better than older ones
	if ( pLeft->bIsInSquad && pRight->bIsInSquad )
		return pRight->iSquadIndex - pLeft->iSquadIndex;

	// Finally, just take the closer
	return (int)(pRight->distSq - pLeft->distSq);
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CNPC_MetroPolice::FixupPlayerSquad()
{
	if ( !AI_IsSinglePlayer() )
		return;

	m_flTimeJoinedPlayerSquad = gpGlobals->curtime;
	m_bWasInPlayerSquad = true;
	if ( m_pSquad->NumMembers() > MAX_PLAYER_SQUAD )
	{
		CAI_BaseNPC *pFirstMember = m_pSquad->GetFirstMember(NULL);
		m_pSquad->RemoveFromSquad( pFirstMember );
		pFirstMember->ClearCommandGoal();

		CNPC_MetroPolice *pFirstMemberCitizen = dynamic_cast< CNPC_MetroPolice * >( pFirstMember );
		if ( pFirstMemberCitizen )
		{
			pFirstMemberCitizen->ClearFollowTarget();
		}
		else
		{
			CAI_FollowBehavior *pOldMemberFollowBehavior;
			if ( pFirstMember->GetBehavior( &pOldMemberFollowBehavior ) )
			{
				pOldMemberFollowBehavior->SetFollowTarget( NULL );
			}
		}
	}

	ClearFollowTarget();

	CAI_BaseNPC *pLeader = NULL;
	AISquadIter_t iter;
	for ( CAI_BaseNPC *pAllyNpc = m_pSquad->GetFirstMember(&iter); pAllyNpc; pAllyNpc = m_pSquad->GetNextMember(&iter) )
	{
		if ( pAllyNpc->IsCommandable() )
		{
			pLeader = pAllyNpc;
			break;
		}
	}

	if ( pLeader && pLeader != this )
	{
		const Vector &commandGoal = pLeader->GetCommandGoal();
		if ( commandGoal != vec3_invalid )
		{
			SetCommandGoal( commandGoal );
			SetCondition( COND_RECEIVED_ORDERS ); 
			OnMoveOrder();
		}
		else
		{
			CAI_FollowBehavior *pLeaderFollowBehavior;
			if ( pLeader->GetBehavior( &pLeaderFollowBehavior ) )
			{
				m_FollowBehavior.SetFollowTarget( pLeaderFollowBehavior->GetFollowTarget() );
				m_FollowBehavior.SetParameters( m_FollowBehavior.GetFormation() );
			}

		}
	}
	else
	{
		m_FollowBehavior.SetFollowTarget( UTIL_GetLocalPlayer() );
		m_FollowBehavior.SetParameters( AIF_SIMPLE );
	}
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CNPC_MetroPolice::ClearFollowTarget()
{
	m_FollowBehavior.SetFollowTarget( NULL );
	m_FollowBehavior.SetParameters( AIF_SIMPLE );
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CNPC_MetroPolice::UpdateFollowCommandPoint()
{
	if ( !AI_IsSinglePlayer() )
		return;

	if ( IsInPlayerSquad() )
	{
		if ( HaveCommandGoal() )
		{
			CBaseEntity *pFollowTarget = m_FollowBehavior.GetFollowTarget();
			CBaseEntity *pCommandPoint = gEntList.FindEntityByClassname( NULL, COMMAND_POINT_CLASSNAME );
			
			if( !pCommandPoint )
			{
				DevMsg("**\nVERY BAD THING\nCommand point vanished! Creating a new one\n**\n");
				pCommandPoint = CreateEntityByName( COMMAND_POINT_CLASSNAME );
			}

			if ( pFollowTarget != pCommandPoint )
			{
				pFollowTarget = pCommandPoint;
				m_FollowBehavior.SetFollowTarget( pFollowTarget );
				m_FollowBehavior.SetParameters( AIF_COMMANDER );
			}
			
			if ( ( pCommandPoint->GetAbsOrigin() - GetCommandGoal() ).LengthSqr() > 0.01 )
			{
				UTIL_SetOrigin( pCommandPoint, GetCommandGoal(), false );
			}
		}
		else
		{
			if ( IsFollowingCommandPoint() )
				ClearFollowTarget();
			if ( m_FollowBehavior.GetFollowTarget() != UTIL_GetLocalPlayer() )
			{
				DevMsg( "Expected to be following player, but not\n" );
				m_FollowBehavior.SetFollowTarget( UTIL_GetLocalPlayer() );
				m_FollowBehavior.SetParameters( AIF_SIMPLE );
			}
		}
	}
	else if ( IsFollowingCommandPoint() )
		ClearFollowTarget();
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool CNPC_MetroPolice::IsFollowingCommandPoint()
{
	CBaseEntity *pFollowTarget = m_FollowBehavior.GetFollowTarget();
	if ( pFollowTarget )
		return FClassnameIs( pFollowTarget, COMMAND_POINT_CLASSNAME );
	return false;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
struct SquadMemberInfo_t
{
	CNPC_MetroPolice *	pMember;
	bool			bSeesPlayer;
	float			distSq;
};

int __cdecl SquadSortFunc( const SquadMemberInfo_t *pLeft, const SquadMemberInfo_t *pRight )
{
	if ( pLeft->bSeesPlayer && !pRight->bSeesPlayer )
	{
		return -1;
	}

	if ( !pLeft->bSeesPlayer && pRight->bSeesPlayer )
	{
		return 1;
	}

	return ( pLeft->distSq - pRight->distSq );
}

CAI_BaseNPC *CNPC_MetroPolice::GetSquadCommandRepresentative()
{
	if ( !AI_IsSinglePlayer() )
		return NULL;

	if ( IsInPlayerSquad() )
	{
		static float lastTime;
		static AIHANDLE hCurrent;

		if ( gpGlobals->curtime - lastTime > 2.0 || !hCurrent || !hCurrent->IsInPlayerSquad() ) // hCurrent will be NULL after level change
		{
			lastTime = gpGlobals->curtime;
			hCurrent = NULL;

			CUtlVectorFixed<SquadMemberInfo_t, MAX_SQUAD_MEMBERS> candidates;
			CBasePlayer *pPlayer = UTIL_GetLocalPlayer();

			if ( pPlayer )
			{
				AISquadIter_t iter;
				for ( CAI_BaseNPC *pAllyNpc = m_pSquad->GetFirstMember(&iter); pAllyNpc; pAllyNpc = m_pSquad->GetNextMember(&iter) )
				{
					if ( pAllyNpc->IsCommandable() && dynamic_cast<CNPC_MetroPolice *>(pAllyNpc) )
					{
						int i = candidates.AddToTail();
						candidates[i].pMember = (CNPC_MetroPolice *)(pAllyNpc);
						candidates[i].bSeesPlayer = pAllyNpc->HasCondition( COND_SEE_PLAYER );
						candidates[i].distSq = ( pAllyNpc->GetAbsOrigin() - pPlayer->GetAbsOrigin() ).LengthSqr();
					}
				}

				if ( candidates.Count() > 0 )
				{
					candidates.Sort( SquadSortFunc );
					hCurrent = candidates[0].pMember;
				}
			}
		}

		if ( hCurrent != NULL )
		{
			Assert( dynamic_cast<CNPC_MetroPolice *>(hCurrent.Get()) && hCurrent->IsInPlayerSquad() );
			return hCurrent;
		}
	}
	return NULL;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CNPC_MetroPolice::SetSquad( CAI_Squad *pSquad )
{
	bool bWasInPlayerSquad = IsInPlayerSquad();

	BaseClass::SetSquad( pSquad );

	if( IsInPlayerSquad() && !bWasInPlayerSquad )
	{
		m_OnJoinedPlayerSquad.FireOutput(this, this);
		if ( npc_metropolice_insignia.GetBool() )
			AddInsignia();
	}
	else if ( !IsInPlayerSquad() && bWasInPlayerSquad )
	{
		if ( npc_metropolice_insignia.GetBool() )
			RemoveInsignia();
		m_OnLeftPlayerSquad.FireOutput(this, this);
	}
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool CNPC_MetroPolice::CanHeal()
{ 
	if ( !IsMedic() && !IsAmmoResupplier() )
		return false;

	if( !hl2_episodic.GetBool() )
	{
		// If I'm not armed, my priority should be to arm myself.
		if ( IsMedic() && !GetActiveWeapon() )
			return false;
	}

	if ( IsInAScript() || (m_NPCState == NPC_STATE_SCRIPT) )
		return false;

	return true;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool CNPC_MetroPolice::ShouldHealTarget( CBaseEntity *pTarget, bool bActiveUse )
{
	Disposition_t disposition;
	
	if ( !pTarget && ( ( disposition = IRelationType( pTarget ) ) != D_LI && disposition != D_NU ) )
		return false;

	// Don't heal if I'm in the middle of talking
	if ( IsSpeaking() )
		return false;

	bool bTargetIsPlayer = pTarget->IsPlayer();

	// Don't heal or give ammo to targets in vehicles
	CBaseCombatCharacter *pCCTarget = pTarget->MyCombatCharacterPointer();
	if ( pCCTarget != NULL && pCCTarget->IsInAVehicle() )
		return false;

	if ( IsMedic() )
	{
		Vector toPlayer = ( pTarget->GetAbsOrigin() - GetAbsOrigin() );
	 	if (( bActiveUse || !HaveCommandGoal() || toPlayer.Length() < HEAL_TARGET_RANGE) && fabs(toPlayer.z) < HEAL_TARGET_RANGE_Z )
	 	{
			if ( pTarget->m_iHealth > 0 )
			{
	 			if ( bActiveUse )
				{
					// Ignore heal requests if we're going to heal a tiny amount
					float timeFullHeal = m_flPlayerHealTime;
					float timeRecharge = sk_metropolice_heal_player_delay.GetFloat();
					float maximumHealAmount = sk_metropolice_heal_player.GetFloat();
					float healAmt = ( maximumHealAmount * ( 1.0 - ( timeFullHeal - gpGlobals->curtime ) / timeRecharge ) );
					if ( healAmt > pTarget->m_iMaxHealth - pTarget->m_iHealth )
						healAmt = pTarget->m_iMaxHealth - pTarget->m_iHealth;
					if ( healAmt < sk_metropolice_heal_player_min_forced.GetFloat() )
						return false;

	 				return ( pTarget->m_iMaxHealth > pTarget->m_iHealth );
				}
	 				
				// Are we ready to heal again?
				bool bReadyToHeal = ( ( bTargetIsPlayer && m_flPlayerHealTime <= gpGlobals->curtime ) || 
									  ( !bTargetIsPlayer && m_flAllyHealTime <= gpGlobals->curtime ) );

				// Only heal if we're ready
				if ( bReadyToHeal )
				{
					int requiredHealth;

					if ( bTargetIsPlayer )
						requiredHealth = pTarget->GetMaxHealth() - sk_metropolice_heal_player.GetFloat();
					else
						requiredHealth = pTarget->GetMaxHealth() * sk_metropolice_heal_player_min_pct.GetFloat();

					if ( ( pTarget->m_iHealth <= requiredHealth ) && IRelationType( pTarget ) == D_LI )
						return true;
				}
			}
		}
	}

	// Only players need ammo
	if ( IsAmmoResupplier() && bTargetIsPlayer )
	{
		if ( m_flPlayerGiveAmmoTime <= gpGlobals->curtime )
		{
			int iAmmoType = -1;
			

			if ( stricmp( "Current", STRING(m_iszAmmoSupply) ) == 0 )
				iAmmoType = ((CBasePlayer*)pTarget)->GetPrimaryWeaponAmmoType();
			else	
				iAmmoType = GetAmmoDef()->Index( STRING(m_iszAmmoSupply) );

			if ( iAmmoType == -1 )
			{
				DevMsg("ERROR: Citizen attempting to give unknown ammo type (%s)\n", STRING(m_iszAmmoSupply) );
			}
			else
			{
				// Does the player need the ammo we can give him?
				int iMax = GetAmmoDef()->MaxCarry(iAmmoType);
				int iCount = ((CBasePlayer*)pTarget)->GetAmmoCount(iAmmoType);
				if ( !iCount || ((iMax - iCount) >= m_iAmmoAmount) )
				{
					// Only give the player ammo if he has a weapon that uses it
					if ( ((CBasePlayer*)pTarget)->Weapon_GetWpnForAmmo( iAmmoType ) )
					{
						//TERO: We are going to try to give ammo.
						//		Make sure the others are not doing it the same time.
						for ( CNPC_UniqueMetrocop *pUnique = CNPC_UniqueMetrocop::GetUniqueMetrocopList(); pUnique != NULL; pUnique = pUnique->m_pNext )
						{
							if ( pUnique == NULL )
								continue;

							if ( pUnique != this )
							{
								pUnique->UpdatePlayerGiveAmmoTime( gpGlobals->curtime + 2.0f );
							}
						}


						return true;
					}
				}
			}
		}
	}
	return false;
}

bool CNPC_MetroPolice::ShouldHealItself()
{

	// Don't heal if I'm in the middle of talking
	if ( IsSpeaking() )
		return false;

	// Don't healmyself if I'm in vehicle
	CBaseCombatCharacter *pCCTarget = MyCombatCharacterPointer();
	if ( pCCTarget != NULL && pCCTarget->IsInAVehicle() )
		return false;

	if ( IsMedic() )
	{
		// Only heal if we're ready
		if (  m_flAllyHealTime <= gpGlobals->curtime )
		{
			int requiredHealth = GetMaxHealth() * sk_metropolice_heal_player_min_pct.GetFloat();

			if ( ( m_iHealth <= requiredHealth ) )
				return true;
		}
	}

	return false;
}


//-----------------------------------------------------------------------------
// Determine if the citizen is in a position to be throwing medkits
//-----------------------------------------------------------------------------
bool CNPC_MetroPolice::ShouldHealTossTarget( CBaseEntity *pTarget, bool bActiveUse )
{
	Disposition_t disposition;

	Assert( IsMedic() );
	if ( !IsMedic() )
		return false;
	
	if ( !pTarget && ( ( disposition = IRelationType( pTarget ) ) != D_LI && disposition != D_NU ) )
		return false;

	// Don't heal if I'm in the middle of talking
	if ( IsSpeaking() )
		return false;

	bool bTargetIsPlayer = pTarget->IsPlayer();

	// Don't heal or give ammo to targets in vehicles
	CBaseCombatCharacter *pCCTarget = pTarget->MyCombatCharacterPointer();
	if ( pCCTarget != NULL && pCCTarget->IsInAVehicle() )
		return false;

	Vector toPlayer = ( pTarget->GetAbsOrigin() - GetAbsOrigin() );
	if ( bActiveUse || !HaveCommandGoal() || toPlayer.Length() < HEAL_TOSS_TARGET_RANGE )
	{
		if ( pTarget->m_iHealth > 0 )
		{
			if ( bActiveUse )
			{
				// Ignore heal requests if we're going to heal a tiny amount
				float timeFullHeal = m_flPlayerHealTime;
				float timeRecharge = sk_metropolice_heal_player_delay.GetFloat();
				float maximumHealAmount = sk_metropolice_heal_player.GetFloat();
				float healAmt = ( maximumHealAmount * ( 1.0 - ( timeFullHeal - gpGlobals->curtime ) / timeRecharge ) );
				if ( healAmt > pTarget->m_iMaxHealth - pTarget->m_iHealth )
					healAmt = pTarget->m_iMaxHealth - pTarget->m_iHealth;
				if ( healAmt < sk_metropolice_heal_player_min_forced.GetFloat() )
					return false;

				return ( pTarget->m_iMaxHealth > pTarget->m_iHealth );
			}

			// Are we ready to heal again?
			bool bReadyToHeal = ( ( bTargetIsPlayer && m_flPlayerHealTime <= gpGlobals->curtime ) || 
				( !bTargetIsPlayer && m_flAllyHealTime <= gpGlobals->curtime ) );

			// Only heal if we're ready
			if ( bReadyToHeal )
			{
				int requiredHealth;

				if ( bTargetIsPlayer )
					requiredHealth = pTarget->GetMaxHealth() - sk_metropolice_heal_player.GetFloat();
				else
					requiredHealth = pTarget->GetMaxHealth() * sk_metropolice_heal_player_min_pct.GetFloat();

				if ( ( pTarget->m_iHealth <= requiredHealth ) && IRelationType( pTarget ) == D_LI )
					return true;
			}
		}
	}
	
	return false;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CNPC_MetroPolice::Heal()
{
	if ( !CanHeal() )
		  return;

	CBaseEntity *pTarget = GetTarget();

	if (!pTarget)
		return;

	Vector target = pTarget->GetAbsOrigin() - GetAbsOrigin();
	if ( target.Length() > HEAL_TARGET_RANGE * 2 )
		return;

	// Don't heal a player that's staring at you until a few seconds have passed.
	m_flTimeNextHealStare = gpGlobals->curtime + sk_metropolice_stare_heal_time.GetFloat();

	if ( IsMedic() )
	{
		float timeFullHeal;
		float timeRecharge;
		float maximumHealAmount;
		if ( pTarget->IsPlayer() )
		{
			timeFullHeal 		= m_flPlayerHealTime;
			timeRecharge 		= sk_metropolice_heal_player_delay.GetFloat();
			maximumHealAmount 	= sk_metropolice_heal_player.GetFloat();
			m_flPlayerHealTime 	= gpGlobals->curtime + timeRecharge;
		}
		else
		{
			timeFullHeal 		= m_flAllyHealTime;
			timeRecharge 		= sk_metropolice_heal_ally_delay.GetFloat();
			maximumHealAmount 	= sk_metropolice_heal_ally.GetFloat();
			m_flAllyHealTime 	= gpGlobals->curtime + timeRecharge;
		}
		
		float healAmt = ( maximumHealAmount * ( 1.0 - ( timeFullHeal - gpGlobals->curtime ) / timeRecharge ) );
		
		if ( healAmt > maximumHealAmount )
			healAmt = maximumHealAmount;
		else
			healAmt = RoundFloatToInt( healAmt );
		
		if ( healAmt > 0 )
		{
			if ( pTarget->IsPlayer() && npc_metropolice_medic_emit_sound.GetBool() )
			{
				CPASAttenuationFilter filter(pTarget, "HealthKit.Touch");
				EmitSound(filter, pTarget->entindex(), "HealthKit.Touch");
				//EmitSound( CPASAttenuationFilter( pTarget, "HealthKit.Touch" ), pTarget->entindex(), "HealthKit.Touch" );
			}

			pTarget->TakeHealth( healAmt, DMG_GENERIC );
			pTarget->RemoveAllDecals();
		}
	}

	if ( IsAmmoResupplier() )
	{
		// Non-players don't use ammo
		if ( pTarget->IsPlayer() )
		{
			int iAmmoType = -1; //GetAmmoDef()->Index( STRING(m_iszAmmoSupply) );
			if ( stricmp( "Current", STRING(m_iszAmmoSupply) ) == 0 )
				iAmmoType = ((CBasePlayer*)pTarget)->GetPrimaryWeaponAmmoType();
			else	
				iAmmoType = GetAmmoDef()->Index( STRING(m_iszAmmoSupply) );

			if ( iAmmoType == -1 )
			{
				DevMsg("ERROR: Citizen attempting to give unknown ammo type (%s)\n", STRING(m_iszAmmoSupply) );
			}
			else
			{
				((CBasePlayer*)pTarget)->GiveAmmo( m_iAmmoAmount, iAmmoType, false );
			}

			m_flPlayerGiveAmmoTime = gpGlobals->curtime + sk_metropolice_giveammo_player_delay.GetFloat();

			for ( CNPC_UniqueMetrocop *pUnique = CNPC_UniqueMetrocop::GetUniqueMetrocopList(); pUnique != NULL; pUnique = pUnique->m_pNext )
			{
				if ( pUnique == NULL )
					continue;

				if ( pUnique != this )
				{
					pUnique->UpdatePlayerGiveAmmoTime( m_flPlayerGiveAmmoTime - 2.0f );
				}
			}

			//TERO: lets update unique member ammo times
			/*char *uniqueClassNames[3] = { "npc_eloise", "npc_noah", "npc_larson", };
			for (int i=0; i<3; i++)
			{
				//CN *pEntity = dynamic_cast<CBaseEntity *>
				CNPC_MetroPolice *pEntity = dynamic_cast<CNPC_MetroPolice*>(gEntList.FindEntityByClassname( NULL, uniqueClassNames[i] ));
				if (pEntity && pEntity != this)
				{
					//TERO: yep, that 2.0f is a magic number!
					pEntity->UpdatePlayerGiveAmmoTime( m_flPlayerGiveAmmoTime - 2.0f );
				}
			}*/
		}
	}
}

void	CNPC_MetroPolice::UpdatePlayerGiveAmmoTime(float flGiveAmmoTime) 
{
	if (m_flPlayerGiveAmmoTime < flGiveAmmoTime)
	{
		m_flPlayerGiveAmmoTime = flGiveAmmoTime;
	}
}

//-----------------------------------------------------------------------------
// Like Heal(), but tosses a healthkit in front of the player rather than just juicing him up.
//-----------------------------------------------------------------------------
void	CNPC_MetroPolice::TossHealthKit(CBaseCombatCharacter *pThrowAt, const Vector &offset)
{
	Assert( pThrowAt );

	Vector forward, right, up;
	GetVectors( &forward, &right, &up );
	Vector medKitOriginPoint = WorldSpaceCenter() + ( forward * 20.0f );
	Vector destinationPoint;
	// this doesn't work without a moveparent: pThrowAt->ComputeAbsPosition( offset, &destinationPoint );
	VectorTransform( offset, pThrowAt->EntityToWorldTransform(), destinationPoint );
	// flatten out any z change due to player looking up/down
	destinationPoint.z = pThrowAt->EyePosition().z;

	Vector tossVelocity;

	if (npc_metropolice_medic_throw_style.GetInt() == 0)
	{
		CTraceFilterSkipTwoEntities tracefilter( this, pThrowAt, COLLISION_GROUP_NONE );
		tossVelocity = VecCheckToss( this, &tracefilter, medKitOriginPoint, destinationPoint, 0.233f, 1.0f, false );
	}
	else
	{
		tossVelocity = VecCheckThrow( this, medKitOriginPoint, destinationPoint, MEDIC_THROW_SPEED, 1.0f );

		if (vec3_origin == tossVelocity)
		{
			// if out of range, just throw it as close as I can
			tossVelocity = destinationPoint - medKitOriginPoint;

			// rotate upwards against gravity
			float len = VectorLength(tossVelocity);
			tossVelocity *= (MEDIC_THROW_SPEED / len);
			tossVelocity.z += 0.57735026918962576450914878050196 * MEDIC_THROW_SPEED;
		}
	}

	// create a healthkit and toss it into the world
	CBaseEntity *pHealthKit = CreateEntityByName( "item_healthkit" );
	Assert(pHealthKit);
	if (pHealthKit)
	{
		pHealthKit->SetAbsOrigin( medKitOriginPoint );
		pHealthKit->SetOwnerEntity( this );
		// pHealthKit->SetAbsVelocity( tossVelocity );
		DispatchSpawn( pHealthKit );

		{
			IPhysicsObject *pPhysicsObject = pHealthKit->VPhysicsGetObject();
			Assert( pPhysicsObject );
			if ( pPhysicsObject )
			{
				unsigned int cointoss = random->RandomInt(0,0xFF); // int bits used for bools

				// some random precession
				Vector angDummy(random->RandomFloat(-200,200), random->RandomFloat(-200,200), 
					cointoss & 0x01 ? random->RandomFloat(200,600) : -1.0f * random->RandomFloat(200,600));
				pPhysicsObject->SetVelocity( &tossVelocity, &angDummy );
			}
		}
	}
	else
	{
		Warning("Citizen tried to heal but could not spawn item_healthkit!\n");
	}

}

//-----------------------------------------------------------------------------
// cause an immediate call to TossHealthKit with some default numbers
//-----------------------------------------------------------------------------
void	CNPC_MetroPolice::InputForceHealthKitToss( inputdata_t &inputdata )
{
	TossHealthKit( UTIL_GetLocalPlayer(), Vector(48.0f, 0.0f, 0.0f)  );
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
void CNPC_MetroPolice::InputSetCommandable( inputdata_t &inputdata )
{
	RemoveSpawnFlags( SF_METROPOLICE_NOT_COMMANDABLE );
	gm_PlayerSquadEvaluateTimer.Force();
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
void CNPC_MetroPolice::InputSetNotCommandable( inputdata_t &inputdata )
{
	AddSpawnFlags( SF_METROPOLICE_NOT_COMMANDABLE );
	
	if (IsInPlayerSquad())
	{
		RemoveFromPlayerSquad();
	}
	else
	{
		ClearFollowTarget();
		ClearCommandGoal();
		if ( m_iszOriginalSquad != NULL_STRING && strcmp( STRING( m_iszOriginalSquad ), PLAYER_SQUADNAME ) != 0 )
			AddToSquad( m_iszOriginalSquad );

		if ( m_hSavedFollowGoalEnt )
			m_FollowBehavior.SetFollowGoal( m_hSavedFollowGoalEnt );
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &inputdata - 
//-----------------------------------------------------------------------------
void CNPC_MetroPolice::InputSetMedicOn( inputdata_t &inputdata )
{
	AddSpawnFlags( SF_METROPOLICE_MEDIC );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &inputdata - 
//-----------------------------------------------------------------------------
void CNPC_MetroPolice::InputSetMedicOff( inputdata_t &inputdata )
{
	RemoveSpawnFlags( SF_METROPOLICE_MEDIC );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &inputdata - 
//-----------------------------------------------------------------------------
void CNPC_MetroPolice::InputSetAmmoResupplierOn( inputdata_t &inputdata )
{
	AddSpawnFlags( SF_METROPOLICE_AMMORESUPPLIER );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &inputdata - 
//-----------------------------------------------------------------------------
void CNPC_MetroPolice::InputSetAmmoResupplierOff( inputdata_t &inputdata )
{
	RemoveSpawnFlags( SF_METROPOLICE_AMMORESUPPLIER );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &inputdata - 
//-----------------------------------------------------------------------------
void CNPC_MetroPolice::InputSetRechargerOn( inputdata_t &inputdata )
{
	m_bCanRecharge = true;
	m_RechargeBehavior.SetCanRecharge( true );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &inputdata - 
//-----------------------------------------------------------------------------
void CNPC_MetroPolice::InputSetRechargerOff( inputdata_t &inputdata )
{
	m_bCanRecharge = false;
	m_RechargeBehavior.SetCanRecharge( false );
}


//-----------------------------------------------------------------------------
//
// Schedules
//
//-----------------------------------------------------------------------------
AI_BEGIN_CUSTOM_NPC( npc_metropolice, CNPC_MetroPolice )

	gm_flTimeLastSpokePeek = 0;

	DECLARE_ANIMEVENT( AE_METROPOLICE_BATON_ON );
	DECLARE_ANIMEVENT( AE_METROPOLICE_BATON_OFF );
	DECLARE_ANIMEVENT( AE_METROPOLICE_SHOVE );
	DECLARE_ANIMEVENT( AE_METROPOLICE_START_DEPLOY );
	DECLARE_ANIMEVENT( AE_METROPOLICE_DRAW_PISTOL );
	DECLARE_ANIMEVENT( AE_METROPOLICE_DEPLOY_MANHACK );

	DECLARE_ANIMEVENT( AE_METROPOLICE_HEAL_SELF );
	DECLARE_ANIMEVENT( AE_METROPOLICE_HEAL_TARGET );
	DECLARE_ANIMEVENT( AE_METROPOLICE_TAKE_HEALTHKIT );

#ifdef ELOISE_KICK_BALLS

	DECLARE_ANIMEVENT( AE_METROPOLICE_KICK_BALLS );

#endif

	DECLARE_SQUADSLOT( SQUAD_SLOT_POLICE_CHARGE_ENEMY );
	DECLARE_SQUADSLOT( SQUAD_SLOT_POLICE_HARASS );
	DECLARE_SQUADSLOT( SQUAD_SLOT_POLICE_DEPLOY_MANHACK );
	DECLARE_SQUADSLOT( SQUAD_SLOT_POLICE_ATTACK_OCCLUDER1 );
	DECLARE_SQUADSLOT( SQUAD_SLOT_POLICE_ATTACK_OCCLUDER2 );
//	DECLARE_SQUADSLOT( SQUAD_SLOT_POLICE_ARREST_ENEMY );

	DECLARE_ACTIVITY( ACT_METROPOLICE_DRAW_PISTOL );
	DECLARE_ACTIVITY( ACT_METROPOLICE_DEPLOY_MANHACK );
	DECLARE_ACTIVITY( ACT_METROPOLICE_FLINCH_BEHIND );
	DECLARE_ACTIVITY( ACT_PUSH_PLAYER );
	DECLARE_ACTIVITY( ACT_MELEE_ATTACK_THRUST );
	DECLARE_ACTIVITY( ACT_ACTIVATE_BATON );
	DECLARE_ACTIVITY( ACT_DEACTIVATE_BATON );
	DECLARE_ACTIVITY( ACT_WALK_BATON );
	DECLARE_ACTIVITY( ACT_IDLE_ANGRY_BATON );

	DECLARE_ACTIVITY( ACT_METROPOLICE_HEAL_SELF );
	DECLARE_ACTIVITY( ACT_METROPOLICE_HEAL_TARGET );

	DECLARE_ACTIVITY( ACT_IDLE_ANGRY_357 );
	DECLARE_ACTIVITY( ACT_RANGE_ATTACK_357 );
	DECLARE_ACTIVITY( ACT_GESTURE_RANGE_ATTACK_357 );
	DECLARE_ACTIVITY( ACT_METROPOLICE_DRAW_357 );
	DECLARE_ACTIVITY( ACT_RELOAD_357 );
	DECLARE_ACTIVITY( ACT_GESTURE_RELOAD_357 );

	DECLARE_INTERACTION( g_interactionMetrocopIdleChatter );	
	DECLARE_INTERACTION( g_interactionMetrocopClearSentenceQueues );

	DECLARE_TASK( TASK_METROPOLICE_HARASS );
	DECLARE_TASK( TASK_METROPOLICE_DIE_INSTANTLY );
	DECLARE_TASK( TASK_METROPOLICE_GET_PATH_TO_BESTSOUND_LOS );
	DECLARE_TASK( TASK_METROPOLICE_ACTIVATE_BATON );
	DECLARE_TASK( TASK_METROPOLICE_WAIT_FOR_SENTENCE );

	DECLARE_TASK( TASK_CP_HEAL );
	DECLARE_TASK( TASK_CP_HEAL_TOSS );

	DECLARE_CONDITION( COND_METROPOLICE_ON_FIRE );
	//DECLARE_CONDITION( COND_METROPOLICE_ENEMY_RESISTING_ARREST );
	DECLARE_CONDITION( COND_METROPOLICE_CHANGE_BATON_STATE );
	DECLARE_CONDITION( COND_METROPOLICE_PHYSOBJECT_ASSAULT );

	DECLARE_CONDITION( COND_CP_PLAYERHEALREQUEST );
	DECLARE_CONDITION( COND_CP_COMMANDHEAL );
	DECLARE_CONDITION( COND_CP_SCRIPT_INTERRUPT );
	
//=========================================================
// > SCHED_METROPOLICE_HEAL
//=========================================================
DEFINE_SCHEDULE
(
	SCHED_METROPOLICE_HEAL,

	"	Tasks"
	"		TASK_GET_PATH_TO_TARGET				0"
	"		TASK_MOVE_TO_TARGET_RANGE			50"
	"		TASK_STOP_MOVING					0"
	"		TASK_FACE_IDEAL						0"
	"		TASK_CP_HEAL						0"
	"	"
	"	Interrupts"
	"		COND_CP_SCRIPT_INTERRUPT"
);


//=========================================================
// > SCHED_METROPOLICE_HEAL_TOSS
//=========================================================
DEFINE_SCHEDULE
(
	SCHED_METROPOLICE_HEAL_TOSS,

	"	Tasks"
	"		TASK_STOP_MOVING					0"
	"		TASK_FACE_IDEAL						0"
	"		TASK_CP_HEAL_TOSS					0"
	"	"
	"	Interrupts"
	"		COND_CP_SCRIPT_INTERRUPT"
);

/*//=========================================================
// > SCHED_METROPOLICE_HEAL_SELF
//=========================================================
DEFINE_SCHEDULE
(
	SCHED_METROPOLICE_HEAL_SELF,

	"	Tasks"
	"		TASK_STOP_MOVING					0"
	"		TASK_CP_HEAL_SELF					0"
	"	"
	"	Interrupts"
);*/

//=========================================================
//=========================================================
DEFINE_SCHEDULE
(
	SCHED_METROPOLICE_WAKE_ANGRY,

	"	Tasks"
	"		TASK_STOP_MOVING				0"
	"		TASK_SET_ACTIVITY				ACTIVITY:ACT_IDLE"
	"		TASK_FACE_ENEMY					0"
	"	"
	"	Interrupts"
);

//=========================================================
// > InvestigateSound
//
//	sends a monster to the location of the
//	sound that was just heard to check things out.
//=========================================================
DEFINE_SCHEDULE
(
	SCHED_METROPOLICE_INVESTIGATE_SOUND,

	"	Tasks"
	"		TASK_STOP_MOVING				0"
	"		TASK_STORE_LASTPOSITION			0"
	"		TASK_METROPOLICE_GET_PATH_TO_BESTSOUND_LOS		0"
	"		TASK_FACE_IDEAL					0"
//	"		TASK_SET_TOLERANCE_DISTANCE		32"
	"		TASK_RUN_PATH					0"
	"		TASK_WAIT_FOR_MOVEMENT			0"
	"		TASK_STOP_MOVING				0"
	"		TASK_WAIT						5"
	"		TASK_GET_PATH_TO_LASTPOSITION	0"
	"		TASK_WALK_PATH					0"
	"		TASK_WAIT_FOR_MOVEMENT			0"
	"		TASK_STOP_MOVING				0"
	"		TASK_CLEAR_LASTPOSITION			0"
	"		TASK_FACE_REASONABLE			0"
	""
	"	Interrupts"
	"		COND_NEW_ENEMY"
	"		COND_SEE_FEAR"
	"		COND_SEE_ENEMY"
	"		COND_LIGHT_DAMAGE"
	"		COND_HEAVY_DAMAGE"
	"		COND_HEAR_DANGER"
	"		COND_CP_PLAYERHEALREQUEST"
	"		COND_CP_COMMANDHEAL"
);


//=========================================================
//=========================================================
DEFINE_SCHEDULE
(
	SCHED_METROPOLICE_HARASS,

	"	Tasks"
	"		TASK_STOP_MOVING				0"
	"		TASK_FACE_ENEMY					0"
	"		TASK_WAIT_FACE_ENEMY			6"
	"		TASK_METROPOLICE_HARASS			0"
	"		TASK_WAIT_PVS					0"
	"	"
	"	Interrupts"
//	"	"
	"		COND_CAN_RANGE_ATTACK1"
	"		COND_NEW_ENEMY"
	"		COND_CP_PLAYERHEALREQUEST"
	"		COND_CP_COMMANDHEAL"
);


//=========================================================
//=========================================================
DEFINE_SCHEDULE
(
	SCHED_METROPOLICE_DRAW_PISTOL,

	"	Tasks"
	"		TASK_STOP_MOVING				0"
	"		TASK_PLAY_SEQUENCE_FACE_ENEMY	ACTIVITY:ACT_METROPOLICE_DRAW_PISTOL"
	"		TASK_WAIT_FACE_ENEMY			0.1"
	"	"
	"	Interrupts"
	"	"
);


//=========================================================
// > ChaseEnemy
//=========================================================
DEFINE_SCHEDULE
(
	SCHED_METROPOLICE_CHASE_ENEMY,

	"	Tasks"
	"		TASK_STOP_MOVING				0"
	"		TASK_SET_FAIL_SCHEDULE			SCHEDULE:SCHED_METROPOLICE_ESTABLISH_LINE_OF_FIRE"
	"		TASK_SET_TOLERANCE_DISTANCE		24"
	"		TASK_GET_CHASE_PATH_TO_ENEMY	300"
	"		TASK_SPEAK_SENTENCE				6"	// METROPOLICE_SENTENCE_MOVE_INTO_POSITION
	"		TASK_RUN_PATH					0"
	"		TASK_WAIT_FOR_MOVEMENT			0"
	"		TASK_FACE_ENEMY					0"
	"	"
	"	Interrupts"
	"		COND_NEW_ENEMY"
	"		COND_ENEMY_DEAD"
	"		COND_ENEMY_UNREACHABLE"
	"		COND_CAN_RANGE_ATTACK1"
	"		COND_CAN_MELEE_ATTACK1"
	"		COND_CAN_RANGE_ATTACK2"
	"		COND_CAN_MELEE_ATTACK2"
	"		COND_TOO_CLOSE_TO_ATTACK"
	"		COND_TASK_FAILED"
	"		COND_LOST_ENEMY"
	"		COND_BETTER_WEAPON_AVAILABLE"
	"		COND_HEAR_DANGER"
	"		COND_CP_PLAYERHEALREQUEST"
	"		COND_CP_COMMANDHEAL"
);


DEFINE_SCHEDULE
(
	SCHED_METROPOLICE_ESTABLISH_LINE_OF_FIRE,

	"	Tasks "
	"		TASK_SET_FAIL_SCHEDULE			SCHEDULE:SCHED_FAIL_ESTABLISH_LINE_OF_FIRE"
	"		TASK_FACE_ENEMY					0"
	"		TASK_SET_TOLERANCE_DISTANCE		48"
	"		TASK_GET_PATH_TO_ENEMY_LKP_LOS	0"
	"		TASK_SPEAK_SENTENCE				6"	// METROPOLICE_SENTENCE_MOVE_INTO_POSITION
	"		TASK_RUN_PATH					0"
	"		TASK_WAIT_FOR_MOVEMENT			0"
	"		TASK_SET_SCHEDULE				SCHEDULE:SCHED_COMBAT_FACE"
	"	"
	"	Interrupts "
	"		COND_NEW_ENEMY"
	"		COND_ENEMY_DEAD"
	"		COND_CAN_RANGE_ATTACK1"
	"		COND_CAN_RANGE_ATTACK2"
	"		COND_CAN_MELEE_ATTACK1"
	"		COND_CAN_MELEE_ATTACK2"
	"		COND_HEAR_DANGER"
	"		COND_HEAVY_DAMAGE"
	"		COND_CP_PLAYERHEALREQUEST"
	"		COND_CP_COMMANDHEAL"
);


//=========================================================
// The uninterruptible portion of this behavior, whereupon 
// the police actually releases the manhack.
//=========================================================
DEFINE_SCHEDULE
(
	SCHED_METROPOLICE_DEPLOY_MANHACK,

	"	Tasks"
	"		TASK_SPEAK_SENTENCE					5"	// METROPOLICE_SENTENCE_DEPLOY_MANHACK
	"		TASK_PLAY_SEQUENCE					ACTIVITY:ACT_METROPOLICE_DEPLOY_MANHACK"
	"	"
	"	Interrupts"
	"	"
);


//===============================================
//===============================================

DEFINE_SCHEDULE
(
	SCHED_METROPOLICE_ADVANCE,

	"	Tasks"
	"		TASK_STOP_MOVING					0"
	"		TASK_SET_ACTIVITY					ACTIVITY:ACT_IDLE_ANGRY"
	"		TASK_FACE_ENEMY						0"
	"		TASK_WAIT_FACE_ENEMY				1" // give the guy some time to come out on his own
	"		TASK_WAIT_FACE_ENEMY_RANDOM			3" 
	"		TASK_GET_PATH_TO_ENEMY_LOS			0"
	"		TASK_RUN_PATH						0"
	"		TASK_WAIT_FOR_MOVEMENT				0"
	"		TASK_SET_ACTIVITY					ACTIVITY:ACT_IDLE_ANGRY"
	"		TASK_FACE_ENEMY						0"
	""
	"	Interrupts"
	"		COND_CAN_RANGE_ATTACK1"
	"		COND_ENEMY_DEAD"
	"		COND_CP_PLAYERHEALREQUEST"
	"		COND_CP_COMMANDHEAL"
);

//===============================================
//===============================================

DEFINE_SCHEDULE
(
	SCHED_METROPOLICE_CHARGE,

	"	Tasks"
	"		TASK_STOP_MOVING				0"
	"		TASK_SET_FAIL_SCHEDULE			SCHEDULE:SCHED_METROPOLICE_ADVANCE"
	"		TASK_STORE_LASTPOSITION			0"
	"		TASK_GET_CHASE_PATH_TO_ENEMY	300"
	"		TASK_RUN_PATH_FOR_UNITS			150"
	"		TASK_STOP_MOVING				1"
	"		TASK_FACE_ENEMY			0"
	""
	"	Interrupts"
	"		COND_NEW_ENEMY"
	"		COND_ENEMY_DEAD"
	"		COND_LOST_ENEMY"
	"		COND_CAN_MELEE_ATTACK1"
	"		COND_CAN_MELEE_ATTACK2"
	"		COND_HEAR_DANGER"
	"		COND_CP_PLAYERHEALREQUEST"
	"		COND_CP_COMMANDHEAL"
);

//=========================================================
//=========================================================
DEFINE_SCHEDULE
(
	SCHED_METROPOLICE_BURNING_RUN,

	"	Tasks"
	"		TASK_SET_FAIL_SCHEDULE			SCHEDULE:SCHED_METROPOLICE_BURNING_STAND"
	"		TASK_SET_TOLERANCE_DISTANCE		24"
	"		TASK_GET_PATH_TO_ENEMY			0"
	"		TASK_RUN_PATH_TIMED				10"	
	"		TASK_METROPOLICE_DIE_INSTANTLY	0"
	"	"
	"	Interrupts"
);

//=========================================================
//=========================================================
DEFINE_SCHEDULE
(
	SCHED_METROPOLICE_BURNING_STAND,

	"	Tasks"
	"		TASK_SET_ACTIVITY				ACTIVITY:ACT_IDLE_ON_FIRE"
	"		TASK_WAIT						1.5"
	"		TASK_METROPOLICE_DIE_INSTANTLY	DMG_BURN"
	"		TASK_WAIT						1.0"
	"	"
	"	Interrupts"
);

//===============================================
//===============================================
DEFINE_SCHEDULE
(
	SCHED_METROPOLICE_ALERT_FACE_BESTSOUND,

	"	Tasks"
	"		TASK_SPEAK_SENTENCE		7"	// METROPOLICE_SENTENCE_HEARD_SOMETHING
	"		TASK_SET_SCHEDULE		SCHEDULE:SCHED_ALERT_FACE_BESTSOUND"
	""
	"	Interrupts"
)

DEFINE_SCHEDULE
(
	SCHED_METROPOLICE_SHOVE,

	"	Tasks"
	"		TASK_STOP_MOVING				0"
	"		TASK_FACE_PLAYER				0.1"	//FIXME: This needs to be the target or enemy
	"		TASK_METROPOLICE_ACTIVATE_BATON	1"
	"		TASK_PLAY_SEQUENCE				ACTIVITY:ACT_PUSH_PLAYER"
	""
	"	Interrupts"
);

DEFINE_SCHEDULE
(
	SCHED_METROPOLICE_ACTIVATE_BATON,

	"	Tasks"
	"		TASK_STOP_MOVING				0"
	"		TASK_FACE_TARGET				0"
	"		TASK_METROPOLICE_ACTIVATE_BATON	1"
	""
	"	Interrupts"
);

DEFINE_SCHEDULE
(
	SCHED_METROPOLICE_DEACTIVATE_BATON,

	"	Tasks"
	"		TASK_STOP_MOVING				0"
	"		TASK_METROPOLICE_ACTIVATE_BATON	0"
	""
	"	Interrupts"
);

DEFINE_SCHEDULE
(
	SCHED_METROPOLICE_SMASH_PROP,

	"	Tasks"
	"		TASK_GET_PATH_TO_TARGET		0"
	"		TASK_MOVE_TO_TARGET_RANGE	50"
	"		TASK_STOP_MOVING			0"
	"		TASK_FACE_TARGET			0"
	"		TASK_ANNOUNCE_ATTACK		1"	// 1 = primary attack
	"		TASK_PLAY_SEQUENCE			ACTIVITY:ACT_MELEE_ATTACK1"
	""
	"	Interrupts"
	"		COND_NEW_ENEMY"
	"		COND_ENEMY_DEAD"
);

AI_END_CUSTOM_NPC()



void CNPC_MetroPolice::AddInsignia()
{
	CBaseEntity *pMark = CreateEntityByName( "squadinsignia" );
	pMark->SetOwnerEntity( this );
	pMark->Spawn();
}

void CNPC_MetroPolice::RemoveInsignia()
{
	// This is crap right now.
	CBaseEntity *FirstEnt();
	CBaseEntity *pEntity = gEntList.FirstEnt();

	while( pEntity )
	{
		if( pEntity->GetOwnerEntity() == this )
		{
			// Is this my insignia?
			CSquadInsignia *pInsignia = dynamic_cast<CSquadInsignia *>(pEntity);

			if( pInsignia )
			{
				UTIL_Remove( pInsignia );
				return;
			}
		}

		pEntity = gEntList.NextEnt( pEntity );
	}
}

//-----------------------------------------------------------------------------
LINK_ENTITY_TO_CLASS( squadinsignia, CSquadInsignia );

void CSquadInsignia::Spawn()
{
	CAI_BaseNPC *pOwner = ( GetOwnerEntity() ) ? GetOwnerEntity()->MyNPCPointer() : NULL;

	if ( pOwner )
	{
		int attachment = pOwner->LookupAttachment( "eyes" );
		if ( attachment )
		{
			SetAbsAngles( GetOwnerEntity()->GetAbsAngles() );
			SetParent( GetOwnerEntity(), attachment );

			Vector vecPosition;
			vecPosition.Init( -2.5, 0, 3.9 );
			SetLocalOrigin( vecPosition );
		}
	}

	#define INSIGNIA_MODEL "models/chefhat.mdl"
	SetModel( INSIGNIA_MODEL );
	SetSolid( SOLID_NONE );	
}



