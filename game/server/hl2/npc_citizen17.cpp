//========= Copyright � 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: The downtrodden citizens of City 17.
//
//=============================================================================//

#include "cbase.h"

#include "npc_citizen17.h"

#include "ammodef.h"
#include "globalstate.h"
#include "soundent.h"
#include "BasePropDoor.h"
#include "weapon_rpg.h"
#include "hl2_player.h"
#include "items.h"

#ifdef HL2MP
#include "hl2mp/weapon_crowbar.h"
#else
#include "weapon_crowbar.h"
#endif

#include "eventqueue.h"

#include "ai_squad.h"
#include "ai_pathfinder.h"
#include "ai_route.h"
#include "ai_hint.h"
#include "ai_interactions.h"
#include "ai_looktarget.h"
#include "sceneentity.h"
#include "tier0/icommandline.h"
#include "movevars_shared.h"

#include "weapon_molotov.h"
#include "grenade_molotov.h"
#include "Human_Error/grenade_smoke.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define INSIGNIA_MODEL "models/chefhat.mdl"

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

#define CIT_INSPECTED_DELAY_TIME 120  //How often I'm allowed to be inspected

extern ConVar sk_healthkit;
extern ConVar sk_healthvial;

const int MAX_PLAYER_SQUAD = 4;

ConVar	sk_citizen_health				( "sk_citizen_health",					"0");
ConVar	sk_citizen_heal_ally			( "sk_citizen_heal_ally",				"30");
ConVar	sk_citizen_heal_ally_delay		( "sk_citizen_heal_ally_delay",			"20");
ConVar	sk_citizen_heal_ally_min_pct	( "sk_citizen_heal_ally_min_pct",		"0.90");
ConVar	sk_citizen_stare_heal_time		( "sk_citizen_stare_heal_time",			"5" );
ConVar	g_ai_citizen_show_enemy( "g_ai_citizen_show_enemy", "0" );
ConVar	npc_citizen_dont_precache_all( "npc_citizen_dont_precache_all", "0" );

ConVar	sk_citizen_player_stare_time	( "sk_citizen_player_stare_time",		"6.0" );
ConVar  sk_citizen_player_stare_dist	( "sk_citizen_player_stare_dist",		"72" );

ConVar	sk_citizen_reaction_delay		( "sk_citizen_reaction_delay", "0.75");



ConVar  npc_citizen_medic_emit_sound("npc_citizen_medic_emit_sound", "1" );
#ifdef HL2_EPISODIC
// todo: bake these into pound constants (for now they're not just for tuning purposes)
ConVar  npc_citizen_heal_chuck_medkit("npc_citizen_heal_chuck_medkit" , "1" , FCVAR_ARCHIVE, "Set to 1 to use new experimental healthkit-throwing medic.");
ConVar npc_citizen_medic_throw_style( "npc_citizen_medic_throw_style", "1", FCVAR_ARCHIVE, "Set to 0 for a lobbier trajectory" );
ConVar npc_citizen_medic_throw_speed( "npc_citizen_medic_throw_speed", "650" );

ConVar	sk_citizen_heal_ally_min_forced( "sk_citizen_heal_player_min_forced",		"10.0");

ConVar	sk_citizen_heal_toss_ally_delay("sk_citizen_heal_toss_player_delay", "26", FCVAR_NONE, "how long between throwing healthkits" );

#define MEDIC_THROW_SPEED npc_citizen_medic_throw_speed.GetFloat()
#define USE_EXPERIMENTAL_MEDIC_CODE() (npc_citizen_heal_chuck_medkit.GetBool() && NameMatches("griggs"))
#endif

enum SquadSlot_T
{
	SQUAD_SLOT_CITIZEN_RPG1	= LAST_SHARED_SQUADSLOT,
	SQUAD_SLOT_CITIZEN_RPG2,
	SQUAD_SLOT_CITIZEN_MOLOTOV,
};

const float HEAL_MOVE_RANGE = 30*12;
const float HEAL_TARGET_RANGE = 120; // 10 feet
#ifdef HL2_EPISODIC
const float HEAL_TOSS_TARGET_RANGE = 480; // 40 feet when we are throwing medkits 
const float HEAL_TARGET_RANGE_Z = 72; // a second check that Gordon isn't too far above us -- 6 feet
#endif

// player must be at least this distance away from an enemy before we fire an RPG at him
const float RPG_SAFE_DISTANCE = CMissile::EXPLOSION_RADIUS + 64.0;

// Animation events
int AE_CITIZEN_GET_PACKAGE;
int AE_CITIZEN_HEAL;


//-----------------------------------------------------------------------------
// Citizen expressions for the citizen expression types
//-----------------------------------------------------------------------------
#define STATES_WITH_EXPRESSIONS		3		// Idle, Alert, Combat
#define EXPRESSIONS_PER_STATE		1

char *szExpressionTypes[CIT_EXP_LAST_TYPE] =
{
	"Unassigned",
	"Scared",
	"Normal",
	"Angry"
};

struct citizen_expression_list_t
{
	char *szExpressions[EXPRESSIONS_PER_STATE];
};
// Scared
citizen_expression_list_t ScaredExpressions[STATES_WITH_EXPRESSIONS] =
{
	{ "scenes/Expressions/citizen_scared_idle_01.vcd" },
	{ "scenes/Expressions/citizen_scared_alert_01.vcd" },
	{ "scenes/Expressions/citizen_scared_combat_01.vcd" },
};
// Normal
citizen_expression_list_t NormalExpressions[STATES_WITH_EXPRESSIONS] =
{
	{ "scenes/Expressions/citizen_normal_idle_01.vcd" },
	{ "scenes/Expressions/citizen_normal_alert_01.vcd" },
	{ "scenes/Expressions/citizen_normal_combat_01.vcd" },
};
// Angry
citizen_expression_list_t AngryExpressions[STATES_WITH_EXPRESSIONS] =
{
	{ "scenes/Expressions/citizen_angry_idle_01.vcd" },
	{ "scenes/Expressions/citizen_angry_alert_01.vcd" },
	{ "scenes/Expressions/citizen_angry_combat_01.vcd" },
};


class CMattsPipe : public CWeaponCrowbar
{
	DECLARE_CLASS( CMattsPipe, CWeaponCrowbar );

	const char *GetWorldModel() const	{ return "models/props_canal/mattpipe.mdl"; }
	void SetPickupTouch( void )	{	/* do nothing */ }
};

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

//---------------------------------------------------------
// Citizen models
//---------------------------------------------------------

static const char *g_ppszRandomHeads[] = 
{
	"male_01.mdl",
	"male_02.mdl",
	"female_01.mdl",
	"male_03.mdl",
	"female_02.mdl",
	"male_04.mdl",
	"female_03.mdl",
	"male_05.mdl",
	"female_04.mdl",
	"male_06.mdl",
	"female_06.mdl",
	"male_07.mdl",
	"female_07.mdl",
	"male_08.mdl",
	"male_09.mdl",
};

static const char *g_ppszModelLocs[] =
{
	"Group01",
	"Group01",
	"Group02",
	"Group03%s",
};

#define IsExcludedHead( type, bMedic, iHead) false // see XBox codeline for an implementation


//---------------------------------------------------------
// Citizen activities
//---------------------------------------------------------

int ACT_CIT_HANDSUP;
int	ACT_CIT_BLINDED;		// Blinded by scanner photo
int ACT_CIT_SHOWARMBAND;
int ACT_CIT_HEAL;
int	ACT_CIT_STARTLED;		// Startled by sneaky scanner

//---------------------------------------------------------

LINK_ENTITY_TO_CLASS( npc_citizen, CNPC_Citizen );

//---------------------------------------------------------

BEGIN_DATADESC( CNPC_Citizen )

	DEFINE_CUSTOM_FIELD( m_nInspectActivity,		ActivityDataOps() ),
	DEFINE_FIELD( 		m_flNextFearSoundTime, 		FIELD_TIME ),
	DEFINE_FIELD( 		m_flStopManhackFlinch, 		FIELD_TIME ),
	DEFINE_FIELD( 		m_fNextInspectTime, 		FIELD_TIME ),
	DEFINE_FIELD(		m_flNextHealthSearchTime,	FIELD_TIME ),
	DEFINE_FIELD( 		m_flAllyHealTime, 			FIELD_TIME ),
//						gm_PlayerSquadEvaluateTimer
//						m_AssaultBehavior
//						m_FollowBehavior
//						m_StandoffBehavior
//						m_LeadBehavior
//						m_FuncTankBehavior
	DEFINE_KEYFIELD(	m_Type, 					FIELD_INTEGER,			"citizentype" ),
	DEFINE_KEYFIELD(	m_ExpressionType,			FIELD_INTEGER,			"expressiontype" ),
	DEFINE_FIELD(		m_iHead,					FIELD_INTEGER ),
	DEFINE_FIELD(		m_flTimePlayerStare,		FIELD_TIME ),
	DEFINE_FIELD(		m_flTimeNextHealStare,		FIELD_TIME ),
	DEFINE_FIELD( 		m_hSavedFollowGoalEnt,		FIELD_EHANDLE ),
	//DEFINE_FIELD(		m_bIsInVan,					FIELD_BOOLEAN ),

	DEFINE_FIELD(		m_hSmokeGrenade,			FIELD_EHANDLE ),

	DEFINE_FIELD(		m_bParentedToTruck,			FIELD_BOOLEAN ),

#ifdef CITIZEN_MAKE_BLIND_IN_SMOKE
	DEFINE_FIELD(		m_bIsBlind,					FIELD_BOOLEAN ),
#endif

	DEFINE_FIELD( m_flStopMoveShootTime,			FIELD_TIME ),

	DEFINE_KEYFIELD( m_iNumberMolotovCocktails,		FIELD_INTEGER,			"molotovcocktails" ),
	DEFINE_FIELD( m_flNextMolotovCocktail,			FIELD_TIME ),
//	DEFINE_FIELD( m_flTimeHasHadMolotov,			FIELD_TIME ),
	DEFINE_FIELD( m_vecTossVelocity,				FIELD_VECTOR ),

	DEFINE_OUTPUT(		m_OnPlayerUse,				"OnPlayerUse" ),
	DEFINE_OUTPUT(		m_OnNavFailBlocked,			"OnNavFailBlocked" ),

	DEFINE_INPUTFUNC( FIELD_VOID,					"StartPatrolling",		InputStartPatrolling ),
	DEFINE_INPUTFUNC( FIELD_VOID,					"StopPatrolling",		InputStopPatrolling ),
	DEFINE_INPUTFUNC( FIELD_VOID,					"SetMedicOn",			InputSetMedicOn ),
	DEFINE_INPUTFUNC( FIELD_VOID,					"SetMedicOff",			InputSetMedicOff ),
	DEFINE_INPUTFUNC( FIELD_VOID,					"SpeakIdleResponse",	InputSpeakIdleResponse ),

	DEFINE_INPUTFUNC( FIELD_VOID,					"ReturnNormalMovement", InputReturnNormalMovement ),

#if HL2_EPISODIC
	DEFINE_INPUTFUNC( FIELD_VOID,   "				ThrowHealthKit",		InputForceHealthKitToss ),
#endif
	DEFINE_USEFUNC( SimpleUse ),

END_DATADESC()



//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

bool CNPC_Citizen::CreateBehaviors()
{
	BaseClass::CreateBehaviors();
	AddBehavior( &m_FuncTankBehavior );
	
	return true;
}

extern ConVar ai_reaction_delay_idle;

