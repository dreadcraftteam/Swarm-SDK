/*//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $Workfile:     $
// $Date:         $
// $NoKeywords: $
//=============================================================================//
#if !defined( CLIENTMODE_HLNORMAL_H )
#define CLIENTMODE_HLNORMAL_H
#ifdef _WIN32
#pragma once
#endif

#include "clientmode_shared.h"
#include <vgui_controls/EditablePanel.h>
#include <vgui/Cursor.h>

class CHudViewport;

namespace vgui
{
	typedef unsigned long HScheme;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class ClientModeHLNormal : public ClientModeShared
{
public:
	DECLARE_CLASS( ClientModeHLNormal, ClientModeShared );

	ClientModeHLNormal();
	~ClientModeHLNormal();

	virtual void	Init();
	virtual bool	ShouldDrawCrosshair( void );
	virtual void	DoPostScreenSpaceEffects(const CViewSetup* pSetup);
};

extern IClientMode *GetClientModeNormal();
extern vgui::HScheme g_hVGuiCombineScheme;

#endif // CLIENTMODE_HLNORMAL_H
*/
//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#ifndef HL_CLIENTMODE_H
#define HL_CLIENTMODE_H
#ifdef _WIN32
#pragma once
#endif

#include "clientmode_shared.h"
#include <vgui_controls/EditablePanel.h>
#include <vgui/Cursor.h>

class CHudViewport;

namespace vgui
{
	typedef unsigned long HScheme;
}

class ClientModeHLNormal : public ClientModeShared
{
	DECLARE_CLASS(ClientModeHLNormal, ClientModeShared);

private:

	// IClientMode overrides.
public:

	ClientModeHLNormal();
	virtual			~ClientModeHLNormal();

	virtual void	Init();
	virtual void	InitViewport();
	virtual void	LevelInit(const char* newmap);
	virtual void	DoPostScreenSpaceEffects(const CViewSetup* pSetup);

	virtual void	SetBlurFade(float scale);
	virtual float	GetBlurFade(void) { return m_BlurFadeScale; }

private:

	float m_BlurFadeScale;
	//	void	UpdateSpectatorMode( void );

};


extern IClientMode* GetClientModeNormal();
extern ClientModeHLNormal* GetClientModeHLNormal();
extern vgui::HScheme g_hVGuiCombineScheme;

#endif // HL_CLIENTMODE_H
