//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef NPC_ASSASSIN_H
#define NPC_ASSASSIN_H
#ifdef _WIN32
#pragma once
#endif

#include "ai_basenpc.h"
#include "Sprite.h"
#include "SpriteTrail.h"
#include "soundent.h"

//Eye states
enum eyeState_t
{
	ASSASSIN_EYE_SEE_TARGET = 0,		//Sees the target, bright and big
	ASSASSIN_EYE_SEEKING_TARGET,	//Looking for a target, blinking (bright)
	ASSASSIN_EYE_ACTIVE,			//Actively looking
	ASSASSIN_EYE_DORMANT,			//Not active
	ASSASSIN_EYE_DEAD,				//Completely invisible
};

//=========================================================
//=========================================================
class CNPC_Assassin : public CAI_BaseNPC
{
public:
	DECLARE_CLASS( CNPC_Assassin, CAI_BaseNPC );
	// DECLARE_SERVERCLASS();
	DECLARE_DATADESC();

	CNPC_Assassin( void );
	
	Class_T		Classify( void )			{ return CLASS_CITIZEN_REBEL;	}
	int			GetSoundInterests ( void )	{ return (SOUND_WORLD|SOUND_COMBAT|SOUND_PLAYER);	}

	int			SelectSchedule ( void );
	int			MeleeAttack1Conditions ( float flDot, float flDist );
	int			RangeAttack1Conditions ( float flDot, float flDist );
	int			RangeAttack2Conditions ( float flDot, float flDist );

//	void PainSound( const CTakeDamageInfo &info );
//	void AlertSound( void );
//	void IdleSound( void );

	bool		OverrideMoveFacing( const AILocalMoveGoal_t &move, float flInterval );

	void		Precache( void );
	void		Spawn( void );
	void		PrescheduleThink( void );
	void		HandleAnimEvent( animevent_t *pEvent );
	void		StartTask( const Task_t *pTask );
	void		RunTask( const Task_t *pTask );
	void		OnScheduleChange( void );
	void		GatherEnemyConditions( CBaseEntity *pEnemy );
	void		BuildScheduleTestBits( void );
	void		Event_Killed( const CTakeDamageInfo &info );

	bool		FValidateHintType ( CAI_Hint *pHint );
	bool		IsJumpLegal(const Vector &startPos, const Vector &apex, const Vector &endPos) const;
	bool		MovementCost( int moveType, const Vector &vecStart, const Vector &vecEnd, float *pCost );

	float		MaxYawSpeed( void );

	const Vector &GetViewOffset( void );

	Activity	NPC_TranslateActivity( Activity eNewActivity );

private:

#ifdef ASSASSIN_PINKYEYE
	void		SetEyeState( eyeState_t state );
#endif
	void		FirePistol( int hand );
	bool		CanFlip( int flipType, Activity &activity, const Vector *avoidPosition );

	bool		FlipAndRunPath();

	int			m_nNumFlips;
	int			m_nNumLunge;
	int			m_nLastFlipType;
	float		m_flNextFlipTime;	//Next earliest time the assassin can flip again
	float		m_flNextLungeTime;
	float		m_flNextShotTime;
	int			m_nNumShots;

	bool		m_bEvade;
	bool		m_bAggressive;		// Sets certain state, including whether or not her eye is visible

#ifdef ASSASSIN_PINKYEYE
	bool		m_bBlinkState;

	CSprite				*m_pEyeSprite;
	CSpriteTrail		*m_pEyeTrail;
#endif

	float		m_flGaveEnemyDamageTime;

	bool		m_bIsFlipping;
	float		m_flNextMeleeTime;

	//float		m_flTimeToEvade;
	float		m_flTimeToBackstab;
	float		m_flTimeToPressAttack;
	int			m_iBackstabAttempts;

	DEFINE_CUSTOM_AI;

public:

	//=========================================================
	// Private conditions
	//=========================================================
	enum 
	{
		COND_ASSASSIN_ENEMY_TARGETTING_ME  = BaseClass::NEXT_CONDITION,
		COND_ASSASSIN_LUNGE,
		COND_ASSASSIN_SHOOT,
	};

	//=========================================================
	// Assassin schedules
	//=========================================================
	enum
	{
		SCHED_ASSASSIN_FIND_VANTAGE_POINT= BaseClass::NEXT_SCHEDULE,
		SCHED_ASSASSIN_EVADE,
		SCHED_ASSASSIN_STALK_ENEMY,
		SCHED_ASSASSIN_LUNGE,
		SCHED_ASSASSIN_TRY_TO_BACKSTAB,
		SCHED_ASSASSIN_PRESS_ATTACK,
		SCHED_ASSASSIN_MELEE,
		SCHED_ASSASSIN_SHOOT,
	};

	//=========================================================
	// Assassin tasks
	//=========================================================
	enum 
	{
		TASK_ASSASSIN_GET_PATH_TO_VANTAGE_POINT = BaseClass::NEXT_TASK,
		TASK_ASSASSIN_EVADE,
#ifdef ASSASSIN_PINKYEYE
		TASK_ASSASSIN_SET_EYE_STATE,
#endif
		TASK_ASSASSIN_LUNGE,
		TASK_ASSASSIN_SHOOT,
		TASK_ASSASSIN_FLIP_PATH,
		TASK_ASSASSIN_GET_BEHIND_ENEMY,
	};
};


#endif // NPC_ASSASSIN_H
