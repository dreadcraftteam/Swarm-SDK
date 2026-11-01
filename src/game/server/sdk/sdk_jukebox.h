//============ Copyright Valve Corporation, All rights reserved. ==============//
//
// Purpose:
//
//=============================================================================//

#ifndef SDK_JUKEBOX_H
#define SDK_JUKEBOX_H

#include "cbase.h"

class CSDK_Jukebox : public CBaseEntity
{
public:
	DECLARE_CLASS( CSDK_Jukebox, CBaseEntity );
	DECLARE_DATADESC();

	void InputMusicStart( inputdata_t &inputdata );
	void InputMusicStop( inputdata_t &inputdata );

protected:
	float m_fFadeInTime;
	float m_fFadeOutTime;
	char *m_szDefaultMusic;
};

#endif // SDK_JUKEBOX_H