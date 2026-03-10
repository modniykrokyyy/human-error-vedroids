//=================== Half-Life 2: Short Stories Mod 2007 =====================//
//
// Purpose:	Manhack Vehicle, see weapon_manhack
//			CONTROL. WE HAVE IT.
//
//=============================================================================//

#include "cbase.h"
#include "vehicle_base.h"
#include "movevars_shared.h"
#include "hl2_player.h"
#include "globalstate.h"

#include "in_buttons.h"

#include "vehicle_manhack.h"
#include "npc_manhack.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

ConVar  manhack_blend_view("hlss_manhack_blend_view", "16");

ConVar	manhack_small_distances_max_distance("hlss_manhack_small_distances_max_distance", "1024");
ConVar	manhack_small_distances_max_height("hlss_manhack_small_distances_max_height", "196");
ConVar	manhack_max_distance("hlss_manhack_max_distance", "2304");
ConVar	manhack_dont_draw("hlss_manhack_dont_draw", "1" );

#define MANHACK_DELTA_LENGTH_MAX	24.0f			// 1 foot
#define MANHACK_FRAMETIME_MIN		1e-6

int HLSS_SelectTargetType(CBaseEntity *pEntity);

// global pointer to Larson for fast lookups
CEntityClassList<CPropVehicleManhack> g_ManhackVehicleList;
template <>  CPropVehicleManhack *CEntityClassList< CPropVehicleManhack>::m_pClassList = NULL;

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------

BEGIN_DATADESC( CPropVehicleManhack )
	//DEFINE_INPUTFUNC( FIELD_STRING, "ForcePlayerIn", InputForcePlayerIn ),
	//DEFINE_INPUTFUNC( FIELD_VOID, "ForcePlayerOut", InputForcePlayerOut ),

	// Keys
	DEFINE_EMBEDDED( m_ServerVehicle ),

	DEFINE_FIELD( m_hPlayer,				FIELD_EHANDLE ),
	DEFINE_FIELD( m_hTarget,				FIELD_EHANDLE ),
	DEFINE_FIELD( m_iTargetType,			FIELD_INTEGER ),
	DEFINE_FIELD( m_bDriverDucked,			FIELD_BOOLEAN ),
	DEFINE_FIELD( m_vecManhackEye,			FIELD_POSITION_VECTOR ),
	DEFINE_FIELD( m_angManhackEye,			FIELD_VECTOR ),
	DEFINE_FIELD( m_iManhackHealth,			FIELD_INTEGER ),
	DEFINE_FIELD( m_iManhackDistance,		FIELD_INTEGER ),
	DEFINE_FIELD( m_bHadDriver,				FIELD_BOOLEAN ),

	DEFINE_FIELD( m_hManhack,				FIELD_EHANDLE ),
	DEFINE_ARRAY( m_hSetOfManhacks,			FIELD_EHANDLE, NUMBER_OF_MAX_CONTROLLABLE_MANHACKS),
	DEFINE_FIELD( m_iNumberOfManhacks,		FIELD_INTEGER ),

	DEFINE_FIELD( m_iCurrentManhackIndex,	FIELD_INTEGER ),
	DEFINE_FIELD( m_fTimeToChangeManhack,	FIELD_TIME ),

	DEFINE_FIELD( m_iHintTimesShown,		FIELD_INTEGER ),
	DEFINE_FIELD( m_iHintNoSwapTimesShown,	FIELD_INTEGER ),

	DEFINE_FIELD( m_vecLastEyePos,			FIELD_POSITION_VECTOR ),
	DEFINE_FIELD( m_vecLastEyeTarget,		FIELD_POSITION_VECTOR ),
	DEFINE_FIELD( m_vecEyeSpeed,			FIELD_POSITION_VECTOR ),
	DEFINE_FIELD( m_vecTargetSpeed,			FIELD_POSITION_VECTOR ),

	DEFINE_FIELD( m_vecFlyingDirection,		FIELD_VECTOR ),

	DEFINE_FIELD( m_flLastWarningSound,		FIELD_TIME ),

	DEFINE_KEYFIELD( m_vehicleScript,		FIELD_STRING, "vehiclescript" ),
END_DATADESC()

LINK_ENTITY_TO_CLASS( vehicle_manhack, CPropVehicleManhack );


IMPLEMENT_SERVERCLASS_ST(CPropVehicleManhack, DT_PropVehicleManhack)
	SendPropEHandle(SENDINFO(m_hPlayer)),
	SendPropEHandle(SENDINFO(m_hTarget)),
	SendPropInt(SENDINFO(m_iTargetType)),
	SendPropQAngles(SENDINFO(m_angManhackEye), SPROP_CHANGES_OFTEN),
	SendPropVector(SENDINFO(m_vecManhackEye), SPROP_CHANGES_OFTEN),
	SendPropVector(SENDINFO(m_vecFlyingDirection), SPROP_CHANGES_OFTEN),
	SendPropInt(SENDINFO(m_iManhackHealth), SPROP_CHANGES_OFTEN),		//SPROP_CHANGES_OFTEN used to be 9
	SendPropInt(SENDINFO(m_iManhackDistance), SPROP_CHANGES_OFTEN),
END_SEND_TABLE();


CPropVehicleManhack::CPropVehicleManhack( void )
{
	m_ServerVehicle.SetVehicle( this );

	g_ManhackVehicleList.Insert(this);
}

CPropVehicleManhack::~CPropVehicleManhack( void )
{
	g_ManhackVehicleList.Remove(this);
}

//=========================================================
// Returns a pointer to Eloise's entity
//=========================================================
CPropVehicleManhack *CPropVehicleManhack::GetManhackVehicle( void )
{
	return g_ManhackVehicleList.m_pClassList;
}


void CPropVehicleManhack::Precache( void )
{
	SetModelName( AllocPooledString("models/vehicles/vehicle_manhackcontroller.mdl") );

	BaseClass::Precache();

//	PrecacheModel( GetModelName() );
	PrecacheScriptSound("NPC_Turret.Retire");

	m_ServerVehicle.Initialize( "scripts/vehicles/manhack.txt" );
}


