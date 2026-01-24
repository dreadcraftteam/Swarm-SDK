//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: SDK player resource
//
//========================================================================//

#ifndef SDK_PLAYER_RESOURCE_H
#define SDK_PLAYER_RESOURCE_H

class CSDKPlayerResource : public CPlayerResource
{
	DECLARE_CLASS( CSDKPlayerResource, CPlayerResource );
	
public:
	DECLARE_SERVERCLASS();
	DECLARE_DATADESC();

	CSDKPlayerResource();

	virtual void UpdatePlayerData( void );
	virtual void Spawn( void );

protected:
#if defined ( SDK_USE_PLAYERCLASSES )
	CNetworkArray( int, m_iPlayerClass, MAX_PLAYERS+1 );
#endif
};

#endif // SDK_PLAYER_RESOURCE_H