float CNPC_Citizen::GetReactionDelay( CBaseEntity *pEnemy )
{
	return ( m_NPCState == NPC_STATE_ALERT || m_NPCState == NPC_STATE_COMBAT ) ? 
				sk_citizen_reaction_delay.GetFloat() : 
				ai_reaction_delay_idle.GetFloat();

}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CNPC_Citizen::Precache()
{
	SelectModel();
	SelectExpressionType();

	if ( !npc_citizen_dont_precache_all.GetBool() )
		PrecacheAllOfType( m_Type );
	else
		PrecacheModel( STRING( GetModelName() ) );

	if ( NameMatches( "matt" ) )
		PrecacheModel( "models/props_canal/mattpipe.mdl" );

	PrecacheModel( INSIGNIA_MODEL );

	PrecacheScriptSound( "NPC_Citizen.FootstepLeft" );
	PrecacheScriptSound( "NPC_Citizen.FootstepRight" );
	PrecacheScriptSound( "NPC_Citizen.Die" );

	PrecacheScriptSound( "NPC_Citizen.cough" );

	PrecacheInstancedScene( "scenes/Expressions/CitizenIdle.vcd" );
	PrecacheInstancedScene( "scenes/Expressions/CitizenAlert_loop.vcd" );
	PrecacheInstancedScene( "scenes/Expressions/CitizenCombat_loop.vcd" );

	for ( int i = 0; i < STATES_WITH_EXPRESSIONS; i++ )
	{
		for ( int j = 0; j < ARRAYSIZE(ScaredExpressions[i].szExpressions); j++ )
		{
			PrecacheInstancedScene( ScaredExpressions[i].szExpressions[j] );
		}
		for ( int j = 0; j < ARRAYSIZE(NormalExpressions[i].szExpressions); j++ )
		{
			PrecacheInstancedScene( NormalExpressions[i].szExpressions[j] );
		}
		for ( int j = 0; j < ARRAYSIZE(AngryExpressions[i].szExpressions); j++ )
		{
			PrecacheInstancedScene( AngryExpressions[i].szExpressions[j] );
		}
	}

	BaseClass::Precache();
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CNPC_Citizen::PrecacheAllOfType( CitizenType_t type )
{
	if ( m_Type == CT_UNIQUE )
		return;

	int nHeads = ARRAYSIZE( g_ppszRandomHeads );
	int i;
	for ( i = 0; i < nHeads; ++i )
	{
		if ( !IsExcludedHead( type, false, i ) )
		{
			PrecacheModel( CFmtStr( "models/Humans/%s/%s", (const char *)(CFmtStr(g_ppszModelLocs[m_Type], "")), g_ppszRandomHeads[i] ) );
		}
	}

	if ( m_Type == CT_REBEL )
	{
		for ( i = 0; i < nHeads; ++i )
		{
			if ( !IsExcludedHead( type, true, i ) )
			{
				PrecacheModel( CFmtStr( "models/Humans/%s/%s", (const char *)(CFmtStr(g_ppszModelLocs[m_Type], "m")), g_ppszRandomHeads[i] ) );
			}
		}
	}
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CNPC_Citizen::Spawn()
{
	BaseClass::Spawn();

#ifdef _XBOX
	// Always fade the corpse
	AddSpawnFlags( SF_NPC_FADE_CORPSE );
#endif // _XBOX

	m_bShouldPatrol = false;
	m_iHealth = sk_citizen_health.GetFloat();

	//m_bIsInVan = false;

	CapabilitiesRemove( bits_CAP_NO_HIT_PLAYER );
	CapabilitiesAdd( bits_CAP_INNATE_RANGE_ATTACK2 );

	m_bParentedToTruck = false;
	
	// Are we on a train? Used in trainstation to have NPCs on trains.
	if ( Classify() == CLASS_CITIZEN_PASSIVE && GetMoveParent() && FClassnameIs( GetMoveParent(), "func_tracktrain" ) )
	{
		CapabilitiesRemove( bits_CAP_MOVE_GROUND );
		SetMoveType( MOVETYPE_NONE );
		if ( NameMatches("citizen_train_2") )
		{
			SetSequenceByName( "d1_t01_TrainRide_Sit_Idle" );
			SetIdealActivity( ACT_DO_NOT_DISTURB );
		}
		else
		{
			SetSequenceByName( "d1_t01_TrainRide_Stand" );
			SetIdealActivity( ACT_DO_NOT_DISTURB );
		}
	} else if ( GetMoveParent() && FClassnameIs( GetMoveParent(), "func_tracktrain" ))
	{
		CapabilitiesRemove( bits_CAP_MOVE_GROUND );
		CapabilitiesAdd( bits_CAP_SKIP_NAV_GROUND_CHECK );
		SetMoveType( MOVETYPE_NONE );
		GetMotor()->SetYawLocked( true );

		AddFlag( FL_FLY );

		m_NPCState = NPC_STATE_ALERT;

		m_bParentedToTruck = true;

		DevMsg("In a truck\n" );
	}

	m_flStopManhackFlinch = -1;

	m_iszIdleExpression = MAKE_STRING("scenes/expressions/citizenidle.vcd");
	m_iszAlertExpression = MAKE_STRING("scenes/expressions/citizenalert_loop.vcd");
	m_iszCombatExpression = MAKE_STRING("scenes/expressions/citizencombat_loop.vcd");

	m_flNextHealthSearchTime = gpGlobals->curtime;

	CWeaponRPG *pRPG = dynamic_cast<CWeaponRPG*>(GetActiveWeapon());
	if ( pRPG )
	{
		CapabilitiesRemove( bits_CAP_USE_SHOT_REGULATOR );
		pRPG->StopGuiding();
	}

	m_flTimePlayerStare = FLT_MAX;

	//AddEFlags( EFL_NO_DISSOLVE | EFL_NO_MEGAPHYSCANNON_RAGDOLL | EFL_NO_PHYSCANNON_INTERACTION );

	NPCInit();

	ClearCondition(COND_CIT_CAN_HAVE_MOLOTOV);

	//DevMsg("m_iNumberMolotovCocktails %d\n", m_iNumberMolotovCocktails);

	// Use render bounds instead of human hull for guys sitting in chairs, etc.
	m_ActBusyBehavior.SetUseRenderBounds( HasSpawnFlags( SF_CITIZEN_USE_RENDER_BOUNDS ) );

	m_flNextMolotovCocktail = 0.0f;
//	m_flTimeHasHadMolotov   = 0.0f;

	//TERO: copied from npc_combine.cpp
	m_flStopMoveShootTime = FLT_MAX;
	m_MoveAndShootOverlay.SetInitialDelay( 0.75f ); //sk_citizen_reaction_delay.GetFloat()

#ifdef CITIZEN_MAKE_BLIND_IN_SMOKE
	m_bIsBlind			= false;
#endif
	m_hSmokeGrenade			= NULL;
}

/*void CNPC_Citizen::CalculateIKLocks( float currentTime ) 
{
	if (m_bIsInVan)
		return;

	BaseClass::CalculateIKLocks( currentTime );
}

void CNPC_Citizen::UpdateStepOrigin()
{
	if (m_bIsInVan)
		return;

	BaseClass::UpdateStepOrigin();
}*/

void CNPC_Citizen::InputReturnNormalMovement(inputdata_t &inputdata)
{
	if ( GetMoveType() == MOVETYPE_NONE )
	{
		SetParent( NULL );
		SetGroundEntity( NULL );

		CapabilitiesAdd( bits_CAP_MOVE_GROUND );
		CapabilitiesRemove( bits_CAP_SKIP_NAV_GROUND_CHECK );
		SetMoveType( MOVETYPE_STEP );
		GetMotor()->SetYawLocked( false );

		RemoveFlag(FL_FLY);

		m_bParentedToTruck = false;

		SetAbsOrigin( GetAbsOrigin() + Vector(0,0,1) );

	}

	//m_NPCState = NPC_STATE_ALERT;
}

/*bool CNPC_Citizen::OverrideMoveFacing( const AILocalMoveGoal_t &move, float flInterval )
{
	if (m_bParentedToTruck) // && !IsCurSchedule( SCHED_COMBAT_FACE ) )
		return true;

	return OverrideMoveFacing( AILocalMoveGoal_t &move, flInterval )
}*/

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CNPC_Citizen::PostNPCInit()
{
	BaseClass::PostNPCInit();
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
struct HeadCandidate_t
{
	int iHead;
	int nHeads;

	static int __cdecl Sort( const HeadCandidate_t *pLeft, const HeadCandidate_t *pRight )
	{
		return ( pLeft->nHeads - pRight->nHeads );
	}
};

void CNPC_Citizen::SelectModel()
{
	// If making reslists, precache everything!!!
	static bool madereslists = false;

	if ( CommandLine()->CheckParm("-makereslists") && !madereslists )
	{
		madereslists = true;

		PrecacheAllOfType( CT_DOWNTRODDEN );
		PrecacheAllOfType( CT_REFUGEE );
		PrecacheAllOfType( CT_REBEL );
	}

	const char *pszModelName = NULL;

	if ( m_Type == CT_DEFAULT )
	{
		struct CitizenTypeMapping
		{
			const char *pszMapTag;
			CitizenType_t type;
		};

		static CitizenTypeMapping CitizenTypeMappings[] = 
		{
			{ "trainstation",	CT_DOWNTRODDEN	},
			{ "canals",			CT_REFUGEE		},
			{ "town",			CT_REFUGEE		},
			{ "coast",			CT_REFUGEE		},
			{ "prison",			CT_DOWNTRODDEN	},
			{ "c17",			CT_REBEL		},
			{ "citadel",		CT_DOWNTRODDEN	},
		};

		char szMapName[256];
		Q_strncpy(szMapName, STRING(gpGlobals->mapname), sizeof(szMapName) );
		Q_strlower(szMapName);

		for ( int i = 0; i < ARRAYSIZE(CitizenTypeMappings); i++ )
		{
			if ( Q_stristr( szMapName, CitizenTypeMappings[i].pszMapTag ) )
			{
				m_Type = CitizenTypeMappings[i].type;
				break;
			}
		}

		if ( m_Type == CT_DEFAULT )
			m_Type = CT_DOWNTRODDEN;
	}

	if( HasSpawnFlags( SF_CITIZEN_RANDOM_HEAD | SF_CITIZEN_RANDOM_HEAD_MALE | SF_CITIZEN_RANDOM_HEAD_FEMALE ) || GetModelName() == NULL_STRING )
	{
		Assert( m_iHead == -1 );
		char gender = ( HasSpawnFlags( SF_CITIZEN_RANDOM_HEAD_MALE ) ) ? 'm' : 
					  ( HasSpawnFlags( SF_CITIZEN_RANDOM_HEAD_FEMALE ) ) ? 'f' : 0;

		RemoveSpawnFlags( SF_CITIZEN_RANDOM_HEAD | SF_CITIZEN_RANDOM_HEAD_MALE | SF_CITIZEN_RANDOM_HEAD_FEMALE );
		if( HasSpawnFlags( SF_NPC_START_EFFICIENT ) )
		{
			SetModelName( AllocPooledString("models/humans/male_cheaple.mdl" ) );
			return;
		}
		else
		{
			// Count the heads
			int headCounts[ARRAYSIZE(g_ppszRandomHeads)] = { 0 };
			int i;

			for ( i = 0; i < g_AI_Manager.NumAIs(); i++ )
			{
				CNPC_Citizen *pCitizen = dynamic_cast<CNPC_Citizen *>(g_AI_Manager.AccessAIs()[i]);
				if ( pCitizen && pCitizen != this && pCitizen->m_iHead >= 0 && pCitizen->m_iHead < ARRAYSIZE(g_ppszRandomHeads) )
				{
					headCounts[pCitizen->m_iHead]++;
				}
			}

			// Find all candidates
			CUtlVectorFixed<HeadCandidate_t, ARRAYSIZE(g_ppszRandomHeads)> candidates;

			for ( i = 0; i < ARRAYSIZE(g_ppszRandomHeads); i++ )
			{
				if ( !gender || g_ppszRandomHeads[i][0] == gender )
				{
					if ( !IsExcludedHead( m_Type, IsMedic(), i ) )
					{
						HeadCandidate_t candidate = { i, headCounts[i] };
						candidates.AddToTail( candidate );
					}
				}
			}

			Assert( candidates.Count() );
			candidates.Sort( &HeadCandidate_t::Sort );

			int iSmallestCount = candidates[0].nHeads;
			int iLimit;

			for ( iLimit = 0; iLimit < candidates.Count(); iLimit++ )
			{
				if ( candidates[iLimit].nHeads > iSmallestCount )
					break;
			}

			m_iHead = candidates[random->RandomInt( 0, iLimit - 1 )].iHead;
			pszModelName = g_ppszRandomHeads[m_iHead];
			SetModelName(NULL_STRING);
		}
	}

	Assert( pszModelName || GetModelName() != NULL_STRING );

	if ( !pszModelName )
	{
		if ( GetModelName() == NULL_STRING )
			return;
		pszModelName = strrchr(STRING(GetModelName()), '/' );
		if ( !pszModelName )
			pszModelName = STRING(GetModelName());
		else
		{
			pszModelName++;
			if ( m_iHead == -1 )
			{
				for ( int i = 0; i < ARRAYSIZE(g_ppszRandomHeads); i++ )
				{
					if ( Q_stricmp( g_ppszRandomHeads[i], pszModelName ) == 0 )
					{
						m_iHead = i;
						break;
					}
				}
			}
		}
		if ( !*pszModelName )
			return;
	}

	// Unique citizen models are left alone
	if ( m_Type != CT_UNIQUE )
	{
		SetModelName( AllocPooledString( CFmtStr( "models/Humans/%s/%s", (const char *)(CFmtStr(g_ppszModelLocs[ m_Type ], ( IsMedic() ) ? "m" : "" )), pszModelName ) ) );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_Citizen::SelectExpressionType()
{
	// If we've got a mapmaker assigned type, leave it alone
	if ( m_ExpressionType != CIT_EXP_UNASSIGNED )
		return;

	switch ( m_Type )
	{
	case CT_DOWNTRODDEN:
		m_ExpressionType = (CitizenExpressionTypes_t)RandomInt( CIT_EXP_SCARED, CIT_EXP_NORMAL );
		break;
	case CT_REFUGEE:
		m_ExpressionType = (CitizenExpressionTypes_t)RandomInt( CIT_EXP_SCARED, CIT_EXP_NORMAL );
		break;
	case CT_REBEL:
		m_ExpressionType = (CitizenExpressionTypes_t)RandomInt( CIT_EXP_SCARED, CIT_EXP_ANGRY );
		break;

	case CT_DEFAULT:
	case CT_UNIQUE:
	default:
		break;
	}
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CNPC_Citizen::FixupMattWeapon()
{
	CBaseCombatWeapon *pWeapon = GetActiveWeapon();
	if ( pWeapon && pWeapon->ClassMatches( "weapon_crowbar" ) && NameMatches( "matt" ) )
	{
		Weapon_Drop( pWeapon );
		UTIL_Remove( pWeapon );
		pWeapon = (CBaseCombatWeapon *)CREATE_UNSAVED_ENTITY( CMattsPipe, "weapon_crowbar" );
		pWeapon->SetName( AllocPooledString( "matt_weapon" ) );
		DispatchSpawn( pWeapon );

#ifdef DEBUG
		extern bool g_bReceivedChainedActivate;
		g_bReceivedChainedActivate = false;
#endif
		pWeapon->Activate();
		Weapon_Equip( pWeapon );
	}
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

void CNPC_Citizen::Activate()
{
	BaseClass::Activate();
	FixupMattWeapon();
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CNPC_Citizen::OnRestore()
{
	BaseClass::OnRestore();
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
string_t CNPC_Citizen::GetModelName() const
{
	string_t iszModelName = BaseClass::GetModelName();

	//
	// If the model refers to an obsolete model, pretend it was blank
	// so that we pick the new default model.
	//
	if (!Q_strnicmp(STRING(iszModelName), "models/c17_", 11) ||
		!Q_strnicmp(STRING(iszModelName), "models/male", 11) ||
		!Q_strnicmp(STRING(iszModelName), "models/female", 13) ||
		!Q_strnicmp(STRING(iszModelName), "models/citizen", 14))
	{
		return NULL_STRING;
	}

	return iszModelName;
}

//-----------------------------------------------------------------------------
// Purpose: Overridden to switch our behavior between passive and rebel. We
//			become combative after Gordon becomes a criminal.
//-----------------------------------------------------------------------------
Class_T	CNPC_Citizen::Classify()
{
	if (GlobalEntity_GetState("gordon_precriminal") == GLOBAL_ON)
		return CLASS_CITIZEN_PASSIVE;

	if (GlobalEntity_GetState("citizens_passive") == GLOBAL_ON)
		return CLASS_CITIZEN_PASSIVE;

	return CLASS_CITIZEN_REBEL;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool CNPC_Citizen::ShouldAlwaysThink() 
{ 
	return ( BaseClass::ShouldAlwaysThink() || IsInPlayerSquad() ); 
}
	
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
#define CITIZEN_FOLLOWER_DESERT_FUNCTANK_DIST	45.0f*12.0f
bool CNPC_Citizen::ShouldBehaviorSelectSchedule( CAI_BehaviorBase *pBehavior )
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
			if( vecFollowGoal.DistToSqr( GetAbsOrigin() ) > Square(CITIZEN_FOLLOWER_DESERT_FUNCTANK_DIST) )
			{
				return false;
			}
		}
	}

	return BaseClass::ShouldBehaviorSelectSchedule( pBehavior );
}

void CNPC_Citizen::OnChangeRunningBehavior( CAI_BehaviorBase *pOldBehavior,  CAI_BehaviorBase *pNewBehavior )
{
	if ( pNewBehavior == &m_FuncTankBehavior )
	{
		m_bReadinessCapable = false;
	}
	else if ( pOldBehavior == &m_FuncTankBehavior )
	{
		m_bReadinessCapable = IsReadinessCapable();
	}

	BaseClass::OnChangeRunningBehavior( pOldBehavior, pNewBehavior );
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CNPC_Citizen::GatherConditions()
{
	BaseClass::GatherConditions();

	/*
	if( ShouldLookForHealthItem() )
	{
		if( FindHealthItem( GetAbsOrigin(), Vector( 240, 240, 240 ) ) )
			SetCondition( COND_HEALTH_ITEM_AVAILABLE );
		else
			ClearCondition( COND_HEALTH_ITEM_AVAILABLE );

		m_flNextHealthSearchTime = gpGlobals->curtime + 4.0;
	}*/

	MolotovThrowCondition();

	// If the player is standing near a medic and can see the medic, 
	// assume the player is 'staring' and wants health.
	if( Classify() == CLASS_CITIZEN_PASSIVE )
	{
		CBasePlayer *pPlayer = AI_GetSinglePlayer();

		if ( !pPlayer )
		{
			m_flTimePlayerStare = FLT_MAX;
			return;
		}

		float flDistSqr = ( GetAbsOrigin() - pPlayer->GetAbsOrigin() ).Length2DSqr();
		float flStareDist = sk_citizen_player_stare_dist.GetFloat();

		if( pPlayer->IsAlive() && (flDistSqr <= flStareDist * flStareDist) && pPlayer->FInViewCone( this ) && pPlayer->FVisible( this ) )
		{
			if( m_flTimePlayerStare == FLT_MAX )
			{
				// Player wasn't looking at me at last think. He started staring now.
				m_flTimePlayerStare = gpGlobals->curtime;
			}

			// Heal if it's been long enough since last time I healed a staring player.
			if( gpGlobals->curtime - m_flTimePlayerStare >= sk_citizen_player_stare_time.GetFloat() && gpGlobals->curtime > m_flTimeNextHealStare )
			{
				//TERO: lets do something here. The player is a Metropolice so the citizen might say something like "please don't beat me"
				m_flTimePlayerStare = gpGlobals->curtime + sk_citizen_player_stare_time.GetFloat() * .5f;
				Speak( TLK_STARE );
			}
		}
		else
		{
			m_flTimePlayerStare = FLT_MAX;
		}
	}
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CNPC_Citizen::PredictPlayerPush()
{
	//we don't want them to act nice on us if they try to kill us
	if (Classify() != CLASS_CITIZEN_PASSIVE)
		return;

	CBasePlayer *pPlayer = AI_GetSinglePlayer();
	if ( !pPlayer )
		return;

	BaseClass::PredictPlayerPush();
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CNPC_Citizen::PrescheduleThink()
{
	BaseClass::PrescheduleThink();

	if( GetEnemy() && g_ai_citizen_show_enemy.GetBool() )
	{
		NDebugOverlay::Line( EyePosition(), GetEnemy()->EyePosition(), 255, 0, 0, false, .1 );
	}

	//TERO: copied from Combine Soldiers
	if( gpGlobals->curtime >= m_flStopMoveShootTime )
	{
		// Time to stop move and shoot and start facing the way I'm running.
		// This makes the combine look attentive when disengaging, but prevents
		// them from always running around facing you.
		//
		// Only do this if it won't be immediately shut off again.
		if( GetNavigator()->GetPathTimeToGoal() > 1.0f )
		{
			m_MoveAndShootOverlay.SuspendMoveAndShoot( 5.0f );
			m_flStopMoveShootTime = FLT_MAX;
		}
	}

	if( m_flGroundSpeed > 0 && GetState() == NPC_STATE_COMBAT && m_MoveAndShootOverlay.IsSuspended() )
	{
		// Return to move and shoot when near my goal so that I 'tuck into' the location facing my enemy.
		if( GetNavigator()->GetPathTimeToGoal() <= 1.0f )
		{
			m_MoveAndShootOverlay.SuspendMoveAndShoot( 0 );
		}
	}
}

//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
bool CNPC_Citizen::ShouldMoveAndShoot()
{
	// Set this timer so that gpGlobals->curtime can't catch up to it. 
	// Essentially, we're saying that we're not going to interfere with 
	// what the AI wants to do with move and shoot. 
	//
	// If any code below changes this timer, the code is saying 
	// "It's OK to move and shoot until gpGlobals->curtime == m_flStopMoveShootTime"
	m_flStopMoveShootTime = FLT_MAX;

	if( IsCurSchedule( SCHED_HIDE_AND_RELOAD, false ) )
		m_flStopMoveShootTime = gpGlobals->curtime + random->RandomFloat( 0.4f, 0.6f );

	if( IsCurSchedule( SCHED_TAKE_COVER_FROM_BEST_SOUND, false ) )
		return false;

	/*if( IsCurSchedule( SCHED_COMBINE_TAKE_COVER_FROM_BEST_SOUND, false ) )
		return false;

	if( IsCurSchedule( SCHED_COMBINE_RUN_AWAY_FROM_BEST_SOUND, false ) )
		return false;*/

	if ( IsCurSchedule( SCHED_TAKE_COVER_FROM_ORIGIN, false ) ) 
		return false;

	if( HasCondition( COND_NO_PRIMARY_AMMO, false ) )
		m_flStopMoveShootTime = gpGlobals->curtime + random->RandomFloat( 0.4f, 0.6f );

	if( m_pSquad && IsCurSchedule( SCHED_TAKE_COVER_FROM_ENEMY, false ) )
		m_flStopMoveShootTime = gpGlobals->curtime + random->RandomFloat( 0.4f, 0.6f );

	return BaseClass::ShouldMoveAndShoot();
}


//-----------------------------------------------------------------------------
// Purpose: Allows for modification of the interrupt mask for the current schedule.
//			In the most cases the base implementation should be called first.
//-----------------------------------------------------------------------------
void CNPC_Citizen::BuildScheduleTestBits()
{
	BaseClass::BuildScheduleTestBits();

	if ( IsCurSchedule( SCHED_IDLE_STAND ) || IsCurSchedule( SCHED_ALERT_STAND ) )
	{
		SetCustomInterruptCondition( COND_CIT_START_INSPECTION );
	}

	if ( IsMedic() && IsCustomInterruptConditionSet( COND_HEAR_MOVE_AWAY ) )
	{
		if( !IsCurSchedule(SCHED_RELOAD, false) )
		{
			// Since schedule selection code prioritizes reloading over requests to heal
			// the player, we must prevent this condition from breaking the reload schedule.
			SetCustomInterruptCondition( COND_CIT_PLAYERHEALREQUEST );
		}

		SetCustomInterruptCondition( COND_CIT_COMMANDHEAL );
	}

	if( !IsCurSchedule( SCHED_NEW_WEAPON ) )
	{
		SetCustomInterruptCondition( COND_RECEIVED_ORDERS );
	}

	if( GetCurSchedule()->HasInterrupt( COND_IDLE_INTERRUPT ) )
	{
		SetCustomInterruptCondition( COND_BETTER_WEAPON_AVAILABLE );
	}

#ifdef HL2_EPISODIC
	if( IsMedic() && m_AssaultBehavior.IsRunning() )
	{
		if( !IsCurSchedule(SCHED_RELOAD, false) )
		{
			SetCustomInterruptCondition( COND_CIT_PLAYERHEALREQUEST );
		}

		SetCustomInterruptCondition( COND_CIT_COMMANDHEAL );
	}
#else
	if( IsMedic() && m_AssaultBehavior.IsRunning() && !IsMoving() )
	{
		if( !IsCurSchedule(SCHED_RELOAD, false) )
		{
			SetCustomInterruptCondition( COND_CIT_PLAYERHEALREQUEST );
		}

		SetCustomInterruptCondition( COND_CIT_COMMANDHEAL );
	}
#endif
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
/*bool CNPC_Citizen::FInViewCone( CBaseEntity *pEntity )
{
#if 0
	if ( IsMortar( pEntity ) )
	{
		// @TODO (toml 11-20-03): do this only if have heard mortar shell recently and it's active
		return true;
	}
#endif
	return BaseClass::FInViewCone( pEntity );
}*/

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
int CNPC_Citizen::SelectFailSchedule( int failedSchedule, int failedTask, AI_TaskFailureCode_t taskFailCode )
{
	switch( failedSchedule )
	{
	case SCHED_NEW_WEAPON:
		// If failed trying to pick up a weapon, try again in one second. This is because other AI code
		// has put this off for 10 seconds under the assumption that the citizen would be able to 
		// pick up the weapon that they found. 
		m_flNextWeaponSearchTime = gpGlobals->curtime + 1.0f;
		break;

	case SCHED_ESTABLISH_LINE_OF_FIRE_FALLBACK:
	case SCHED_MOVE_TO_WEAPON_RANGE:
		if( !IsMortar( GetEnemy() ) )
		{
			if ( GetActiveWeapon() && ( GetActiveWeapon()->CapabilitiesGet() & bits_CAP_WEAPON_RANGE_ATTACK1 ) && random->RandomInt( 0, 1 ) && HasCondition(COND_SEE_ENEMY) && !HasCondition ( COND_NO_PRIMARY_AMMO ) )
				return TranslateSchedule( SCHED_RANGE_ATTACK1 );

			return SCHED_STANDOFF;
		}
		break;
	}

	return BaseClass::SelectFailSchedule( failedSchedule, failedTask, taskFailCode );
}

void CNPC_Citizen::RunAwayFromSmoke( CBaseEntity *pSmokeGrenade )
{
	m_hSmokeGrenade = pSmokeGrenade;

	//TERO: if we are not already suspended, then do so, plz
	if (!m_MoveAndShootOverlay.IsSuspended())
		m_MoveAndShootOverlay.SuspendMoveAndShoot( 1.5f );

//#ifdef CITIZEN_MAKE_BLIND_IN_SMOKE
//	m_bIsBlind	= true;
//#endif
}

void CNPC_Citizen::PainSound( const CTakeDamageInfo &info )
{
	if (m_hSmokeGrenade)
	{
		if (UTIL_DistApprox( GetAbsOrigin(), m_hSmokeGrenade->GetAbsOrigin() ) < smoke_grenade_radius.GetFloat())
		{
			EmitSound("npc_citizen.cough");
			return;
		}
	}

	SpeakIfAllowed( TLK_WOUND );
}


bool CNPC_Citizen::ShouldRunAwayFromSmoke( void )
{
	if (m_hSmokeGrenade)
	{
		if (UTIL_DistApprox( GetAbsOrigin(), m_hSmokeGrenade->GetAbsOrigin() ) < smoke_grenade_radius.GetFloat())
		{

#ifdef CITIZEN_MAKE_BLIND_IN_SMOKE
			if (!m_bIsBlind)
			{
				MakeBlind(true);
			}
#endif

			return true;
		}
	}

#ifdef CITIZEN_MAKE_BLIND_IN_SMOKE
	if (m_bIsBlind)
	{
		MakeBlind(false);
	}
#endif

	return false;
}

#ifdef CITIZEN_MAKE_BLIND_IN_SMOKE
void CNPC_Citizen::MakeBlind(bool bBlind)
{
	if (bBlind)
	{
		SetDistLook( 64.0 );
		m_bIsBlind = true;
	}
	else
	{
		m_bIsBlind = false;

		if ( HasSpawnFlags( SF_NPC_LONG_RANGE ) )
		{
			m_flDistTooFar	= 1e9f;
			SetDistLook( 6000.0 );
		}
		else
		{
			SetDistLook( 2048.0 );
		}
	}
}
#endif

int CNPC_Citizen::SelectIceCreamTruckSchedule()
{
	//TERO: hmm, this might create so problems!
	//if ( m_hForcedInteractionPartner )
	//	return SelectInteractionSchedule();

	int nSched = SelectFlinchSchedule();
	if ( nSched != SCHED_NONE )
		return nSched;

	if ( !GetEnemy())
	{
		//TERO: this is copied from SelectAlertSchedule
		// Scan around for new enemies
		if ( HasCondition( COND_ENEMY_DEAD ) && SelectWeightedSequence( ACT_VICTORY_DANCE ) != ACTIVITY_NOT_AVAILABLE )
			return SCHED_ALERT_SCAN;

		if ( gpGlobals->curtime - GetEnemies()->LastTimeSeen( AI_UNKNOWN_ENEMY ) < TIME_CARE_ABOUT_DAMAGE )
			return SCHED_ALERT_FACE;

		return SCHED_ALERT_STAND;
	}

	if ( HasCondition(COND_NEW_ENEMY) && gpGlobals->curtime - GetEnemies()->FirstTimeSeen(GetEnemy()) < 2.0 )
	{
		return SCHED_WAKE_ANGRY;
	}
	
	if ( HasCondition( COND_ENEMY_DEAD ) )
	{
		// clear the current (dead) enemy and try to find another.
		SetEnemy( NULL );
		 
		if ( ChooseEnemy() )
		{
			ClearCondition( COND_ENEMY_DEAD );
			return SelectSchedule();
		}

		SetState( NPC_STATE_ALERT );
		return SelectSchedule();
	}

	// Check if need to reload
	if ( HasCondition( COND_LOW_PRIMARY_AMMO ) || HasCondition( COND_NO_PRIMARY_AMMO ) )
	{
		return SCHED_RELOAD;
	}

	//ChangeToMolotov();

	if ( GetShotRegulator()->IsInRestInterval() )
	{
		if ( HasCondition(COND_CAN_RANGE_ATTACK1) )
			return SCHED_COMBAT_FACE;
	}

	// we can see the enemy
	if ( HasCondition(COND_CAN_RANGE_ATTACK1) )
	{
		if ( !UseAttackSquadSlots() || OccupyStrategySlotRange( SQUAD_SLOT_ATTACK1, SQUAD_SLOT_ATTACK2 ) )
			return SCHED_RANGE_ATTACK1;
		return SCHED_COMBAT_FACE;
	}

	if ( HasCondition(COND_CAN_RANGE_ATTACK2) && OccupyStrategySlot( SQUAD_SLOT_CITIZEN_MOLOTOV ))
	{
		return SCHED_CITIZEN_THROW_MOLOTOV; //SCHED_RANGE_ATTACK2;
	}

	if ( HasCondition(COND_CAN_MELEE_ATTACK1) )
		return SCHED_MELEE_ATTACK1;

	if ( HasCondition(COND_CAN_MELEE_ATTACK2) )
		return SCHED_MELEE_ATTACK2;


	//TERO: this is to make sure the rebels in the truck are always facing the player
	//		it needs a bit to enable covering, though
	return SCHED_COMBAT_FACE;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
int CNPC_Citizen::SelectSchedule()
{
	// If we can't move, we're on a train, and should be sitting.
	if ( GetMoveType() == MOVETYPE_NONE) 
	{
		if (!m_bParentedToTruck)
		{
			// For now, we're only ever parented to trains. If you hit this assert, you've parented a citizen
			// to something else, and now we need to figure out a better system.
			Assert( GetMoveParent() && FClassnameIs( GetMoveParent(), "func_tracktrain" ) );
			return SCHED_CITIZEN_SIT_ON_TRAIN;
		}
		else
		{
			return SelectIceCreamTruckSchedule();
		}
	}

	if (ShouldRunAwayFromSmoke())
	{
		return SCHED_TAKE_COVER_FROM_ORIGIN;
	}

	CWeaponRPG *pRPG = dynamic_cast<CWeaponRPG*>(GetActiveWeapon());
	if ( pRPG && pRPG->IsGuiding() )
	{
		DevMsg( "Citizen in select schedule but RPG is guiding?\n");
		pRPG->StopGuiding();
	}

	//TERO: lets change to molotov cocktails if it's time
	//ChangeToMolotov();
	
	return BaseClass::SelectSchedule();
}



/*void CNPC_Citizen::ChangeToMolotov()
{
	if (HasCondition( COND_CIT_CAN_HAVE_MOLOTOV ))
	{
		if 	( m_iNumberMolotovCocktails != 0 &&
			  m_flNextMolotovCocktail < gpGlobals->curtime &&
			  GetEnemy() &&
			( GetEnemy()->Classify() == CLASS_PLAYER ||
			  GetEnemy()->Classify() == CLASS_PLAYER_ALLY ||
			  GetEnemy()->Classify() == CLASS_PLAYER_ALLY_VITAL ||
			  GetEnemy()->Classify() == CLASS_METROPOLICE ||
			  GetEnemy()->Classify() == CLASS_COMBINE ||
			  GetEnemy()->Classify() == CLASS_ZOMBIE ) )
		{
			if (GetActiveWeapon() && FClassnameIs( GetActiveWeapon(), "weapon_molotov" ))
			{
				//TERO: lets wait a bit until we have thrown our last molotov away before trying to change again
				m_flNextMolotovCocktail = gpGlobals->curtime + 10.0f;
			}
			else
			{
				if (m_iNumberMolotovCocktails>0)
				{
					m_iNumberMolotovCocktails--;
					DevMsg("npc_citizen %s molotov cocktails left %d\n", GetDebugName(), m_iNumberMolotovCocktails);
				}
			
				GiveWeapon( MAKE_STRING("weapon_molotov") );
				m_flNextMolotovCocktail = gpGlobals->curtime + 15.0f;

				m_flTimeHasHadMolotov = gpGlobals->curtime + 10.0f;
			}
		}
	} else 
	{
		//If we don't no longer have the condition lets turn back to the normal weapon

		if (m_flNextMolotovCocktail!= 0 )
		{
			if ( GetActiveWeapon() && 
				 FClassnameIs( GetActiveWeapon(), "weapon_molotov" ) && 
				 GetActivity() != ACT_RANGE_ATTACK_THROW )
			{
				if (GetWeapon(0) && !FClassnameIs( GetWeapon(0), "weapon_molotov") )	
				{
					CBaseCombatWeapon *pMolotov = GetActiveWeapon();
					if (pMolotov)
					{
						Weapon_Drop( pMolotov );
						UTIL_Remove( pMolotov );
					}

					if (m_iNumberMolotovCocktails>=0)
					{
						m_iNumberMolotovCocktails++;
					}

					DevMsg("Changing back to our previous weapon\n");
					Weapon_Switch( GetWeapon(0) );

					m_flTimeHasHadMolotov = gpGlobals->curtime + 15.0f;
				}
			}//end check current weapon

			m_flNextMolotovCocktail = 0;
		}//end check time
	}//end if no condition
}*/


void CNPC_Citizen::MolotovThrowCondition()
{
	if (!GetEnemy() || 
		GetEnemy()->Classify() == CLASS_MANHACK || 
		GetEnemy()->Classify() == CLASS_SCANNER ||
		GetEnemy()->Classify() == CLASS_HEADCRAB ||
		GetEnemy()->Classify() == CLASS_COMBINE_GUNSHIP )
	{
		//TERO: if our enemy has changed, change this shit
		ClearCondition(COND_CIT_CAN_HAVE_MOLOTOV);
		return;
	}

	if (m_flNextMolotovCocktail > gpGlobals->curtime)
	{
		return;
	}

	m_flNextMolotovCocktail = gpGlobals->curtime + 1.0f;

	if (GetEnemy()->GetWaterLevel() != WL_NotInWater)
	{
		//TERO: we don't clear it here so that if player runs water after we have already decided to throw one it wont be cancelled
		DevMsg("enemy in water\n");
		return;
	}

	ClearCondition(COND_CIT_CAN_HAVE_MOLOTOV);

	if (m_iNumberMolotovCocktails == 0)
	{
		return;
	}

	Vector vecTarget = GetEnemies()->LastKnownPosition( GetEnemy() );

	float flDist = (vecTarget - GetAbsOrigin()).Length();

	if ( flDist < 128) 
	{
		//DevMsg("too close");
		return;
	}
	else if (flDist > 1024) 
	{
		//DevMsg("too far\n");
		return;
	}
	else if ( m_flGroundSpeed != 0 )
	{
		//DevMsg("moving\n");
		return;
	} 
	else if (GetEnemy())
	{
		Vector vecToss = vec3_origin;
		Vector vecMins = -Vector(4,4,4);
		Vector vecMaxs = Vector(4,4,4);
		if( FInViewCone( vecTarget ) && CBaseEntity::FVisible( vecTarget ) )
		{
			//DevMsg("regular toss\n");
			vecToss = VecCheckThrow( this, EyePosition(), vecTarget, 650, 1.0, &vecMins, &vecMaxs );
		}
		else
		{
			// Have to try a high toss. Do I have enough room?
			trace_t tr;
			AI_TraceLine( EyePosition(), EyePosition() + Vector( 0, 0, 64 ), MASK_SHOT, this, COLLISION_GROUP_NONE, &tr );
			if( tr.fraction == 1.0 )
			{
				//DevMsg("high toss\n");
				vecToss = VecCheckToss( this, EyePosition(), vecTarget, -1, 1.0, true, &vecMins, &vecMaxs );
			}
		}

		if ( vecToss != vec3_origin )
		{
			m_vecTossVelocity = vecToss;
			//DevMsg("Can throw\n");
			SetCondition( COND_CIT_CAN_HAVE_MOLOTOV );
		}
	}
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
void CNPC_Citizen::GiveWeapon( string_t iszWeaponName )
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
//-----------------------------------------------------------------------------
int CNPC_Citizen::SelectSchedulePriorityAction()
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
// Determine if citizen should perform heal action.
//-----------------------------------------------------------------------------
int CNPC_Citizen::SelectScheduleHeal()
{
	// episodic medics may toss the healthkits rather than poke you with them

	if ( CanHeal() )
	{
		/*CBaseEntity *pEntity = PlayerInRange( GetLocalOrigin(), HEAL_TOSS_TARGET_RANGE );
		if ( pEntity )
		{
			if ( USE_EXPERIMENTAL_MEDIC_CODE() && IsMedic() )
			{
				// use the new heal toss algorithm
				if ( ShouldHealTossTarget( pEntity, HasCondition( COND_CIT_PLAYERHEALREQUEST ) ) )
				{
					SetTarget( pEntity );
					return SCHED_CITIZEN_HEAL_TOSS;
				}
			}
			else if ( PlayerInRange( GetLocalOrigin(), HEAL_MOVE_RANGE ) )
			{
				// use old mechanism for ammo
				if ( ShouldHealTarget( pEntity, HasCondition( COND_CIT_PLAYERHEALREQUEST ) ) )
				{
					SetTarget( pEntity );
					return SCHED_CITIZEN_HEAL;
				}
			}

		}*/
		
		if ( m_pSquad )
		{
			CBaseEntity *pEntity = NULL;
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
				return SCHED_CITIZEN_HEAL;
			}
		}
	}
	
	return SCHED_NONE;


}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
int CNPC_Citizen::SelectScheduleRetrieveItem()
{
	if ( HasCondition(COND_BETTER_WEAPON_AVAILABLE) )
	{
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
				return SCHED_GET_HEALTHKIT;
			}
		}
	}
	return SCHED_NONE;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
int CNPC_Citizen::SelectScheduleNonCombat()
{
	if ( m_NPCState == NPC_STATE_IDLE )
	{
		// Handle being inspected by the scanner
		if ( HasCondition( COND_CIT_START_INSPECTION ) )
		{
			ClearCondition( COND_CIT_START_INSPECTION );
			return SCHED_CITIZEN_PLAY_INSPECT_ACTIVITY;
		}
	}
	
	ClearCondition( COND_CIT_START_INSPECTION );

	if ( m_bShouldPatrol )
		return SCHED_CITIZEN_PATROL;
	
	return SCHED_NONE;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
int CNPC_Citizen::SelectScheduleManhackCombat()
{
	if ( m_NPCState == NPC_STATE_COMBAT && IsManhackMeleeCombatant() )
	{
		if ( !HasCondition( COND_CAN_MELEE_ATTACK1 ) )
		{
			float distSqEnemy = ( GetEnemy()->GetAbsOrigin() - EyePosition() ).LengthSqr();
			if ( distSqEnemy < 48.0*48.0 &&
				 ( ( GetEnemy()->GetAbsOrigin() + GetEnemy()->GetSmoothedVelocity() * .1 ) - EyePosition() ).LengthSqr() < distSqEnemy )
				return SCHED_COWER;

			int iRoll = random->RandomInt( 1, 4 );
			if ( iRoll == 1 )
				return SCHED_BACK_AWAY_FROM_ENEMY;
			else if ( iRoll == 2 )
				return SCHED_CHASE_ENEMY;
		}
	}
	
	return SCHED_NONE;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
int CNPC_Citizen::SelectScheduleCombat()
{
	int schedule = SelectScheduleManhackCombat();
	if ( schedule != SCHED_NONE )
		return schedule;

	if ( HasCondition(COND_CIT_CAN_HAVE_MOLOTOV) )
	{
		DevMsg("Selecting throwing schedule\n");
		ClearCondition(COND_CIT_CAN_HAVE_MOLOTOV);
		return SCHED_CITIZEN_THROW_MOLOTOV; //SCHED_RANGE_ATTACK2;
	}
		
	return BaseClass::SelectScheduleCombat();
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool CNPC_Citizen::ShouldDeferToFollowBehavior()
{
#if 0
	if ( HaveCommandGoal() )
		return false;
#endif
		
	return BaseClass::ShouldDeferToFollowBehavior();
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
int CNPC_Citizen::TranslateSchedule( int scheduleType ) 
{
	switch( scheduleType )
	{
	case SCHED_CHASE_ENEMY:
	case SCHED_TAKE_COVER_FROM_ENEMY:
	case SCHED_BACK_AWAY_FROM_ENEMY:
	case SCHED_MOVE_TO_WEAPON_RANGE:
		{
			if (m_bParentedToTruck)
			{
				return SCHED_COMBAT_FACE;
			}
			else
				BaseClass::TranslateSchedule( scheduleType );
		}
		break;
	case SCHED_RANGE_ATTACK2:
		{
			return SCHED_CITIZEN_THROW_MOLOTOV;
		}
		break;
	case SCHED_RANGE_ATTACK1:
		if ( !IsMortar( GetEnemy() ) && GetActiveWeapon() && FClassnameIs( GetActiveWeapon(), "weapon_rpg" ) )
		{
			if ( GetEnemy() && GetEnemy()->ClassMatches( "npc_strider" ) )
			{
				if (OccupyStrategySlotRange( SQUAD_SLOT_CITIZEN_RPG1, SQUAD_SLOT_CITIZEN_RPG2 ) )
				{
					return SCHED_CITIZEN_STRIDER_RANGE_ATTACK1_RPG;
				}
				else
				{
					return SCHED_STANDOFF;
				}
			}
			else
			{
				return SCHED_CITIZEN_RANGE_ATTACK1_RPG;
			}
		}
		break;
	}

	return BaseClass::TranslateSchedule( scheduleType );
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool CNPC_Citizen::ShouldAcceptGoal( CAI_BehaviorBase *pBehavior, CAI_GoalEntity *pGoal )
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
void CNPC_Citizen::OnClearGoal( CAI_BehaviorBase *pBehavior, CAI_GoalEntity *pGoal )
{
	if ( m_hSavedFollowGoalEnt == pGoal )
		m_hSavedFollowGoalEnt = NULL;
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CNPC_Citizen::StartTask( const Task_t *pTask )
{
	switch( pTask->iTask )
	{
	case TASK_CIT_FACE_THROW_TARGET:
		break;

	case TASK_CIT_PLAY_INSPECT_SEQUENCE:
		SetIdealActivity( (Activity) m_nInspectActivity );
		break;

	case TASK_ANNOUNCE_ATTACK:
		{
			if ( (int)pTask->flTaskData == 2 )
			{
				if ( SpeakIfAllowed( TLK_ATTACKING ) )
				{
					m_AnnounceAttackTimer.Set( 10, 30 );
				}

				TaskComplete();
			}

			BaseClass::StartTask( pTask );
			break;
		}
		break;

	case TASK_CIT_SIT_ON_TRAIN:
		if ( NameMatches("citizen_train_2") )
		{
			SetSequenceByName( "d1_t01_TrainRide_Sit_Idle" );
			SetIdealActivity( ACT_DO_NOT_DISTURB );
		}
		else
		{
			SetSequenceByName( "d1_t01_TrainRide_Stand" );
			SetIdealActivity( ACT_DO_NOT_DISTURB );
		}
		break;

	case TASK_CIT_LEAVE_TRAIN:
		if ( NameMatches("citizen_train_2") )
		{
			SetSequenceByName( "d1_t01_TrainRide_Sit_Exit" );
			SetIdealActivity( ACT_DO_NOT_DISTURB );
		}
		else
		{
			SetSequenceByName( "d1_t01_TrainRide_Stand_Exit" );
			SetIdealActivity( ACT_DO_NOT_DISTURB );
		}
		break;
		
	case TASK_CIT_HEAL:
#if HL2_EPISODIC
	case TASK_CIT_HEAL_TOSS:
#endif
		if ( IsMedic() )
		{
			if ( GetTarget() && GetTarget()->IsPlayer() && GetTarget()->m_iMaxHealth == GetTarget()->m_iHealth )
			{
				// Doesn't need us anymore
				TaskComplete();
				break;
			}

			Speak( TLK_HEAL );
		}
		SetIdealActivity( (Activity)ACT_CIT_HEAL );
		break;
	
	case TASK_CIT_SPEAK_MOURNING:
		if ( !IsSpeaking() && CanSpeakAfterMyself() )
		{
 			//CAI_AllySpeechManager *pSpeechManager = GetAllySpeechManager();

			//if ( pSpeechManager-> )

			Speak(TLK_PLDEAD);
		}
		TaskComplete();
		break;

	case TASK_CIT_RPG_AUGER:
		SetWait( 15.0 ); // maximum time auger before giving up
		break;

	case TASK_FACE_IDEAL:
	case TASK_FACE_ENEMY:
		{
			if (m_bParentedToTruck)
			{
				//CBasePlayer *pPlayer = UTIL_PlayerByIndex(1);
				Vector flEnemyLKP = GetEnemyLKP();

				/*if (GetEnemy())
				{
					if (GetEnemy()->IsPlayer() && pPlayer && pPlayer->GetVehicle() && pPlayer->GetVehicle()->GetVehicleEnt())
					{
						flEnemyLKP = pPlayer->GetVehicle()->GetVehicleEnt()->GetAbsOrigin();
					}
					else 
					{
						flEnemyLKP = GetEnemy()->GetAbsOrigin();
					}
				}*/

				float base = UTIL_VecToYaw ( GetLocalOrigin() - flEnemyLKP);

				GetMotor()->SetIdealYaw( base );
				//GetMotor()->SnapYaw();*/
			}
			else
			{
				BaseClass::StartTask( pTask );
			}
		}
		break;

	default:
		BaseClass::StartTask( pTask );
		break;
	}
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CNPC_Citizen::RunTask( const Task_t *pTask )
{
	/*if (GetCurSchedule())
		DevMsg("npc_citizen: running schedule with name: %s, with task id %d\n", GetCurSchedule()->GetName(), pTask->iTask );
	else
		DevMsg("npc_citizen: running schedule with task: %d\n", pTask->iTask );*/


	switch( pTask->iTask )
	{
		case TASK_FACE_IDEAL:
		case TASK_FACE_ENEMY:
		{
			if (m_bParentedToTruck)
			{
				//CBasePlayer *pPlayer = UTIL_PlayerByIndex(1);
				/*Vector flEnemyLKP = GetEnemyLKP();

				float base = UTIL_VecToYaw ( GetLocalOrigin() - flEnemyLKP);

				GetMotor()->SetIdealYaw( base );
				GetMotor()->UpdateYaw();
				
				if ( FacingIdeal() )
				{
					TaskComplete();
				}*/

				TaskComplete();

			}
			else
			{
				BaseClass::RunTask( pTask );
			}
			break;
		}
		case TASK_WAIT_FOR_MOVEMENT:
		{
			if ( IsManhackMeleeCombatant() )
			{
				AddFacingTarget( GetEnemy(), 1.0, 0.5 );
			}

			BaseClass::RunTask( pTask );
			break;
		}

		case TASK_MOVE_TO_TARGET_RANGE:
		{
			// If we're moving to heal a target, and the target dies, stop
			if ( IsCurSchedule( SCHED_CITIZEN_HEAL ) && (!GetTarget() || !GetTarget()->IsAlive()) )
			{
				TaskFail(FAIL_NO_TARGET);
				return;
			}

			BaseClass::RunTask( pTask );
			break;
		}

		case TASK_CIT_PLAY_INSPECT_SEQUENCE:
		{
			AutoMovement();
			
			if ( IsSequenceFinished() )
			{
				TaskComplete();
			}
			break;
		}
		case TASK_CIT_SIT_ON_TRAIN:
		{
			// If we were on a train, but we're not anymore, enable movement
			if ( !GetMoveParent() )
			{
				SetMoveType( MOVETYPE_STEP );
				CapabilitiesAdd( bits_CAP_MOVE_GROUND );
				TaskComplete();
			}
			break;
		}

		case TASK_CIT_LEAVE_TRAIN:
		{
			if ( IsSequenceFinished() )
			{
				SetupVPhysicsHull();
				TaskComplete();
			}
			break;
		}

		case TASK_CIT_HEAL:
			if ( IsSequenceFinished() )
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
		case TASK_CIT_FACE_THROW_TARGET:
		{
			// project a point along the toss vector and turn to face that point.
			GetMotor()->SetIdealYawToTargetAndUpdate( GetLocalOrigin() + m_vecTossVelocity * 64, AI_KEEP_YAW_SPEED );

			if ( FacingIdeal() )
			{
				TaskComplete( true );
			}
			break;
		}

#if HL2_EPISODIC
		case TASK_CIT_HEAL_TOSS:
			if ( IsSequenceFinished() )
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

#endif

		case TASK_CIT_RPG_AUGER:
			{
				// Keep augering until the RPG has been destroyed
				CWeaponRPG *pRPG = dynamic_cast<CWeaponRPG*>(GetActiveWeapon());
				if ( !pRPG )
				{
					TaskFail( FAIL_ITEM_NO_FIND );
					return;
				}

				// Has the RPG detonated?
				if ( !pRPG->GetMissile() )
				{
					pRPG->StopGuiding();
					TaskComplete();
					return;
				}

				Vector vecLaserPos = pRPG->GetNPCLaserPosition();


					// Abort if we've lost our enemy
					if ( !GetEnemy() )
					{
						pRPG->StopGuiding();
						TaskFail( FAIL_NO_ENEMY );
						return;
					}

					// Is our enemy occluded?
					if ( HasCondition( COND_ENEMY_OCCLUDED ) )
					{
						// Turn off the laserdot, but don't stop augering
						pRPG->StopGuiding();
						return;
					}
					else if ( pRPG->IsGuiding() == false )
					{
						pRPG->StartGuiding();
					}

					Vector vecEnemyPos = GetEnemy()->BodyTarget(GetAbsOrigin(), false);
				
					// Pull the laserdot towards the target
					Vector vecToTarget = (vecEnemyPos - vecLaserPos);
					float distToMove = VectorNormalize( vecToTarget );
					if ( distToMove > 90 )
						distToMove = 90;
					vecLaserPos += vecToTarget * distToMove;

				if ( IsWaitFinished() )
				{
					pRPG->StopGuiding();
					TaskFail( FAIL_NO_SHOOT );
					return;
				}
				// Add imprecision to avoid obvious robotic perfection stationary targets
				float imprecision = 18*sin(gpGlobals->curtime);
				vecLaserPos.x += imprecision;
				vecLaserPos.y += imprecision;
				vecLaserPos.z += imprecision;
				pRPG->UpdateNPCLaserPosition( vecLaserPos );
			}
			break;

		default:
			BaseClass::RunTask( pTask );
			break;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : code - 
//-----------------------------------------------------------------------------
void CNPC_Citizen::TaskFail( AI_TaskFailureCode_t code )
{
	// If our heal task has failed, push out the heal time
	if ( IsCurSchedule( SCHED_CITIZEN_HEAL ) )
	{
		m_flAllyHealTime 	= gpGlobals->curtime + sk_citizen_heal_ally_delay.GetFloat();
	}

	if( code == FAIL_NO_ROUTE_BLOCKED && m_bNotifyNavFailBlocked )
	{
		m_OnNavFailBlocked.FireOutput( this, this );
	}

	BaseClass::TaskFail( code );
}

//-----------------------------------------------------------------------------
// Purpose: Override base class activiites
//-----------------------------------------------------------------------------
Activity CNPC_Citizen::NPC_TranslateActivity( Activity activity )
{
	if ( activity == ACT_RANGE_ATTACK_THROW )
	{
		//TERO: lets not get interrupted by flinch if we are throwing
		m_flNextFlinchTime = gpGlobals->curtime + random->RandomFloat( 3, 5 );
	}

	if ( activity == ACT_MELEE_ATTACK1 )
	{
		return ACT_MELEE_ATTACK_SWING;
	}

	if ( activity == ACT_CROUCHIDLE_STIMULATED ||
		 activity == ACT_CROUCHIDLE_AIM_STIMULATED || 
		 activity == ACT_CROUCHIDLE_AGITATED )
	{
		return ACT_COVER_LOW; 
	}

	/*if (m_bIsInVan)
	{
		DevMsg("Activity %d\n", activity );

	}*/

	// !!!HACK - Citizens don't have the required animations for shotguns, 
	// so trick them into using the rifle counterparts for now (sjb)
	if ( activity == ACT_RUN_AIM_SHOTGUN )
		return ACT_RUN_AIM_RIFLE;
	if ( activity == ACT_WALK_AIM_SHOTGUN )
		return ACT_WALK_AIM_RIFLE;
	if ( activity == ACT_IDLE_ANGRY_SHOTGUN )
		return ACT_IDLE_ANGRY_SMG1;
	if ( activity == ACT_RANGE_ATTACK_SHOTGUN_LOW )
		return ACT_RANGE_ATTACK_SMG1_LOW;

	return BaseClass::NPC_TranslateActivity( activity );
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
void CNPC_Citizen::HandleAnimEvent( animevent_t *pEvent )
{
	if ( pEvent->event == AE_CITIZEN_GET_PACKAGE )
	{
		// Give the citizen a package
		CBaseCombatWeapon *pWeapon = Weapon_Create( "weapon_citizenpackage" );
		if ( pWeapon )
		{
			// If I have a name, make my weapon match it with "_weapon" appended
			if ( GetEntityName() != NULL_STRING )
			{
				pWeapon->SetName( AllocPooledString(UTIL_VarArgs("%s_weapon", GetEntityName())) );
			}
			Weapon_Equip( pWeapon );
		}
		return;
	}
	else if ( pEvent->event == AE_CITIZEN_HEAL )
	{
		// Heal my target (if within range)
#if HL2_EPISODIC
		if ( USE_EXPERIMENTAL_MEDIC_CODE() && IsMedic() )
		{
			CBaseCombatCharacter *pTarget = dynamic_cast<CBaseCombatCharacter *>( GetTarget() );
			Assert(pTarget);
			if ( pTarget )
			{
				m_flAllyHealTime 	= gpGlobals->curtime + sk_citizen_heal_toss_ally_delay.GetFloat();;
				TossHealthKit( pTarget, Vector(48.0f, 0.0f, 0.0f)  );
			}
		}
		else
		{
			Heal();
		}
#else
		Heal();
#endif
		return;
	}

	switch( pEvent->event )
	{
	case NPC_EVENT_LEFTFOOT:
		{
			EmitSound( "NPC_Citizen.FootstepLeft", pEvent->eventtime );
		}
		break;

	case NPC_EVENT_RIGHTFOOT:
		{
			EmitSound( "NPC_Citizen.FootstepRight", pEvent->eventtime );
		}
		break;
	case EVENT_WEAPON_THROW:
		{
			Vector vecSpin;
			vecSpin.x = random->RandomFloat( -1000.0, 1000.0 );
			vecSpin.y = random->RandomFloat( -1000.0, 1000.0 );
			vecSpin.z = random->RandomFloat( -1000.0, 1000.0 ); 

			Vector vecStart;
			GetAttachment( "righthand", vecStart );

			if( m_NPCState == NPC_STATE_SCRIPT )
			{
				// Use a fixed velocity for grenades thrown in scripted state.
				// Grenades thrown from a script do not count against grenades remaining for the AI to use.
				Vector forward, up, vecThrow;

				GetVectors( &forward, NULL, &up );
				vecThrow = forward * 750 + up * 175;

				CGrenade_Molotov *pMolotov = (CGrenade_Molotov*)Create( "grenade_molotov", vecStart, vec3_angle, this );
				pMolotov->SetVelocity( vecThrow, vecSpin );
				// Tumble through the air
				//pMolotov->SetLocalAngularVelocity( angVel );
				pMolotov->SetThrower( this );
				pMolotov->SetOwnerEntity( this );
			}
			else
			{
				// Use the Velocity that AI gave us.
				CGrenade_Molotov *pMolotov = (CGrenade_Molotov*)Create( "grenade_molotov", vecStart, vec3_angle, this );
				pMolotov->SetVelocity( m_vecTossVelocity, vecSpin );
				// Tumble through the air	
				//pMolotov->SetLocalAngularVelocity( angVel );
				pMolotov->SetThrower( this );
				pMolotov->SetOwnerEntity( this );
				
				// wait six seconds before even looking again to see if a grenade can be thrown.
				if (m_iNumberMolotovCocktails > 0)
				{
					m_iNumberMolotovCocktails--;
					DevMsg("npc_citizen %s molotov cocktails left %d\n", GetDebugName(), m_iNumberMolotovCocktails);
				}
			
				ClearCondition(COND_CIT_CAN_HAVE_MOLOTOV);
				m_flNextMolotovCocktail = gpGlobals->curtime + 15.0f;
			}
		}
		break;

	default:
		BaseClass::HandleAnimEvent( pEvent );
		break;
	}
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
void CNPC_Citizen::PickupItem( CBaseEntity *pItem )
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
		DevMsg("Citizen doesn't know how to pick up %s!\n", pItem->GetClassname() );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CNPC_Citizen::IgnorePlayerPushing( void )
{
	// If the NPC's on a func_tank that the player cannot man, ignore player pushing
	if ( m_FuncTankBehavior.IsMounted() )
	{
		CFuncTank *pTank = m_FuncTankBehavior.GetFuncTank();
		if ( pTank && !pTank->IsControllable() )
			return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Return a random expression for the specified state to play over 
//			the state's expression loop.
//-----------------------------------------------------------------------------
const char *CNPC_Citizen::SelectRandomExpressionForState( NPC_STATE state )
{
	// Hacky remap of NPC states to expression states that we care about
	int iExpressionState = 0;
	switch ( state )
	{
	case NPC_STATE_IDLE:
		iExpressionState = 0;
		break;

	case NPC_STATE_ALERT:
		iExpressionState = 1;
		break;

	case NPC_STATE_COMBAT:
		iExpressionState = 2;
		break;

	default:
		// An NPC state we don't have expressions for
		return NULL;
	}

	// Now pick the right one for our expression type
	switch ( m_ExpressionType )
	{
	case CIT_EXP_SCARED:
		{
			int iRandom = RandomInt( 0, ARRAYSIZE(ScaredExpressions[iExpressionState].szExpressions)-1 );
			return ScaredExpressions[iExpressionState].szExpressions[iRandom];
		}

	case CIT_EXP_NORMAL:
		{
			int iRandom = RandomInt( 0, ARRAYSIZE(NormalExpressions[iExpressionState].szExpressions)-1 );
			return NormalExpressions[iExpressionState].szExpressions[iRandom];
		}

	case CIT_EXP_ANGRY:
		{
			int iRandom = RandomInt( 0, ARRAYSIZE(AngryExpressions[iExpressionState].szExpressions)-1 );
			return AngryExpressions[iExpressionState].szExpressions[iRandom];
		}

	default:
		break;
	}

	return NULL;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CNPC_Citizen::SimpleUse( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	// Under these conditions, citizens will refuse to go with the player.
	// Robin: NPCs should always respond to +USE even if someone else has the semaphore.
	m_bDontUseSemaphore = true;

	// First, try to speak the +USE concept
	if ( !SelectPlayerUseSpeech() )
	{
		if ( HasSpawnFlags(SF_CITIZEN_NOT_COMMANDABLE) || IRelationType( pActivator ) == D_NU )
		{
			// If I'm denying commander mode because a level designer has made that decision,
			// then fire this output in case they've hooked it to an event.
			m_OnDenyCommanderUse.FireOutput( this, this );
		}
	}

	m_bDontUseSemaphore = false;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool CNPC_Citizen::OnBeginMoveAndShoot()
{
	if ( BaseClass::OnBeginMoveAndShoot() )
	{
		if( m_iMySquadSlot == SQUAD_SLOT_ATTACK1 || m_iMySquadSlot == SQUAD_SLOT_ATTACK2 )
			return true; // already have the slot I need

		if( m_iMySquadSlot == SQUAD_SLOT_NONE && OccupyStrategySlotRange( SQUAD_SLOT_ATTACK1, SQUAD_SLOT_ATTACK2 ) )
			return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CNPC_Citizen::OnEndMoveAndShoot()
{
	VacateStrategySlot();
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CNPC_Citizen::LocateEnemySound()
{
#if 0
	if ( !GetEnemy() )
		return;

	float flZDiff = GetLocalOrigin().z - GetEnemy()->GetLocalOrigin().z;

	if( flZDiff < -128 )
	{
		EmitSound( "NPC_Citizen.UpThere" );
	}
	else if( flZDiff > 128 )
	{
		EmitSound( "NPC_Citizen.DownThere" );
	}
	else
	{
		EmitSound( "NPC_Citizen.OverHere" );
	}
#endif
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool CNPC_Citizen::IsManhackMeleeCombatant()
{
	CBaseCombatWeapon *pWeapon = GetActiveWeapon();
	CBaseEntity *pEnemy = GetEnemy();
	return ( pEnemy && pWeapon && pEnemy->Classify() == CLASS_MANHACK && pWeapon->ClassMatches( "weapon_crowbar" ) );
}

//-----------------------------------------------------------------------------
// Purpose: Return the actual position the NPC wants to fire at when it's trying
//			to hit it's current enemy.
//-----------------------------------------------------------------------------
Vector CNPC_Citizen::GetActualShootPosition( const Vector &shootOrigin )
{
	Vector vecTarget = BaseClass::GetActualShootPosition( shootOrigin );

	CWeaponRPG *pRPG = dynamic_cast<CWeaponRPG*>(GetActiveWeapon());
	// If we're firing an RPG at a gunship, aim off to it's side, because we'll auger towards it.
	if ( pRPG && GetEnemy() )
	{
		if ( FClassnameIs( GetEnemy(), "npc_combinegunship" ) )
		{
			Vector vecRight;
			GetVectors( NULL, &vecRight, NULL );
			// Random height
			vecRight.z = 0;

			// Find a clear shot by checking for clear shots around it
			float flShotOffsets[] =
			{
				512,
				-512,
				128,
				-128
			};
			for ( int i = 0; i < ARRAYSIZE(flShotOffsets); i++ )
			{
				Vector vecTest = vecTarget + (vecRight * flShotOffsets[i]);
				// Add some random height to it
				vecTest.z += RandomFloat( -512, 512 );
				trace_t tr;
				AI_TraceLine( shootOrigin, vecTest, MASK_SHOT, this, COLLISION_GROUP_NONE, &tr);

				// If we can see the point, it's a clear shot
				if ( tr.fraction == 1.0 && tr.m_pEnt != GetEnemy() )
				{
					pRPG->SetNPCLaserPosition( vecTest );
					return vecTest;
				}
			}
		}
		else
		{
			pRPG->SetNPCLaserPosition( vecTarget );
		}

	}

	return vecTarget;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CNPC_Citizen::OnChangeActiveWeapon( CBaseCombatWeapon *pOldWeapon, CBaseCombatWeapon *pNewWeapon )
{
	if ( pNewWeapon )
	{
		GetShotRegulator()->SetParameters( pNewWeapon->GetMinBurst(), pNewWeapon->GetMaxBurst(), pNewWeapon->GetMinRestTime(), pNewWeapon->GetMaxRestTime() );
	}
	BaseClass::OnChangeActiveWeapon( pOldWeapon, pNewWeapon );
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
#define SHOTGUN_DEFER_SEARCH_TIME	20.0f
#define OTHER_DEFER_SEARCH_TIME		FLT_MAX
bool CNPC_Citizen::ShouldLookForBetterWeapon()
{
	if ( BaseClass::ShouldLookForBetterWeapon() )
	{
		if ( IsInPlayerSquad() && (GetActiveWeapon()&&IsMoving()) && ( m_FollowBehavior.GetFollowTarget() && m_FollowBehavior.GetFollowTarget()->IsPlayer() ) )
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
int CNPC_Citizen::OnTakeDamage_Alive( const CTakeDamageInfo &info )
{
	if( (info.GetDamageType() & DMG_BURN) && (info.GetDamageType() & DMG_DIRECT) )
	{
#define CITIZEN_SCORCH_RATE		6
#define CITIZEN_SCORCH_FLOOR	75

		Scorch( CITIZEN_SCORCH_RATE, CITIZEN_SCORCH_FLOOR );
	}

	CTakeDamageInfo newInfo = info;

	return BaseClass::OnTakeDamage_Alive( newInfo );
}






//-----------------------------------------------------------------------------
// Purpose:  This is a generic function (to be implemented by sub-classes) to
//			 handle specific interactions between different types of characters
//			 (For example the barnacle grabbing an NPC)
// Input  :  Constant for the type of interaction
// Output :	 true  - if sub-class has a response for the interaction
//			 false - if sub-class has no response
//-----------------------------------------------------------------------------
bool CNPC_Citizen::HandleInteraction(int interactionType, void *data, CBaseCombatCharacter* sourceEnt)
{
	// TODO:  As citizen gets more complex, we will have to only allow
	//		  these interruptions to happen from certain schedules
	if (interactionType ==	g_interactionScannerInspect)
	{
		if ( gpGlobals->curtime > m_fNextInspectTime )
		{
			//SetLookTarget(sourceEnt);

			// Don't let anyone else pick me for a couple seconds
			SetNextScannerInspectTime( gpGlobals->curtime + 5.0 );
			return true;
		}
		return false;
	}
	else if (interactionType ==	g_interactionScannerInspectBegin)
	{
		// Don't inspect me again for a while
		SetNextScannerInspectTime( gpGlobals->curtime + CIT_INSPECTED_DELAY_TIME );
		
		Vector	targetDir = ( sourceEnt->WorldSpaceCenter() - WorldSpaceCenter() );
		VectorNormalize( targetDir );

		// If we're hit from behind, startle
		if ( DotProduct( targetDir, BodyDirection3D() ) < 0 )
		{
			m_nInspectActivity = ACT_CIT_STARTLED;
		}
		else
		{
			// Otherwise we're blinded by the flash
			m_nInspectActivity = ACT_CIT_BLINDED;
		}
		
		SetCondition( COND_CIT_START_INSPECTION );
		return true;
	}
	else if (interactionType ==	g_interactionScannerInspectHandsUp)
	{
		m_nInspectActivity = ACT_CIT_HANDSUP;
		SetSchedule(SCHED_CITIZEN_PLAY_INSPECT_ACTIVITY);
		return true;
	}
	else if (interactionType ==	g_interactionScannerInspectShowArmband)
	{
		m_nInspectActivity = ACT_CIT_SHOWARMBAND;
		SetSchedule(SCHED_CITIZEN_PLAY_INSPECT_ACTIVITY);
		return true;
	}
	else if (interactionType ==	g_interactionScannerInspectDone)
	{
		SetSchedule(SCHED_IDLE_WANDER);
		return true;
	}
	else if (interactionType == g_interactionHitByPlayerThrownPhysObj )
	{
		if ( IsOkToSpeakInResponseToPlayer() )
		{
			Speak( TLK_PLYR_PHYSATK );
		}
		return true;
	}

	return BaseClass::HandleInteraction( interactionType, data, sourceEnt );
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
WeaponProficiency_t CNPC_Citizen::CalcWeaponProficiency( CBaseCombatWeapon *pWeapon )
{
	return WEAPON_PROFICIENCY_AVERAGE;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool CNPC_Citizen::FValidateHintType( CAI_Hint *pHint )
{
	switch( pHint->HintType() )
	{
	case HINT_WORLD_VISUALLY_INTERESTING:
		return true;
		break;

	default:
		break;
	}

	return BaseClass::FValidateHintType( pHint );
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool CNPC_Citizen::CanHeal()
{ 
	if ( !IsMedic() )
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
bool CNPC_Citizen::ShouldHealTarget( CBaseEntity *pTarget, bool bActiveUse )
{
	Disposition_t disposition;
	
	if ( !pTarget && ( ( disposition = IRelationType( pTarget ) ) != D_LI && disposition != D_NU ) )
		return false;

	// Don't heal if I'm in the middle of talking
	if ( IsSpeaking() )
		return false;

	// Don't heal or give ammo to targets in vehicles
	CBaseCombatCharacter *pCCTarget = pTarget->MyCombatCharacterPointer();
	if ( pCCTarget != NULL && pCCTarget->IsInAVehicle() )
		return false;

	if ( IsMedic() )
	{
		Vector toPlayer = ( pTarget->GetAbsOrigin() - GetAbsOrigin() );
	 	if (( bActiveUse || toPlayer.Length() < HEAL_TARGET_RANGE) 
#ifdef HL2_EPISODIC
			&& fabs(toPlayer.z) < HEAL_TARGET_RANGE_Z
#endif
			)
	 	{
			if ( pTarget->m_iHealth > 0 )
			{
	 			if ( bActiveUse )
				{
					// Ignore heal requests if we're going to heal a tiny amount
					float timeFullHeal = m_flAllyHealTime;
					float timeRecharge = sk_citizen_heal_ally_delay.GetFloat();
					float maximumHealAmount = sk_citizen_heal_ally.GetFloat();
					float healAmt = ( maximumHealAmount * ( 1.0 - ( timeFullHeal - gpGlobals->curtime ) / timeRecharge ) );
					if ( healAmt > pTarget->m_iMaxHealth - pTarget->m_iHealth )
						healAmt = pTarget->m_iMaxHealth - pTarget->m_iHealth;

	 				return ( pTarget->m_iMaxHealth > pTarget->m_iHealth );
				}
	 				
				// Are we ready to heal again?
				bool bReadyToHeal =  ( m_flAllyHealTime <= gpGlobals->curtime );

				// Only heal if we're ready
				if ( bReadyToHeal )
				{
					int requiredHealth;

					requiredHealth = pTarget->GetMaxHealth() * sk_citizen_health.GetFloat();

					if ( ( pTarget->m_iHealth <= requiredHealth ) && IRelationType( pTarget ) == D_LI )
						return true;
				}
			}
		}
	}
	return false;
}

#ifdef HL2_EPISODIC
//-----------------------------------------------------------------------------
// Determine if the citizen is in a position to be throwing medkits
//-----------------------------------------------------------------------------
bool CNPC_Citizen::ShouldHealTossTarget( CBaseEntity *pTarget, bool bActiveUse )
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

	//bool bTargetIsPlayer = pTarget->IsPlayer();

	// Don't heal or give ammo to targets in vehicles
	CBaseCombatCharacter *pCCTarget = pTarget->MyCombatCharacterPointer();
	if ( pCCTarget != NULL && pCCTarget->IsInAVehicle() )
		return false;

	Vector toPlayer = ( pTarget->GetAbsOrigin() - GetAbsOrigin() );
	if ( bActiveUse )
	{
		if ( pTarget->m_iHealth > 0 )
		{
			if ( bActiveUse )
			{
				// Ignore heal requests if we're going to heal a tiny amount
				float timeFullHeal = m_flAllyHealTime;
				float timeRecharge = sk_citizen_heal_ally_delay.GetFloat();
				float maximumHealAmount = sk_citizen_heal_ally.GetFloat();
				float healAmt = ( maximumHealAmount * ( 1.0 - ( timeFullHeal - gpGlobals->curtime ) / timeRecharge ) );
				if ( healAmt > pTarget->m_iMaxHealth - pTarget->m_iHealth )
					healAmt = pTarget->m_iMaxHealth - pTarget->m_iHealth;
				if ( healAmt < sk_citizen_heal_ally_min_forced.GetFloat() )
					return false;

				return ( pTarget->m_iMaxHealth > pTarget->m_iHealth );
			}

			// Are we ready to heal again?
			bool bReadyToHeal = ( m_flAllyHealTime <= gpGlobals->curtime );

			// Only heal if we're ready
			if ( bReadyToHeal )
			{
				int requiredHealth = pTarget->GetMaxHealth() * sk_citizen_heal_ally_min_pct.GetFloat();

				if ( ( pTarget->m_iHealth <= requiredHealth ) && IRelationType( pTarget ) == D_LI )
					return true;
			}
		}
	}
	
	return false;
}
#endif


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CNPC_Citizen::Heal()
{
	if ( !CanHeal() )
		  return;

	CBaseEntity *pTarget = GetTarget();

	Vector target = pTarget->GetAbsOrigin() - GetAbsOrigin();
	if ( target.Length() > HEAL_TARGET_RANGE * 2 )
		return;

	// Don't heal a player that's staring at you until a few seconds have passed.
	m_flTimeNextHealStare = gpGlobals->curtime + sk_citizen_stare_heal_time.GetFloat();

	if ( IsMedic() )
	{
		float timeFullHeal;
		float timeRecharge;
		float maximumHealAmount;

		timeFullHeal 		= m_flAllyHealTime;
		timeRecharge 		= sk_citizen_heal_ally_delay.GetFloat();
		maximumHealAmount 	= sk_citizen_heal_ally.GetFloat();
		m_flAllyHealTime 	= gpGlobals->curtime + timeRecharge;
		
		float healAmt = ( maximumHealAmount * ( 1.0 - ( timeFullHeal - gpGlobals->curtime ) / timeRecharge ) );
		
		if ( healAmt > maximumHealAmount )
			healAmt = maximumHealAmount;
		else
			healAmt = RoundFloatToInt( healAmt );
		
		if ( healAmt > 0 )
		{
			if ( pTarget->IsPlayer() && npc_citizen_medic_emit_sound.GetBool() )
			{
				CPASAttenuationFilter filter(pTarget, "HealthKit.Touch");//EmitSound( CPASAttenuationFilter( pTarget, "HealthKit.Touch" ), pTarget->entindex(), "HealthKit.Touch" );
				EmitSound(filter, pTarget->entindex(), "HealthKit.Touch");
			}

			pTarget->TakeHealth( healAmt, DMG_GENERIC );
			pTarget->RemoveAllDecals();
		}
	}
}



#if HL2_EPISODIC
//-----------------------------------------------------------------------------
// Like Heal(), but tosses a healthkit in front of the player rather than just juicing him up.
//-----------------------------------------------------------------------------
void	CNPC_Citizen::TossHealthKit(CBaseCombatCharacter *pThrowAt, const Vector &offset)
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

	if (npc_citizen_medic_throw_style.GetInt() == 0)
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
void	CNPC_Citizen::InputForceHealthKitToss( inputdata_t &inputdata )
{
	TossHealthKit( UTIL_GetLocalPlayer(), Vector(48.0f, 0.0f, 0.0f)  );
}

#endif



//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool CNPC_Citizen::ShouldLookForHealthItem()
{
	// Definitely do not take health if not in the player's squad.
	return false;

	if( gpGlobals->curtime < m_flNextHealthSearchTime )
		return false;

	// I'm fully healthy.
	if( GetHealth() >= GetMaxHealth() )
		return false;

	CBasePlayer *pPlayer = AI_GetSinglePlayer();

	// Player is hurt, don't steal his health.
	if( pPlayer && pPlayer->GetHealth() <= pPlayer->GetMaxHealth() * 0.75f )
		return false;

	// Wait till you're standing still.
	if( IsMoving() )
		return false;

	return true;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CNPC_Citizen::InputStartPatrolling( inputdata_t &inputdata )
{
	m_bShouldPatrol = true;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CNPC_Citizen::InputStopPatrolling( inputdata_t &inputdata )
{
	m_bShouldPatrol = false;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
void CNPC_Citizen::OnGivenWeapon( CBaseCombatWeapon *pNewWeapon )
{
	FixupMattWeapon();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &inputdata - 
//-----------------------------------------------------------------------------
void CNPC_Citizen::InputSetMedicOn( inputdata_t &inputdata )
{
	AddSpawnFlags( SF_CITIZEN_MEDIC );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &inputdata - 
//-----------------------------------------------------------------------------
void CNPC_Citizen::InputSetMedicOff( inputdata_t &inputdata )
{
	RemoveSpawnFlags( SF_CITIZEN_MEDIC );
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
void CNPC_Citizen::InputSpeakIdleResponse( inputdata_t &inputdata )
{
	SpeakIfAllowed( TLK_ANSWER, NULL, true );
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CNPC_Citizen::DeathSound( const CTakeDamageInfo &info )
{
	// Sentences don't play on dead NPCs
	SentenceStop();

	EmitSound( "NPC_Citizen.Die" );
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
void CNPC_Citizen::FearSound( void )
{
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CNPC_Citizen::UseSemaphore( void )
{
	// Ignore semaphore if we're told to work outside it
	if ( HasSpawnFlags(SF_CITIZEN_IGNORE_SEMAPHORE) )
		return false;

	return BaseClass::UseSemaphore();
}

//-----------------------------------------------------------------------------
//
// Schedules
//
//-----------------------------------------------------------------------------

AI_BEGIN_CUSTOM_NPC( npc_citizen, CNPC_Citizen )

	DECLARE_TASK( TASK_CIT_HEAL )
	DECLARE_TASK( TASK_CIT_RPG_AUGER )
	DECLARE_TASK( TASK_CIT_PLAY_INSPECT_SEQUENCE )
	DECLARE_TASK( TASK_CIT_SIT_ON_TRAIN )
	DECLARE_TASK( TASK_CIT_LEAVE_TRAIN )
	DECLARE_TASK( TASK_CIT_SPEAK_MOURNING )
	DECLARE_TASK( TASK_CIT_FACE_THROW_TARGET )
#if HL2_EPISODIC
	DECLARE_TASK( TASK_CIT_HEAL_TOSS )
#endif

	DECLARE_SQUADSLOT( SQUAD_SLOT_CITIZEN_RPG1 )
	DECLARE_SQUADSLOT( SQUAD_SLOT_CITIZEN_RPG2 )
	DECLARE_SQUADSLOT( SQUAD_SLOT_CITIZEN_MOLOTOV )

	DECLARE_ACTIVITY( ACT_CIT_HANDSUP )
	DECLARE_ACTIVITY( ACT_CIT_BLINDED )
	DECLARE_ACTIVITY( ACT_CIT_SHOWARMBAND )
	DECLARE_ACTIVITY( ACT_CIT_HEAL )
	DECLARE_ACTIVITY( ACT_CIT_STARTLED )

	DECLARE_CONDITION( COND_CIT_PLAYERHEALREQUEST )
	DECLARE_CONDITION( COND_CIT_COMMANDHEAL )
	DECLARE_CONDITION( COND_CIT_START_INSPECTION )
	DECLARE_CONDITION( COND_CIT_CAN_HAVE_MOLOTOV )

	//Events
	DECLARE_ANIMEVENT( AE_CITIZEN_GET_PACKAGE )
	DECLARE_ANIMEVENT( AE_CITIZEN_HEAL )

	//=========================================================
	// > SCHED_CITIZEN_THROW_MOLOTOV
	//=========================================================
	DEFINE_SCHEDULE
	(
		SCHED_CITIZEN_THROW_MOLOTOV,

		"	Tasks"
		"		TASK_STOP_MOVING			0"
		"		TASK_CIT_FACE_THROW_TARGET	0"
		"		TASK_ANNOUNCE_ATTACK		2"	// 2 = grenade
		"		TASK_WAIT					1"
		"		TASK_PLAY_SEQUENCE			ACTIVITY:ACT_RANGE_ATTACK_THROW"
	//	"		TASK_SET_SCHEDULE			SCHEDULE:SCHED_TAKE_COVER_FROM_ENEMY"
		""
		"	Interrupts"
	)

	//=========================================================
	// > SCHED_SCI_HEAL
	//=========================================================
	DEFINE_SCHEDULE
	(
		SCHED_CITIZEN_HEAL,

		"	Tasks"
		"		TASK_GET_PATH_TO_TARGET				0"
		"		TASK_MOVE_TO_TARGET_RANGE			50"
		"		TASK_STOP_MOVING					0"
		"		TASK_FACE_IDEAL						0"
//		"		TASK_SAY_HEAL						0"
//		"		TASK_PLAY_SEQUENCE_FACE_TARGET		ACTIVITY:ACT_ARM"
		"		TASK_CIT_HEAL							0"
//		"		TASK_PLAY_SEQUENCE_FACE_TARGET		ACTIVITY:ACT_DISARM"
		"	"
		"	Interrupts"
	)

#if HL2_EPISODIC
	//=========================================================
	// > SCHED_CITIZEN_HEAL_TOSS
	// this is for the episodic behavior where the citizen hurls the medkit
	//=========================================================
	DEFINE_SCHEDULE
	(
		SCHED_CITIZEN_HEAL_TOSS,

	"	Tasks"
//  "		TASK_GET_PATH_TO_TARGET				0"
//  "		TASK_MOVE_TO_TARGET_RANGE			50"
	"		TASK_STOP_MOVING					0"
	"		TASK_FACE_IDEAL						0"
//	"		TASK_SAY_HEAL						0"
//	"		TASK_PLAY_SEQUENCE_FACE_TARGET		ACTIVITY:ACT_ARM"
	"		TASK_CIT_HEAL_TOSS							0"
//	"		TASK_PLAY_SEQUENCE_FACE_TARGET		ACTIVITY:ACT_DISARM"
	"	"
	"	Interrupts"
	)
#endif

	//=========================================================
	// > SCHED_CITIZEN_RANGE_ATTACK1_RPG
	//=========================================================
	DEFINE_SCHEDULE
	(
		SCHED_CITIZEN_RANGE_ATTACK1_RPG,

		"	Tasks"
		"		TASK_STOP_MOVING			0"
		"		TASK_FACE_ENEMY				0"
		"		TASK_ANNOUNCE_ATTACK		1"	// 1 = primary attack
		"		TASK_RANGE_ATTACK1			0"
		"		TASK_CIT_RPG_AUGER			1"
		""
		"	Interrupts"
	)

	//=========================================================
	// > SCHED_CITIZEN_RANGE_ATTACK1_RPG
	//=========================================================
	DEFINE_SCHEDULE
	(
		SCHED_CITIZEN_STRIDER_RANGE_ATTACK1_RPG,

		"	Tasks"
		"		TASK_STOP_MOVING			0"
		"		TASK_FACE_ENEMY				0"
		"		TASK_ANNOUNCE_ATTACK		1"	// 1 = primary attack
		"		TASK_WAIT					1"
		"		TASK_RANGE_ATTACK1			0"
		"		TASK_CIT_RPG_AUGER			1"
		"		TASK_SET_SCHEDULE			SCHEDULE:SCHED_TAKE_COVER_FROM_ENEMY"
		""
		"	Interrupts"
	)


	//=========================================================
	// > SCHED_CITIZEN_PATROL
	//=========================================================
	DEFINE_SCHEDULE	
	(
		SCHED_CITIZEN_PATROL,
		  
		"	Tasks"
		"		TASK_STOP_MOVING				0"
		"		TASK_WANDER						901024"		// 90 to 1024 units
		"		TASK_WALK_PATH					0"
		"		TASK_WAIT_FOR_MOVEMENT			0"
		"		TASK_STOP_MOVING				0"
		"		TASK_WAIT						3"
		"		TASK_WAIT_RANDOM				3"
		"		TASK_SET_SCHEDULE				SCHEDULE:SCHED_CITIZEN_PATROL" // keep doing it
		""
		"	Interrupts"
		"		COND_ENEMY_DEAD"
		"		COND_LIGHT_DAMAGE"
		"		COND_HEAVY_DAMAGE"
		"		COND_HEAR_DANGER"
		"		COND_NEW_ENEMY"
	)

	DEFINE_SCHEDULE	
	(
		SCHED_CITIZEN_MOURN_PLAYER,
		  
		"	Tasks"
		"		TASK_GET_PATH_TO_PLAYER		0"
		"		TASK_RUN_PATH_WITHIN_DIST	180"
		"		TASK_WAIT_FOR_MOVEMENT		0"
		"		TASK_STOP_MOVING			0"
		"		TASK_TARGET_PLAYER			0"
		"		TASK_FACE_TARGET			0"
		"		TASK_CIT_SPEAK_MOURNING		0"
		"		TASK_SUGGEST_STATE			STATE:IDLE"
		""
		"	Interrupts"
		"		COND_LIGHT_DAMAGE"
		"		COND_HEAVY_DAMAGE"
		"		COND_HEAR_DANGER"
		"		COND_NEW_ENEMY"
	)

	DEFINE_SCHEDULE	
	(
		SCHED_CITIZEN_PLAY_INSPECT_ACTIVITY,
		  
		"	Tasks"
		"		TASK_STOP_MOVING				0"
		"		TASK_CIT_PLAY_INSPECT_SEQUENCE	0"	// Play the sequence the scanner requires
		"		TASK_WAIT						2"
		""
		"	Interrupts"
		"		"
	)

	DEFINE_SCHEDULE
	(
		SCHED_CITIZEN_SIT_ON_TRAIN,

		"	Tasks"
		"		TASK_CIT_SIT_ON_TRAIN		0"
		"		TASK_WAIT_RANDOM			1"
		"		TASK_CIT_LEAVE_TRAIN		0"
		""
		"	Interrupts"
	)

AI_END_CUSTOM_NPC()



//==================================================================================================================
// CITIZEN PLAYER-RESPONSE SYSTEM
//
// NOTE: This system is obsolete, and left here for legacy support.
//		 It has been superseded by the ai_eventresponse system.
//
//==================================================================================================================
CHandle<CCitizenResponseSystem>	g_pCitizenResponseSystem = NULL;

CCitizenResponseSystem	*GetCitizenResponse()
{
	return g_pCitizenResponseSystem;
}

char *CitizenResponseConcepts[MAX_CITIZEN_RESPONSES] = 
{
	"TLK_CITIZEN_RESPONSE_SHOT_GUNSHIP",
	"TLK_CITIZEN_RESPONSE_KILLED_GUNSHIP",
	"TLK_VITALNPC_DIED",
};

LINK_ENTITY_TO_CLASS( ai_citizen_response_system, CCitizenResponseSystem );

BEGIN_DATADESC( CCitizenResponseSystem )
	DEFINE_ARRAY( m_flResponseAddedTime, FIELD_FLOAT, MAX_CITIZEN_RESPONSES ),
	DEFINE_FIELD( m_flNextResponseTime, FIELD_FLOAT ),

	DEFINE_INPUTFUNC( FIELD_VOID,	"ResponseVitalNPC",	InputResponseVitalNPC ),

	DEFINE_THINKFUNC( ResponseThink ),
END_DATADESC()

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CCitizenResponseSystem::Spawn()
{
	if ( g_pCitizenResponseSystem )
	{
		Warning("Multiple citizen response systems in level.\n");
		UTIL_Remove( this );
		return;
	}
	g_pCitizenResponseSystem = this;

	// Invisible, non solid.
	AddSolidFlags( FSOLID_NOT_SOLID );
	AddEffects( EF_NODRAW );
	SetThink( &CCitizenResponseSystem::ResponseThink );

	m_flNextResponseTime = 0;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CCitizenResponseSystem::OnRestore()
{
	BaseClass::OnRestore();

	g_pCitizenResponseSystem = this;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CCitizenResponseSystem::AddResponseTrigger( citizenresponses_t	iTrigger )
{
	m_flResponseAddedTime[ iTrigger ] = gpGlobals->curtime;

	SetNextThink( gpGlobals->curtime + 0.1 );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CCitizenResponseSystem::InputResponseVitalNPC( inputdata_t &inputdata )
{
	AddResponseTrigger( CR_VITALNPC_DIED );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CCitizenResponseSystem::ResponseThink()
{
	bool bStayActive = false;

	CBasePlayer *pPlayer = AI_GetSinglePlayer();


	if ( pPlayer )
	{
		for ( int i = 0; i < MAX_CITIZEN_RESPONSES; i++ )
		{
			if ( m_flResponseAddedTime[i] )
			{
				// Should it have expired by now?
				if ( (m_flResponseAddedTime[i] + CITIZEN_RESPONSE_GIVEUP_TIME) < gpGlobals->curtime )
				{
					m_flResponseAddedTime[i] = 0;
				}
				else if ( m_flNextResponseTime < gpGlobals->curtime )
				{
					// Try and find the nearest citizen to the player
					float flNearestDist = (CITIZEN_RESPONSE_DISTANCE * CITIZEN_RESPONSE_DISTANCE);
					CBaseEntity *pNearestCitizen = NULL;
					CBaseEntity *pCitizen = NULL;
					//CBasePlayer *pPlayer = UTIL_GetLocalPlayer();
					while ( (pCitizen = gEntList.FindEntityByClassname( pCitizen, "npc_citizen" ) ) != NULL)
					{
						float flDistToPlayer = (pPlayer->WorldSpaceCenter() - pCitizen->WorldSpaceCenter()).LengthSqr();
						if ( flDistToPlayer < flNearestDist )
						{
							flNearestDist = flDistToPlayer;
							pNearestCitizen = pCitizen;
						}
					}

					// Found one?
					if ( pNearestCitizen && ((CNPC_Citizen*)pNearestCitizen)->RespondedTo( CitizenResponseConcepts[i], false, false ) )
					{
						m_flResponseAddedTime[i] = 0;
						m_flNextResponseTime = gpGlobals->curtime + CITIZEN_RESPONSE_REFIRE_TIME;

						// Don't issue multiple responses
						break;
					}
				}
				else
				{
					bStayActive = true;
				}
			}
		}
	}

	// Do we need to keep thinking?
	if ( bStayActive )
	{
		SetNextThink( gpGlobals->curtime + 0.1 );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Draw any debug text overlays
// Input  :
// Output : Current text offset from the top
//-----------------------------------------------------------------------------
int CNPC_Citizen::DrawDebugTextOverlays( void ) 
{
	int text_offset = BaseClass::DrawDebugTextOverlays();

	if (m_debugOverlays & OVERLAY_TEXT_BIT) 
	{
		char tempstr[512];

		Q_snprintf(tempstr,sizeof(tempstr),"Expression type: %s", szExpressionTypes[m_ExpressionType]);
		EntityText(text_offset,tempstr,0);
		text_offset++;
	}
	return text_offset;
}