//------------------------------------------------
// Spawn
//------------------------------------------------
void CPropVehicleManhack::Spawn( void )
{
	Precache();

	SetModel( "models/vehicles/vehicle_manhackcontroller.mdl" );

	SetCollisionGroup( COLLISION_GROUP_VEHICLE );

	BaseClass::Spawn();

	SetSolid( SOLID_BBOX );
	AddSolidFlags( FSOLID_NOT_SOLID );

	SetCollisionBounds( Vector(-16,-16,0), Vector(16,16,72) );
	SetMoveType( MOVETYPE_NONE );	

	//SetRenderMode(kRenderTransColor );
	//SetRenderColorA(0);
	AddEffects( EF_NODRAW );

	//VPhysicsInitShadow( true, false );

	m_takedamage = DAMAGE_EVENTS_ONLY;

	m_hManhack = NULL;
	for (int i=0; i<NUMBER_OF_MAX_CONTROLLABLE_MANHACKS; i++)
	{
		m_hSetOfManhacks[i] = NULL;
	}

	m_hTarget = NULL;

	m_bDriverDucked	= false;
	m_bHadDriver	= false;
	m_fTimeToChangeManhack = 0;
	m_iHintTimesShown = 0;
	m_iHintNoSwapTimesShown = 0;
	m_iCurrentManhackIndex = -1;

	m_flLastWarningSound = 0;
}

bool CPropVehicleManhack::CreateControllableManhack( const Vector &position, const QAngle &angles, CBaseEntity *pOwner )
{
	int manhackIndex=-1;
	for (int i=0; i<NUMBER_OF_MAX_CONTROLLABLE_MANHACKS; i++)
	{
		if (m_hSetOfManhacks[i] == NULL)
		{
			manhackIndex = i;
			break;
		}
	}

	if (manhackIndex<0)
		return false;

	CNPC_Manhack *pManhack = (CNPC_Manhack *)CBaseEntity::Create( "npc_manhack", position, angles, pOwner );

	if (pManhack == NULL)
		return false;
	
	pManhack->KeyValue( "squadname", "controllable_manhack_squad" );
	pManhack->Spawn();
	pManhack->Activate();
	pManhack->AddSpawnFlags( SF_MANHACK_PACKED_UP);
	pManhack->ShouldFollowPlayer(true);

	pManhack->KeyValue( "targetname", "controllable_manhack" );

	//TERO: commented out because otherwise you would have to set the last one free before setting this controllable
	//pManhack->SetControllable(true);

	m_hManhack = pManhack;

	m_iManhackHealth = pManhack->m_iHealth;

	if (GlobalEntity_GetState("manhacks_use_short_distances") == GLOBAL_ON)
	{
		Vector vecDistance = (pManhack->GetAbsOrigin() - GetAbsOrigin());
		float flHeight	= vecDistance.z / manhack_small_distances_max_height.GetFloat();
		vecDistance.z	= 0.0f;
		vecDistance		= vecDistance / manhack_small_distances_max_distance.GetFloat();		
		vecDistance.z	= flHeight;
		m_iManhackDistance	= (int)( vecDistance.Length() * 100.0f );
	}
	else
	{
		m_iManhackDistance	= (int)(( (float)(pManhack->GetAbsOrigin() - GetAbsOrigin()).Length()) / manhack_max_distance.GetFloat() * 100.0f);
	}

	m_iCurrentManhackIndex = manhackIndex;

	m_hSetOfManhacks[m_iCurrentManhackIndex] = pManhack;

	//TERO: this is the same as SetLocator but with the difference of pOwner
	CBasePlayer *pPlayer = ToBasePlayer( pOwner );
	if (pPlayer)
	{
		CHL2_Player *pHL2Player = dynamic_cast<CHL2_Player*>( pPlayer );
		if (pHL2Player)
		{
			DevMsg("Setting locator\n");
			pHL2Player->SetLocatorTargetEntity( m_hManhack );
		}
	}

	//This is normally in Spawn but we don't want to think before we have a manhack
	//SetNextThink( gpGlobals->curtime );
		
	return true;
}

void CPropVehicleManhack::SetLocator( CBaseEntity *pEntity )
{
	CBasePlayer *pPlayer = m_hPlayer;
	if (pPlayer)
	{
		CHL2_Player *pHL2Player = dynamic_cast<CHL2_Player*>( pPlayer );
		if (pHL2Player)
		{
			DevMsg("Setting locator\n");
			pHL2Player->SetLocatorTargetEntity( pEntity );
		}
	}
}

int CPropVehicleManhack::GetNumberOfHacks(bool bReleaseSlot)
{
	int numberOfHacks=0;
	CNPC_Manhack *newManhack;

	if (bReleaseSlot)
	{
		CBasePlayer *pPlayer = ToBasePlayer(GetOwnerEntity());
		if (pPlayer)
			SetAbsOrigin(pPlayer->GetAbsOrigin());
	}

	for (int i=0; i<NUMBER_OF_MAX_CONTROLLABLE_MANHACKS; i++)
	{
		if (m_hSetOfManhacks[i]!=NULL)
		{
			newManhack = dynamic_cast<CNPC_Manhack*>((CBaseEntity*)m_hSetOfManhacks[i]);

			//TERO: if a manhack is too far away, lets destroy it
			if ( bReleaseSlot && 
				 (newManhack->GetAbsOrigin() - GetAbsOrigin()).Length() >= manhack_max_distance.GetFloat() )
			{
				if (m_iNumberOfManhacks >= NUMBER_OF_MAX_CONTROLLABLE_MANHACKS)
				{
					newManhack->SetHealth(-1);
					m_hSetOfManhacks[i] = NULL;
				} else 
				{
					CBasePlayer *pPlayer = ToBasePlayer(GetOwnerEntity());
					if (pPlayer)
						newManhack->ComeBackToPlayer(pPlayer, 8.0f);
				}
			}
			else
				numberOfHacks++;
		}
	}

	m_iNumberOfManhacks = numberOfHacks;

	return numberOfHacks;
}

