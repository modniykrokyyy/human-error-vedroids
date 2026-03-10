//=================== Half-Life 2: Short Stories Mod 2007 =====================//
//
// Purpose:	Alien Controllers from HL1 now in updated form
//
//=============================================================================//


#ifndef WEAPON_MANHACK
#define WEAPON_MANHACK


#include "cbase.h"
#include "basehlcombatweapon.h"

class CWeapon_Manhack : public CBaseHLCombatWeapon
{
public:
	DECLARE_CLASS( CWeapon_Manhack, CBaseHLCombatWeapon );

	DECLARE_SERVERCLASS();

	CWeapon_Manhack();

	//void			Spawn( void );
	void			Precache( void );

	void			ItemBusyFrame( void );
	void			ItemPostFrame( void );
	void			PrimaryAttack( void );
	void			SecondaryAttack( void );

	void			EnableManhackSubModel(bool bEnable);

	bool			Deploy(void);
	bool			Reload(void);

	void			DecrementAmmo( CBaseCombatCharacter *pOwner );

	void			WeaponIdle(void);
	bool			HasAnyAmmo(void);

	void			WeaponSwitch( void );

	bool			FindVehicleManhack();

	void			Operator_HandleAnimEvent( animevent_t *pEvent, CBaseCombatCharacter *pOperator );
	bool			CreateControllableVehicleManhack( CBasePlayer *pPlayer );
	bool			CreateControllableNPCManhack( CBasePlayer *pPlayer );
	void			DriveControllableManhack();

	bool			HasNPCManhack();
	bool			HasFreeSlot();
	int				NumberOfManhacks();

	void			CallManhacksBack();
	void			TellManhacksToGoThere();

	//screen stuff

	void			GetControlPanelInfo( int nPanelIndex, const char *&pPanelName );
	bool			ShouldShowControlPanels() { return m_bShouldShowPanel; }

	void			UpdateControllerPanel();

	virtual void			AddViewmodelBob( CBaseViewModel *viewmodel, Vector &origin, QAngle &angles ) { return; }
	virtual float			CalcViewmodelBob( void );

private:

	//EHANDLE			m_hManhack;

	//Animation booleans
	bool			m_bIsDrawing;
	bool			m_bIsDoingShit;
	bool			m_bIsDoingShitToo;
	bool			m_bIsDoingController;
	bool			m_bSpawnSomeMore;
	bool			m_bSkip;
	bool			m_bRedraw;
	bool			m_bHoldingSpawn;

	bool			m_bToggleCallback;

	bool			m_bShouldShowPanel;

	bool			m_bHasAmmo;
	bool			m_bHasFreeSlot;

	int				m_iManhackHintTimeShown;
	bool			m_bHadControllable;		//If Manhacks used to be controllable, but now they are not, other the other way around

	int				m_iLastNumberOfManhacks;

	EHANDLE			m_hScreen;

	CNetworkVar( int, m_iManhackDistance );

	DECLARE_ACTTABLE();
	DECLARE_DATADESC();
};

#endif //WEAPON_MANHACK
