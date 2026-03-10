
#include "cbase.h"
#include "c_weapon__stubs.h"
#include "c_basehlcombatweapon.h"
#include "basehlcombatweapon_shared.h"
/*#include "model_types.h"
#include "ClientEffectPrecacheSystem.h"
#include "fx.h"
#include "c_te_effect_dispatch.h"
#include "beamdraw.h"*/
#include "c_manhack_screen.h"

class C_Weapon_Manhack : public C_BaseHLCombatWeapon
{
	DECLARE_CLASS( C_Weapon_Manhack, C_BaseHLCombatWeapon );
public:
	DECLARE_CLIENTCLASS();
	DECLARE_PREDICTABLE();

	//virtual void ClientThink( void );

	virtual bool IsWeaponManhack() { return true; }

	void OnDataChanged( DataUpdateType_t updateType );
	
	C_Weapon_Manhack( void );
	~C_Weapon_Manhack( void );
private:
	int	m_iManhackDistance;
};

STUB_WEAPON_CLASS_IMPLEMENT( weapon_manhack, C_Weapon_Manhack );

IMPLEMENT_CLIENTCLASS_DT(C_Weapon_Manhack, DT_Weapon_Manhack, CWeapon_Manhack)
	RecvPropInt( RECVINFO( m_iManhackDistance ) ),
END_RECV_TABLE()

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
C_Weapon_Manhack::C_Weapon_Manhack( void )
{
}

C_Weapon_Manhack::~C_Weapon_Manhack( void )
{
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void C_Weapon_Manhack::OnDataChanged( DataUpdateType_t updateType )
{
	BaseClass::OnDataChanged( updateType );
	
	if (GetManhackScreen())
		GetManhackScreen()->SetManhackDistance( m_iManhackDistance );
}


/*void C_Weapon_Manhack::ClientThink( void )
{
	BaseClass::ClientThink();

	if (GetManhackScreen())
		GetManhackScreen()->SetManhackDistance( m_iManhackDistance );
}*/