bool CPropVehicleManhack::FindNextManhack(bool bRemoveIfNone)
{
	int manhackIndex=-1;
	CNPC_Manhack *newManhack;

	bool bFound = false;

	for (int i=1; i<NUMBER_OF_MAX_CONTROLLABLE_MANHACKS; i++)
	{
		manhackIndex = (m_iCurrentManhackIndex + i) % NUMBER_OF_MAX_CONTROLLABLE_MANHACKS;
		
		if (m_hSetOfManhacks[manhackIndex] != NULL)
		{
			newManhack = dynamic_cast<CNPC_Manhack*>((CBaseEntity*)m_hSetOfManhacks[manhackIndex]);

			int iDistance = -1;

			if (newManhack != NULL )
			{
				if (GlobalEntity_GetState("manhacks_use_short_distances") == GLOBAL_ON)
				{
					Vector vecDistance = (newManhack->GetAbsOrigin() - GetAbsOrigin());
					float flHeight	= vecDistance.z / manhack_small_distances_max_height.GetFloat();
					vecDistance.z	= 0.0f;
					vecDistance		= vecDistance / manhack_small_distances_max_distance.GetFloat();		
					vecDistance.z	= flHeight;
					iDistance	= (int)( vecDistance.Length() * 100.0f );
				}
				else
				{
					iDistance = (int)( (float) (newManhack->GetAbsOrigin() - GetAbsOrigin()).Length() / manhack_max_distance.GetFloat() * 100.0f);
				}
			}

			if (newManhack != NULL && newManhack->m_iHealth > 0 )//(newManhack->GetAbsOrigin() - GetAbsOrigin()).Length() < manhack_max_distance.GetFloat() )
			{
				if (iDistance <= 100)
				{
					m_iCurrentManhackIndex = manhackIndex;
					m_hManhack = m_hSetOfManhacks[manhackIndex];
					SetLocator( m_hManhack );

					m_iManhackHealth = newManhack->m_iHealth;

					if (GetDriver())
					{
						newManhack->SetControllable(true);
						if (manhack_dont_draw.GetBool())
						{
							newManhack->AddEffects( EF_NODRAW );
							newManhack->ShowRedGlow(false);
						}
					}
					return true;
				}
			}
			else 
			{
				m_hSetOfManhacks[manhackIndex] = NULL;
			}
		}
	}

	if (!bFound && bRemoveIfNone)
	{
		SetLocator( NULL );
		ForcePlayerOut();
		DestroyAllManhacks();
		UTIL_Remove(this);

		return true;
	}

	return false;
}

void CPropVehicleManhack::DestroyAllManhacks(void)
{
	CNPC_Manhack *newManhack;

	for (int i=0; i<NUMBER_OF_MAX_CONTROLLABLE_MANHACKS; i++)
	{
		if (m_hSetOfManhacks[i] != NULL)
		{
			newManhack = dynamic_cast<CNPC_Manhack*>((CBaseEntity*)m_hSetOfManhacks[i]);

			if (newManhack)
			{
				newManhack->SetHealth(-1);
				m_hSetOfManhacks[i] = NULL;
			}
		}
	}

}

void CPropVehicleManhack::CallManhacksBack(CBasePlayer *pPlayer, float flComeBackTime)
{
	CNPC_Manhack *newManhack;

	for (int i=0; i<NUMBER_OF_MAX_CONTROLLABLE_MANHACKS; i++)
	{
		if (m_hSetOfManhacks[i] != NULL)
		{
			newManhack = dynamic_cast<CNPC_Manhack*>((CBaseEntity*)m_hSetOfManhacks[i]);

			if (newManhack)
			{
				newManhack->ComeBackToPlayer(pPlayer, flComeBackTime);
			}
		}
	}
}

void CPropVehicleManhack::TellManhacksToGoThere(CBasePlayer *pPlayer, float flGoThereTime)
{
	CNPC_Manhack *newManhack;

	for (int i=0; i<NUMBER_OF_MAX_CONTROLLABLE_MANHACKS; i++)
	{
		if (m_hSetOfManhacks[i] != NULL)
		{
			newManhack = dynamic_cast<CNPC_Manhack*>((CBaseEntity*)m_hSetOfManhacks[i]);

			if (newManhack)
			{
				newManhack->GoThere(pPlayer, flGoThereTime);
			}
		}
	}
}

