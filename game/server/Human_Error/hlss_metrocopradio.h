//=================== Half-Life 2: Short Stories Mod 2007 =====================//
//
// Purpose:	Unique Metrocops Noah, Eloise, Larson
//
//=============================================================================//
#ifndef HLSS_NPC_METROCOPRADIO_H
#define HLSS_NPC_METROCOPRADIO_H
#include "cbase.h"


class CHLSS_MetrocopRadio : public CPointEntity
{
public: 
	DECLARE_CLASS( CHLSS_MetrocopRadio, CLogicalEntity );
	DECLARE_DATADESC();

	CHLSS_MetrocopRadio();
	~CHLSS_MetrocopRadio();

	CHLSS_MetrocopRadio *m_pNext;

	static CHLSS_MetrocopRadio *GetMetrocopRadio();

	void Spawn( void );
	void Precache( void );
	void OnRestore( void );

	void PlayRadio(string_t sample, bool playedByNPC);
	void FakeRadio(float flTime);
	//void StopRadio(string_t sample);

	//virtual void	UpdateOnRemove( void );

private:
	string_t		m_iszSound;				// Path/filename of WAV file to play.
	static int		m_iNumberOfRadios;
	static float	m_flLastSoundDuration;
	static string_t m_iszLastSound;

	void InputPlaySound(inputdata_t &inputdata );
	void InputStopSound(inputdata_t &inputdata );

	void UpdateRadioHud();

	COutputEvent		m_OnMessageEnd;
};

#endif