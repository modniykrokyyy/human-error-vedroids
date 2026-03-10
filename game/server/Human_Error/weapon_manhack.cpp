//=================== Half-Life 2: Short Stories Mod 2007 =====================//
//
// Purpose:	weapon_manhack, create a manhack (vehicle_manhack) and control it
//			CONTROL. WE HAVE IT.
//
//=============================================================================//



#include "cbase.h"
#include "basehlcombatweapon.h"
#include "player.h"
#include "gamerules.h"
#include "ammodef.h"
#include "in_buttons.h"
#include "globalstate.h"

#include "engine/IEngineSound.h"
#include "items.h"
#include "soundent.h"

#include "vguiscreen.h"

#include "npcevent.h"

#include "hl2_player_shared.h" //For Bob

#include "npc_manhack.h"
#include "vehicle_manhack.h"
#include "weapon_manhack.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


#define SCREEN_OVERLAY_MATERIAL "engine/writez" 

//---------------------------------------------------------
// Save/Restore
//---------------------------------------------------------
BEGIN_DATADESC( CWeapon_Manhack )
//	DEFINE_FIELD( m_hManhack,				FIELD_EHANDLE),
	DEFINE_FIELD( m_bIsDrawing,				FIELD_BOOLEAN),
	DEFINE_FIELD( m_bIsDoingShit,			FIELD_BOOLEAN),
	DEFINE_FIELD( m_bIsDoingShitToo,		FIELD_BOOLEAN),
	DEFINE_FIELD( m_bRedraw,				FIELD_BOOLEAN),
	DEFINE_FIELD( m_bHasAmmo,				FIELD_BOOLEAN),
	DEFINE_FIELD( m_bIsDoingController,		FIELD_BOOLEAN),
	DEFINE_FIELD( m_bSpawnSomeMore,			FIELD_BOOLEAN),
	DEFINE_FIELD( m_bSkip,					FIELD_BOOLEAN),
	DEFINE_FIELD( m_bHoldingSpawn,			FIELD_BOOLEAN),
	DEFINE_FIELD( m_iManhackHintTimeShown,  FIELD_INTEGER),
	DEFINE_FIELD( m_iLastNumberOfManhacks,	FIELD_INTEGER),
	DEFINE_FIELD( m_bShouldShowPanel,		FIELD_BOOLEAN),
	DEFINE_FIELD( m_bToggleCallback,		FIELD_BOOLEAN),
	DEFINE_FIELD( m_bHadControllable,		FIELD_BOOLEAN),
	DEFINE_FIELD( m_iManhackDistance,		FIELD_INTEGER),

	DEFINE_FIELD( m_hScreen,				FIELD_EHANDLE),
END_DATADESC()

acttable_t	CWeapon_Manhack::m_acttable[] = 
{
	{ ACT_RANGE_ATTACK1, ACT_RANGE_ATTACK_SLAM, true },
};

IMPLEMENT_ACTTABLE(CWeapon_Manhack);


IMPLEMENT_SERVERCLASS_ST( CWeapon_Manhack, DT_Weapon_Manhack)
	SendPropInt(SENDINFO(m_iManhackDistance), SPROP_CHANGES_OFTEN),
END_SEND_TABLE()

LINK_ENTITY_TO_CLASS( weapon_manhack, CWeapon_Manhack );
PRECACHE_WEAPON_REGISTER(weapon_manhack);