bool CPropVehicleManhack::HasDrivableManhack(CBasePlayer *pPlayer)
{
	if (m_hManhack)
	{
		m_iManhackHealth = m_hManhack->m_iHealth;
		m_iManhackDistance = UpdateManhackDistance(pPlayer);
	}


	if (m_hManhack == NULL || m_iManhackHealth <= 0 || m_iManhackDistance >= 100)
	{
		if (m_iManhackDistance >= 100)
		{
			CNPC_Manhack *pManhack = m_hManhack;

			if (pManhack)
			{
				pManhack->ComeBackToPlayer(m_hPlayer, 8.0f);
			}
		}

		if (FindNextManhack())
		{
			return true;
		}

		return false;
	}

	return true;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPropVehicleManhack::Think(void)
{
	if ( GetDriver() )
	{
		if (GlobalEntity_GetState("manhacks_not_controllable") == GLOBAL_ON)
			ForcePlayerOut();

		BaseClass::Think();

		SetNextThink( gpGlobals->curtime );

		StudioFrameAdvance();

		if (m_hManhack == NULL || m_iManhackHealth <= 0 || m_iManhackDistance >= 100)
		{
			if (m_iManhackDistance >= 100)
			{
				CNPC_Manhack *pManhack = m_hManhack;

				if (pManhack)
				{
					pManhack->ComeBackToPlayer(m_hPlayer, 8.0f);
				}
			}

			/*if (!FindNextManhack())
			{
				SetLocator( NULL );
				ForcePlayerOut();
				DestroyAllManhacks();
				UTIL_Remove(this);
			}*/

			if (!FindNextManhack(true))
			{
				ForcePlayerOut();
			}
		} 
		else if ( m_hPlayer  )
		{
			UpdateManhackView();	
			UpdateManhackDistance(this);
		}

		if (!GetDriver() && m_bHadDriver)
		{
			DevMsg("We have no driver, lets disappear");
			m_bHadDriver = false;
			//SetRenderMode(kRenderTransColor );
			//SetRenderColorA(0);
			AddEffects( EF_NODRAW );
			SetSolid(SOLID_NONE);
			AddSolidFlags( FSOLID_NOT_SOLID );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Vehicles are permanently oriented off angle for vphysics.
//-----------------------------------------------------------------------------
void CPropVehicleManhack::GetVectors(Vector* pForward, Vector* pRight, Vector* pUp) const
{
	// This call is necessary to cause m_rgflCoordinateFrame to be recomputed
	const matrix3x4_t &entityToWorld = EntityToWorldTransform();

	if (pForward != NULL)
	{
		MatrixGetColumn( entityToWorld, 1, *pForward ); 
	}

	if (pRight != NULL)
	{
		MatrixGetColumn( entityToWorld, 0, *pRight ); 
	}

	if (pUp != NULL)
	{
		MatrixGetColumn( entityToWorld, 2, *pUp ); 
	}
}

//-----------------------------------------------------------------------------
// Purpose: Override base class to add display 
//-----------------------------------------------------------------------------
void CPropVehicleManhack::DrawDebugGeometryOverlays(void) 
{

	BaseClass::DrawDebugGeometryOverlays();
}

int CPropVehicleManhack::UpdateManhackDistance(CBaseEntity *pOrigin) 
{
	CNPC_Manhack *pManhack=GetManhack();
	
	if (pManhack != NULL && pOrigin) //!GetDriver()) 
	{
		if (GlobalEntity_GetState("manhacks_use_short_distances") == GLOBAL_ON)
		{
			Vector vecDistance = (pManhack->GetAbsOrigin() - pOrigin->GetAbsOrigin());
			float flHeight	= vecDistance.z / manhack_small_distances_max_height.GetFloat();
			vecDistance.z	= 0.0f;
			vecDistance		= vecDistance / manhack_small_distances_max_distance.GetFloat();		
			vecDistance.z	= flHeight;
			m_iManhackDistance	= (int)( vecDistance.Length() * 100.0f );
		}
		else
		{
			m_iManhackDistance = (int)( (float) (pManhack->GetAbsOrigin() - pOrigin->GetAbsOrigin()).Length() / manhack_max_distance.GetFloat() * 100.0f);
		}

		return m_iManhackDistance;
	}
	return -1;
}

void CPropVehicleManhack::UpdateManhackView()
{
	CNPC_Manhack *pManhack=GetManhack();
	
	if (pManhack != NULL) 
	{


		m_angManhackEye = pManhack->GetAbsAngles(); //vehicleEyeAngles;
		
		m_vecManhackEye = pManhack->GetManhackView(); //vehicleEyeOrigin;

		m_iManhackHealth	= pManhack->m_iHealth;

		Vector vecDir = pManhack->GetAbsVelocity();
		VectorNormalize(vecDir);
		m_vecFlyingDirection = vecDir;

		UpdateHead();
		
		if (GlobalEntity_GetState("manhacks_use_short_distances") == GLOBAL_ON)
		{
			Vector vecDistance = (pManhack->GetAbsOrigin() - GetAbsOrigin());
			float flHeight	= vecDistance.z / manhack_small_distances_max_height.GetFloat();
			vecDistance.z	= 0.0f;
			vecDistance		= vecDistance / manhack_small_distances_max_distance.GetFloat();		
			vecDistance.z	= flHeight;
			m_iManhackDistance	= (int)( vecDistance.Length() * 100.0f );
		}
		else
		{
			m_iManhackDistance = (int)( (float) (pManhack->GetAbsOrigin() - GetAbsOrigin()).Length() / manhack_max_distance.GetFloat() * 100.0f);
		}

		if (m_iManhackDistance >= 90 && m_flLastWarningSound < gpGlobals->curtime )
		{
			pManhack->EmitSound("NPC_Turret.Retire");
			m_flLastWarningSound = gpGlobals->curtime + 1.00f;
			
		}

		//

		Vector vecForward;
		m_hPlayer->GetVectors(&vecForward, NULL, NULL);

		trace_t	tr;
		UTIL_TraceLine( m_vecManhackEye, m_vecManhackEye + (vecForward * 1024), MASK_SHOT, pManhack, COLLISION_GROUP_NONE, &tr );

		//NDebugOverlay::Line( m_vecManhackEye, tr.endpos, 255, 255, 0, true, 1.0f );

		if (tr.m_pEnt && tr.m_pEnt->MyNPCPointer() &&
			 CBaseCombatCharacter::GetDefaultRelationshipDispositionBetweenClasses( CLASS_PLAYER, tr.m_pEnt->Classify() ) == D_HT )
		{
			m_hTarget = tr.m_pEnt;
		}
		else 
		{
			m_hTarget = pManhack->GetEnemy();
		}

		m_iTargetType = HLSS_SelectTargetType(m_hTarget);

	} else 
	{	
		DevMsg("NULL Manhack\n");
		m_hManhack = NULL;
		SetLocator( NULL );
	}
}

void CPropVehicleManhack::FixManhackView()
{
		Vector eyeDir;
		AngleVectors(m_angManhackEye, &eyeDir);

		m_vecManhackEye = m_vecManhackEye - (eyeDir * manhack_blend_view.GetFloat());

		eyeDir = manhack_blend_view.GetFloat() * eyeDir;

		/*trace_t tr;
		TIL_TraceLine( m_vecManhackEye, m_vecManhackEye + eyeDir, MASK_ALL, GetManhack(), COLLISION_GROUP_NPC, &tr );

		if (tr.fraction!=1)
		{
			m_vecManhackEye = (m_vecManhackEye  
		}*/
}

void CPropVehicleManhack::UpdateHead( void )
{
	float yaw = GetPoseParameter( "head_yaw" );
	float pitch = GetPoseParameter( "head_pitch" );

	// If we should be watching our enemy, turn our head
	CNPC_Manhack *pManhack=GetManhack();
	
	if (pManhack != NULL) 
	{
		Vector vehicleEyeOrigin;
		QAngle vehicleEyeAngles;
		GetAttachment( "vehicle_driver_eyes", vehicleEyeOrigin, vehicleEyeAngles );

		// FIXME: cache this
		Vector vBodyDir;
		AngleVectors( vehicleEyeAngles, &vBodyDir );

		Vector	manhackDir = pManhack->GetAbsOrigin() - vehicleEyeOrigin;
		VectorNormalize( manhackDir );
		
		float angle = UTIL_VecToYaw( vBodyDir );
		float angleDiff = UTIL_VecToYaw( manhackDir );
		angleDiff = UTIL_AngleDiff( angleDiff, angle + yaw );

		SetPoseParameter( "head_yaw", UTIL_Approach( yaw + angleDiff, yaw, 1 ) );

		angle = UTIL_VecToPitch( vBodyDir );
		angleDiff = UTIL_VecToPitch( manhackDir );
		angleDiff = UTIL_AngleDiff( angleDiff, angle + pitch );

		SetPoseParameter( "head_pitch", UTIL_Approach( pitch + angleDiff, pitch, 1 ) );
	}
	else
	{
		// Otherwise turn the head back to its normal position
		SetPoseParameter( "head_yaw",	UTIL_Approach( 0, yaw, 10 ) );
		SetPoseParameter( "head_pitch", UTIL_Approach( 0, pitch, 10 ) );
	}
}

void CPropVehicleManhack::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	CBasePlayer *pPlayer = ToBasePlayer( pActivator );
	if ( !pPlayer )
		return;

	ResetUseKey( pPlayer );

	GetServerVehicle()->HandlePassengerEntry( pPlayer, 1 );
}

Vector CPropVehicleManhack::BodyTarget( const Vector &posSrc, bool bNoisy )
{
	Vector	shotPos;
	//matrix3x4_t	matrix;

	int eyeAttachmentIndex = LookupAttachment("vehicle_driver_eyes");
	GetAttachment( eyeAttachmentIndex, shotPos );
	//shotPos = WorldSpaceCenter();

	return shotPos;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPropVehicleManhack::SetupMove( CBasePlayer *player, CUserCmd *ucmd, IMoveHelper *pHelper, CMoveData *move )
{
	int buttons = ucmd->buttons;
	if ( !(buttons & (IN_MOVELEFT|IN_MOVERIGHT)) )
	{
		if ( ucmd->sidemove < 0 )
		{
			buttons |= IN_MOVELEFT;
		}
		else if ( ucmd->sidemove > 0 )
		{
			buttons |= IN_MOVERIGHT;
		}
	}

	DriveManhack(player, buttons);
}

void CPropVehicleManhack::DriveManhack(CBasePlayer *player,  int nButtons ) //float flFrameTime, CUserCmd *ucmd, int iButtonsDown, int iButtonsReleased )
{
	if (m_hManhack == NULL) 
		return;

	CNPC_Manhack *pManhack = dynamic_cast<CNPC_Manhack*>((CBaseEntity*)m_hManhack);

	if (pManhack == NULL)
		return;

	if ( nButtons & IN_ATTACK2 )
	{
		if (gpGlobals->curtime > m_fTimeToChangeManhack )
		{
			if (FindNextManhack())
			{
				m_fTimeToChangeManhack = gpGlobals->curtime + 2.0f;

				pManhack->SetControllable(false);
				
				pManhack->RemoveEffects( EF_NODRAW );
				pManhack->ShowRedGlow(true);
			}
		}
	}

	if ( (nButtons & IN_FORWARD) && !(nButtons & IN_BACK) )
	{

		if ( (nButtons & IN_MOVELEFT) && !(nButtons & IN_MOVERIGHT) )
			pManhack->MoveForwardBack(1, player->EyeAngles()+QAngle(0,45,0));
		else if ( (nButtons & IN_MOVERIGHT) && !(nButtons & IN_MOVELEFT))
			pManhack->MoveForwardBack(1, player->EyeAngles()+QAngle(0,-45,0));
		else
			pManhack->MoveForwardBack(1, player->EyeAngles());

	}
	else if ( (nButtons & IN_BACK) && !(nButtons & IN_FORWARD) )
	{
		if ( (nButtons & IN_MOVELEFT) && !(nButtons & IN_MOVERIGHT) )
			pManhack->MoveForwardBack(-1, player->EyeAngles()+QAngle(0,-45,0));
		else if ( (nButtons & IN_MOVERIGHT) && !(nButtons & IN_MOVELEFT))
			pManhack->MoveForwardBack(-1, player->EyeAngles()+QAngle(0,45,0));
		else
			pManhack->MoveForwardBack(-1, player->EyeAngles());
		

		//pManhack->MoveForwardBack(-1, player->EyeAngles());
	} 
	else if ( !(nButtons & IN_BACK) && !(nButtons & IN_FORWARD) )
	{
		QAngle FixedAngles = player->EyeAngles();

		if ( (nButtons & IN_MOVELEFT) && !(nButtons & IN_MOVERIGHT) )
		{
			FixedAngles.x = 0;
			pManhack->MoveForwardBack(1, FixedAngles+QAngle(0,90,0));
			//pManhack->TurnLeft();
		} 
		else if ( (nButtons & IN_MOVERIGHT) && !(nButtons & IN_MOVELEFT) ) 
		{
			FixedAngles.x=0;
			pManhack->MoveForwardBack(1, FixedAngles+QAngle(0,-90,0));
			//pManhack->TurnRight();
		}
		else
			pManhack->MoveForwardBack(0, player->EyeAngles());
	}
	else //if nothing's pressed or simultaneous to differen directions we do this
	{
		//pManhack->MoveForwardBack(0, m_angManhackEye + GetAbsAngles() );
		pManhack->MoveForwardBack(0, player->EyeAngles());
	} 

	//NOTE: up down should be last because they only affect z
	if ( (nButtons & IN_JUMP) && !(nButtons & IN_DUCK))
	{
		pManhack->MoveUpDown(1);
	}
	else if ( (nButtons & IN_DUCK) && !(nButtons & IN_JUMP) )
	{
		pManhack->MoveUpDown(-1);
	}
	else if ( (nButtons & IN_DUCK) && (nButtons & IN_JUMP) )
	{
		pManhack->MoveUpDown(0);
	}
}

CBaseEntity *CPropVehicleManhack::GetDriver( void ) 
{ 
	return m_hPlayer; 
}

bool CPropVehicleManhack::CanEnterVehicle( CBaseEntity *pEntity )
{
	// Prevent entering if the vehicle's being driven by an NPC
	if ( GetDriver() && GetDriver() != pEntity )
		return false;

	return true; 
}

//-----------------------------------------------------------------------------
// Purpose: Return true of the player's allowed to enter / exit the vehicle
//-----------------------------------------------------------------------------
bool CPropVehicleManhack::CanExitVehicle( CBaseEntity *pEntity )
{
	// Prevent exiting if the vehicle's locked, or rotating
	// Adrian: Check also if I'm currently jumping in or out.
	//return ( !m_bLocked && (GetLocalAngularVelocity() == vec3_angle) && m_bExitAnimOn == false && m_bEnterAnimOn == false );
	return true;
}


void CPropVehicleManhack::ItemPostFrame( CBasePlayer *player )
{
}

void CPropVehicleManhack::PreExitVehicle( CBaseCombatCharacter *pPlayer, int nRole )
{
	//if the passanger is exiting the vehicle then we want to make the "vehicle" invisible and not solid
	//SetRenderMode(kRenderTransColor );
	//SetRenderColorA(0);
	AddEffects( EF_NODRAW );
	SetSolid(SOLID_NONE);
	AddSolidFlags( FSOLID_NOT_SOLID );
}

//-----------------------------------------------------------------------------
// Purpose: 
//    NOTE: Doesn't call the base call enter vehicle on purpose!
//-----------------------------------------------------------------------------
void CPropVehicleManhack::EnterVehicle( CBaseCombatCharacter *pPassenger )
{
	if ( pPassenger == NULL )
		return;

	DevMsg("CPropVehicleManhack: EnterVehicl(...)\n");

	CBasePlayer *pPlayer = ToBasePlayer( pPassenger );
	if ( pPlayer != NULL )
	{
		// Remove any player who may be in the vehicle at the moment
		if ( m_hPlayer )
		{
			ExitVehicle( VEHICLE_ROLE_DRIVER );
		}

		m_hPlayer = pPlayer;
		m_bHadDriver = true;

		if (GetNumberOfHacks(false)>1 && m_iHintTimesShown < 2)
		{
			m_iHintTimesShown++;
			m_iHintNoSwapTimesShown++;
			UTIL_HudHintText( pPlayer, "#HLSS_Hint_ManhackSwap" );
		} else if (m_iHintNoSwapTimesShown < 2)
		{
			m_iHintNoSwapTimesShown++;
			UTIL_HudHintText( pPlayer, "#HLSS_Hint_ManhackExit" );
		}
		else UTIL_HudHintText( pPlayer, "" );
	

		pPlayer->SetViewOffset( vec3_origin );
		pPlayer->ShowCrosshair( false );


		CHL2_Player *pHL2Player = dynamic_cast<CHL2_Player*>( pPlayer );
		if ( pHL2Player )
		{
			if ( pHL2Player->IsSprinting() )
			{
				pHL2Player->StopSprinting();
			}

			if ( pHL2Player->FlashlightIsOn() )
			{
				pHL2Player->FlashlightTurnOff();
			}
		}

		CNPC_Manhack *pManhack = dynamic_cast<CNPC_Manhack*>((CBaseEntity*)m_hManhack);
		if (pManhack!=NULL)
		{
			pManhack->SetControllable(true);

			if (manhack_dont_draw.GetBool())
			{
				pManhack->AddEffects( EF_NODRAW );
				pManhack->ShowRedGlow(false);
			}

			m_vecLastEyeTarget = pManhack->GetManhackView();
			m_vecLastEyePos    = pManhack->GetManhackView();
			m_vecTargetSpeed   = pManhack->GetAbsVelocity();

			m_vecFlyingDirection = pManhack->GetAbsVelocity();
		}

		//We want to see and feel the "vehicle" in manhack mode
		//SetRenderMode(kRenderNormal);
		RemoveEffects( EF_NODRAW );
		SetSolid(SOLID_BBOX);
		RemoveSolidFlags( FSOLID_NOT_SOLID );

		if (m_bDriverDucked) 
		{
			SetCollisionBounds( Vector(-16,-16,0), Vector(16,16,45) );

			int nSequence = LookupSequence( "crouch" );

			DevMsg("Player is ducking\n");

			// Set to the desired anim, or default anim if the desired is not present
			if ( nSequence > ACTIVITY_NOT_AVAILABLE )
			{
				SetCycle( 0 );
				m_flAnimTime = gpGlobals->curtime;
				ResetSequence( nSequence );
				ResetClientsideFrame();
			}
			else 
			{
				// Not available try to get default anim
				Msg( "Manhack Controller %s: missing crouch sequence\n", GetDebugName() );
				SetSequence( 0 );
			}
		}
		else
		{
			SetCollisionBounds( Vector(-16,-16,0), Vector(16,16,72) );

			int nSequence = LookupSequence( "idle" );

			// Set to the desired anim, or default anim if the desired is not present
			if ( nSequence > ACTIVITY_NOT_AVAILABLE )
			{
				SetCycle( 0 );
				m_flAnimTime = gpGlobals->curtime;
				ResetSequence( nSequence );
				ResetClientsideFrame();
			}
		}

		// Start Thinking
		SetNextThink( gpGlobals->curtime );

	}
	else
	{
		// NPCs not yet supported - jdw
		Assert( 0 );
	}


}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPropVehicleManhack::ExitVehicle( int nRole )
{
	CBasePlayer *pPlayer = m_hPlayer;
	if ( !pPlayer )
		return;

	m_hPlayer = NULL;
	ResetUseKey( pPlayer );


	//We have to snap the eye angles because otherwise the player will look into the same direction that the manhack just did
	Vector vehicleEyeOrigin;
	QAngle vehicleEyeAngles;
	GetAttachment( "vehicle_driver_eyes", vehicleEyeOrigin, vehicleEyeAngles );
	vehicleEyeAngles.x = 0;
	vehicleEyeAngles.y = 0;
	//pPlayer->SetAbsAngles( vehicleEyeAngles );
	//pPlayer->SnapEyeAngles( vehicleEyeAngles );
	pPlayer->SetAbsAngles(GetAbsAngles());
	pPlayer->SnapEyeAngles(GetAbsAngles());
	pPlayer->SetAbsOrigin(GetAbsOrigin());

	if (m_bDriverDucked)
	{
		pPlayer->m_nButtons |= IN_DUCK;
		pPlayer->AddFlag( FL_DUCKING );
		pPlayer->m_Local.m_bDucked = true;
		pPlayer->m_Local.m_bDucking = true;
		pPlayer->m_Local.m_flDucktime = 0.0f;
		pPlayer->SetViewOffset( VEC_DUCK_VIEW );
		pPlayer->SetCollisionBounds( VEC_DUCK_HULL_MIN, VEC_DUCK_HULL_MAX );
	}

	DevMsg("Yes, this did get updated\n");

	CNPC_Manhack *pManhack = dynamic_cast<CNPC_Manhack*>((CBaseEntity*)m_hManhack);
	if (pManhack!=NULL)
	{
		pManhack->SetControllable(false);

		//pManhack->SetRenderMode(kRenderNormal);
		pManhack->RemoveEffects( EF_NODRAW );

		pManhack->ShowRedGlow(true);
	}
}

void CPropVehicleManhack::ResetUseKey( CBasePlayer *pPlayer )
{
	pPlayer->m_afButtonPressed &= ~IN_USE;

	pPlayer->SetRenderMode( kRenderTransColor );
}


//-----------------------------------------------------------------------------
// Purpose: These will be useful for weapon_manhack
//-----------------------------------------------------------------------------
void CPropVehicleManhack::ForcePlayerIn(CBaseEntity *pOwner)
{
	CBasePlayer *pPlayer = ToBasePlayer( pOwner );
	if ( !pPlayer || m_hPlayer )
		return;

	if (pPlayer->GetFlags() & FL_DUCKING)
	{
		m_bDriverDucked=true;
		DevMsg("Player ducking\n");
	}
	else m_bDriverDucked=false;

	//TERO: Lets move the vehicle where we are standing
	QAngle vehicleStartAngle = QAngle(0, pPlayer->GetAbsAngles().y, 0);
	SetAbsAngles(vehicleStartAngle);
	SetAbsOrigin(pPlayer->GetAbsOrigin());

	// Make sure we successfully got in the vehicle
	if ( pPlayer->GetInVehicle( GetServerVehicle(), VEHICLE_ROLE_DRIVER ) == false )
	{
		// The player was unable to enter the vehicle and the output has failed
//		int falseGetInVehicle=0;
		Assert( 0 ); //muista vaihtaa tuohon nolla sitten kun poistat intin;
		return;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPropVehicleManhack::ForcePlayerOut()
{
	if ( !GetDriver() )
		return;

	CBasePlayer *pPlayer = m_hPlayer;
	if ( !pPlayer )
		return;

	pPlayer->LeaveVehicle(GetAbsOrigin(), GetAbsAngles());

	//Lets make the "vehicle" invisible and not solid
	//SetRenderMode(kRenderTransColor );
	//SetRenderColorA(0);
	AddEffects( EF_NODRAW );
	SetSolid(SOLID_NONE);
	AddSolidFlags( FSOLID_NOT_SOLID );
}

//TERO: We want to take damage normally since this is not really a vehicle but a fake body
//		We exit the vehicle so that it wouldn't look silly stiff man from the manhack point of view
int CPropVehicleManhack::OnTakeDamage( const CTakeDamageInfo &inputInfo )
{
	//Check to do damage to driver
	if ( GetDriver() )
	{
		GetDriver()->TakeDamage( inputInfo );
		ForcePlayerOut();
	}

	return 0;
}

void CPropVehicleManhack::TraceAttack( const CTakeDamageInfo &inputInfo, const Vector &vecDir, trace_t *ptr )
{
	if ( m_hPlayer != NULL )
	{
		m_hPlayer->TakeDamage( inputInfo );
		ForcePlayerOut();
	}

}

CPropVehicleManhack *VehicleManhack_Create( const Vector &position, const QAngle &angles, CBaseEntity *pOwner  )
{
	QAngle vehicleStartAngle = QAngle(0, pOwner->GetAbsAngles().y, 0);

	CPropVehicleManhack *pManhackVehicle = (CPropVehicleManhack *)CBaseEntity::Create( "vehicle_manhack", pOwner->GetAbsOrigin(), vehicleStartAngle, pOwner );
	
	pManhackVehicle->KeyValue( "targetname", "manhack_controller" );
	pManhackVehicle->KeyValue( "model", "models/vehicles/vehicle_manhackcontroller.mdl" );
	pManhackVehicle->KeyValue( "vehiclescript", "scripts/vehicles/manhack.txt" );

	pManhackVehicle->Spawn();

	pManhackVehicle->Activate();

	if (!pManhackVehicle->CreateControllableManhack(position, angles, pOwner)) 
	{
		pManhackVehicle->Remove();
		return NULL;
	}

	DevMsg("Spawning manhack vehicle\n");

	return pManhackVehicle;
}

CNPC_Manhack *CPropVehicleManhack::GetManhack( void )
{
	if (!m_hManhack)
		return NULL;

	CNPC_Manhack *pManhack = dynamic_cast<CNPC_Manhack*>((CBaseEntity*)m_hManhack);

	return pManhack;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPropVehicleManhack::DampenEyePosition( Vector &vecVehicleEyePos, QAngle &vecVehicleEyeAngles )
{
	// Get the frametime. (Check to see if enough time has passed to warrent dampening).
	float flFrameTime = gpGlobals->frametime;
	if ( flFrameTime < MANHACK_FRAMETIME_MIN )
	{
		vecVehicleEyePos = m_vecLastEyePos;
		return;
	}

	// Keep static the sideways motion.

	// Dampen forward/backward motion.
	DampenForwardMotion( vecVehicleEyePos, vecVehicleEyeAngles, flFrameTime );
}

//-----------------------------------------------------------------------------
// Use the controller as follows:
// speed += ( pCoefficientsOut[0] * ( targetPos - currentPos ) + pCoefficientsOut[1] * ( targetSpeed - currentSpeed ) ) * flDeltaTime;
//-----------------------------------------------------------------------------
void CPropVehicleManhack::ComputePDControllerCoefficients( float *pCoefficientsOut,
												  float flFrequency, float flDampening,
												  float flDeltaTime )
{
	float flKs = 9.0f * flFrequency * flFrequency;
	float flKd = 4.5f * flFrequency * flDampening;

	float flScale = 1.0f / ( 1.0f + flKd * flDeltaTime + flKs * flDeltaTime * flDeltaTime );

	pCoefficientsOut[0] = flKs * flScale;
	pCoefficientsOut[1] = ( flKd + flKs * flDeltaTime ) * flScale;
}
 
//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CPropVehicleManhack::DampenForwardMotion( Vector &vecVehicleEyePos, QAngle &vecVehicleEyeAngles, float flFrameTime )
{
	// Simulate the eye position forward based on the data from last frame
	// (assumes no acceleration - it will get that from the "spring").
	Vector vecCurrentEyePos = m_vecLastEyePos + m_vecEyeSpeed * flFrameTime;

	Vector vecForward = m_vecFlyingDirection;
	VectorNormalize(vecForward);

	// Calculate target speed based on the current vehicle eye position and the last vehicle eye position and frametime.
	Vector vecVehicleEyeSpeed = ( vecVehicleEyePos - m_vecLastEyeTarget ) / flFrameTime;
	m_vecLastEyeTarget = vecVehicleEyePos;	

	// Calculate the speed and position deltas.
	Vector vecDeltaSpeed = vecVehicleEyeSpeed - m_vecEyeSpeed;
	Vector vecDeltaPos = vecVehicleEyePos - vecCurrentEyePos;

	// Clamp.
	if ( vecDeltaPos.Length() > MANHACK_DELTA_LENGTH_MAX )
	{
		float flSign = vecForward.Dot( vecVehicleEyeSpeed ) >= 0.0f ? -1.0f : 1.0f;
		vecVehicleEyePos += flSign * ( vecForward * MANHACK_DELTA_LENGTH_MAX );
		m_vecLastEyePos = vecVehicleEyePos;
		m_vecEyeSpeed = vecVehicleEyeSpeed;
		return;
	}

	// Generate an updated (dampening) speed for use in next frames position extrapolation.
	float flCoefficients[2];
	ComputePDControllerCoefficients( flCoefficients, r_ManhackViewDampenFreq.GetFloat(), r_ManhackViewDampenDamp.GetFloat(), flFrameTime );
	m_vecEyeSpeed += ( ( flCoefficients[0] * vecDeltaPos + flCoefficients[1] * vecDeltaSpeed ) * flFrameTime );

	// Save off data for next frame.
	m_vecLastEyePos = vecCurrentEyePos;

	// Move eye forward/backward.
	Vector vecForwardOffset = vecForward * ( vecForward.Dot( vecDeltaPos ) );
	vecVehicleEyePos -= vecForwardOffset;
}


//========================================================================================================================================
// CRANE VEHICLE SERVER VEHICLE
//========================================================================================================================================
CPropVehicleManhack *CPropServerVehicleManhack::GetManhack( void )
{
	return (CPropVehicleManhack *)GetDrivableVehicle();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPropServerVehicleManhack::GetVehicleViewPosition( int nRole, Vector *pAbsOrigin, QAngle *pAbsAngles, float *pFOV )
{
	Assert( nRole == VEHICLE_ROLE_DRIVER );
	CBasePlayer *pPlayer = ToBasePlayer( GetDrivableVehicle()->GetDriver() );
	Assert( pPlayer );//*/

	//commented out because this really should be setting the manhack angle, not the vehicle angle
	*pAbsAngles = pPlayer->EyeAngles(); // yuck. this is an in/out parameter.
	//*pAbsOrigin = pPlayer->EyePosition();

	CNPC_Manhack *pManhack=NULL;

	if (GetManhack())
		pManhack=GetManhack()->GetManhack();
	
	if (pManhack != NULL) 
	{
		Vector  m_vecManhackEye = GetManhack()->GetManhackEyePosition(); //pManhack->GetManhackView();
		QAngle m_angManhackEye = pManhack->GetAbsAngles();

		matrix3x4_t vehicleEyePosToWorld;

		AngleMatrix( m_angManhackEye, vehicleEyePosToWorld );

		// Dampen the eye positional change as we drive around.
		//*pAbsAngles = pPlayer->EyeAngles();

		CPropVehicleManhack *pDriveable = assert_cast<CPropVehicleManhack*>(GetDrivableVehicle());
		if (pDriveable) pDriveable->DampenEyePosition( m_vecManhackEye, m_angManhackEye );
	
		// Compute the relative rotation between the unperturbed eye attachment + the eye angles
		matrix3x4_t cameraToWorld;
		AngleMatrix( *pAbsAngles, cameraToWorld );

		matrix3x4_t worldToEyePos;
		MatrixInvert( vehicleEyePosToWorld, worldToEyePos );

		matrix3x4_t vehicleCameraToEyePos;
		ConcatTransforms( worldToEyePos, cameraToWorld, vehicleCameraToEyePos );

		AngleMatrix( m_angManhackEye, m_vecManhackEye, vehicleEyePosToWorld );

		// Now treat the relative eye angles as being relative to this new, perturbed view position...
		matrix3x4_t newCameraToWorld;
		ConcatTransforms( vehicleEyePosToWorld, vehicleCameraToEyePos, newCameraToWorld );

		// output new view abs angles
		MatrixAngles( newCameraToWorld, *pAbsAngles );

		// UNDONE: *pOrigin would already be correct in single player if the HandleView() on the server ran after vphysics
		MatrixGetColumn( newCameraToWorld, 3, *pAbsOrigin );

	} else DevMsg("fail\n");
}

int HLSS_SelectTargetType(CBaseEntity *pEntity)
{
	if (pEntity)
	{
		switch ( pEntity->Classify() )
		{
		case CLASS_CITIZEN_REBEL:
			return 1;
			break;
		case CLASS_ALIENCONTROLLER:
		case CLASS_ALIENGRUNT:
		case CLASS_VORTIGAUNT:
		case CLASS_MANTARAY_TELEPORTER:
			return 2;
			break;
		case CLASS_HEADCRAB:
		case CLASS_ZOMBIE:
		case CLASS_ANTLION:
				return 3;
				break;	
		case CLASS_MILITARY_HACKED:
		case CLASS_COMBINE_HACKED:
		case CLASS_AIR_DEFENSE_HACKED:
		case CLASS_HACKED_ROLLERMINE:
				return 4;
				break;	
		default:
				return 0;
				break;	
		}
	}

	return 0;
}
