//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Basic BOT handling
//
//========================================================================//

#ifndef SDK_BOT_TEMP_H
#define SDK_BOT_TEMP_H

// If iTeam or iClass is -1, then a team or class is randomly chosen.
CBasePlayer *BotPutInServer( bool bFrozen, int iTeam, int iClass );

void Bot_RunAll();

#endif // SDK_BOT_TEMP_H