void CWeapon_Manhack::Operator_HandleAnimEvent( animevent_t *pEvent, CBaseCombatCharacter *pOperator )
{
	CBasePlayer *pOwner = ToBasePlayer( GetOwner() );

	switch( pEvent->event )
	{
		case EVENT_WEAPON_SEQUENCE_FINISHED:
			m_bIsDrawing = false;
			m_bIsDoingShit = false;
			m_bIsDoingShitToo = false;

			if (m_bRedraw)
			{
				m_bRedraw=false;
				Deploy();
			}
			break;

		case EVENT_WEAPON_THROW:
			if (CPropVehicleManhack::GetManhackVehicle()!=NULL)
			{
				if (m_bHasFreeSlot)	
				{
					if (CreateControllableNPCManhack( pOwner))
					{
						DecrementAmmo( pOwner );
						EnableManhackSubModel(false);
						m_bRedraw=true;

						CBasePlayer *pPlayer = ToBasePlayer( pOwner );
						if ( pPlayer != NULL && m_iManhackHintTimeShown < 2)
						{
							if (GlobalEntity_GetState("manhacks_not_controllable") == GLOBAL_ON )
								UTIL_HudHintText( pPlayer, "#HLSS_Hint_ManhackSend" );
							else
								UTIL_HudHintText( pPlayer, "#HLSS_Hint_ManhackControl" );

							m_iManhackHintTimeShown++;
						}
//						fSpawnedManhack = true;
					}
				}
			}
			else if (CreateControllableVehicleManhack( pOwner))
			{
				DecrementAmmo( pOwner );	
				EnableManhackSubModel(false);	
				m_bRedraw=true;
//				fSpawnedManhack = true;

				//CBasePlayer *pPlayer = ToBasePlayer( pOwner );
				if ( pOwner != NULL && m_iManhackHintTimeShown <2)
				{
					if (GlobalEntity_GetState("manhacks_not_controllable") == GLOBAL_ON )
						UTIL_HudHintText( pOwner, "#HLSS_Hint_ManhackSend" );
					else
						UTIL_HudHintText( pOwner, "#HLSS_Hint_ManhackControl" );

					m_iManhackHintTimeShown++;
				}
			}

			m_bToggleCallback = true;

			if (!m_bHoldingSpawn)
			{
				m_bSpawnSomeMore = false;
				m_bIsDoingController = true;
			}
			else
			{
				m_bSpawnSomeMore = true;
				m_bIsDoingController = false;
				m_bRedraw = true;
			}

			break;
		case EVENT_WEAPON_THROW2:
			if ( pOwner != NULL && pOwner->GetFlags() & FL_ONGROUND )
			{
				if (CPropVehicleManhack::GetManhackVehicle() != NULL) 
				{
					if (GlobalEntity_GetState("manhacks_not_controllable") == GLOBAL_OFF)
					{
						m_bShouldShowPanel = false;
						DriveControllableManhack();
					}
					else
					{
						if (m_bToggleCallback)
						{
							TellManhacksToGoThere();
							m_bToggleCallback = false;
						}
						else
						{
							CallManhacksBack();
							m_bToggleCallback = true;
						}

					}
				}
				else
				{
					//TERO: Lets do an error sound
				}
			}
			break;
		case EVENT_WEAPON_THROW3:
			if (m_bToggleCallback)
			{
				TellManhacksToGoThere();
				m_bToggleCallback = false;
			}
			else
			{
				CallManhacksBack();
				m_bToggleCallback = true;
			}
			break;
		default:
			BaseClass::Operator_HandleAnimEvent( pEvent, pOperator );
			break;
	}

#define RETHROW_DELAY	0.5
/*	if( fSpawnedManhack )
	{
		m_flNextPrimaryAttack	= gpGlobals->curtime + RETHROW_DELAY;
		m_flNextSecondaryAttack	= gpGlobals->curtime + RETHROW_DELAY;
		m_flTimeWeaponIdle = FLT_MAX; //NOTE: This is set once the animation has finished up!

	}
	*/
}

void CWeapon_Manhack::GetControlPanelInfo( int PanelIndex, const char *&pPanelName)
{
	pPanelName = NULL; //"manhack_screen";
}

CWeapon_Manhack::CWeapon_Manhack(void)
{
//	m_hManhack = NULL;
	m_bIsDrawing=false;
	m_bIsDoingShit=false;
	m_bIsDoingShitToo=false;
	m_bRedraw=false;
	m_bHasAmmo=true;
	m_bIsDoingController=false;
	m_bSpawnSomeMore=false;
	m_bSkip=true;
	m_iManhackHintTimeShown=0;
	m_bShouldShowPanel=false;
	m_bHadControllable = false;

	m_iLastNumberOfManhacks=-1;
}

//------------------------------------------------------------------------------
// Purpose :
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CWeapon_Manhack::ItemBusyFrame( void )
{
	CBasePlayer *pOwner = ToBasePlayer( GetOwner() );

	if (pOwner)
	{
		if ( m_bHoldingSpawn && pOwner->m_nButtons & IN_ATTACK )
		{
			m_bHoldingSpawn = true;
		}
		else
		{
			m_bHoldingSpawn = false;
		}
	}

	BaseClass::ItemBusyFrame();
}

