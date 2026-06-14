//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Client-side C_SDKTeam class
//
//========================================================================//

#ifndef C_SDK_TEAM_H
#define C_SDK_TEAM_H
#ifdef _WIN32
#pragma once
#endif

#include "c_team.h"
#include "shareddefs.h"
#include "sdk_playerclass_info_parse.h"

class C_BaseEntity;
class C_BaseObject;
class CBaseTechnology;

// Tony; so we can call this from shared code!
#define CSDKTeam C_SDKTeam

//-----------------------------------------------------------------------------
// Purpose: SDK's team manager
//-----------------------------------------------------------------------------
class C_SDKTeam : public C_Team
{
	DECLARE_CLASS( C_SDKTeam, C_Team );
	DECLARE_CLIENTCLASS();

public:
					C_SDKTeam();
	virtual			~C_SDKTeam();

	virtual char	*Get_Name( void );

private:
};

class C_SDKTeam_Unassigned : public C_SDKTeam
{
	DECLARE_CLASS( C_SDKTeam_Unassigned, C_SDKTeam );
public:
	DECLARE_CLIENTCLASS();

				     C_SDKTeam_Unassigned();
	 virtual		~C_SDKTeam_Unassigned() {}
};

extern C_SDKTeam *GetGlobalSDKTeam( int iIndex );

#endif // C_SDK_TEAM_H