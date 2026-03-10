//=================== Half-Life 2: Short Stories Mod 2009 =====================//
//
// Purpose:	Manta Ray
//
//=============================================================================//


//#ifndef NPC_OLIVIA
//#define NPC_OLIVIA

//#include	"cbase.h"
//#include	"npcevent.h"
//#include	"ai_basenpc.h"

#include	"cbase.h"
#include	"npcevent.h"
#include	"ai_basenpc.h"
#include	"ai_hull.h"
#include	"ai_baseactor.h"
#include	"hl2_player.h"
#include	"pathtrack.h"
#include	"hl2_shareddefs.h"
#include	"soundenvelope.h"

#include "ai_hint.h"
#include "ai_route.h"
#include "ai_moveprobe.h"

#include "beam_shared.h"
#include "Sprite.h"
#include "physics_prop_ragdoll.h"
#include "physics_bone_follower.h"

#include "ai_senses.h"
#include "ai_memory.h"

#include "gib.h"
#include "EntityFlame.h"
#include "te_effect_dispatch.h"
#include "particle_parse.h"

#include "TemplateEntities.h"
#include "mapentities.h"
#include "datacache/imdlcache.h"

#include "mantaray_shared.h"
#include "npc_mantaray.h"
//#include "cbasehelicopter.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

ConVar	mantaray_max_speed("hlss_mantaray_max_speed", "300");
ConVar  sk_mantaray_health("sk_mantaray_health", "400");



BEGIN_DATADESC( CHLSS_Mantaray_Teleport_Target )
	DEFINE_FIELD( m_bIsBeingUsed,				FIELD_BOOLEAN ),
	DEFINE_OUTPUT( m_OnSpawnedNPC,	"OnSpawnedNPC" ),
	DEFINE_OUTPUT( m_OnTeleported,	"OnTeleported" ),
END_DATADESC()

LINK_ENTITY_TO_CLASS(hlss_mantaray_teleport_target, CHLSS_Mantaray_Teleport_Target);

CHLSS_Mantaray_Teleport_Target::CHLSS_Mantaray_Teleport_Target()
{
	m_bIsBeingUsed = false;
}



// THE RAY HIMSELF

BEGIN_DATADESC( CNPC_MantaRay )
	DEFINE_FIELD( m_bFlyBlocked,				FIELD_BOOLEAN ),

	DEFINE_FIELD( m_pBeam,						FIELD_CLASSPTR ),
	DEFINE_FIELD( m_pLightGlow,					FIELD_CLASSPTR ),
	DEFINE_FIELD( m_nGunAttachment,				FIELD_INTEGER ),
	DEFINE_FIELD( m_iLaserTarget,				FIELD_INTEGER ),
	DEFINE_FIELD( m_flAttackStartedTime,		FIELD_TIME ),
	DEFINE_FIELD( m_bBallyStarted,				FIELD_BOOLEAN ),
	DEFINE_FIELD( m_vecLaserEndPos,				FIELD_POSITION_VECTOR ),

	DEFINE_FIELD( m_flNextPainSoundTime,		FIELD_TIME ),

	DEFINE_FIELD( m_flDeathTime,				FIELD_TIME ),

	DEFINE_FIELD( m_flEscapeTimeEnds,			FIELD_TIME ),
	DEFINE_FIELD( m_pCurrentEscapePath,			FIELD_EHANDLE ), //FIELD_CLASSPTR ),
	DEFINE_FIELD( m_iDamageToEscape,			FIELD_INTEGER ),

	DEFINE_FIELD( m_pCurrentPathTarget,			FIELD_EHANDLE ), //FIELD_CLASSPTR ),
	DEFINE_KEYFIELD( m_strCurrentPathName,		FIELD_STRING, "ray_path" ),
	DEFINE_EMBEDDED( m_BoneFollowerManager ),

	DEFINE_KEYFIELD( m_bTeleporter,				FIELD_BOOLEAN, "teleporter" ),
	DEFINE_FIELD( m_hTeleportTarget,			FIELD_EHANDLE ),
	DEFINE_FIELD( m_bTeleporting,				FIELD_BOOLEAN ),
	DEFINE_KEYFIELD( m_iszTeleportTargetName,	FIELD_STRING,	"teleport_target" ),

	//NPCs
	DEFINE_KEYFIELD( m_iszTemplateName[0],		FIELD_STRING, "Template_1" ),
	DEFINE_FIELD( m_iszTemplateData[0],			FIELD_STRING ),
	DEFINE_FIELD( m_bNPC_Alive[0],				FIELD_BOOLEAN ),
	DEFINE_KEYFIELD( m_bMustBeDead[0],				FIELD_BOOLEAN, "MustDead_1" ),
	DEFINE_KEYFIELD( m_iszTemplateName[1],		FIELD_STRING, "Template_2" ),
	DEFINE_FIELD( m_iszTemplateData[1],			FIELD_STRING ),
	DEFINE_FIELD( m_bNPC_Alive[1],				FIELD_BOOLEAN ),
	DEFINE_KEYFIELD(m_bMustBeDead[1],			FIELD_BOOLEAN, "MustDead_2" ),
	DEFINE_KEYFIELD( m_iszTemplateName[2],		FIELD_STRING, "Template_3" ),
	DEFINE_FIELD( m_iszTemplateData[2],			FIELD_STRING ),
	DEFINE_FIELD( m_bNPC_Alive[2],				FIELD_BOOLEAN ),
	DEFINE_KEYFIELD( m_bMustBeDead[2],			FIELD_BOOLEAN, "MustDead_3" ),

	DEFINE_SOUNDPATCH( m_pIdleSound ),

	DEFINE_INPUTFUNC( FIELD_VOID,	"MakeTeleporter",		InputTeleporter ),
	DEFINE_INPUTFUNC( FIELD_VOID,	"MakeHostile",			InputHostile ),
	DEFINE_INPUTFUNC( FIELD_STRING, "TeleportTarget",		InputTeleportTarget ),	
	DEFINE_INPUTFUNC( FIELD_STRING,	"TeleportTargetName",	InputTeleportTargetName ),
	DEFINE_INPUTFUNC( FIELD_STRING, "SetPath",				InputEscapePath ),

	DEFINE_OUTPUT( m_OnSpawnedNPC,	"OnSpawnedNPC" ),
	DEFINE_OUTPUT( m_OnTeleported,	"OnTeleported" ),
END_DATADESC()

LINK_ENTITY_TO_CLASS(npc_mantaray, CNPC_MantaRay);

IMPLEMENT_SERVERCLASS_ST( CNPC_MantaRay, DT_NPC_MantaRay )
	SendPropFloat( SENDINFO( m_flAttackStartedTime ) ), //SPROP_CHANGES_OFTEN ),
	SendPropVector(SENDINFO(m_vecLaserEndPos) ), // SPROP_CHANGES_OFTEN),
	SendPropInt(SENDINFO(m_nGunAttachment) ), //, SPROP_CHANGES_OFTEN),
	SendPropInt(SENDINFO(m_iLaserTarget) ), // SPROP_CHANGES_OFTEN),
	SendPropBool(SENDINFO(m_bTeleporting ) ),
END_SEND_TABLE()

CNPC_MantaRay::CNPC_MantaRay()
{
	m_bFlyBlocked = false;

	m_flAttackStartedTime = 0;

	m_bBallyStarted = false;

	m_pCurrentPathTarget = NULL;

	m_flEscapeTimeEnds = 0;
	m_pCurrentEscapePath = NULL;

	m_flDeathTime = 0;

	m_bTeleporter = false;
	m_hTeleportTarget = NULL;
	m_iszTeleportTargetName = NULL_STRING;

	for (int i=0; i<HLSS_MANTARAY_MAX_NPCS; i++)
	{
		m_iszTemplateName[i] = NULL_STRING;
		m_iszTemplateData[i] = NULL_STRING;
		m_bNPC_Alive[i] = false;
		m_bMustBeDead[i] = true;
	}
}

void CNPC_MantaRay::InputTeleporter( inputdata_t &inputdata )
{
	m_bTeleporter = true;
}

void CNPC_MantaRay::InputHostile( inputdata_t &inputdata )
{
	m_bTeleporter = false;

	GetEnemies()->SetFreeKnowledgeDuration( 5.0f );
}

void CNPC_MantaRay::InputTeleportTarget( inputdata_t &inputdata )
{
	FindTeleportTarget( inputdata.value.StringID() );
}

