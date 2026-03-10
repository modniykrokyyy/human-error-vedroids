
#include	"cbase.h"
#include	"npcevent.h"
#include	"ai_basenpc.h"
#include	"ai_hull.h"
#include	"ai_baseactor.h"
#include	"ai_network.h"
#include	"hl2_player.h"
#include	"npc_olivia.h"

#include "te_effect_dispatch.h"
#include "particle_parse.h"


#include "ai_hint.h"
#include "ai_route.h"
#include "ai_moveprobe.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


#define HLSS_OLIVIA_GLASSES_BODYGROUP 2
#define HLSS_OLIVIA_CIG_BODYGROUP 1

int AE_OLIVIA_BLOW_SMOKE;

Activity ACT_OLIVIA_IDLE;
Activity ACT_OLIVIA_WALK;
Activity ACT_OLIVIA_RUN;

BEGIN_DATADESC( CNPC_Olivia )
	DEFINE_FIELD( m_bShouldAppear,				FIELD_BOOLEAN ),
	DEFINE_FIELD( m_flNextAppearCheckTime,		FIELD_TIME ),
	DEFINE_FIELD( m_flAppearedTime,				FIELD_TIME ),
	DEFINE_FIELD( m_flTimeToDisappear,			FIELD_TIME ),
	DEFINE_KEYFIELD( m_flDisappearTime,			FIELD_FLOAT,	"disappear_time" ),
	DEFINE_FIELD( m_AppearDestination,			FIELD_STRING ),
	DEFINE_KEYFIELD( m_flMinAppearDistance,		FIELD_FLOAT,	"minappear" ),
	DEFINE_KEYFIELD( m_flMinDisappearDistance,	FIELD_FLOAT,	"mindisappear" ),

	DEFINE_FIELD( m_bFlyBlocked,				FIELD_BOOLEAN ),
	DEFINE_FIELD( m_vOldGoal,					FIELD_POSITION_VECTOR ),

	DEFINE_FIELD( m_bDandelions,				FIELD_BOOLEAN ),
	DEFINE_FIELD( m_bSmoking,					FIELD_BOOLEAN ),
	DEFINE_FIELD( m_bOliviaLight,				FIELD_BOOLEAN ),
	DEFINE_FIELD( m_bOliviaColorCorrection,		FIELD_BOOLEAN ),
	DEFINE_FIELD( m_nSmokeAttachment,			FIELD_INTEGER ),

	DEFINE_KEYFIELD( m_flMaxFlySpeed,			FIELD_FLOAT, "max_fly_speed" ),
	DEFINE_KEYFIELD( m_flCloseFlyDistance,		FIELD_FLOAT, "close_fly_distance" ),

	DEFINE_FIELD( m_flNextCircleAroundPlayer,	FIELD_TIME ),
	DEFINE_FIELD( m_flCircleAngle,				FIELD_FLOAT ),
	DEFINE_FIELD( m_iAppearBehindPlayerTries,	FIELD_INTEGER ),

	DEFINE_FIELD( m_bWearingDress,				FIELD_BOOLEAN ),

#ifdef OLIVIA_GIVE_DAMAGE_TO_PLAYER_WITH_THINK
	DEFINE_FIELD( m_flDamageToGive,				FIELD_FLOAT ),
#endif

	// Inputs
	DEFINE_INPUTFUNC( FIELD_VOID,	"ForceAppear",		InputForceAppear ),
	DEFINE_INPUTFUNC( FIELD_VOID,	"ForceDisappear",	InputForceDisappear ),
	DEFINE_INPUTFUNC( FIELD_VOID,	"Appear",			InputAppear ),
	DEFINE_INPUTFUNC( FIELD_VOID,	"Disappear",		InputDisappear ),
	DEFINE_INPUTFUNC( FIELD_STRING,	"FindBestAppear",	InputBestAppear ),
	DEFINE_INPUTFUNC( FIELD_FLOAT,	"MinDisappear",		InputMinDisappear ),
	DEFINE_INPUTFUNC( FIELD_FLOAT,	"MinAppear",		InputMinAppear ),
	DEFINE_INPUTFUNC( FIELD_FLOAT,	"SetDisappearTime",	InputSetDisappearTime ),
	DEFINE_INPUTFUNC( FIELD_VOID,	"StartDandelions",	InputStartDandelions ),
	DEFINE_INPUTFUNC( FIELD_VOID,	"StopDandelions",	InputStopDandelions ),
	DEFINE_INPUTFUNC( FIELD_VOID,	"Land",				InputLand ),

	DEFINE_INPUTFUNC( FIELD_VOID,	"StartCircleAroundPlayer",	InputStartCircleAroundPlayer ),
	DEFINE_INPUTFUNC( FIELD_VOID,	"StopCircleAroundPlayer",	InputStopCircleAroundPlayer ),
	DEFINE_INPUTFUNC( FIELD_VOID,	"AppearBehindPlayer",		InputAppearBehindPlayer ),

	// Outputs
	DEFINE_OUTPUT( m_OnAppear,		"OnAppear" ),
	DEFINE_OUTPUT( m_OnDisappear,	"OnDisappear" ),
	DEFINE_OUTPUT( m_OnPlayerUse,	"OnPlayerUse" ),

	DEFINE_USEFUNC( OliviaUse ),
END_DATADESC()

LINK_ENTITY_TO_CLASS( npc_olivia, CNPC_Olivia );

IMPLEMENT_SERVERCLASS_ST( CNPC_Olivia, DT_NPC_Olivia )
	SendPropBool( SENDINFO( m_bDandelions )),
	SendPropBool( SENDINFO ( m_bSmoking ) ),
	SendPropBool( SENDINFO ( m_bOliviaLight ) ),
	SendPropBool( SENDINFO ( m_bOliviaColorCorrection ) ),
	SendPropInt(SENDINFO( m_nSmokeAttachment ) ),
	SendPropFloat(SENDINFO( m_flAppearedTime ) ),
END_SEND_TABLE()

CNPC_Olivia::CNPC_Olivia()
{
	m_bShouldAppear = false;
	m_flMinAppearDistance = 64.0f;
	m_flMinDisappearDistance = 64.0f;
	m_AppearDestination = NULL_STRING;

	m_flMaxFlySpeed			= 400.0f;
	m_flCloseFlyDistance	= 512.0f;

	m_flTimeToDisappear = 0;
	m_flDisappearTime = 1.0f;

	m_bFlyBlocked = false;

	m_bDandelions = false;
	m_bSmoking = false;

	m_bOliviaLight = true;

	m_nSmokeAttachment = -1;

	m_flNextCircleAroundPlayer = 0;
	m_flCircleAngle = 0;
	m_iAppearBehindPlayerTries = 0;

#ifdef OLIVIA_GIVE_DAMAGE_TO_PLAYER_WITH_THINK
	m_flDamageToGive = 0;
#endif

	m_bWearingDress = false;
}

void CNPC_Olivia::InputStartCircleAroundPlayer( inputdata_t &inputdata )
{
	m_flNextCircleAroundPlayer = gpGlobals->curtime;
}

void CNPC_Olivia::InputStopCircleAroundPlayer( inputdata_t &inputdata )
{
	m_flNextCircleAroundPlayer = 0;
}