//------------------------------------------------------------------------------
// Purpose :
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CWeapon_Manhack::ItemPostFrame( void )
{
	m_bHasAmmo = false;
	m_bHasFreeSlot = HasFreeSlot();

	CBasePlayer *pOwner = ToBasePlayer( GetOwner() );

	if (pOwner)
	{
		if (pOwner->GetAmmoCount(m_iPrimaryAmmoType) > 0)
		{
			m_bHasAmmo=true;
		}

		if ( pOwner->m_nButtons & IN_ATTACK )
		{
			m_bHoldingSpawn = true;
		}
		else
		{
			m_bHoldingSpawn = false;
		}
	}

	if (pOwner && !m_bIsDoingShit && !m_bIsDrawing && !m_bIsDoingShitToo && HasAnyAmmo() )
	{
			
		if( pOwner->m_nButtons & IN_ATTACK )
		{
			if ( m_bHasAmmo && (CPropVehicleManhack::GetManhackVehicle() == NULL || m_bHasFreeSlot ) && !m_bIsDoingController)
			{
				PrimaryAttack();
			}
			else 
			{
				SecondaryAttack();
			}

			return;
		} 
		else
		{
			if ( pOwner->m_nButtons & IN_ATTACK2 && m_bHasFreeSlot )
			{
				//SendWeaponAnim(ACT_SLAM_DETONATOR_THROW_DRAW);
				//m_bIsDoingShitToo=true;

				CBaseCombatCharacter *pOwner  = GetOwner();
				if (pOwner && pOwner->GetAmmoCount(m_iPrimaryAmmoType) > 0 && !m_bSpawnSomeMore )
				{
					m_bSpawnSomeMore=true;
					m_bIsDoingShitToo=true;
				} 
				else if ( HasNPCManhack() && m_bSpawnSomeMore)
				{
					m_bSpawnSomeMore=false;
					//m_bIsDoingShitToo=true;
					//m_bIsDoingController=true;
					m_bIsDoingShitToo=true;
					m_bRedraw=true;
					m_bSkip = true;
				}
			}
		} 
	}

	WeaponIdle( );
}

bool CWeapon_Manhack::FindVehicleManhack()
{
	/*if (m_hManhack==NULL)
	{
		CBaseEntity *pVehicleManhack = gEntList.FindEntityByName( NULL, "manhack_controller" );
		if (pVehicleManhack!=NULL)
		{
			m_hManhack=pVehicleManhack;
			return true;
		}
	}*/
	if (CPropVehicleManhack::GetManhackVehicle() != NULL)
		return true;

	return false;
}

void CWeapon_Manhack::TellManhacksToGoThere() 
{
	CBasePlayer *pPlayer = ToBasePlayer( GetOwner() );
	CPropVehicleManhack *pVehicleManhack = CPropVehicleManhack::GetManhackVehicle(); //dynamic_cast<CPropVehicleManhack*>((CBaseEntity*)m_hManhack);
	if (pVehicleManhack!=NULL && pPlayer != NULL) 
	{
		pVehicleManhack->TellManhacksToGoThere(pPlayer, 4.0f);
	}
}

void CWeapon_Manhack::CallManhacksBack()
{
	CBasePlayer *pPlayer = ToBasePlayer( GetOwner() );
	CPropVehicleManhack *pVehicleManhack = CPropVehicleManhack::GetManhackVehicle(); //dynamic_cast<CPropVehicleManhack*>((CBaseEntity*)m_hManhack);
	if (pVehicleManhack!=NULL && pPlayer != NULL) 
	{
		pVehicleManhack->CallManhacksBack(pPlayer, 4.0f);
	}
}

void CWeapon_Manhack::Precache( void )
{
	BaseClass::Precache();

	UTIL_PrecacheOther( "npc_manhack" );
	UTIL_PrecacheOther( "vehicle_manhack" );

	//if panel:
	PrecacheMaterial( SCREEN_OVERLAY_MATERIAL );
	PrecacheMaterial( "vgui/screens/manhack_back" );
}


//------------------------------------------------------------------------------
// Purpose :
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CWeapon_Manhack::PrimaryAttack()
{
	if ( !m_bHasFreeSlot )
	{
		return;
	}

	CBaseCombatCharacter *pOwner  = GetOwner();
	
	if ( pOwner == NULL )
	{ 
		return;
	}

	CBasePlayer *pPlayer = ToBasePlayer( GetOwner() );;

	if ( !pPlayer )
		return;

	m_bIsDoingShit = true;

	// Note that this is a primary attack and prepare the grenade attack to pause.
	SendWeaponAnim( ACT_VM_THROW );

	DevMsg("                return true\n");

	// If I'm now out of ammo, switch away
	/*if ( !HasPrimaryAmmo() )
	{
		pPlayer->SwitchToNextBestWeapon( this );
	}*/
}

