//=================== Half-Life 2: Short Stories Mod 2007 =====================//
//
// Purpose:	Unique Metrocops Noah, Eloise, Larson
//
//=============================================================================//

#include "cbase.h"
#include "npc_unique_metropolice.h"


// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

class CNPC_Larson : public CNPC_UniqueMetrocop 
{
public:
	DECLARE_CLASS( CNPC_Larson, CNPC_MetroPolice ); 

	void Precache()
	{
		m_iUniqueMetropolice = METROPOLICE_LARSON;
		SetModelName( AllocPooledString("models/HLSS_Characters/police_larson.mdl") );
		BaseClass::Precache();
	}

	virtual const char		*GetDeathMessageText( void ) { return "LARSON_DEAD"; }
};

class CNPC_Noah : public CNPC_UniqueMetrocop 
{
public:
	DECLARE_CLASS( CNPC_Noah, CNPC_MetroPolice ); 

	void Precache()
	{
		m_iUniqueMetropolice = METROPOLICE_NOAH;
		SetModelName( AllocPooledString("models/HLSS_Characters/police_noah.mdl") );
		BaseClass::Precache();
	}

	virtual const char		*GetDeathMessageText( void ) { return "NOAH_DEAD"; }
};

class CNPC_Eloise : public CNPC_UniqueMetrocop 
{
public:
	DECLARE_CLASS( CNPC_Eloise, CNPC_MetroPolice ); 

	void Precache()
	{
		m_iUniqueMetropolice = METROPOLICE_ELOISE;
		SetModelName( AllocPooledString("models/HLSS_Characters/police_eloise.mdl") );
		BaseClass::Precache();
	}

	virtual const char		*GetDeathMessageText( void ) { return "ELOISE_DEAD"; }
};

// global pointer to Larson for fast lookups
CEntityClassList<CNPC_UniqueMetrocop> g_UniqueMetrocopList;
template <> CNPC_UniqueMetrocop *CEntityClassList<CNPC_UniqueMetrocop>::m_pClassList = NULL;

LINK_ENTITY_TO_CLASS( npc_noah,		CNPC_Noah );
LINK_ENTITY_TO_CLASS( npc_eloise,	CNPC_Eloise );
LINK_ENTITY_TO_CLASS( npc_larson,	CNPC_Larson );

//=========================================================
// Returns a pointer to Eloise's entity
//=========================================================
CNPC_UniqueMetrocop *CNPC_UniqueMetrocop::GetUniqueMetrocopList( void )
{
	return g_UniqueMetrocopList.m_pClassList;
}

CNPC_UniqueMetrocop::CNPC_UniqueMetrocop( void ) 
{
	g_UniqueMetrocopList.Insert(this);
}

CNPC_UniqueMetrocop::~CNPC_UniqueMetrocop( )
{
	g_UniqueMetrocopList.Remove(this);
}


