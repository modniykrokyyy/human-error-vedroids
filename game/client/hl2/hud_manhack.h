//=================== Half-Life 2: Short Stories Mod 2007 =====================//
//
// Purpose:	Alien Controllers from HL1 now in updated form
//
//=============================================================================//


#ifndef HUD_MANHACK
#define HUD_MANHACK

#include "cbase.h"
#include "hud.h"
#include "hudelement.h"
#include "hud_macros.h"
#include "hud_numericdisplay.h"
#include "Human_Error/hud_icondisplay.h"

//-----------------------------------------------------------------------------
// Purpose: Displays current Manhack Health
//-----------------------------------------------------------------------------
class CHudManhackHealth : public CHudNumericDisplay, public CHudElement
{
	DECLARE_CLASS_SIMPLE( CHudManhackHealth, CHudNumericDisplay );

public:
	CHudManhackHealth( const char *pElementName );
	void Init( void );
	void VidInit( void );
	void Reset();
//	void Paint();

	void SetHealthAndDistance(int health, int distance, bool playAnimation);
		
protected:
	virtual void OnThink();

	void UpdateManhackHealth( C_BasePlayer *player, IClientVehicle *pVehicle );

private:

	int		m_iHealth;
	int		m_iDistance;
//	int		m_iImageID;
};

//-----------------------------------------------------------------------------
// Purpose: Displays current ammunition level
//-----------------------------------------------------------------------------
class CHudAmmo : public CHudIconDisplay, public CHudElement
{
	DECLARE_CLASS_SIMPLE( CHudAmmo, CHudIconDisplay );

	//~CHudAmmo();
public:
	CHudAmmo( const char *pElementName );
	void Init( void );
	void VidInit( void );
	void Reset();

	void SetAmmo(int ammo, bool playAnimation);
	void SetAmmo2(int ammo2);

	//void CreateImages();
		
protected:
	virtual void OnThink();

	void UpdateAmmoDisplays();
	void UpdatePlayerAmmo( C_BasePlayer *player );
	void UpdateVehicleAmmo( C_BasePlayer *player, IClientVehicle *pVehicle );
	
private:
	CHandle< C_BaseCombatWeapon > m_hCurrentActiveWeapon;
	CHandle< C_BaseEntity > m_hCurrentVehicle;
	int		m_iAmmo;
	int		m_iAmmo2;

	int		m_iStunstickType;
	int		m_iAltFireType;
	int		m_iAlyxGunType;
	int		m_iAlyxGunBurst;
};

//-----------------------------------------------------------------------------
// Purpose: Displays current ammunition level
//-----------------------------------------------------------------------------
class CHudAR2AltFire : public CHudIconDisplay, public CHudElement
{
	DECLARE_CLASS_SIMPLE( CHudAR2AltFire, CHudIconDisplay );

	//~CHudAmmo();
public:
	CHudAR2AltFire( const char *pElementName );
	void Init( void );
	void VidInit( void );
	void Reset();

	void OnThink( void );
		
	static int m_iAmmo;
	static bool m_bHide;

private:
	int m_iOldAmmo;
	bool m_bHidden;
};

#endif