//------------------------------------------------------------------------------
// Purpose :
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CWeapon_Manhack::SecondaryAttack()
{
	if ( CPropVehicleManhack::GetManhackVehicle() == NULL )
	{
		return;
	}

	CBaseCombatCharacter *pOwner  = GetOwner();
	
	if ( pOwner == NULL )
	{ 
		return;
	}

	CBasePlayer *pPlayer = ToBasePlayer( GetOwner() );;

	if ( !pPlayer )
		return;


	m_bIsDoingShit = true;

	SendWeaponAnim( ACT_SLAM_DETONATOR_DETONATE );
}

void CWeapon_Manhack::UpdateControllerPanel()
{
	if (GlobalEntity_GetState("manhacks_not_controllable") == GLOBAL_ON )
	{
		m_iManhackDistance = 100;
		return;
	}

	CPropVehicleManhack *pManhack = CPropVehicleManhack::GetManhackVehicle(); //dynamic_cast<CPropVehicleManhack*>((CBaseEntity*)m_hManhack);

	if (pManhack)
	{
		m_iManhackDistance = pManhack->UpdateManhackDistance(pManhack);
	}
}

void CWeapon_Manhack::WeaponIdle( void )
{
	/*bool hasAmmo = false;
	CBaseCombatCharacter *pOwner  = GetOwner();
	if (pOwner==NULL) return false;
	if (pOwner->GetAmmoCount(m_iSecondaryAmmoType) > 0)
		hasAmmo=true;*/

	// Ready to switch animations?
 
	int	 iAnim;

	//TERO: lets set how many Manhacks we have online
	CBaseCombatCharacter *pOwner  = GetOwner();
	if (pOwner)
	{
		pOwner->SetAmmoCount( NumberOfManhacks(), m_iSecondaryAmmoType );
	}

	if ( (HasNPCManhack() || !m_bHasAmmo) && !m_bSpawnSomeMore ) 
	{
		iAnim=ACT_SLAM_DETONATOR_IDLE;
		m_bIsDoingController=true;

		UpdateControllerPanel();

		//TERO:

		if (m_bIsDrawing)
			iAnim=ACT_SLAM_DETONATOR_DRAW;
		else if (m_bIsDoingShit)
			iAnim=ACT_SLAM_DETONATOR_DETONATE;
		else if (m_bIsDoingShitToo)
			iAnim=ACT_VM_HOLSTER; //ACT_SLAM_DETONATOR_THROW_DRAW
			
		
		if (HasWeaponIdleTimeElapsed() || m_bSkip)
		{
			SendWeaponAnim( iAnim );
			m_bSkip=false;
		}
	}
	else
	{
		UpdateControllerPanel();

		iAnim=ACT_VM_IDLE;

		m_bSkip =false;

		if (m_bIsDoingController)
		{
			m_bIsDoingShitToo=true;
			m_bRedraw=true;
			m_bSkip=true;

			m_bIsDoingController=false;
		}
	
		if ( HasWeaponIdleTimeElapsed() || m_bSkip )
		{
			if (m_bIsDrawing)
				iAnim=ACT_VM_DRAW;
			else if (m_bIsDoingShit)
				iAnim=ACT_VM_THROW;
			else if (m_bIsDoingShitToo)
				iAnim=ACT_SLAM_DETONATOR_HOLSTER;

			SendWeaponAnim( iAnim );

		}
	}
		
}

void CWeapon_Manhack::EnableManhackSubModel(bool bEnable)
{
	//SetBodygroup( 0, bEnable );
	if (bEnable)
		DevMsg("weapon_manhack: enabling manhack submodel\n");
	else
		DevMsg("weapon_manhack: disabling manhack: submodel\n");
}

