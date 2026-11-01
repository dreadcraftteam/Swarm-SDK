//============ Copyright Valve Corporation, All rights reserved. ==============//
//
// Purpose: Client handler implementations for instruction players how to play
//
//=============================================================================//

#include "cbase.h"

#include "c_gameinstructor.h"
#include "c_baselesson.h"
#include "c_sdk_player.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


void CScriptedIconLesson::Mod_PreReadLessonsFromFile( void )
{
	// Write implementation here.
}


bool CScriptedIconLesson::Mod_ProcessElementAction( int iAction, bool bNot, const char *pchVarName, EHANDLE &hVar, const CGameInstructorSymbol *pchParamName, float fParam, C_BaseEntity *pParam, const char *pchParam, bool &bModHandled )
{
	// Write implementation here.
	return false;
}

bool C_GameInstructor::Mod_HiddenByOtherElements( void )
{
	C_SDKPlayer* pPlayer;

	// for example, hiding our lessons when player in vehicle
	if (pPlayer->IsInAVehicle())
	{
		return true;
	}

	return false;
}
