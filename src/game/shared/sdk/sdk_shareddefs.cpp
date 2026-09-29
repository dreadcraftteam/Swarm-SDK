//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Shared defines
//
//========================================================================//

#include "cbase.h"
#include "weapon_sdkbase.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

const char *pszTeamNames[] =
{
	"#SDK_Team_Unassigned",
	"#SDK_Team_Spectator",
};

// Tony; We need to precache all possible player models that we're going to use
const char *pszPossiblePlayerModels[] =
{
	SDK_PLAYER_MODEL,
	"models/player/blue_player.mdl",
	"models/player/red_player.mdl",
	NULL
};

//-----------------------------------------------------------------------------
// Global Weapon Definitions
//-----------------------------------------------------------------------------
static const char * s_WeaponAliasInfo[] = 
{
	"none",		// WEAPON_NONE

	"mp5",		// SDK_WEAPON_MP5
	"shotgun",	// SDK_WEAPON_SHOTGUN
	"grenade",	// SDK_WEAPON_GRENADE
	"pistol",	// SDK_WEAPON_PISTOL
	"crowbar",	// SDK_WEAPON_CROWBAR

	NULL,		// WEAPON_NONE
};

//-----------------------------------------------------------------------------
// Given an alias, return the associated weapon ID
//-----------------------------------------------------------------------------
int AliasToWeaponID( const char *alias )
{
	if (alias)
	{
		for( int i=0; s_WeaponAliasInfo[i] != NULL; ++i )
			if (!Q_stricmp( s_WeaponAliasInfo[i], alias ))
				return i;
	}

	return WEAPON_NONE;
}

//-----------------------------------------------------------------------------
// Given a weapon ID, return its alias
//-----------------------------------------------------------------------------
const char *WeaponIDToAlias( int id )
{
	if ( (id >= SDK_WEAPON_MAX) || (id < 0) )
		return NULL;

	return s_WeaponAliasInfo[id];
}