void CNPC_MantaRay::InputTeleportTargetName( inputdata_t &inputdata )
{
	m_iszTeleportTargetName = inputdata.value.StringID();
}

void CNPC_MantaRay::FindTeleportTarget( string_t iszTargetName )
{
	if (iszTargetName == NULL_STRING)
		return;

	CBaseEntity *pTarget = gEntList.FindEntityByName( NULL, iszTargetName  );
	CHLSS_Mantaray_Teleport_Target *pTeleport = dynamic_cast <CHLSS_Mantaray_Teleport_Target*>(pTarget);

	int iNum = 0;

	while ( pTarget )
	{
		if (!pTeleport->m_bIsBeingUsed)
		{
			iNum++;
		}

		pTarget = gEntList.FindEntityByName( pTarget, iszTargetName  );
		pTeleport = dynamic_cast <CHLSS_Mantaray_Teleport_Target*>(pTarget);
	}

	if (iNum == 0)
	{
		Warning("%s no teleport target %s found!\n", GetDebugName(), iszTargetName );
	}

	iNum = random->RandomInt(0, iNum-1);

	pTarget = gEntList.FindEntityByName( NULL, iszTargetName  );
	pTeleport = dynamic_cast <CHLSS_Mantaray_Teleport_Target*>(pTarget);
	
	while ( pTarget )
	{
		if (!pTeleport->m_bIsBeingUsed)
		{
			iNum--;
		}
		
		if ( iNum >= 0 )
		{
			pTarget = gEntList.FindEntityByName( pTarget, iszTargetName  );
			pTeleport = dynamic_cast <CHLSS_Mantaray_Teleport_Target*>(pTarget);
		}
		else
		{
			pTarget = NULL;
		}
	}

	m_hTeleportTarget = pTeleport;
	m_hTeleportTarget->m_bIsBeingUsed = true;
}

static const char *pFollowerBoneNames[] =
{
	"Manta.Body",
	"Manta.Spine1",
	"Manta.Spine2",
	"Manta.RightWing1",
	"Manta.RightWing2",
	"Manta.RightWing3",
	"Manta.LeftWing1",
	"Manta.LeftWing2",
	"Manta.LeftWing3",
	"Manta.RightTail1",
	"Manta.RightTail2",
	"Manta.LeftTail1",
	"Manta.LeftTail2",
};

#define RAY_MAX_CHUNKS	9
static const char *s_pChunkModelName[RAY_MAX_CHUNKS] = 
{
	"models/gibs/ray_gib01.mdl",
	"models/gibs/ray_gib02.mdl",
	"models/gibs/ray_gib03.mdl",
	"models/gibs/ray_gib04.mdl",
	"models/gibs/ray_gib05.mdl",
	"models/gibs/ray_gib06.mdl",
	"models/gibs/ray_gib07.mdl",
	"models/gibs/ray_gib08.mdl",
	"models/gibs/ray_gib09.mdl",
};

void CNPC_MantaRay::Spawn( void )
{
	Precache();
	SetModel( "models/ray.mdl" );

	BaseClass::Spawn();

	SetSolid( SOLID_BBOX );

	SetHullType( HULL_LARGE_CENTERED ); 
	SetHullSizeNormal();

	//Don't collide with other mantarays
	SetCollisionGroup( HL2COLLISION_GROUP_GUNSHIP );

	CreateVPhysics();
	InitBoneControllers();

	SetGroundEntity( NULL );
	SetNavType( NAV_FLY );

	AddFlag( FL_FLY );
	RemoveFlag( FL_ONGROUND ); 
		
	CapabilitiesAdd ( bits_CAP_MOVE_FLY  | bits_CAP_SKIP_NAV_GROUND_CHECK ); 
	CapabilitiesAdd ( bits_CAP_SIMPLE_RADIUS_DAMAGE );
	CapabilitiesRemove( bits_CAP_MOVE_GROUND );
	CapabilitiesRemove( bits_CAP_TURN_HEAD );

	//SetMoveType( MOVETYPE_STEP );

	SetMoveType( MOVETYPE_STEP ); //edge handling

	AddEFlags( EFL_NO_DISSOLVE | EFL_NO_MEGAPHYSCANNON_RAGDOLL | EFL_NO_PHYSCANNON_INTERACTION );

	m_iHealth			= sk_mantaray_health.GetInt();
	m_iDamageToEscape	= m_iHealth - RAY_ESCAPE_DAMAGE_DIFFERENCE;

	SetBloodColor( BLOOD_COLOR_YELLOW );
	m_flFieldOfView		= VIEW_FIELD_FULL;
	AddSpawnFlags( SF_NPC_LONG_RANGE );
	m_NPCState			= NPC_STATE_NONE;

	m_flNextPainSoundTime = 0;

	NPCInit();

	m_nGunAttachment = LookupAttachment("Gun");

	// Restore current path
	if ( m_strCurrentPathName != NULL_STRING )
	{
		m_pCurrentPathTarget = (CPathTrack *) gEntList.FindEntityByName( NULL, m_strCurrentPathName );
	}
	else
	{
		m_pCurrentPathTarget = NULL;
	}

	if (!m_bTeleporter)
	{
		GetEnemies()->SetFreeKnowledgeDuration( 5.0f );
	}

	//Stagger our starting times
	SetNextThink( gpGlobals->curtime );
}

void CNPC_MantaRay::InputEscapePath( inputdata_t &inputdata )
{
	m_strCurrentPathName = MAKE_STRING( inputdata.value.String() );

	// Restore current path
	if ( m_strCurrentPathName != NULL_STRING )
	{
		m_pCurrentPathTarget = (CPathTrack *) gEntList.FindEntityByName( NULL, m_strCurrentPathName );
	}
	else
	{
		m_pCurrentPathTarget = NULL;
	}
}

