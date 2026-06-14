//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: SDK player resource
//
//========================================================================//

#include "cbase.h"

#include "c_sdk_player_resource.h"
#include "c_sdk_player.h"
#include "sdk_gamerules.h"
#include <shareddefs.h>
#include "hud.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

IMPLEMENT_CLIENTCLASS_DT(C_SDK_PlayerResource, DT_SDKPlayerResource, CSDKPlayerResource)

END_RECV_TABLE()

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
C_SDK_PlayerResource::C_SDK_PlayerResource()
{
	m_Colors[TEAM_UNASSIGNED] = COLOR_YELLOW;
	m_Colors[TEAM_SPECTATOR] = COLOR_GREY;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
C_SDK_PlayerResource::~C_SDK_PlayerResource()
{
}

C_SDK_PlayerResource * SDKGameResources( void )
{
	return dynamic_cast<C_SDK_PlayerResource *>(GameResources());
}