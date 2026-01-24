//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Shared defines
//
//========================================================================//

#ifndef SDK_SHAREDDEFS_H
#define SDK_SHAREDDEFS_H

#define SDK_GAME_DESCRIPTION	"Swarm SDK"

//-----------------------------------------------------------------------------
// Most elements below here are specific to the options above.
//-----------------------------------------------------------------------------
#if defined ( SDK_USE_SPRINTING )
	#define INITIAL_SPRINT_STAMINA_PENALTY 15
	#define LOW_STAMINA_THRESHOLD	35
#endif // SDK_USE_SPRINTING

#define SDK_PLAYER_MODEL "models/player/blue_player.mdl"

// Tony; We need to precache all possible player models that we're going to use
extern const char *pszPossiblePlayerModels[];

extern const char *pszTeamNames[];

// Tony; these defines handle the default speeds for all of these - all are listed regardless of which option is enabled.
#define SDK_DEFAULT_PLAYER_RUNSPEED			220
#define SDK_DEFAULT_PLAYER_SPRINTSPEED		330
#define SDK_DEFAULT_PLAYER_PRONESPEED		50

//-----------------------------------------------------------------------------
// Weapon IDs for all SDK Game weapons
//-----------------------------------------------------------------------------
typedef enum
{
	WEAPON_NONE = 0,

	SDK_WEAPON_NONE = WEAPON_NONE,
	SDK_WEAPON_MP5,
	SDK_WEAPON_SHOTGUN,
	SDK_WEAPON_GRENADE,
	SDK_WEAPON_PISTOL,
	SDK_WEAPON_CROWBAR,

	SDK_WEAPON_MAX,	
} SDKWeaponID;

#if defined(TF_DLL) || defined(TF_CLIENT_DLL)
	#define TIME_TO_DUCK		0.2
	#define TIME_TO_DUCK_MS		200.0f
#else
	#define TIME_TO_DUCK		0.4
	#define TIME_TO_DUCK_MS		400.0f
#endif 

#define TIME_TO_UNDUCK		0.2
#define TIME_TO_UNDUCK_MS	200.0f

typedef enum
{
	FM_AUTOMATIC = 0,
	FM_SEMIAUTOMATIC,
	FM_BURST,

} SDK_Weapon_Firemodes;

const char *WeaponIDToAlias( int id );
int AliasToWeaponID( const char *alias );

// The various states the player can be in during the join game process.
enum SDKPlayerState
{
	// Happily running around in the game.
	// You can't move though if CSGameRules()->IsFreezePeriod() returns true.
	// This state can jump to a bunch of other states like STATE_PICKINGCLASS or STATE_DEATH_ANIM.
	STATE_ACTIVE=0,
	
	// This is the state you're in when you first enter the server.
	// It's switching between intro cameras every few seconds, and there's a level info 
	// screen up.
	STATE_WELCOME,			// Show the level intro screen.
	
	STATE_DEATH_ANIM,			// Playing death anim, waiting for that to finish.
	STATE_OBSERVER_MODE,		// Noclipping around, watching players, etc.

	NUM_PLAYER_STATES
};

#define SDK_PLAYER_DEATH_TIME	1.0f	//Minimum Time before respawning

// Special Damage types
enum
{
	SDK_DMG_CUSTOM_NONE = 0,
	SDK_DMG_CUSTOM_SUICIDE,
};

// Player avoidance
#define PUSHAWAY_THINK_INTERVAL		(1.0f / 20.0f)

#endif // SDK_SHAREDDEFS_H