bool CWeapon_Manhack::Deploy( void )
{
	CBasePlayer *pOwner = ToBasePlayer( GetOwner() );
	if ( !pOwner )
	{
		return false;
	}

	// ------------------------------
	// Pick the right draw animation
	// ------------------------------
	int iActivity;

	m_bHasFreeSlot = HasFreeSlot();

	bool bNoDrivable = true;

	CPropVehicleManhack *pManhack = CPropVehicleManhack::GetManhackVehicle();

	if (pManhack) //m_hManhack)
	{
		// = dynamic_cast<CPropVehicleManhack*>((CBaseEntity*)m_hManhack);

		if (pManhack->HasDrivableManhack(pOwner))
		{
			bNoDrivable = false;
		}
	}

	if ( m_bHasAmmo && m_bHasFreeSlot && (!m_bIsDoingController || bNoDrivable))
	{
		iActivity = ACT_VM_DRAW;
		//EnableManhackSubModel(true);

		CBasePlayer *pPlayer = ToBasePlayer( pOwner );
			if ( pPlayer != NULL)

		m_bIsDoingController = false;
		m_bSpawnSomeMore = true; //TERO: not sure about this
	} 
	else
	{
		m_bIsDoingController = true;
		m_bSpawnSomeMore = false;
		iActivity = ACT_SLAM_DETONATOR_DRAW;
	}

	m_bIsDrawing=true;

	m_bShouldShowPanel = true;


	//TERO: this next bit is to make sure the player gets the right hint message if the controllable state has changed
	if (GlobalEntity_GetState("manhacks_not_controllable") == GLOBAL_ON )
	{
		if (m_bHadControllable)
		{
			m_iManhackHintTimeShown=0;
			CBasePlayer *pPlayer = ToBasePlayer( pOwner );
			if ( pPlayer != NULL && (CPropVehicleManhack::GetManhackVehicle() != NULL) )
			{
				UTIL_HudHintText( pPlayer, "#HLSS_Hint_ManhackSend" );
				m_iManhackHintTimeShown++;
			}
		}

		m_bHadControllable = false;
	} 
	else
	{
		if (!m_bHadControllable)
		{
			m_iManhackHintTimeShown=0;
			CBasePlayer *pPlayer = ToBasePlayer( pOwner );
			if ( pPlayer != NULL && (CPropVehicleManhack::GetManhackVehicle() != NULL) )
			{
				UTIL_HudHintText( pPlayer, "#HLSS_Hint_ManhackControl" );
				m_iManhackHintTimeShown++;
			}
		}
		m_bHadControllable = true;
	}

	if (!m_hScreen)
	{
		/*CBaseViewModel *vm = pOwner->GetViewModel( m_nViewModelIndex );
		if (vm)
		{

			//vm->SpawnControlPanels();
			int nLLAttachmentIndex = vm->LookupAttachment("controlpanel0_ll");
			int nURAttachmentIndex = vm->LookupAttachment("controlpanel0_ur");

			matrix3x4_t	panelToWorld;
			vm->GetAttachment( nLLAttachmentIndex, panelToWorld );

			matrix3x4_t	worldToPanel;
			MatrixInvert( panelToWorld, worldToPanel );

			// Now get the lower right position + transform into panel space
			Vector lr, lrlocal;
			vm->GetAttachment( nURAttachmentIndex, panelToWorld );
			MatrixGetColumn( panelToWorld, 3, lr );
			VectorTransform( lr, worldToPanel, lrlocal );

			float flWidth = lrlocal.x;
			float flHeight = lrlocal.y;

			CVGuiScreen *pScreen = CreateVGuiScreen( "vgui_screen", "manhack_screen", this, this, 0 );
			pScreen->ChangeTeam( 69 );
			pScreen->SetActualSize( 128, 64 );
			pScreen->SetActive( true );
			pScreen->MakeVisibleOnlyToTeammates( true );
			pScreen->SetAttachedToViewModel(true);
			m_hScreen.Set( pScreen );
		}*/

		CVGuiScreen *pScreen = CreateVGuiScreen( "vgui_screen", "manhack_screen", this, this, 0 );
		//pScreen->SetActualSize( 128, 64 );
		pScreen->SetActive( true );
		pScreen->MakeVisibleOnlyToTeammates( true );
		pScreen->SetAttachedToViewModel(true);
		m_hScreen.Set( pScreen );

	}

	return DefaultDeploy( (char*)GetViewModel(), (char*)GetWorldModel(), iActivity, (char*)GetAnimPrefix() );
}

bool CWeapon_Manhack::HasAnyAmmo( void )
{
	FindVehicleManhack();

	if ( HasNPCManhack() )
	{
		return true;
	}

	CBaseCombatCharacter *pOwner  = GetOwner();
	if (pOwner==NULL) return false;
	if (pOwner->GetAmmoCount(m_iPrimaryAmmoType) > 0)
		return true;

	return false;
}

void CWeapon_Manhack::WeaponSwitch( void )
{
	CBaseCombatCharacter *pOwner  = GetOwner();
	if (pOwner==NULL) return;
	pOwner->SwitchToNextBestWeapon( pOwner->GetActiveWeapon() );

	if (CPropVehicleManhack::GetManhackVehicle() == NULL && pOwner->GetAmmoCount(m_iPrimaryAmmoType) <= 0)
	{
		pOwner->ClearActiveWeapon();
	}
}

