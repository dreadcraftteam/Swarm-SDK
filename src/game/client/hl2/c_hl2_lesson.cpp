//=============== Copyright DREADCRAFT, All rights reserved. ==================//
//
// Purpose:	HL2 client handler implementations for instruction players how to play
//
//=============================================================================//

#include "cbase.h"

#include "c_gameinstructor.h"
#include "c_baselesson.h"

#include "hl2_gamerules.h"
#include "c_basehlplayer.h"
#include "c_basehlcombatweapon.h"
#include "weapon_parse.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


extern CUtlDict< int, int > g_LessonActionMap;

extern ConVar gameinstructor_verbose;


enum Mod_LessonAction
{
	// Enum starts from end of LessonAction
	// here your actions

	LESSON_ACTION_TOTAL
};

void CScriptedIconLesson::Mod_PreReadLessonsFromFile(void)
{
	// Add custom actions to the map
	// example CScriptedIconLesson::LessonActionMap.Insert("is allowed item", LESSON_ACTION_IS_ALLOWED_ITEM);
}


bool CScriptedIconLesson::Mod_ProcessElementAction(int iAction, bool bNot, const char* pchVarName, EHANDLE& hVar, const CGameInstructorSymbol* pchParamName, float fParam, C_BaseEntity* pParam, const char* pchParam, bool& bModHandled)
{
	// Assume we're going to handle the action

	return false;
}

bool C_GameInstructor::Mod_HiddenByOtherElements(void)
{
	return false;
}