//---------------------------------------------------------
//---------------------------------------------------------
void CNPC_Olivia::GatherConditions( void )
{
	if (m_flNextCircleAroundPlayer == 0)
	{
		SetCondition( COND_OLIVIA_STOP_CIRCLE_AROUND_PLAYER );
	}
	else
	{
		ClearCondition( COND_OLIVIA_STOP_CIRCLE_AROUND_PLAYER );
	}

	BaseClass::GatherConditions();
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
int CNPC_Olivia::SelectSchedule()
{
	if (!HasCondition(COND_OLIVIA_STOP_CIRCLE_AROUND_PLAYER))
	{
		return SCHED_OLIVIA_CIRCLE_AROUND_PLAYER;
	}

	return BaseClass::SelectSchedule();
}

void CNPC_Olivia::StartTask( const Task_t *pTask )
{
	switch ( pTask->iTask )
	{
	case TASK_OLIVIA_CIRCLE_AROUND_PLAYER:
		{
			CircleAroundPlayer();
		}
		break;

	default:
		BaseClass::StartTask( pTask );
		break;
	}
}

void CNPC_Olivia::CircleAroundPlayer()
{
	CBasePlayer *pPlayer = UTIL_PlayerByIndex(1);

	if (pPlayer)
	{
		if (m_flNextCircleAroundPlayer > gpGlobals->curtime)
			return;

		m_flNextCircleAroundPlayer = gpGlobals->curtime + 0.5f;


		//TERO: start the actual shit

		Vector vecForward;
		pPlayer->GetVectors(&vecForward, NULL, NULL);
		vecForward.z = 0;

		Vector vecPlayer = GetAbsOrigin() - pPlayer->GetAbsOrigin();
		vecPlayer.z = 0;

		float flDist = VectorNormalize(vecPlayer);

		QAngle angPlayer;

		VectorAngles(vecPlayer, angPlayer);
		
		Vector vecPos;

		float flDot = DotProduct( vecPlayer, -vecForward );

		//DevMsg("dot product %f\n", flDot);

		if ( flDot > -0.8)
		{
			//find new angle
			if (m_flCircleAngle == 0)
			{
				QAngle angForward;
				VectorAngles(vecForward, angForward);

				float diff = angForward.y - angPlayer.y;

				if (diff < 0)
				{
					m_flCircleAngle = -45;
				}
				else
				{
					m_flCircleAngle = 45;
				}

				//DevMsg("Diff was %f, new circle angle is %f\n", diff, m_flCircleAngle);
			}

			float flOriginal = angPlayer.y;

			angPlayer.y = flOriginal + m_flCircleAngle;
			AngleVectors(angPlayer, &vecPlayer);
			vecPos = pPlayer->GetAbsOrigin() + (vecPlayer * OLIVIA_PLAYER_CIRCLE_DISTANCE);

			//AI_NavGoal_t  goal (GOALTYPE_LOCATION, vecPos, ACT_WALK, AIN_HULL_TOLERANCE );

			if (GetNavigator()->SetGoal( vecPos ))
			{
				return;
			}

			m_flCircleAngle = -m_flCircleAngle;
			angPlayer.y = flOriginal + m_flCircleAngle;
			AngleVectors(angPlayer, &vecPlayer);
			vecPos = pPlayer->GetAbsOrigin() + (vecPlayer * OLIVIA_PLAYER_CIRCLE_DISTANCE);

			if (GetNavigator()->SetGoal( vecPos ))
			{
				return;
			}

			m_flCircleAngle = 0;
		}
		else if (flDist > (OLIVIA_PLAYER_CIRCLE_DISTANCE * 1.5f))
		{
			vecPos = pPlayer->GetAbsOrigin() + (vecPlayer * OLIVIA_PLAYER_CIRCLE_DISTANCE);

			//AI_NavGoal_t  goal (GOALTYPE_LOCATION, vecPos, ACT_WALK, AIN_HULL_TOLERANCE );

			GetNavigator()->SetGoal( vecPos );
		}

		m_flCircleAngle = 0;
	}
}

//---------------------------------------------------------
//---------------------------------------------------------
void CNPC_Olivia::RunTask( const Task_t *pTask )
{


	switch( pTask->iTask )
	{
	case TASK_OLIVIA_CIRCLE_AROUND_PLAYER:
		{
			if (m_flNextCircleAroundPlayer != 0)
			{
				CircleAroundPlayer();
			}
			else
			{
				TaskComplete();
			}



			/*if ( GetNavigator()->SetGoal( GOALTYPE_ENEMY ) )


			bool fTimeExpired = ( pTask->flTaskData != 0 && pTask->flTaskData < gpGlobals->curtime - GetTimeTaskStarted() );
			
			if (fTimeExpired || GetNavigator()->GetGoalType() == GOALTYPE_NONE)
			{
				TaskComplete();
				GetNavigator()->StopMoving();		// Stop moving
			}
			else if (!GetNavigator()->IsGoalActive())
			{
				SetIdealActivity( GetStoppedActivity() );
			}
			else
			{
				// Check validity of goal type
				ValidateNavGoal();
			}*/
			break;
		}
		default:
			BaseClass::RunTask( pTask );
			break;
	}
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CNPC_Olivia::OliviaUse( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	DevMsg("Olivia use\n");
	m_OnPlayerUse.FireOutput( pActivator, pCaller );
}

void CNPC_Olivia::Appear()
{
	if (HasSpawnFlags(SF_OLIVIA_INVISIBLE))
	{
		RemoveSpawnFlags(SF_OLIVIA_INVISIBLE);

		RemoveEffects( EF_NODRAW );
		SetSolid( SOLID_BBOX );
		RemoveSolidFlags( FSOLID_NOT_SOLID );

		m_OnAppear.FireOutput( this, this );

		m_bShouldAppear = false;

		m_iAppearBehindPlayerTries = 0;

		if (HasSpawnFlags(SF_OLIVIA_SMOKING))
		{
			m_bSmoking = true;
		}

		if (HasSpawnFlags(SF_OLIVIA_LIGHT))
		{
			m_bOliviaLight = true;
			m_flAppearedTime = gpGlobals->curtime;
		}

		if (HasSpawnFlags(SF_OLIVIA_COLOR_CORRECTION))
		{
			m_bOliviaColorCorrection = true;
		}

		if (HasSpawnFlags(SF_OLIVIA_DANDELIONS)) //m_bDandelions)
		{
			m_bDandelions = true;
		}
	}
}

void CNPC_Olivia::InputAppearBehindPlayer( inputdata_t &inputdata )
{
	if (HasSpawnFlags(SF_OLIVIA_INVISIBLE))
	{
		m_iAppearBehindPlayerTries = 5;

		m_AppearDestination = MAKE_STRING(inputdata.value.String());
		m_bShouldAppear = true;
		m_flNextAppearCheckTime = 0;
	}
}

void CNPC_Olivia::Disappear()
{
	if (!HasSpawnFlags(SF_OLIVIA_INVISIBLE))
	{
		AddSpawnFlags(SF_OLIVIA_INVISIBLE);

		AddEffects( EF_NODRAW );
		SetSolid( SOLID_NONE );
		AddSolidFlags( FSOLID_NOT_SOLID );

		m_OnDisappear.FireOutput( this, this );

		m_bShouldAppear = false;

		m_iAppearBehindPlayerTries = 0;
		m_flNextCircleAroundPlayer = 0;

		m_bOliviaLight = false;
		m_bOliviaColorCorrection = false;

		m_bSmoking = false;
		m_bDandelions = false;
	}
}

void CNPC_Olivia::InputForceAppear( inputdata_t &inputdata )
{
	Appear();
}

void CNPC_Olivia::InputForceDisappear( inputdata_t &inputdata )
{
	Disappear();
}

void CNPC_Olivia::InputAppear( inputdata_t &inputdata )
{
	if (HasSpawnFlags(SF_OLIVIA_INVISIBLE))
	{
		m_bShouldAppear = true;
		m_flNextAppearCheckTime = 0;
	}
}

void CNPC_Olivia::InputDisappear( inputdata_t &inputdata )
{
	if (!HasSpawnFlags(SF_OLIVIA_INVISIBLE))
	{
		m_bShouldAppear = true;
		m_flNextAppearCheckTime = 0;
	}
}

void CNPC_Olivia::InputBestAppear( inputdata_t &inputdata )
{
	if (HasSpawnFlags(SF_OLIVIA_INVISIBLE))
	{
		m_AppearDestination = MAKE_STRING(inputdata.value.String());
		m_bShouldAppear = true;
		m_flNextAppearCheckTime = 0;
	}
}

void CNPC_Olivia::InputMinDisappear( inputdata_t &inputdata )
{
	m_flMinDisappearDistance = inputdata.value.Float();
}

void CNPC_Olivia::InputMinAppear( inputdata_t &inputdata )
{
	m_flMinAppearDistance = inputdata.value.Float();
}

void CNPC_Olivia::InputSetDisappearTime( inputdata_t &inputdata )
{
	m_flDisappearTime = inputdata.value.Float();
}


void CNPC_Olivia::NPCThink( void )
{
	/*if (HasSpawnFlags(SF_OLIVIA_SMOKING))
	{
		m_bSmoking = true;
		RemoveSpawnFlags(SF_OLIVIA_SMOKING);
	}*/

	BaseClass::NPCThink();

	CBasePlayer *pPlayer = UTIL_PlayerByIndex(1);

	if (!pPlayer)
		return;

	if (m_bShouldAppear && m_flNextAppearCheckTime < gpGlobals->curtime)
	{
		m_flNextAppearCheckTime = gpGlobals->curtime + 0.1f;

		if ( m_iAppearBehindPlayerTries > 0 )
		{
			Vector vecPlayer, vecPosition;
			QAngle angPlayer;

			pPlayer->GetVectors(&vecPlayer,NULL,NULL);
			vecPlayer.z = 0;
			VectorAngles(vecPlayer,angPlayer);
			angPlayer.z = angPlayer.x = 0;

			bool bFailed= true;

			QAngle angTry(0,0,0);

			Vector vecTopOfHull = NAI_Hull::Maxs( HULL_HUMAN );
			vecTopOfHull.x = 0;
			vecTopOfHull.y = 0;

			for (int i=-1; (i<2 && bFailed); i++)
			{
				angTry.y = angPlayer.y + (i * 45.0f);
				DevMsg("attempt %d, angle %f\n", i, angTry.y);
				AngleVectors(angTry, &vecPlayer);
				vecPosition = pPlayer->GetAbsOrigin() - (vecPlayer * OLIVIA_PLAYER_CIRCLE_DISTANCE);
			
				if ( pPlayer->FVisible( vecPosition ) || pPlayer->FVisible( vecPosition + vecTopOfHull ) )
				{
					//TERO: even if player is facing us, it's ok as long as he doesn't see us
					trace_t tr;
					UTIL_TraceHull( vecPosition + Vector(0,0,12),
					vecPosition,
					NAI_Hull::Mins(HULL_HUMAN),
					NAI_Hull::Maxs(HULL_HUMAN),
					MASK_NPCSOLID,
					this,
					COLLISION_GROUP_NPC,
					&tr );

					if ( tr.fraction > 0.1 )
					{
						bFailed = false;
						vecPosition = tr.endpos;
					}
				}
			}

			if( bFailed ) //tr.fraction < 0.1 )
			{

				if (m_AppearDestination != NULL_STRING )
				{
					m_iAppearBehindPlayerTries--;
					DevMsg("Appearing behind player failed, tries left %d\n", m_iAppearBehindPlayerTries);
				}
			}
			else
			{
				DevMsg("appearing behind player SUCCESS\n");

				m_bShouldAppear = false;

				m_OnAppear.FireOutput( this, this );

				Vector vecVelocity = Vector(0,0,0);
				Teleport( &vecPosition, &angPlayer, &vecVelocity );

				Appear();
			}
		}
		//Check if we want to do best appear destination
		else if (m_AppearDestination != NULL_STRING && HasSpawnFlags(SF_OLIVIA_INVISIBLE))
		{


			CBaseEntity *pDestination = gEntList.FindEntityByName( NULL, m_AppearDestination );
			CBaseEntity *pBest = NULL;
			float flBestDist = 4096; //Magic number
			float flDist;

#ifdef DEBUG_OLIVIA_BEST_APPEAR
			DevMsg(" \n");

			//Debug, remove later
			int i = 0;
			int iBest = 0;
#endif

			while (pDestination)
			{
#ifdef DEBUG_OLIVIA_BEST_APPEAR
				i++;
				DevMsg("npc_olivia: found destination %d entity %s\n", i, m_AppearDestination);
				NDebugOverlay::Box( pDestination->GetAbsOrigin(), Vector(10,10,10), Vector(-10,-10,-10), 255, 0, 0, 0, 5 );
#endif

				bool bCanSpawnHere = false; 

				Vector vecForward, vecDir;

				pPlayer->GetVectors(&vecForward, NULL, NULL);
				vecDir = (pDestination->GetAbsOrigin() - pPlayer->GetAbsOrigin());
				flDist = VectorNormalize(vecDir) * 4.0f; //magic!

				SetAbsOrigin( pDestination->GetAbsOrigin() );
				if (GetNavigator()->SetGoal( pPlayer->GetAbsOrigin() ))
				{
					flDist = GetNavigator()->GetPathDistanceToGoal();
				}

				if ( flDist > m_flMinAppearDistance && flDist < flBestDist )
				{

									//!pPlayer->FInViewCone( pDestination->GetAbsOrigin() ); 
					if (DotProduct( vecDir, vecForward ) < 0.5)
					{
						bCanSpawnHere = true;
					}

					//TERO: even if player is facing us, it's ok as long as he doesn't see us
					if (!bCanSpawnHere)
					{

#ifdef DEBUG_OLIVIA_BEST_APPEAR
						DevMsg("npc_olivia: player facing destination, dot product %f\n", DotProduct( vecDir, vecForward ));
#endif

						Vector vecTest = pDestination->GetAbsOrigin();
						Vector vecTopOfHull = NAI_Hull::Maxs( HULL_HUMAN );
						vecTopOfHull.x = 0;
						vecTopOfHull.y = 0;
						bCanSpawnHere = ((!pPlayer->FVisible( vecTest )) && (!pPlayer->FVisible( vecTest + vecTopOfHull )) );
						
#ifdef DEBUG_OLIVIA_BEST_APPEAR
						if (!bCanSpawnHere)
						{
							DevMsg("npc_olivia: player can see destination\n");
						}
#endif
					}
				}

#ifdef DEBUG_OLIVIA_BEST_APPEAR
				else
				{
					DevMsg("npc_olivia: we already have a better, closer destination\n");
				}
#endif

				if (bCanSpawnHere)
				{
					trace_t tr;
					UTIL_TraceHull( pDestination->GetAbsOrigin(),
					pDestination->GetAbsOrigin() + Vector( 0, 0, 1 ),
					NAI_Hull::Mins(HULL_HUMAN),
					NAI_Hull::Maxs(HULL_HUMAN),
					MASK_NPCSOLID,
					this,
					COLLISION_GROUP_NPC,
					&tr );

					if( tr.fraction != 1.0 )
					{

#ifdef DEBUG_OLIVIA_BEST_APPEAR
						DevMsg("npc_olivia: can't spawn here because of collision\n");
#endif

						bCanSpawnHere = false;
					}
				}

				if (bCanSpawnHere)
				{
					pBest = pDestination;
					flBestDist = flDist;

#ifdef DEBUG_OLIVIA_BEST_APPEAR
					iBest = i;
					NDebugOverlay::Line( pDestination->GetAbsOrigin(), pDestination->GetAbsOrigin() + Vector(0,0,72), 255, 255, 0, true, 5 );
					DevMsg("npc_olivia: can spawn here %d!\n", i);
#endif

				}

				pDestination = gEntList.FindEntityByName( pDestination, m_AppearDestination );
			}

			//We have found the best destination, lets appear there
			if (pBest)
			{
#ifdef DEBUG_OLIVIA_BEST_APPEAR
				DevMsg("npc_olivia: spawning on destination %d\n", iBest);
#endif

				m_bShouldAppear = false;

				m_OnAppear.FireOutput( this, this );

				//SetAbsOrigin( pBest->GetAbsOrigin() );
				//SetAbsAngles( pBest->GetAbsAngles() );

				Vector vecPosition = pBest->GetAbsOrigin();
				QAngle angAngles = pBest->GetAbsAngles();
				Vector vecVelocity = Vector(0,0,0);
				Teleport( &vecPosition, &angAngles, &vecVelocity );

				/*RemoveSpawnFlags(SF_OLIVIA_INVISIBLE);

				RemoveEffects( EF_NODRAW );
				SetSolid( SOLID_BBOX );
				RemoveSolidFlags( FSOLID_NOT_SOLID );

				m_AppearDestination = NULL_STRING;*/
				
				Appear();
			}
		}
		else
		{
			//Do normal appear or disappear
			Vector vecForward, vecDir;

			pPlayer->GetVectors(&vecForward, NULL, NULL);
			vecDir = (GetAbsOrigin() - pPlayer->GetAbsOrigin());
			float flDist = VectorNormalize(vecDir);

			DevMsg("npc_olivia: dot product %f\n", DotProduct( vecDir, vecForward ));
			DevMsg("npc_olivia: distance %f, min disappear %f, min appear %f\n", flDist, m_flMinDisappearDistance, m_flMinAppearDistance);

			//If we are not seen by the player or player is not facing us, disappear
			if ( (DotProduct( vecDir, vecForward ) < 0.5) || !(pPlayer->FVisible( this )))
			{
				if (!HasSpawnFlags(SF_OLIVIA_INVISIBLE) && (flDist > m_flMinDisappearDistance))
				{
					if (m_flTimeToDisappear != 0 )
					{
						if (m_flTimeToDisappear < gpGlobals->curtime)
						{
							Disappear();
						}
					}
					else 
					{
						m_flTimeToDisappear = gpGlobals->curtime + m_flDisappearTime;
					}
				}
				else if (flDist > m_flMinAppearDistance)
				{
					Appear();
				}
			}
			else
			{
				m_flTimeToDisappear = 0;
			}
		}

	} //END SHOULD APPEAR
	else
	{
		if (m_flNextCircleAroundPlayer != 0 && m_flNextAppearCheckTime < gpGlobals->curtime)
		{
			m_flNextAppearCheckTime = gpGlobals->curtime + 0.1f;

			Vector forward, vecDist;

			pPlayer->GetVectors(&forward, NULL, NULL);
			vecDist = GetAbsOrigin() - pPlayer->GetAbsOrigin();

			float flDist = VectorNormalize(vecDist);
			float flDot = DotProduct( vecDist, forward );

			//DevMsg("flDot %f, flDist %f\n", flDot, flDist);

			if ((flDist > 256.0f && (flDot < 0.3 || !pPlayer->FVisible( this ))))
			{
				//TERO: we should now try to teleport behind the player
				Vector vecPlayer, vecPosition;
				QAngle angPlayer;

				pPlayer->GetVectors(&vecPlayer,NULL,NULL);
				vecPlayer.z = 0;
				VectorAngles(vecPlayer,angPlayer);
				angPlayer.z = angPlayer.x = 0;

				bool bFailed= true;

				QAngle angTry(0,0,0);

				Vector vecTopOfHull = NAI_Hull::Maxs( HULL_HUMAN );
				vecTopOfHull.x = 0;
				vecTopOfHull.y = 0;

				for (int i=-1; (i<2 && bFailed); i++)
				{
					angTry.y = angPlayer.y + (i * 45.0f);
					AngleVectors(angTry, &vecPlayer);
					vecPosition = pPlayer->GetAbsOrigin() - (vecPlayer * OLIVIA_PLAYER_CIRCLE_DISTANCE);
			
					if ( pPlayer->FVisible( vecPosition ) || pPlayer->FVisible( vecPosition + vecTopOfHull ) )
					{
						//TERO: even if player is facing us, it's ok as long as he doesn't see us
						trace_t tr;
						UTIL_TraceHull( vecPosition + Vector(0,0,12),
						vecPosition,
						NAI_Hull::Mins(HULL_HUMAN),
						NAI_Hull::Maxs(HULL_HUMAN),
						MASK_NPCSOLID,
						this,
						COLLISION_GROUP_NPC,
						&tr );

						if ( tr.fraction > 0.1 )
						{
							/*NDebugOverlay::Box( vecPosition, Vector(-4,-4,-4), Vector(4,4,4),	255, 0, 0, 0, 1.0 );

							NDebugOverlay::Box( tr.endpos, Vector(-4,-4,-4), Vector(4,4,4),		0, 0, 255, 0, 1.0 );*/

							vecPosition = tr.endpos;

							Vector vecVelocity = Vector(0,0,0);
							Teleport( &vecPosition, &angPlayer, &vecVelocity );
							m_flNextCircleAroundPlayer = gpGlobals->curtime; //TERO: update this so we don't try to walk to our previous location

							bFailed = false;
						}
					}
				}
			}
		}
	}

	/*Vector vecTest = GetAbsOrigin();
	float bigA = 32.0f;
	float bigB = 32.0f;
	float a = 5.0f;
	float b = 4.0f;
	float delta = M_PI * 0.5f;
	float t = gpGlobals->curtime;
	float x, y;

	x = (bigA * sin( (a*t) + delta));
	y = (bigB * sin( (b*t) ));
	vecTest.x += x;
	vecTest.y += y;
	vecTest.z += (35 + (sin( 3 * t ) * 70));

	NDebugOverlay::Line( vecTest, m_vTest, 255, 0, 0, 0, 5.0f );

	m_vTest = vecTest;*/

#ifdef OLIVIA_GIVE_DAMAGE_TO_PLAYER_WITH_THINK
	if (m_flDamageToGive != 0)
	{
		DevMsg("npc_olivia: damage to give to player: %f\n", m_flDamageToGive);
		if (pPlayer)
		{
			//pPlayer->m_iHealth = pPlayer->m_iHealth - (int)m_flDamageToGive;
			pPlayer->TakeDamage( CTakeDamageInfo( this, this, (int)m_flDamageToGive, DMG_GENERIC ) );
		}

		RemoveAllDecals();
		m_flDamageToGive = 0;
	}
#endif
}

//-----------------------------------------------------------------------------
// Classify - indicates this NPC's place in the 
// relationship table.
//-----------------------------------------------------------------------------
Class_T	CNPC_Olivia::Classify ( void )
{
	return	CLASS_NONE; //PLAYER_ALLY_VITAL;
}



//-----------------------------------------------------------------------------
// HandleAnimEvent - catches the NPC-specific messages
// that occur when tagged animation frames are played.
//-----------------------------------------------------------------------------
void CNPC_Olivia::HandleAnimEvent( animevent_t *pEvent )
{
	if ( pEvent->event == AE_OLIVIA_BLOW_SMOKE )
	{
		Vector vecOrigin;
		QAngle vecAngles;

		GetAttachment( "mouth", vecOrigin, vecAngles );
		DispatchParticleEffect( "he_blow_smoke", vecOrigin, vecAngles );
		EmitSound("NPC_Olivia.Smoke");
		return;
	}
	
	BaseClass::HandleAnimEvent( pEvent );
}

//-----------------------------------------------------------------------------
// GetSoundInterests - generic NPC can't hear.
//-----------------------------------------------------------------------------
int CNPC_Olivia::GetSoundInterests ( void )
{
	return	NULL;
}

//-----------------------------------------------------------------------------
// Spawn
//-----------------------------------------------------------------------------
void CNPC_Olivia::Spawn()
{
	// Eli is allowed to use multiple models, because he appears in the pod.
	// He defaults to his normal model.
	Precache();
	SetModel( STRING( GetModelName() ) );
	//SetModel( "models/olivia.mdl" );

	BaseClass::Spawn();

	CapabilitiesAdd( bits_CAP_ANIMATEDFACE | bits_CAP_TURN_HEAD );
	CapabilitiesAdd( bits_CAP_FRIENDLY_DMG_IMMUNE );

	if (HasSpawnFlags(SF_OLIVIA_FLY))
	{
		if (HasSpawnFlags(SF_OLIVIA_INVISIBLE))
		{
			AddSolidFlags( FSOLID_NOT_SOLID );
			SetSolid( SOLID_NONE );
			AddEffects( EF_NODRAW );
		}
		else
		{
			SetSolid( SOLID_BBOX );
		}

		m_vOldGoal = vec3_origin;

		SetHullType( HULL_HUMAN_CENTERED ); 
		SetHullSizeNormal();

		SetCollisionBounds( OLIVIA_HULL_MINS, OLIVIA_HULL_MAXS );

		SetGroundEntity( NULL );

		SetNavType( NAV_FLY );

		AddFlag( FL_FLY );
		RemoveFlag( FL_ONGROUND ); 
		
		CapabilitiesAdd ( bits_CAP_MOVE_FLY  | bits_CAP_SKIP_NAV_GROUND_CHECK ); 
		CapabilitiesRemove( bits_CAP_MOVE_GROUND );

		SetMoveType( MOVETYPE_STEP ); //TERO: crow has MOVETYPE_STEP here
	}
	else if ( GetMoveParent() )
	{
		SetHullType(HULL_HUMAN);
		SetHullSizeNormal();

		if (HasSpawnFlags(SF_OLIVIA_INVISIBLE))
		{
			AddSolidFlags( FSOLID_NOT_SOLID );
			SetSolid( SOLID_NONE );
			AddEffects( EF_NODRAW );
		}
		else
		{
			SetSolid( SOLID_BBOX );
		}

		AddSolidFlags( FSOLID_NOT_STANDABLE );
		SetMoveType( MOVETYPE_NONE );
	}
	else
	{
		SetHullType(HULL_HUMAN);
		SetHullSizeNormal();

		SetupWithoutParent();
	}

	AddEFlags( EFL_NO_DISSOLVE | EFL_NO_MEGAPHYSCANNON_RAGDOLL | EFL_NO_PHYSCANNON_INTERACTION );
	m_iHealth			= 1000;
	m_flFieldOfView		= 0.5;// indicates the width of this NPC's forward view cone ( as a dotproduct result )
	m_NPCState			= NPC_STATE_NONE;

	m_bShouldAppear = false;
	m_flNextAppearCheckTime = gpGlobals->curtime;

	m_flAppearedTime = gpGlobals->curtime - 1.0f;

	int iDress = LookupAttachment("dress");

	if (iDress > 0)
	{
		DevMsg("dress attachment id %d\n", iDress);
		m_bWearingDress = true;
	}
	else
	{
		m_bWearingDress = false;
	}

	NPCInit();

	if (HasSpawnFlags(SF_OLIVIA_INFLICT_DAMAGE_ON_PLAYER))
	{
		SetBloodColor( DONT_BLEED );
	}
	else if (!HasSpawnFlags(SF_OLIVIA_TAKE_DAMAGE))
	{
		SetBloodColor( DONT_BLEED );
		m_takedamage		= DAMAGE_NO;
	}
	else
	{
		SetBloodColor( BLOOD_COLOR_RED );
	}

	if (HasSpawnFlags(SF_OLIVIA_GLASSES))
	{
		SetBodygroup( HLSS_OLIVIA_GLASSES_BODYGROUP, true );
	}
	else
	{
		SetBodygroup( HLSS_OLIVIA_GLASSES_BODYGROUP, false );
	}

	if (!HasSpawnFlags(SF_OLIVIA_INVISIBLE) && HasSpawnFlags(SF_OLIVIA_DANDELIONS)) //m_bDandelions)
	{
		m_bDandelions = true;
	}
	else
	{
		m_bDandelions = false;
	}

	if (!HasSpawnFlags(SF_OLIVIA_INVISIBLE) && HasSpawnFlags(SF_OLIVIA_LIGHT))
	{
		m_bOliviaLight = true;
	}
	else
	{
		m_bOliviaLight = false;
	}

	if (!HasSpawnFlags(SF_OLIVIA_INVISIBLE) && HasSpawnFlags(SF_OLIVIA_COLOR_CORRECTION))
	{
		m_bOliviaColorCorrection = true;
	}
	else
	{
		m_bOliviaColorCorrection = false;
	}

	if (HasSpawnFlags(SF_OLIVIA_SMOKING))
	{
		if (!HasSpawnFlags(SF_OLIVIA_INVISIBLE))
		{
			m_bSmoking = true;
		}
		m_nSmokeAttachment = LookupAttachment("cigarette");
		SetBodygroup( HLSS_OLIVIA_CIG_BODYGROUP, true );
	}
	else
	{
		SetBodygroup( HLSS_OLIVIA_CIG_BODYGROUP, false );
	}

	SetUse( &CNPC_Olivia::OliviaUse );
}

void CNPC_Olivia::TraceAttack( const CTakeDamageInfo &info, const Vector &vecDir, trace_t *ptr, CDmgAccumulator *pAccumulator )
{
	if (HasSpawnFlags(SF_OLIVIA_INVISIBLE))
	{
		return;
	}

	if (HasSpawnFlags(SF_OLIVIA_TAKE_DAMAGE) || HasSpawnFlags(SF_OLIVIA_INFLICT_DAMAGE_ON_PLAYER))
	{
		BaseClass::TraceAttack( info, vecDir, ptr, pAccumulator );
	}
}

int	CNPC_Olivia::OnTakeDamage_Alive( const CTakeDamageInfo &info )
{
	if (HasSpawnFlags(SF_OLIVIA_INVISIBLE))
	{
		return 0;
	}

#ifdef OLIVIA_GIVE_DAMAGE_TO_PLAYER_WITH_THINK
	if (HasSpawnFlags(SF_OLIVIA_INFLICT_DAMAGE_ON_PLAYER))
	{	
		//CBaseEntity *pPlayer = info.GetAttacker();
		/*CBasePlayer *pPlayer = ToBasePlayer( info.GetAttacker() );

		if (pPlayer)
		{
			Vector vecDir = pPlayer->WorldSpaceCenter()  - WorldSpaceCenter();
			VectorNormalize( vecDir );
			vecDir *= info.GetDamageForce().Length();

			CTakeDamageInfo inflictDamage;
			inflictDamage.SetDamage( info.GetDamage() );
			inflictDamage.SetAttacker( this );
			inflictDamage.SetInflictor( this );
			inflictDamage.SetDamageType( info.GetDamageType() );
			inflictDamage.SetDamageForce( vecDir );
			inflictDamage.SetDamagePosition( pPlayer->WorldSpaceCenter() ); //info.GetDamagePosition() );
			pPlayer->TakeDamage( inflictDamage );
			pPlayer->TakeDamage( CTakeDamageInfo( pPlayer, pPlayer, 25, DMG_GENERIC ) );
		}*/

		if (info.GetAttacker() && info.GetAttacker()->IsPlayer())
		{
			m_flDamageToGive += (info.GetDamage() * OLIVIA_DAMAGE_INFLIC_SCALAR);
		}
	}
	else
#endif
	if (HasSpawnFlags(SF_OLIVIA_TAKE_DAMAGE))
	{
		return BaseClass::OnTakeDamage_Alive( info );
	}

	return 0;
}

//-----------------------------------------------------------------------------
// Precache - precaches all resources this NPC needs
//-----------------------------------------------------------------------------
void CNPC_Olivia::Precache()
{
	PrecacheParticleSystem( "dandelions_olivia" );
	PrecacheParticleSystem( "he_cigarette" );
	PrecacheParticleSystem( "he_blow_smoke" );

	PrecacheScriptSound("NPC_Olivia.Smoke");

	if (GetModelName() == NULL_STRING)
	{
		SetModelName( MAKE_STRING( "models/olivia.mdl" ) );
	}

	PrecacheModel( STRING( GetModelName() ) );
	BaseClass::Precache();
}	

/*void CNPC_Olivia::ParticleMessages(bool bStart)
{
	if (bStart)
	{
		EntityMessageBegin( this, true );
			WRITE_BYTE( 0 );
		MessageEnd();
	}
	else
	{
		EntityMessageBegin( this, true );
			WRITE_BYTE( 1 );
		MessageEnd();
	}
}

void CNPC_Olivia::OnRestore()
{
	BaseClass::OnRestore();

	if (HasSpawnFlags(SF_OLIVIA_DANDELIONS))
	{
		ParticleMessages(true);
	}
}*/

void CNPC_Olivia::InputStartDandelions( inputdata_t &inputdata )
{
	m_bDandelions = true;
	AddSpawnFlags(SF_OLIVIA_DANDELIONS);
	//ParticleMessages( true );
}

void CNPC_Olivia::InputStopDandelions( inputdata_t &inputdata )
{
	m_bDandelions = false;
	RemoveSpawnFlags(SF_OLIVIA_DANDELIONS);
	//ParticleMessages( false );
}

void CNPC_Olivia::InputLand( inputdata_t &inputdata )
{
	//We are going to walk
	SetNavType( NAV_GROUND );

	RemoveFlag( FL_FLY );
		
	CapabilitiesAdd ( bits_CAP_MOVE_GROUND );
	CapabilitiesRemove( bits_CAP_MOVE_FLY ); //| bits_CAP_SKIP_NAV_GROUND_CHECK ); //| bits_CAP_SKIP_NAV_GROUND_CHECK 
	SetMoveType( MOVETYPE_STEP );

	RemoveSpawnFlags( SF_OLIVIA_FLY );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_Olivia::SetupWithoutParent( void )
{
	if (HasSpawnFlags(SF_OLIVIA_INVISIBLE))
	{
		AddSolidFlags( FSOLID_NOT_SOLID );
		SetSolid( SOLID_NONE );
		AddEffects( EF_NODRAW );
	}
	else
	{
		RemoveSolidFlags( FSOLID_NOT_SOLID );
		RemoveEffects( EF_NODRAW );
		SetSolid( SOLID_BBOX );
	}

	AddSolidFlags( FSOLID_NOT_STANDABLE );

	SetMoveType( MOVETYPE_STEP );

	CapabilitiesAdd( bits_CAP_MOVE_GROUND | bits_CAP_OPEN_DOORS | bits_CAP_ANIMATEDFACE | bits_CAP_TURN_HEAD );
	CapabilitiesAdd( bits_CAP_FRIENDLY_DMG_IMMUNE );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_Olivia::PrescheduleThink( void )
{
	BaseClass::PrescheduleThink();

	// Figure out if Eli has just been removed from his parent
	if ( GetMoveType() == MOVETYPE_NONE && !GetMoveParent() )
	{
		SetupWithoutParent();
		SetupVPhysicsHull();
	}
}

//-----------------------------------------------------------------------------
// AI Schedules Specific to this NPC
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CNPC_Olivia::OliviaFly(float flInterval, Vector vMoveTargetPos, Vector vMoveGoal )
{
	Vector vecMoveDir = ( vMoveTargetPos - (GetAbsOrigin() + OLIVIA_BODY_CENTER) );
	Vector vecGoalDir = ( vMoveGoal		 - (GetAbsOrigin() + OLIVIA_BODY_CENTER) );

	if (vecMoveDir.z > 32)
		vecMoveDir.z *= 2;
	if (vecGoalDir.z > 32)
		vecGoalDir.z *= 2;

	float flDistance = VectorNormalize( vecMoveDir );
	//flDistance = max( flDistance, vecGoalDir.Length() );
	float flSpeed	 = sin( RemapValClamped( flDistance, 0.0f, m_flCloseFlyDistance, 0.0f, M_PI * 0.5f)) * m_flMaxFlySpeed;
	
	//flSpeed = clamp( flSpeed, 0, OLIVIA_FLYSPEED_MAX); 

	// Look to see if we are going to hit anything.
	Vector vecDeflect;
	if ( Probe( vecMoveDir, flSpeed * flInterval * 1.2, vecDeflect ) )
	{
		vecMoveDir = vecDeflect;
		VectorNormalize( vecMoveDir );
	}

	SetAbsVelocity( vecMoveDir * flSpeed );
}

Activity CNPC_Olivia::NPC_TranslateActivity( Activity eNewActivity )
{
	if (HasSpawnFlags(SF_OLIVIA_FLY)) // && !IsInAScript())
	{
		return ACT_FLY;
	}

	if ( m_bWearingDress )
	{
		if ( eNewActivity == ACT_IDLE )
		{
			return ACT_OLIVIA_IDLE;
		}

		if ( eNewActivity == ACT_WALK )
		{
			return ACT_OLIVIA_WALK;
		}

		if ( eNewActivity == ACT_RUN )
		{
			return ACT_OLIVIA_RUN;
		}
	}

	return BaseClass::NPC_TranslateActivity( eNewActivity );
}

bool CNPC_Olivia::OliviaProgressFlyPath()
{
	AI_ProgressFlyPathParams_t params( MASK_NPCSOLID );
	params.waypointTolerance = 8.0f;
	params.strictPointTolerance = 4.0f;

	float waypointDist = ( GetNavigator()->GetCurWaypointPos() - (GetAbsOrigin() + OLIVIA_BODY_CENTER)).Length();

	if ( GetNavigator()->CurWaypointIsGoal() )
	{
		float tolerance = max( params.goalTolerance, GetNavigator()->GetPath()->GetGoalTolerance() );
		if ( waypointDist <= tolerance )
				return true; //AINPP_COMPLETE;
	}
	else
	{
		bool bIsStrictWaypoint = ( (GetNavigator()->GetPath()->CurWaypointFlags() & (bits_WP_TO_PATHCORNER|bits_WP_DONT_SIMPLIFY) ) != 0 );
		float tolerance = (bIsStrictWaypoint) ? params.strictPointTolerance : params.waypointTolerance;
		if ( waypointDist <= tolerance )
		{
			trace_t tr;
			AI_TraceLine( GetAbsOrigin(), GetNavigator()->GetPath()->GetCurWaypoint()->GetNext()->GetPos(), MASK_NPCSOLID, this, COLLISION_GROUP_NPC, &tr );
			if ( tr.fraction == 1.0f )
			{
				GetNavigator()->AdvancePath();	
				return false; //AINPP_ADVANCED;
			}
		}

		if ( GetNavigator()->SimplifyFlyPath( params ) )
			return false; //AINPP_ADVANCED;
	}

	return false;
}

/*bool CNPC_Olivia::OverrideMoveFacing( const AILocalMoveGoal_t &move, float flInterval )
{
	if (!HasSpawnFlags(SF_OLIVIA_FLY))
	{
		if (m_flNextCircleAroundPlayer != 0 )
		{
			CBasePlayer *pPlayer = UTIL_PlayerByIndex(1);

			if ( pPlayer && (pPlayer->WorldSpaceCenter() - WorldSpaceCenter()).Length() < (OLIVIA_PLAYER_CIRCLE_DISTANCE * 2.0f) )
			{
				DevMsg("adding player as facing target\n");
				AddFacingTarget( pPlayer, pPlayer->WorldSpaceCenter(), 0.3f, 0.2f );
				return BaseClass::OverrideMoveFacing( move, flInterval );
			}
		}

	}

	return BaseClass::OverrideMoveFacing( move, flInterval );
}*/

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : flInterval - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CNPC_Olivia::OverrideMove( float flInterval )
{	
	if (!HasSpawnFlags(SF_OLIVIA_FLY))
	{
		if (m_flNextCircleAroundPlayer != 0 )
		{
			CircleAroundPlayer();
		}

		return BaseClass::OverrideMove( flInterval );
	}

	Vector vMoveTargetPos = GetAbsOrigin();
	CBaseEntity *pMoveTarget = NULL;

	//TERO: if we don't have a path, then get our move target
	if ( !GetNavigator()->IsGoalActive()) // || ( GetNavigator()->GetCurWaypointFlags() | bits_WP_TO_PATHCORNER )) 
	{
		// Select move target 
		if ( GetTarget() != NULL )
		{
			pMoveTarget = GetTarget();
			vMoveTargetPos = pMoveTarget->GetAbsOrigin() - OLIVIA_BODY_CENTER;
		}
	}

	if (m_bFlyBlocked) // && !GetHintNode() ) // &&  !GetHintNode() ) //( m_flNextHintTime < gpGlobals->curtime ||
	{
		DevMsg("npc_olivia: fly blocked\n");

		Vector position = GetAbsOrigin();

		bool bGotPath = false;

		if (GetNavigator()->IsGoalActive()) // && m_vOldGoal == vec3_origin)
		{
			/*AIMoveTrace_t moveTrace;

			GetMoveProbe()->MoveLimit( NAV_FLY, GetLocalOrigin(), GetNavigator()->GetPath()->NextWaypointPos(),
			MASK_NPCSOLID, NULL, &moveTrace);

			//DevMsg("olivia origin to trace end: %f, ", (moveTrace.vEndPosition - GetLocalOrigin()).Length());
			//DevMsg("way point to trace end: %f\n", (moveTrace.vEndPosition - GetNavigator()->GetCurWaypointPos()).Length());
			//DevMsg("dist obstructed: %f\n", moveTrace.flDistObstructed);

			bGotPath = GetNavigator()->PrependLocalAvoidance( olivia_avoid_dist.GetFloat(), moveTrace );
			m_bFlyBlocked = false;*/
		}
		else // if ( !GetNavigator()->IsGoalActive())
		{
			DevMsg("No old path\n");

			AI_NavGoal_t goal( GOALTYPE_LOCATION, GetNavigator()->GetGoalPos(), ACT_FLY, AIN_DEF_TOLERANCE, AIN_YAW_TO_DEST, NULL );
			bGotPath = GetNavigator()->SetGoal( goal );
			m_bFlyBlocked = false;
		}

		if (!GetHintNode())
		{
			if (!bGotPath)
			{
				DevMsg("trying to find a node");

				ClearHintNode(0.0f);

				CHintCriteria criteria;
				criteria.SetGroup( GetHintGroup() );
				criteria.SetHintTypeRange( HINT_TACTICAL_COVER_MED, HINT_TACTICAL_ENEMY_DISADVANTAGED );
				criteria.SetFlag( bits_HINT_NODE_NEAREST | bits_HINT_NODE_USE_GROUP );
				criteria.AddIncludePosition(GetAbsOrigin() + OLIVIA_BODY_CENTER, 512);
				SetHintNode( CAI_HintManager::FindHint( this, position, criteria  ));
			}

			if ( GetHintNode() )
			{
				Vector vNodePos = vMoveTargetPos;
				GetHintNode()->GetPosition(this, &vNodePos);

				bool bGroundNode = ( GetHintNode()->GetNode() && GetHintNode()->GetNode()->GetType() == NODE_GROUND);

				if (m_vOldGoal == vec3_origin)
				{
					if (GetNavigator()->IsGoalActive())
					{
						m_vOldGoal = GetNavigator()->GetGoalPos();
					} 
					else if ( pMoveTarget)
					{
						m_vOldGoal = vMoveTargetPos;
					}
				}

				if ( bGroundNode ) //TERO: this node is too low
				{
					vMoveTargetPos = vNodePos + OLIVIA_BODY_CENTER;
				}
				else
				{
					vMoveTargetPos = vNodePos; 
				}

				AI_NavGoal_t goal( GOALTYPE_LOCATION, vMoveTargetPos , ACT_FLY, AIN_DEF_TOLERANCE, AIN_YAW_TO_DEST );
				GetNavigator()->SetGoal( goal );
				m_bFlyBlocked = false;
			}
		} 
	}

	//TERO: I start a new if here on purpose
	if (GetNavigator()->IsGoalActive())
	{
		DevMsg("npc_olivia: flying a path\n");

		vMoveTargetPos = GetNavigator()->GetCurWaypointPos();

#ifdef OLIVIA_DEBUG
		NDebugOverlay::Line( GetAbsOrigin(), vMoveTargetPos, 255, 255, 0, true, 1.0f );
#endif

		Vector vecGoal = GetNavigator()->GetGoalPos(); //vMoveTargetPos;

		if ( GetNavigator()->IsGoalActive() )
		{

			/*AI_ProgressFlyPathParams_t params( MASK_NPCSOLID );
			params.waypointTolerance = 4.0f;
			params.strictPointTolerance = 2.0f;
			params.pTarget = GetNavigator()->GetGoalTarget();*/
			if (OliviaProgressFlyPath()) //GetNavigator()->ProgressFlyPath( params ) == AINPP_COMPLETE)
			{
				ClearHintNode(0.0f);

				m_bFlyBlocked = false;

				//GetNavigator()->ClearGoal();
				TaskMovementComplete();

				if (m_vOldGoal != vec3_origin)
				{
					AI_NavGoal_t goal( GOALTYPE_LOCATION, m_vOldGoal , ACT_FLY, AIN_DEF_TOLERANCE, AIN_YAW_TO_DEST );
					GetNavigator()->SetGoal( goal);

					m_vOldGoal = vec3_origin;
				}
			}

			/*if ( !GetNavigator()->CurWaypointIsGoal() )
			{
				AI_ProgressFlyPathParams_t params( MASK_NPCSOLID );
				params.waypointTolerance = 0.0f;
				params.pTarget = GetNavigator()->GetGoalTarget();
				if (GetNavigator()->SimplifyFlyPath( params ))
				{
					DevMsg("SIMPLIFIED PATH\n");
				}
				else if (OliviaReachedPoint(GetNavigator()->GetCurWaypointPos())) // || )
				{
					DevMsg("npc_olivia: reached point\n");
					GetNavigator()->AdvancePath();
				}

				//GetNavigator()->ProgressFlyPath( params ); 

			} else 
			{
				if (OliviaReachedPoint(GetNavigator()->GetCurWaypointPos()))
				{
					DevMsg("npc_olivia: reached goal\n");

					ClearHintNode(0.0f);

					m_bFlyBlocked = false;

					GetNavigator()->ClearGoal();

					if (m_vOldGoal != vec3_origin)
					{

						AI_NavGoal_t goal( GOALTYPE_LOCATION, m_vOldGoal , ACT_FLY, AIN_DEF_TOLERANCE, AIN_YAW_TO_DEST );
						GetNavigator()->SetGoal( goal);

						m_vOldGoal = vec3_origin;
					}
				}
			}*/
		}

		OliviaFly(flInterval, vMoveTargetPos, vecGoal );
	}
	else if ( pMoveTarget != NULL )
	{
		DevMsg("npc_olivia: flying to a move target\n");

		//Since we are not going on a path, we are not blocked and should clear the hint
		ClearHintNode(0.0f);

		OliviaFly(flInterval, vMoveTargetPos, vMoveTargetPos );
	} 
	else 
	{
		DevMsg("npc_olivia: slowing down\n");

		//Since we are not going on a path, we are not blocked and should clear the hint
		ClearHintNode(0.0f);

		//We don't have a target, lets slow down
		Vector vecSpeed = GetAbsVelocity();
		float flSpeed   = VectorNormalize(vecSpeed);

		if (flSpeed < 5)
			flSpeed = 0;
		else
		{
			if (flInterval > 1.0f)
				flInterval = 1.0f;
			flSpeed = flSpeed * (1.0f-flInterval) * 0.7f;
		}

		SetAbsVelocity( vecSpeed * flSpeed );
	}


	if (!m_bFlyBlocked) // && !GetHintNode())
	{
		DevMsg("npc_olivia: checking blocking\n");

		//TERO: blocking detection

		trace_t tr;
		AI_TraceHull( GetAbsOrigin(), vMoveTargetPos, OLIVIA_HULL_MINS, OLIVIA_HULL_MAXS, MASK_NPCSOLID, this, COLLISION_GROUP_NPC, &tr );

		float fTargetDist = (1.0f-tr.fraction) * ((GetAbsOrigin() - vMoveTargetPos).Length());

		DevMsg("npc_olivia: distance %f\n", fTargetDist);
			
		if ( ( tr.m_pEnt == pMoveTarget ) || ( fTargetDist < 50 ) )
		{
			m_bFlyBlocked = false;
		}
		else		
		{
			DevMsg("npc_olivia: marking as being blocked\n");
			m_bFlyBlocked = true;
		}
	}

	return true;
}

// Purpose: Looks ahead to see if we are going to hit something. If we are, a
//			recommended avoidance path is returned.
// Input  : vecMoveDir - 
//			flSpeed - 
//			vecDeflect - 
// Output : Returns true if we hit something and need to deflect our course,
//			false if all is well.
//-----------------------------------------------------------------------------
bool CNPC_Olivia::Probe( const Vector &vecMoveDir, float flSpeed, Vector &vecDeflect )
{
	//
	// Look 1/2 second ahead.
	//
	trace_t tr;
	AI_TraceHull( GetAbsOrigin(), GetAbsOrigin() + vecMoveDir * flSpeed, OLIVIA_HULL_MINS, OLIVIA_HULL_MAXS, MASK_NPCSOLID, this, COLLISION_GROUP_NPC, &tr );
	if ( tr.fraction < 1.0f )
	{
		//
		// If we hit something, deflect flight path parallel to surface hit.
		//
		Vector vecUp;
		CrossProduct( vecMoveDir, tr.plane.normal, vecUp );
		CrossProduct( tr.plane.normal, vecUp, vecDeflect );
		VectorNormalize( vecDeflect );
		return true;
	}

	vecDeflect = vec3_origin;
	return false;
}

bool CNPC_Olivia::OliviaReachedPoint(Vector vecOrigin)
{
	Vector vecDir = vecOrigin - (GetAbsOrigin() + OLIVIA_BODY_CENTER);
	Vector vecVelocity = GetAbsVelocity();

	float flZDist  = vecDir.z;
	vecDir.z	   = 0;
	float flXYDist = VectorNormalize(vecDir);  
	VectorNormalize(vecVelocity);
	float flDot	   = DotProduct( vecDir, vecVelocity );

	if (flDot < 0.8)
	{
		if (flZDist < OLIVIA_Z_DISTANCE_NOT_FACING && flXYDist < OLIVIA_XY_DISTANCE_NOT_FACING)
		{
			return true;
		}
	}
	else
	{
		if (flZDist < OLIVIA_Z_DISTANCE_FACING && flXYDist < OLIVIA_XY_DISTANCE_FACING)
		{
			return true;
		}
	}

	return false;
}

AI_BEGIN_CUSTOM_NPC( npc_olivia, CNPC_Olivia )

	DECLARE_ACTIVITY(ACT_OLIVIA_IDLE)
	DECLARE_ACTIVITY(ACT_OLIVIA_WALK)
	DECLARE_ACTIVITY(ACT_OLIVIA_RUN)

	DECLARE_ANIMEVENT( AE_OLIVIA_BLOW_SMOKE )

	DECLARE_CONDITION(COND_OLIVIA_STOP_CIRCLE_AROUND_PLAYER)

	DECLARE_TASK(TASK_OLIVIA_CIRCLE_AROUND_PLAYER)

	DEFINE_SCHEDULE
	(
		SCHED_OLIVIA_CIRCLE_AROUND_PLAYER,

		"	Tasks"
		"		TASK_OLIVIA_CIRCLE_AROUND_PLAYER		0"
		"		TASK_WALK_PATH					0"
		"		TASK_WAIT_FOR_MOVEMENT			0"
		"	"
		"	Interrupts"
		"		COND_OLIVIA_STOP_CIRCLE_AROUND_PLAYER"
	)

AI_END_CUSTOM_NPC()