void CWeapon_Manhack::DecrementAmmo( CBaseCombatCharacter *pOwner )
{
	pOwner->RemoveAmmo( 1, m_iPrimaryAmmoType ); //Secondary

}

bool CWeapon_Manhack::Reload( void )
{
	//Deploy();
	WeaponIdle();
	return true;
}



//TERO: This is called when we don't have either vehicle or NPC manhack
bool CWeapon_Manhack::CreateControllableVehicleManhack( CBasePlayer *pPlayer )
{
	/*Vector vecOrigin;
	QAngle vecAngles;

	int turretSpawnAttachment = LookupAttachment( "manhackSpawn" );
	GetAttachment( turretSpawnAttachment, vecOrigin, vecAngles );*/

	Vector	vForward, vRight, vUp, vThrowPos;
	
	pPlayer->EyeVectors( &vForward, &vRight, &vUp );

	vThrowPos = pPlayer->EyePosition();

	vThrowPos += vForward * 20.0f;

	/*m_hManhack=VehicleManhack_Create( vThrowPos, pPlayer->EyeAngles(), pPlayer );

	if (m_hManhack == NULL) 
		return false;*/

	VehicleManhack_Create( vThrowPos, pPlayer->EyeAngles(), pPlayer );

	if (CPropVehicleManhack::GetManhackVehicle() == NULL)
		return false;

	//WeaponSound( SINGLE );
	return true;
}

//TERO: This is called when we already have the vehicle manhack (the CP model), it creates the actual NPC manhack
bool CWeapon_Manhack::CreateControllableNPCManhack( CBasePlayer *pPlayer )
{
	/*Vector vecOrigin;
	QAngle vecAngles;

	int turretSpawnAttachment = LookupAttachment( "manhackSpawn" );
	GetAttachment( turretSpawnAttachment, vecOrigin, vecAngles );*/

	Vector	vForward, vRight, vUp, vThrowPos;
	
	pPlayer->EyeVectors( &vForward, &vRight, &vUp );

	vThrowPos = pPlayer->EyePosition();

	vThrowPos += vForward * 20.0f;

	CPropVehicleManhack *pManhack = CPropVehicleManhack::GetManhackVehicle(); //dynamic_cast<CPropVehicleManhack*>((CBaseEntity*)m_hManhack);
	if (pManhack!=NULL) 
	{	
		return pManhack->CreateControllableManhack(vThrowPos, pPlayer->EyeAngles(), pPlayer);
	}	

	//WeaponSound( SINGLE );
	return false;
}

bool CWeapon_Manhack::HasNPCManhack()
{
	CPropVehicleManhack *pManhack = CPropVehicleManhack::GetManhackVehicle(); //dynamic_cast<CPropVehicleManhack*>((CBaseEntity*)m_hManhack);
	if (pManhack!=NULL) 
	{	
		return (pManhack->HasNPCManhack());
	}	
	return false;
}

bool CWeapon_Manhack::HasFreeSlot()
{
	CPropVehicleManhack *pManhack = CPropVehicleManhack::GetManhackVehicle(); //dynamic_cast<CPropVehicleManhack*>((CBaseEntity*)m_hManhack);
	if (pManhack!=NULL)
	{
		if (pManhack->GetNumberOfHacks(true)>=NUMBER_OF_MAX_CONTROLLABLE_MANHACKS)
		{
			return false;
		}
	}
	return true;
}


int CWeapon_Manhack::NumberOfManhacks()
{
	CPropVehicleManhack *pManhack = CPropVehicleManhack::GetManhackVehicle(); //dynamic_cast<CPropVehicleManhack*>((CBaseEntity*)m_hManhack);
	if (pManhack!=NULL)
	{
		return pManhack->GetNumberOfHacks(false);
	}	
	return 0;
}

void CWeapon_Manhack::DriveControllableManhack()
{
	CPropVehicleManhack *pManhack = CPropVehicleManhack::GetManhackVehicle(); //dynamic_cast<CPropVehicleManhack*>((CBaseEntity*)m_hManhack);
	if (pManhack!=NULL) 
	{
		CBaseCombatCharacter *pOwner  = GetOwner();
		if (pOwner!=NULL)
		{
			pManhack->ForcePlayerIn(pOwner);
		}

	}
}

float CWeapon_Manhack::CalcViewmodelBob( void )
{


	return 0.0f;
}