void CNPC_MantaRay::IdleSound( void )
{
	if ( !m_pIdleSound )
	{
		CSoundEnvelopeController &controller = CSoundEnvelopeController::GetController();
		CPASAttenuationFilter filter( this );

		m_pIdleSound = controller.SoundCreate( filter, entindex(), RAY_SOUND_IDLE );
		controller.Play( m_pIdleSound, 1.0f, 200 );
	}
	else
	{
		Assert(m_pIdleSound);
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_MantaRay::PainSound( const CTakeDamageInfo &info )
{
	if ( gpGlobals->curtime < m_flNextPainSoundTime )
		return;

	m_flNextPainSoundTime = gpGlobals->curtime + 1.0f;

	EmitSound( RAY_SOUND_PAIN );
}

//-----------------------------------------------------------------------------
// Purpose: Shuts down looping sounds when we are killed in combat or deleted.
//-----------------------------------------------------------------------------
void CNPC_MantaRay::StopLoopingSounds()
{
	BaseClass::StopLoopingSounds();

	if ( m_pIdleSound )
	{
		CSoundEnvelopeController &controller = CSoundEnvelopeController::GetController();
		controller.SoundDestroy( m_pIdleSound );
		m_pIdleSound = NULL;
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_MantaRay::OnRestore( void )
{
	BaseClass::OnRestore();

	// Restore current path
	if ( m_strCurrentPathName != NULL_STRING )
	{
		m_pCurrentPathTarget = (CPathTrack *) gEntList.FindEntityByName( NULL, m_strCurrentPathName );
	}
	else
	{
		m_pCurrentPathTarget = NULL;
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_MantaRay::Precache( void )
{
	SetModelName( MAKE_STRING( "models/ray.mdl" ) );
	PrecacheModel( STRING( GetModelName() ) );

	for ( int i = 0; i < RAY_MAX_CHUNKS; ++i )
	{
		PrecacheModel( s_pChunkModelName[i] );
	}

	//PrecacheParticleSystem( "mantaray_attack" );
	PrecacheScriptSound( RAY_BEAM_SOUND );
	PrecacheScriptSound( RAY_LIGHT_SOUND );
	PrecacheScriptSound( RAY_LIGHT_START_SOUND );
	PrecacheScriptSound( "NPC_Vortigaunt.Explode" );
	PrecacheScriptSound( RAY_SOUND_PAIN );
	PrecacheScriptSound( RAY_SOUND_IDLE );
	PrecacheScriptSound( RAY_SOUND_DEATH );
	PrecacheScriptSound( RAY_SOUND_EXPLODE );
	PrecacheScriptSound( RAY_SOUND_TELEPORT );

	PrecacheModel(HLSS_MANTARAY_LASER_BEAM);	
	PrecacheModel(HLSS_MANTARAY_LASER_LIGHT);
	PrecacheModel(HLSS_MANTARAY_LASER_LIGHT_BEAM);
	PrecacheModel("sprites/vortring1.vmt");

	PrecacheParticleSystem( "antlion_gib_02" );

	PrecacheParticleSystem( "mantaray_teleport" );
	PrecacheParticleSystem( "mantaray_teleport_npc" );

	BaseClass::Precache();

	for (int i=0; i<HLSS_MANTARAY_MAX_NPCS; i++)
	{
		if ( !m_iszTemplateData[i] )
		{
			//
			// This must be the first time we're activated, not a load from save game.
			// Look up the template in the template database.
			//
			if (m_iszTemplateName[i] != NULL_STRING)
			{
				m_iszTemplateData[i] = Templates_FindByTargetName(STRING(m_iszTemplateName[i]));
				if ( m_iszTemplateData[i] == NULL_STRING )
				{
					DevWarning( "hlss_mantaray %s: template NPC %s not found!\n", STRING(GetEntityName()), STRING(m_iszTemplateName[i]) );
				}
			}
		}

		//Assert( m_iszTemplateData != NULL_STRING );

		// If the mapper marked this as "preload", then instance the entity preache stuff and delete the entity
		//if ( !HasSpawnFlags(SF_NPCMAKER_NOPRELOADMODELS) )
		if ( m_iszTemplateData[i] != NULL_STRING )
		{

			CBaseEntity *pEntity = NULL;
			MapEntity_ParseEntity( pEntity, STRING(m_iszTemplateData[i]), NULL );
			if ( pEntity != NULL )
			{
				pEntity->Precache();
				UTIL_RemoveImmediate( pEntity );
			}
		}
	}
}

void CNPC_MantaRay::DeathNotice( CBaseEntity *pVictim )
{
	BaseClass::DeathNotice( pVictim );

	string_t iszVictimName = pVictim->GetEntityName();

	if (iszVictimName == NULL_STRING)
	{
		return;
	}

	for (int i=0; i<HLSS_MANTARAY_MAX_NPCS; i++)
	{
		if (IDENT_STRINGS(m_iszTemplateName[i], iszVictimName))
		{
			m_bNPC_Alive[i] = false;

			//TERO: not sure if it's possible to use same template NPC for two of the slots
			//return
		}
	}
}
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CNPC_MantaRay::CreateVPhysics( void )
{
	InitBoneFollowers();
	return BaseClass::CreateVPhysics();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_MantaRay::InitBoneFollowers( void )
{
	// Don't do this if we're already loaded
	if ( m_BoneFollowerManager.GetNumBoneFollowers() != 0 )
		return;

	// Init our followers
	m_BoneFollowerManager.InitBoneFollowers( this, ARRAYSIZE(pFollowerBoneNames), pFollowerBoneNames );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CNPC_MantaRay::OverrideMove( float flInterval )
{	
	Vector forward;
	GetVectors(&forward, NULL, NULL );
	Vector vMoveTargetPos = GetAbsOrigin(); // + (forward * 1.0f);
	//Vector vNextTargetPos = vMoveTargetPos;
	CBaseEntity *pMoveTarget = NULL;

	if ( FlyToDeath() )
	{
		vMoveTargetPos = vMoveTargetPos + (forward * 256.0f);
		RayFly(flInterval, vMoveTargetPos, vMoveTargetPos );
		RayCheckBlocked(NULL, vMoveTargetPos); //if we are blocked we explode right away
		return true;
	}

	//TERO: if we don't have a path, then get our move target
	if ( !GetNavigator()->IsGoalActive()) // || ( GetNavigator()->GetCurWaypointFlags() | bits_WP_TO_PATHCORNER ))
	{
		if ( GetTarget() != NULL )
		{
			pMoveTarget = GetTarget();
			vMoveTargetPos = pMoveTarget->GetAbsOrigin();
			//vNextTargetPos = vMoveTargetPos;
		}
		else if (m_hTeleportTarget)
		{
			Vector vecForward; //, vecRight;
			//GetVectors(&vecForward, &vecRight, NULL);
			pMoveTarget = m_hTeleportTarget;

			QAngle angAngles = pMoveTarget->GetAbsAngles();

			angAngles.y += ( gpGlobals->curtime * 60.0f );

			AngleVectors(angAngles, &vecForward);


			vMoveTargetPos = pMoveTarget->WorldSpaceCenter() + (vecForward * 96.0f);
			
			vMoveTargetPos.z += 256.0f; //GetAbsOrigin().z;

			//NDebugOverlay::Box( vMoveTargetPos, Vector(-4,-4,-4), Vector(4,4,4), 0, 0, 255, 255, 0.1 );
			
			//vNextTargetPos = vMoveTargetPos;

			m_pCurrentEscapePath = NULL;
		}
		else if (m_bTeleporter)
		{
			//DevMsg("teleporter, fly path\n"); 

			if ( m_pCurrentEscapePath  )
			{
				vMoveTargetPos = m_pCurrentEscapePath->GetAbsOrigin();

				/*if (m_pCurrentEscapePath->GetNext())
				{
					vNextTargetPos = m_pCurrentEscapePath->GetNext()->GetAbsOrigin();
				}
				else
				{
					vNextTargetPos = vMoveTargetPos;
				}*/

				//NDebugOverlay::Box( vMoveTargetPos, Vector(-4,-4,-4), Vector(4,4,4), 0, 0, 255, 0, 0.1 );

				pMoveTarget = m_pCurrentEscapePath;

				if (RayReachedPoint( vMoveTargetPos ))
				{
					//DevMsg("escape route path node reached\n");

					variant_t emptyVariant;
					m_pCurrentEscapePath->AcceptInput( "InPass", this, this, emptyVariant, 0 );

					//TERO: if there's no next path, then the escape will end
					m_pCurrentEscapePath = m_pCurrentEscapePath->GetNext();

					if (m_pCurrentEscapePath)
					{
						vMoveTargetPos = m_pCurrentEscapePath->GetAbsOrigin();
						//vNextTargetPos = vMoveTargetPos;
						pMoveTarget = m_pCurrentEscapePath;
					}
				}
			}
			else
			{
				//DevMsg("getting new path\n");

				m_pCurrentEscapePath = BestPointOnPath( m_pCurrentPathTarget , WorldSpaceCenter(), 0, false );

				if (m_pCurrentEscapePath)
				{
					vMoveTargetPos = m_pCurrentEscapePath->GetAbsOrigin();
					//vNextTargetPos = vMoveTargetPos;
					pMoveTarget = m_pCurrentEscapePath;
				}
			}
		}
		else if ( m_pCurrentEscapePath && m_flEscapeTimeEnds > gpGlobals->curtime )
		{
			vMoveTargetPos = m_pCurrentEscapePath->GetAbsOrigin();

			/*if (m_pCurrentEscapePath->GetNext())
			{
				vNextTargetPos = m_pCurrentEscapePath->GetNext()->GetAbsOrigin();
			}
			else
			{
				vNextTargetPos = vMoveTargetPos;
			}*/

			//NDebugOverlay::Box( vMoveTargetPos, Vector(-4,-4,-4), Vector(4,4,4), 0, 0, 255, 0, 0.1 );

			pMoveTarget = m_pCurrentEscapePath;

			if (RayReachedPoint( vMoveTargetPos ))
			{
				//DevMsg("escape route path node reached\n");

				variant_t emptyVariant;
				m_pCurrentEscapePath->AcceptInput( "InPass", this, this, emptyVariant, 0 );

				//TERO: if there's no next path, then the escape will end
				m_pCurrentEscapePath = m_pCurrentEscapePath->GetNext();

				if (m_pCurrentEscapePath)
				{
					vMoveTargetPos = m_pCurrentEscapePath->GetAbsOrigin();
					//vNextTargetPos = vMoveTargetPos;
					pMoveTarget = m_pCurrentEscapePath;
				}
				else
				{
					m_flEscapeTimeEnds = 0;
				}
			}
		}
		else
		{
			pMoveTarget = GetEnemyVehicle();

			if ( pMoveTarget )
			{
				Vector vecForward;
				float flSpeed;

				if (pMoveTarget->VPhysicsGetObject())
				{
					pMoveTarget->VPhysicsGetObject()->GetVelocity( &vecForward, NULL );
					flSpeed = VectorNormalize(vecForward);

					flSpeed = clamp(flSpeed, 0, 400.0f);
				}
				else
				{
					pMoveTarget->GetVectors(&vecForward, NULL, NULL);
					flSpeed = 400.0f;
				}

				Vector vecStart = pMoveTarget->WorldSpaceCenter() + Vector(0,0,128.0f);

				trace_t tr;
				AI_TraceHull( vecStart, vecStart + (vecForward * flSpeed), GetHullMins(), GetHullMaxs(), MASK_SOLID, this, HL2COLLISION_GROUP_GUNSHIP, &tr );

				vMoveTargetPos = tr.endpos;
				//vNextTargetPos = vMoveTargetPos;
			
				//NDebugOverlay::Box( vMoveTargetPos, GetHullMins(), GetHullMaxs(), 255, 0, 0, 0, 0.1 );

				pMoveTarget = GetEnemy();
			}
			else if ( GetEnemy() )
			{

				Vector vecForward; //, vecRight;
				//GetVectors(&vecForward, &vecRight, NULL);
				pMoveTarget = GetEnemy();

				QAngle angAngles = pMoveTarget->GetAbsAngles();

				angAngles.y += ( gpGlobals->curtime * 60.0f );

				AngleVectors(angAngles, &vecForward);

				vMoveTargetPos = pMoveTarget->WorldSpaceCenter() + (vecForward * 96.0f);
			
				vMoveTargetPos.z += 256.0f; //GetAbsOrigin().z;

				//vNextTargetPos = vMoveTargetPos;
			}
			else //if ( HasSpawnFlags( SF_RAY_FLY_ESCAPE_PATH_WHEN_NO_ENEMY ) )
			{
				//DevMsg("hostile, flying path\n");

				if ( m_pCurrentEscapePath  )
				{
					vMoveTargetPos = m_pCurrentEscapePath->GetAbsOrigin();

					/*if (m_pCurrentEscapePath->GetNext())
					{
						vNextTargetPos = m_pCurrentEscapePath->GetNext()->GetAbsOrigin();
					}
					else
					{
						vNextTargetPos = vMoveTargetPos;
					}*/

					//NDebugOverlay::Box( vMoveTargetPos, Vector(-4,-4,-4), Vector(4,4,4), 0, 0, 255, 0, 0.1 );

					pMoveTarget = m_pCurrentEscapePath;

					if (RayReachedPoint( vMoveTargetPos ))
					{
						variant_t emptyVariant;
						m_pCurrentEscapePath->AcceptInput( "InPass", this, this, emptyVariant, 0 );

						//DevMsg("escape route path node reached\n");
						//TERO: if there's no next path, then the escape will end
						m_pCurrentEscapePath = m_pCurrentEscapePath->GetNext();

						if (m_pCurrentEscapePath)
						{
							vMoveTargetPos = m_pCurrentEscapePath->GetAbsOrigin();
							//vNextTargetPos = vMoveTargetPos;
							pMoveTarget = m_pCurrentEscapePath;
						}
					}
				}
				else
				{
					//DevMsg("getting new path\n");

					m_pCurrentEscapePath = BestPointOnPath( m_pCurrentPathTarget , WorldSpaceCenter(), 0, false );

					if (m_pCurrentEscapePath)
					{
						vMoveTargetPos = m_pCurrentEscapePath->GetAbsOrigin();
						//vNextTargetPos = vMoveTargetPos;
						pMoveTarget = m_pCurrentEscapePath;
					}
				}
			}
		}//end hostile
	}

	bool bGotPath = false;

	if ( m_bFlyBlocked ) // &&  !GetHintNode() ) //( m_flNextHintTime < gpGlobals->curtime ||
	{
		CPathTrack *pDest =  NULL; 
		
		/*if (!GetNavigator()->IsGoalActive() && (!m_pCurrentEscapePath || (m_flEscapeTimeEnds != 0 && m_flEscapeTimeEnds < gpGlobals->curtime)))
		{
			pDest = BestPointOnPath( m_pCurrentPathTarget , WorldSpaceCenter(), 0, false );
		}*/

		if (pDest)
		{
			DevMsg("fly was blocked, found a node on path track\n");
			m_bFlyBlocked = false;

			vMoveTargetPos = pDest->GetAbsOrigin();
			//vMoveTargetPos = vNextTargetPos;
			pMoveTarget = pDest;
		}
		else
		{
			DevMsg("****************************************************************************************\n OH NOOOOO!\n\n");

			Vector position = GetAbsOrigin();

			bGotPath;

			if (GetNavigator()->IsGoalActive()) // && m_vOldGoal == vec3_origin)
			{
				//TERO: this has been move to RayProgressFlyPath
				//m_bFlyBlocked = false; 
			}
			else 
			{
				if (pMoveTarget && (pMoveTarget->IsPlayer() || pMoveTarget->IsNPC()))
				{
					AI_NavGoal_t goal( GOALTYPE_ENEMY, vMoveTargetPos ); //, ACT_FLY, AIN_DEF_TOLERANCE, AIN_YAW_TO_DEST, pMoveTarget );
					bGotPath = GetNavigator()->SetGoal( goal );
					//m_bFlyBlocked = false;
				}
				else
				{
					AI_NavGoal_t goal( GOALTYPE_LOCATION, vMoveTargetPos ); //, ACT_FLY, AIN_DEF_TOLERANCE, AIN_YAW_TO_DEST, pMoveTarget );
					bGotPath = GetNavigator()->SetGoal( goal );
					//m_bFlyBlocked = false;

					if (bGotPath)
					{
						DevMsg("yay\n");
					}
				}
			}
		}
	}

	//TERO: I start a new if here on purpose
	if (GetNavigator()->IsGoalActive()) // && !(GetNavigator()->GetCurWaypointFlags() | bits_WP_TO_PATHCORNER ))
	{
		//GetNavigator()->DrawDebugRouteOverlay();
	
		vMoveTargetPos = GetNavigator()->GetCurWaypointPos();

		/*if ( GetNavigator()->GetPath()->GetCurWaypoint() && GetNavigator()->GetPath()->GetCurWaypoint()->GetNext())
		{
			vNextTargetPos = GetNavigator()->GetPath()->NextWaypointPos();
		}
		else
		{
			vNextTargetPos = vMoveTargetPos;
		}*/

		Vector vecGoal = vMoveTargetPos;

		/*if ( GetNavigator()->GetGoalTarget() && 
			(GetNavigator()->GetGoalTarget()->IsNPC() || GetNavigator()->GetGoalTarget()->IsPlayer()) &&
			 HasCondition(COND_SEE_ENEMY) )
		{
			vecGoal = GetNavigator()->GetGoalTarget()->GetAbsOrigin();
		}*/

		if (RayProgressFlyPath())
		{
			ClearHintNode(0.0f);

			m_bFlyBlocked = false;

			//GetNavigator()->ClearGoal();
			TaskMovementComplete();
		}

		RayFly(flInterval, vMoveTargetPos, vecGoal );
	}
	else if ( pMoveTarget != NULL )
	{
		if ( pMoveTarget == GetHintNode())
		{
			if ( RayReachedPoint( vMoveTargetPos ) )
			{
				SetTarget( NULL );
				ClearHintNode( 0.0f);
			}
		}
		else
		{
			ClearHintNode(0.0f);
		}

		RayFly(flInterval, vMoveTargetPos, vMoveTargetPos );
	} 
	else 
	{
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

		SetLocalAngularVelocity( QAngle(0,0,0) );
		SetAbsVelocity( vecSpeed * flSpeed );

		return true;
	}

	if (!bGotPath)
	{
		RayCheckBlocked(pMoveTarget, vMoveTargetPos);
	}

	return true;
}

void CNPC_MantaRay::RayCheckBlocked( CBaseEntity *pMoveTarget, Vector vMoveTargetPos )
{
	//Ya, we need to keep checking this or otherwise the movement will be jerky
	if (!m_bFlyBlocked) // || (GetNavigator()->IsGoalActive() && !GetNavigator()->CurWaypointIsGoal()))
	{
		//TERO: blocking detection

		Vector vecDest = (vMoveTargetPos - GetAbsOrigin());
		Vector vecSpeed = GetAbsVelocity();
		float flDist = VectorNormalize(vecDest);

		if (flDist > 512.0f)
		{
			flDist = 512.0f;
		}

		//DevMsg("flDist %f, velocity %f\n", flDist, vecSpeed.Length());

		vecDest = GetAbsOrigin() + (vecDest * flDist); 

		trace_t tr;
		AI_TraceHull( GetAbsOrigin(), vecDest, GetHullMins(), GetHullMaxs(), MASK_SOLID, this, HL2COLLISION_GROUP_GUNSHIP, &tr );

		float fTargetDist = (1.0f-tr.fraction) * ((GetAbsOrigin() - vecDest).Length());
			
		if ( ( tr.m_pEnt == pMoveTarget && pMoveTarget != NULL ) || ( fTargetDist < 20 ) )
		{
			m_bFlyBlocked = false;
		}
		else		
		{
			DevMsg("Blocked\n");
			/*if (tr.m_pEnt && tr.m_pEnt->GetOwnerEntity() == this)
			{
				DevMsg("hurrrr, it's us\n");
			}
			else if (tr.m_pEnt)
			{
				DevMsg("Blocker: %s\n", tr.m_pEnt->GetDebugName());
			}
			else
			{
				NDebugOverlay::Box( tr.endpos, Vector(-10,-10,-10), Vector(10,10,10), 0, 255, 0, 0, 0.1 );
			}*/

			//NDebugOverlay::Box( tr.endpos, Vector(-10,-10,-10), Vector(10,10,10), 0, 0, 255, 0, 1.1 );
			//NDebugOverlay::Box( tr.endpos, Vector(-10,-10,-10), Vector(10,10,10), 0, 255, 0, 0, 1.1 );

			m_bFlyBlocked = true;
		}
	}
}

// Purpose: Looks ahead to see if we are going to hit something. If we are, a
//			recommended avoidance path is returned.
// Input  : vecMoveDir - 
//			flSpeed - 
//			vecDeflect - 
// Output : Returns true if we hit something and need to deflect our course,
//			false if all is well.
//-----------------------------------------------------------------------------
bool CNPC_MantaRay::Probe( const Vector &vecMoveDir, float flSpeed, Vector &vecDeflect )
{
	//
	// Look 1/2 second ahead.
	//
	trace_t tr;
	AI_TraceHull( GetAbsOrigin(), GetAbsOrigin() + vecMoveDir * flSpeed, GetHullMins(), GetHullMaxs(), MASK_SOLID, this, HL2COLLISION_GROUP_GUNSHIP, &tr );
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

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CNPC_MantaRay::RayFly(float flInterval, Vector vMoveTargetPos, Vector vMoveGoal ) //, Vector vNextTargetPos
{
	Vector vecMoveDir = ( vMoveTargetPos - GetAbsOrigin() );

	Vector vecDist	  = vecMoveDir;
	vecDist.z		  = 0;
	VectorNormalize( vecMoveDir );

	/*if (flFirstDist < 100.0f)
	{
		Vector vecSecond = ( vNextTargetPos - GetAbsOrigin() );
		VectorNormalize( vecSecond );

		float flScale = flFirstDist / 100.0f;

		DevMsg("Scale %f\n", flScale);

		NDebugOverlay::Line( GetAbsOrigin(),	GetAbsOrigin() + (vecMoveDir * flFirstDist),	255,	0,		0,		true, 0.1 );

		vecMoveDir = ( flScale * vecMoveDir ) + ( (1.0f - flScale) * vecSecond );
		VectorNormalize( vecMoveDir );

		//NDebugOverlay::Box( vMoveTargetPos, GetHullMins(), GetHullMaxs(), 255,	0,		0,		0, 0.1 );
		//NDebugOverlay::Box( vTarget,		GetHullMins(), GetHullMaxs(), 0,	255,	0,		0, 0.1 );
		//NDebugOverlay::Box( vNextTargetPos, GetHullMins(), GetHullMaxs(), 0,	0,		255,	0, 0.1 );

		NDebugOverlay::Line( GetAbsOrigin(),	GetAbsOrigin() + (vecSecond * flFirstDist),		0,		255,	0,		true, 0.1 );
		NDebugOverlay::Line( GetAbsOrigin(),	GetAbsOrigin() + (vecMoveDir * flFirstDist),	0,		0,		255,	true, 0.1 );
	}*/

	float flDistance  = ( vMoveGoal - GetAbsOrigin() ).Length();

	Vector vecCurVel = GetAbsVelocity();
	float flCurSpeed = VectorNormalize(vecCurVel);

	//ANGLE

	float flTurning = 0.0f;


	QAngle angTarget, angCurrent;
	VectorAngles( vecMoveDir, angTarget );

	angCurrent = GetAbsAngles();

	float angleDiff = UTIL_AngleDiff( angTarget.y, angCurrent.y );


	float flAngSpeed	 = (sin( RemapValClamped( angleDiff, -RAY_ANGULAR_SPEED, RAY_ANGULAR_SPEED, -M_PI * 0.5f, M_PI * 0.5f))) * RAY_ANGULAR_SPEED;

	angTarget.y = flAngSpeed;
	angTarget.z = GetLocalAngularVelocity().z;
	angTarget.x = GetLocalAngularVelocity().x;


	SetLocalAngularVelocity( angTarget );

	if (angleDiff < 0.0f)
	{
		angleDiff = -angleDiff;
	}

	flTurning = clamp( angleDiff / RAY_TURNING_TO_SPEED_DIFFERENCE, 0.0f, 1.0f);

	SetPoseParameter( "turn", -flAngSpeed );

	//VELOCITY


	Vector vecForward;
	GetVectors(&vecForward, NULL, NULL);

	float flDot		 = clamp(DotProduct( vecMoveDir, vecForward ), 0.0f, 1.0f);
	float flSpeed	 = flDot * sin( RemapValClamped( flDistance, 0.0f, RAY_MAX_SPEED_DISTANCE, 0.0f, M_PI * 0.5f)) * RAY_MAX_SPEED;

	if (flSpeed > RAY_MAX_SPEED * (1.0f - flTurning))
	{
		flSpeed = RAY_MAX_SPEED * (1.0f - flTurning);
	}

	if (flSpeed < flTurning * RAY_MIN_TURNING_SPEED)
	{
		flSpeed = flTurning * RAY_MIN_TURNING_SPEED;
	}

	//flSpeed = clamp(flSpeed, flTurning * RAY_MIN_TURNING_SPEED, RAY_MAX_SPEED * (1.0f - flTurning));

	//DevMsg("Interval %f, old speed %f, new speed %f", flInterval, flCurSpeed, flSpeed);
	float flScale = flInterval * 2.0f;

	if (flScale > 0.50f)
	{
		flScale = 0.50f;
	}

	flSpeed = ((flScale + 0.5f) * flSpeed) + ((0.5f - flScale) * flCurSpeed);

	//DevMsg("calc speed %f\n", flSpeed);

	//TERO: hmmm BAD IDEA
	/*vecMoveDir.y = vecForward.y;
	vecMoveDir.x = vecForward.x;*/

	// Look to see if we are going to hit anything.
	Vector vecDeflect;
	if ( Probe( vecMoveDir, flSpeed * flInterval * 1.2, vecDeflect ) )
	{
		vecMoveDir = vecDeflect;
		VectorNormalize( vecMoveDir );
	}

	//NDebugOverlay::Box( vMoveTargetPos, GetHullMins(), GetHullMaxs(), 0, 255, 0, 0, 0.1 );

	SetAbsVelocity( vecMoveDir * flSpeed );
}

bool CNPC_MantaRay::RayProgressFlyPath()
{
	/*if (RayReachedPoint(GetNavigator()->GetCurWaypointPos()))
	{
		if (GetNavigator()->CurWaypointIsGoal())
		{
			return true;
		}

		GetNavigator()->AdvancePath();
	}
	return false;*/

	AI_ProgressFlyPathParams_t params( MASK_SOLID );
	params.waypointTolerance = 16.0f;
	params.strictPointTolerance = 16.0f;

	//TERO: trying this 
	params.bTrySimplify = false;

	float waypointDist = ( GetNavigator()->GetCurWaypointPos() - GetAbsOrigin()).Length();

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
			AI_TraceLine( GetAbsOrigin(), GetNavigator()->GetPath()->GetCurWaypoint()->GetNext()->GetPos(), MASK_SOLID, this, HL2COLLISION_GROUP_GUNSHIP, &tr );
			if ( tr.fraction == 1.0f )
			{
				GetNavigator()->AdvancePath();	
				return false; //AINPP_ADVANCED;
			}
		}

		if ( m_bFlyBlocked && GetNavigator()->SimplifyFlyPath( params ) )
		{
			m_bFlyBlocked = false;
			return false; 
		}
	}

	return false;
}

bool CNPC_MantaRay::RayReachedPoint(Vector vecOrigin)
{
	Vector vecDir = vecOrigin - GetAbsOrigin();
	Vector vecVelocity = GetAbsVelocity();

	float flZDist  = vecDir.z;
	vecDir.z	   = 0;
	float flXYDist = VectorNormalize(vecDir);  
	VectorNormalize(vecVelocity);
	float flDot	   = DotProduct( vecDir, vecVelocity );

	//DevMsg("	dot %f, z %f, xy %f\n", flDot, flZDist, flXYDist);

	if (flDot < 0.8)
	{
		if (flZDist < RAY_Z_DISTANCE_NOT_FACING && flXYDist < RAY_XY_DISTANCE_NOT_FACING)
		{
			return true;
		}
	}
	else
	{
		if (flZDist < RAY_Z_DISTANCE_FACING && flXYDist < RAY_XY_DISTANCE_FACING)
		{
			return true;
		}
	}

	return false;
}

void CNPC_MantaRay::NPCThink()
{
	BaseClass::NPCThink();

	m_BoneFollowerManager.UpdateBoneFollowers(this);

	if ( FlyToDeath() )
	{
		if ((m_flDeathTime < gpGlobals->curtime) || m_bFlyBlocked )
		{
			Explode();
		}

		return;
	}

	IdleSound();


	if ( ShouldLookForTeleporTarget() )
	{
		FindTeleportTarget( m_iszTeleportTargetName );
	}

	CBaseEntity *pTarget = NULL;

	//so that we can't change to teleporting suddenly
	if (m_hTeleportTarget && m_bTeleporting)
	{
		pTarget = m_hTeleportTarget;
	}
	else
	{
		pTarget = GetEnemyVehicle();

		//pTarget = (pTarget!= NULL) ? GetEnemy() : pTarget;

		if (pTarget == NULL)
		{
			pTarget = GetEnemy();
		}

		m_bTeleporting = false;
	}

	float flAngleX = 0;
	float flAngleZ = 0;

	if (pTarget)
	{
		CreateBeam(pTarget, true);

		Vector vecGun;
		QAngle angGun;
		GetAttachment( m_nGunAttachment, vecGun, angGun );

		Vector vecTarget;
		AngleVectors( angGun, &vecTarget );

		Vector	enemyDir = pTarget->WorldSpaceCenter() - vecGun;
		float flDist = VectorNormalize( enemyDir );
		
		QAngle angEnemy, angDir;
		Vector vecEnemy;

		VectorAngles(enemyDir, angEnemy);
		angDir = GetAbsAngles();
		angDir.x = angDir.z = 0;

		angEnemy = angEnemy - angDir;
		AngleVectors(angEnemy, &vecEnemy);

		//DevMsg("npc_mantaray: enemy x %f, y %f, z %f\n", vecEnemy.x, vecEnemy.y, vecEnemy.z);

		SetPoseParameter( "aim_pitch",	vecEnemy.x * -flDist);
		SetPoseParameter( "aim_yaw",	vecEnemy.y * flDist);

		trace_t tr;
		UTIL_TraceLine( vecGun, vecGun + (vecTarget * 512.0f), MASK_SOLID, this, HL2COLLISION_GROUP_GUNSHIP, &tr );

		flAngleX = angEnemy.x * 3.0f;
		flAngleZ = angEnemy.z * 3.0f;
	}
	else
	{	
		CreateBeam(NULL, false);
		SetPoseParameter( "aim_pitch",	0.0f);
		SetPoseParameter( "aim_yaw",	0.0f);
	}

	//UPDATE X ANGLES
	QAngle angCurrent, angCurrentVel;

	angCurrent = GetAbsAngles();

	angCurrentVel = GetLocalAngularVelocity();

	float angleDiff = UTIL_AngleDiff( clamp(flAngleX, -20.0f, 20.0f), angCurrent.x );
	float flAngSpeed	 = (sin( RemapValClamped( angleDiff, -RAY_ANGULAR_SPEED, RAY_ANGULAR_SPEED, -M_PI * 0.5f, M_PI * 0.5f))) * RAY_ANGULAR_SPEED;	

	angCurrentVel.x = flAngSpeed;

	//DevMsg("angular x-speed: %f, dir to enemy x %f\n", flAngSpeed, flAngleX); 

	angleDiff = UTIL_AngleDiff( clamp(flAngleZ, -20.0f, 20.0f), angCurrent.z );
	flAngSpeed	 = (sin( RemapValClamped( angleDiff, -RAY_ANGULAR_SPEED, RAY_ANGULAR_SPEED, -M_PI * 0.5f, M_PI * 0.5f))) * RAY_ANGULAR_SPEED;	

	angCurrentVel.z = flAngSpeed;

	SetLocalAngularVelocity( angCurrentVel );
}

void CNPC_MantaRay::Explode(bool bFire)
{
	EmitSound( RAY_SOUND_EXPLODE );

	CTakeDamageInfo info;
	info.SetDamage( 40000 );
	CalculateExplosiveDamageForce( &info, GetAbsVelocity(), GetAbsOrigin() );

	/*CRagdollProp *pRagdoll = (CRagdollProp *)CreateServerRagdoll( this, 0, info, COLLISION_GROUP_NONE );
	if ( pRagdoll )
	{
		pRagdoll->FadeOut(6.0f, 1.0f);
	}*/

	Vector vecChunkPos = GetAbsOrigin();
	QAngle vecChunkAngles = GetAbsAngles();

	for (int i=0; i<4; i++)
	{
		Vector vecRand = RandomVector( -64.0f, 64.0f );
		vecRand.z *= 0.1f;
		DispatchParticleEffect( "antlion_gib_02", vecChunkPos + vecRand, RandomAngle( 0, 360 ) );
	}

	for (int i=0; i<RAY_MAX_CHUNKS; i++)
	{
			// Drop a flaming, smoking chunk.
		CGib *pChunk = CREATE_ENTITY( CGib, "gib" );
		pChunk->Spawn( s_pChunkModelName[i], 3.0f );
		pChunk->SetBloodColor( BLOOD_COLOR_YELLOW );

		pChunk->SetAbsOrigin( vecChunkPos );
		pChunk->SetAbsAngles( vecChunkAngles );

		pChunk->SetOwnerEntity( this );
	
		//pChunk->m_lifeTime = 5.0f;
	
		pChunk->SetCollisionGroup( COLLISION_GROUP_DEBRIS );
	
		// Set the velocity
		Vector vecVelocity;
		AngularImpulse angImpulse;

		QAngle angles;
		angles.x = random->RandomFloat( -70, 20 );
		angles.y = random->RandomFloat( 0, 360 );
		angles.z = 0.0f;
		AngleVectors( angles, &vecVelocity );
	
		vecVelocity *= random->RandomFloat( 32, 128 );
		vecVelocity += GetAbsVelocity();

		angImpulse = RandomAngularImpulse( -180, 180 );

		pChunk->SetAbsVelocity( vecVelocity );

		IPhysicsObject *pPhysicsObject = pChunk->VPhysicsInitNormal( SOLID_VPHYSICS, pChunk->GetSolidFlags(), false );
		
		if ( pPhysicsObject )
		{
			pPhysicsObject->EnableMotion( true );
			pPhysicsObject->SetVelocity(&vecVelocity, &angImpulse );
		}

		if (bFire)
		{
			CEntityFlame *pFlame = CEntityFlame::Create( pChunk, false );
			if ( pFlame != NULL )
			{
				pFlame->SetLifetime( pChunk->m_lifeTime );
			}
		}
	}

	//UTIL_Remove( this );

	SetThink( &CNPC_MantaRay::SUB_Remove );
	SetNextThink( gpGlobals->curtime + 0.1f );
	AddEffects( EF_NODRAW );

	LightExplode( GetAbsOrigin() );
}

//-----------------------------------------------------------------------------
// Purpose: Set the gunship's paddles flailing!
//-----------------------------------------------------------------------------
void CNPC_MantaRay::Event_Killed( const CTakeDamageInfo &info )
{
	m_takedamage = DAMAGE_NO;

	if (m_hTeleportTarget)
	{
		m_hTeleportTarget->m_bIsBeingUsed = false;
	}

	//StopCannonBurst();

	// Replace the rotor sound with broken engine sound.
	/*CSoundEnvelopeController &controller = CSoundEnvelopeController::GetController();
	controller.SoundDestroy( m_pRotorSound );

	// BUGBUG: Isn't this sound just going to get stomped when the base class calls StopLoopingSounds() ??
	CPASAttenuationFilter filter2( this );
	m_pRotorSound = controller.SoundCreate( filter2, entindex(), "NPC_CombineGunship.DyingSound" );
	controller.Play( m_pRotorSound, 1.0, 100 );*/

	//m_lifeState = LIFE_DYING;

	m_flAttackStartedTime = 0;
	m_flDeathTime = gpGlobals->curtime + 2.0f;

	StopSound(RAY_BEAM_SOUND);
	StopLoopingSounds();
	EmitSound(RAY_SOUND_DEATH);

	m_OnDeath.FireOutput( info.GetAttacker(), this );
	SendOnKilledGameEvent( info );

	//TERO: if we get blast damage, explode right away
	if ( info.GetDamageType() & DMG_BLAST )
	{
		Explode(true);
	}
}

bool CNPC_MantaRay::FlyToDeath()
{
	return (m_flDeathTime != 0);
}

Class_T CNPC_MantaRay::Classify()
{
	if (FlyToDeath())
	{
		return CLASS_NONE;
	}
	
	if (m_bTeleporter)
	{
		return CLASS_MANTARAY_TELEPORTER;
	}

	return CLASS_ALIENCONTROLLER; //MANTARAY_HOSTILE;
}


//------------------------------------------------------------------------------
// Gets a vehicle the enemy is in (if any)
//------------------------------------------------------------------------------
CBaseEntity *CNPC_MantaRay::GetEnemyVehicle()
{
	return NULL;

	if ( !GetEnemy() )
		return NULL;

	if ( !GetEnemy()->IsPlayer() )
		return NULL;

	return static_cast<CBasePlayer*>(GetEnemy())->GetVehicleEntity();
}

void CNPC_MantaRay::CreateBeam(CBaseEntity *pAttack, bool bCreate)
{
	if (m_flNextAttack > gpGlobals->curtime)
	{
		return;
	}

	Vector vecGun, vecUp, vecEnemy;
	GetVectors(NULL, NULL, &vecUp);
	GetAttachment( m_nGunAttachment, vecGun );

	vecUp = -vecUp;

	bool bCanSee = false;
	bool bAttack = false;

	float flDist = 0.0f;

	if (pAttack)
	{
		vecEnemy = pAttack->WorldSpaceCenter() - vecGun;
		flDist = VectorNormalize(vecEnemy);
	}
	
	if (!pAttack || flDist > RAY_FACE_ENEMY_DISTANCE)
	{
		vecEnemy = vecUp;
	}

	if ( m_flAttackStartedTime != 0 && (m_flAttackStartedTime + HLSS_MANTARAY_AIM_TIME < gpGlobals->curtime && m_flAttackStartedTime + HLSS_MANTARAY_SHOOT_TIME > gpGlobals->curtime))
	{
		bAttack = true;
	}

	if ( flDist <  RAY_FACE_ENEMY_DISTANCE || bAttack )
	{
		trace_t tr;
		UTIL_TraceLine( vecGun, vecGun + (vecEnemy * RAY_FACE_ENEMY_DISTANCE), MASK_SOLID, this, HL2COLLISION_GROUP_GUNSHIP, &tr );
		m_vecLaserEndPos = tr.endpos;

		if (pAttack)
		{
			float flDot = DotProduct( vecEnemy, vecUp );

			if (flDot > 0.50f && tr.m_pEnt)
			{
				pAttack = tr.m_pEnt;
				bCanSee = true;
			}
		}
	}

	if ( bCanSee || bAttack ) //DotProduct( vecEnemy, vecUp ) > 0.8 )
	{
		CSoundEnt::InsertSound( SOUND_DANGER, m_vecLaserEndPos, 256.0f, 0.2, this, SOUNDENT_CHANNEL_WEAPON, NULL );

		//Update
		if (pAttack && !m_bTeleporting && bCanSee)
		{
			m_iLaserTarget = pAttack->entindex();
		}
		else
		{
			m_iLaserTarget = -1;
		}

		if (m_flAttackStartedTime == 0)
		{
			m_flAttackStartedTime = gpGlobals->curtime;
			m_bBallyStarted = false;

			EmitSound(RAY_BEAM_SOUND);

			/*unsigned char uchAttachment1 = m_nGunAttachment;	
			EntityMessageBegin( this, true );
				WRITE_BYTE( 0 );
				WRITE_BYTE( uchAttachment1 );

			if (pAttack && !bTeleporting)
			{
				WRITE_LONG( pAttack->entindex() );
			}
			else
			{
				WRITE_LONG( -1 );
			}
			
			MessageEnd();*/
		}
		else if (m_flAttackStartedTime + HLSS_MANTARAY_SHOOT_TIME < gpGlobals->curtime )
		{
			m_flAttackStartedTime = 0;

			StopSound(RAY_BEAM_SOUND);

			m_flNextAttack = gpGlobals->curtime + RAY_ATTACK_DELAY;
			
			if (m_bTeleporting)
			{
				if (SpawnNPCs(m_hTeleportTarget->WorldSpaceCenter(), 64.0f))
				{
					m_hTeleportTarget->m_OnSpawnedNPC.FireOutput( this, this );
				}

				m_hTeleportTarget->m_OnTeleported.FireOutput( this, this );

				m_hTeleportTarget->m_bIsBeingUsed = false;
				m_hTeleportTarget = NULL;
			}
			else
			{
				LightExplode( m_vecLaserEndPos );
			}

			//TERO: we can change to teleporting again
			m_bTeleporting = true;
		}
		else if (m_flAttackStartedTime + HLSS_MANTARAY_AIM_TIME < gpGlobals->curtime)
		{
			if (!m_bBallyStarted)
			{	
				EmitSound(RAY_LIGHT_START_SOUND);
				m_bBallyStarted = true;
			}
		}
	}
	else
	{
		StopSound(RAY_BEAM_SOUND);
		m_flAttackStartedTime = 0;

		//TERO: we can change to teleporting again
		m_bTeleporting = true; 

		if ( m_pBeam )
		{
			UTIL_Remove( m_pBeam );
			m_pBeam = NULL;
		}

		if ( m_pLightGlow )
		{
			UTIL_Remove( m_pLightGlow );
			m_pLightGlow = NULL;
		}
	}
}

void CNPC_MantaRay::LightExplode(Vector vecOrigin)
{
	//VORTIGAUNT
	CSprite *pBlastSprite = CSprite::SpriteCreate( "sprites/vortring1.vmt", vecOrigin, true );
	if ( pBlastSprite != NULL )
	{
		pBlastSprite->SetTransparency( kRenderTransAddFrameBlend, 255, 128, 0, 255, kRenderFxNone );
		pBlastSprite->SetBrightness( 255 );
		pBlastSprite->SetScale( random->RandomFloat( 1.0f, 1.5f ) );
		pBlastSprite->AnimateAndDie( 45.0f );
		pBlastSprite->EmitSound( "NPC_Vortigaunt.Explode" );
	}

	CPVSFilter filter( vecOrigin );
	te->GaussExplosion( filter, 0.0f, vecOrigin, Vector( 0, 0, 1 ), 0 );
	//VORTIGAUNT

	EmitSound(RAY_LIGHT_SOUND);

	RadiusDamage( CTakeDamageInfo( this, this, 25, DMG_SHOCK ), vecOrigin, 64, CLASS_NONE, NULL );
}

//------------------------------------------------------------------------------
// On Remove
//------------------------------------------------------------------------------
void CNPC_MantaRay::UpdateOnRemove()
{
	BaseClass::UpdateOnRemove();
	m_BoneFollowerManager.DestroyBoneFollowers();

	StopLoopingSounds();

	if (m_hTeleportTarget)
	{
		m_hTeleportTarget->m_bIsBeingUsed = false;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &targetPos - 
// Output : CBaseEntity
//-----------------------------------------------------------------------------
CPathTrack *CNPC_MantaRay::BestPointOnPath( CPathTrack *pPath, const Vector &targetPos, float flAvoidRadius, bool facing )
{
	// Find the node nearest to the destination path target if a path is not specified
	if ( pPath == NULL )
	{
		return NULL;
	}

	// If the path node we're trying to use is not valid, then we're done.
	if ( CPathTrack::ValidPath( pPath ) == NULL )
	{
		//FIXME: Implement
		Assert(0);
		return NULL;
	}

	// Our target may be in a vehicle
	CBaseEntity *pVehicle = NULL;
	CBaseEntity *pTargetEnt = GetEnemy(); //GetTrackPatherTargetEnt();	
	if ( pTargetEnt != NULL )
	{
		CBaseCombatCharacter *pCCTarget = pTargetEnt->MyCombatCharacterPointer();
		if ( pCCTarget != NULL && pCCTarget->IsInAVehicle() )
		{
			pVehicle = pCCTarget->GetVehicleEntity();
		}
	}

	// Faster math...
	flAvoidRadius *= flAvoidRadius;

	// Find the nearest node to the target (going forward)
	CPathTrack *pNearestPath	= NULL;
	float		flNearestDist	= 999999999;
	float		flPathDist;

	Vector forward;
	GetVectors(&forward, NULL, NULL);

	//float flFarthestDistSqr = ( m_flFarthestPathDist - 2.0f * m_flTargetDistanceThreshold );
	//flFarthestDistSqr *= flFarthestDistSqr;

	// NOTE: Gotta do it this crazy way because paths can be one-way.
	for ( int i = 0; i < 2; ++i )
	{
		int loopCheck = 0;
		CPathTrack *pTravPath = pPath;
		CPathTrack *pNextPath;

		BEGIN_PATH_TRACK_ITERATION();
		for ( ; CPathTrack::ValidPath( pTravPath ); pTravPath = pNextPath, loopCheck++ )
		{
			// Circular loop checking
			if ( pTravPath->HasBeenVisited() )
				break;

			pTravPath->Visit();

			pNextPath = (i == 0) ? pTravPath->GetPrevious() : pTravPath->GetNext();

			Vector vecToTrack = pTravPath->GetAbsOrigin() - targetPos;

			// Find the distance between this test point and our goal point
			flPathDist = vecToTrack.LengthSqr();

			if ( flPathDist >= flNearestDist ) 
				continue;

			// Don't choose points that are within the avoid radius
			if ( flAvoidRadius && vecToTrack.Length2DSqr() <= flAvoidRadius )
				continue;

			if ( facing )
			{
				VectorNormalize(vecToTrack);
				if (DotProduct(forward, vecToTrack) < 0.8)
					continue;
			}

			/*if ( visible )
			{
				// If it has to be visible, run those checks
				CBaseEntity *pBlocker = FindTrackBlocker( pTravPath->GetAbsOrigin(), targetPos );

				// Check to see if we've hit the target, or the player's vehicle if it's a player in a vehicle
				bool bHitTarget = ( pTargetEnt && ( pTargetEnt == pBlocker ) ) ||
									( pVehicle && ( pVehicle == pBlocker ) );

				// If we hit something, and it wasn't the target or his vehicle, then no dice
				// If we hit the target and forced move was set, *still* no dice
				if ( (pBlocker != NULL) && ( !bHitTarget || m_bForcedMove ) )
					continue;
			}*/

			pNearestPath	= pTravPath;
			flNearestDist	= flPathDist;
		}
	}

	return pNearestPath;
}

/*bool CNPC_MantaRay::IsHeavyDamage( const CTakeDamageInfo &info )
{
	// Struck by blast
	if ( info.GetDamageType() & DMG_BLAST )
	{
		return true;
	}

	return BaseClass::IsHeavyDamage( info );
}*/

int CNPC_MantaRay::OnTakeDamage_Alive( const CTakeDamageInfo &info )
{
	if (FlyToDeath())
	{
		return 0;
	}

	int iReturn = BaseClass::OnTakeDamage_Alive( info );

	PainSound( info );

	if ( m_iHealth < m_iDamageToEscape )
	{
		if ( m_flEscapeTimeEnds < gpGlobals->curtime ) //!m_pCurrentEscapePath && 
		{
			m_pCurrentEscapePath = BestPointOnPath( m_pCurrentPathTarget , WorldSpaceCenter(), 0, false );

			if (m_pCurrentEscapePath)
			{
				m_flEscapeTimeEnds = gpGlobals->curtime + RAY_ESCAPE_TIME;
				DevMsg("npc_mantaray: STARTING ESCAPE\n");

				//NDebugOverlay::Box( m_pCurrentEscapePath->GetAbsOrigin(), Vector(-4,-4,-4), Vector(4,4,4), 0, 0, 255, 0, 0.1 );
			}
		}

		PlayFlinchGesture();
		m_iDamageToEscape = m_iHealth - RAY_ESCAPE_DAMAGE_DIFFERENCE;	
	}

	return iReturn;
}

//-----------------------------------------------------------------------------
// Determines the best type of flinch anim to play.
//-----------------------------------------------------------------------------
Activity CNPC_MantaRay::GetFlinchActivity( bool bHeavyDamage, bool bGesture )
{
	if (bGesture)
	{
		return ACT_GESTURE_BIG_FLINCH;
	}

	return ACT_INVALID;

	//return BaseClass::GetFlinchActivity( bHeavyDamage, true );
}

bool CNPC_MantaRay::ShouldLookForTeleporTarget()
{
	if (m_hTeleportTarget != NULL)
	{
		return false;
	}

	if (m_iszTeleportTargetName == NULL_STRING)
	{
		return false;
	}

	bool bDead = false;

	for (int i=0; i<HLSS_MANTARAY_MAX_NPCS; i++)
	{
		if (m_bNPC_Alive[i])
		{
			if (m_bMustBeDead[i])
			{
				return false;
			}
		}
		else if (m_iszTemplateData[i] != NULL_STRING)
		{
			bDead = true;
		}
	}
	
	return bDead;
}

static void DispatchActivate( CBaseEntity *pEntity )
{
	bool bAsyncAnims = mdlcache->SetAsyncLoad( MDLCACHE_ANIMBLOCK, false );
	pEntity->Activate();
	mdlcache->SetAsyncLoad( MDLCACHE_ANIMBLOCK, bAsyncAnims );
}

bool CNPC_MantaRay::SpawnNPCs(Vector vecOrigin, float flRadius)
{
	DevMsg("Calling SpawnNPCs()\n");

	int iNum=0;

	for (int i=0; i<HLSS_MANTARAY_MAX_NPCS; i++)
	{
		DevMsg("%d:", i);

		if (!m_bNPC_Alive[i] && m_iszTemplateData[i] != NULL_STRING)
		{
			DevMsg("creating");

			CAI_BaseNPC	*pNPC = NULL;
			CBaseEntity *pEntity = NULL;
			MapEntity_ParseEntity( pEntity, STRING(m_iszTemplateData[i]), NULL );
			if ( pEntity != NULL )
			{
				pNPC = (CAI_BaseNPC *)pEntity;
			}

			if ( !pNPC )
			{
				Warning("NULL Ent in %s with template %s!\n", GetDebugName(), m_iszTemplateName[i] );
				continue;
			}

			if (!PlaceNPCInRadius(pNPC, vecOrigin, flRadius))
			{
				Warning("Couldn't find position on radius\n");
				UTIL_RemoveImmediate( pNPC );
				continue;
			}

			pNPC->AddSpawnFlags( SF_NPC_FADE_CORPSE );
			pNPC->RemoveSpawnFlags( SF_NPC_TEMPLATE );

			DispatchSpawn( pNPC );
			pNPC->SetOwnerEntity( this );
			DispatchActivate( pNPC );

			//TERO: delay attacks a bit so that they wont start taking out generators right away
			float flAttackTime = gpGlobals->curtime + 1.5f;
			if ( pNPC->m_flNextAttack < flAttackTime )
			{
				pNPC->m_flNextAttack = flAttackTime;
			}

			m_bNPC_Alive[i] = true;

			m_OnSpawnedNPC.FireOutput( this, this );

			DispatchParticleEffect( "mantaray_teleport_npc", PATTACH_ABSORIGIN, pNPC );

			iNum++;
		}
	}

	DispatchParticleEffect( "mantaray_teleport", vecOrigin, RandomAngle( 0, 360 ) );
	m_OnTeleported.FireOutput( this, this );
	EmitSound(RAY_SOUND_TELEPORT);

	return (iNum>0);
}

//-----------------------------------------------------------------------------
// Purpose: Find a place to spawn an npc within my radius.
//			Right now this function tries to place them on the perimeter of radius.
// Output : false if we couldn't find a spot!
//-----------------------------------------------------------------------------
bool CNPC_MantaRay::PlaceNPCInRadius( CAI_BaseNPC *pNPC, Vector vecOrigin, float flRadius )
{
	Vector vPos;

	if ( CAI_BaseNPC::FindSpotForNPCInRadius( &vPos, vecOrigin, pNPC, flRadius ) )
	{
		pNPC->SetAbsOrigin( vPos );
		return true;
	}

	DevMsg("**Failed to place NPC in radius!\n");
	return false;
}
