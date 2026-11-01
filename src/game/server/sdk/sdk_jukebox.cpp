//============ Copyright Valve Corporation, All rights reserved. ==============//
//
// Purpose:
//
//=============================================================================//

#include "cbase.h"
#include "sdk_jukebox.h"
#include "entityinput.h"
#include "entityoutput.h"

// It's first in this world public code of jukebox for sdk's server.dll
// yea. its important info. !!!
// dreadcraft;

LINK_ENTITY_TO_CLASS( sdk_jukebox, CSDK_Jukebox );

BEGIN_DATADESC( CSDK_Jukebox )
	DEFINE_KEYFIELD( m_fFadeInTime, FIELD_FLOAT, "FadeInTime" ),
	DEFINE_KEYFIELD( m_fFadeOutTime, FIELD_FLOAT, "FadeOutTime" ),
	DEFINE_KEYFIELD( m_szDefaultMusic, FIELD_STRING, "DefaultMusic" ),

	DEFINE_INPUTFUNC( FIELD_VOID, "StartMusic", InputMusicStart ),
	DEFINE_INPUTFUNC( FIELD_VOID, "StopMusic", InputMusicStop ),
END_DATADESC()

void CSDK_Jukebox::InputMusicStart( inputdata_t &inputdata )
{
	IGameEvent * event = gameeventmanager->CreateEvent( "jukebox_play_random" );
	
	// were shooting at our client with events
	if ( event )
	{
		event->SetFloat( "fadeintime", m_fFadeInTime );
		event->SetString( "defaulttrack", m_szDefaultMusic );
		gameeventmanager->FireEvent( event );
	}
}

void CSDK_Jukebox::InputMusicStop( inputdata_t &inputdata )
{
	// tell each client to stop the music
	IGameEvent * event = gameeventmanager->CreateEvent( "jukebox_stop" );
	if ( event )
	{
		event->SetFloat( "fadeouttime", m_fFadeOutTime );
		gameeventmanager->FireEvent( event );
	}
}