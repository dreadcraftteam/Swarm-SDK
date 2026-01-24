//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//========================================================================//

#include "cbase.h"

#include "EventLog.h"
#include "KeyValues.h"

#include "sdk_player.h"
#include "sdk_team.h"

class CSDKEventLog : public CEventLog
{
private:
	typedef CEventLog BaseClass;

public:
	virtual ~CSDKEventLog() {};

public:
	bool PrintEvent( IGameEvent * event )	// override virtual function
	{
		if ( !PrintSDKEvent( event ) )
		{
			return BaseClass::PrintEvent( event );
		}
		else
		{
			return true;
		}

		return false;
	}

	bool Init()
	{
		BaseClass::Init();

		ListenForGameEvent( "player_death" );
		ListenForGameEvent( "player_hurt" );

		return true;
	}

protected:
	bool PrintSDKEvent( IGameEvent * event )	// print Mod specific logs
	{
		const char *eventName = event->GetName();
	
		if ( !Q_strncmp( eventName, "server_", strlen("server_")) )
		{
			return false; // ignore server_ messages, always.
		}
		
		return false;
	}
};

CSDKEventLog g_SDKEventLog;

//-----------------------------------------------------------------------------
// Singleton access
//-----------------------------------------------------------------------------
CEventLog* GameLogSystem()
{
	return &g_SDKEventLog;
}