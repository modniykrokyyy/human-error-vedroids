//=================== Half-Life 2: Short Stories Mod 2007 =====================//
//
// Purpose:	Unique Metrocops Noah, Eloise, Larson
//
//=============================================================================//
#ifndef HLSS_NPC_UNIQUE_METROPOLICE_H
#define HLSS_NPC_UNIQUE_METROPOLICE_H
#include "cbase.h"
#include "npc_metropolice.h"
#include "npc_unique_metropolice.h"


// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

class CNPC_UniqueMetrocop : public CNPC_MetroPolice 
{
public:
	CNPC_UniqueMetrocop *m_pNext;

	CNPC_UniqueMetrocop();
	~CNPC_UniqueMetrocop();

	static CNPC_UniqueMetrocop *GetUniqueMetrocopList();
};

#endif