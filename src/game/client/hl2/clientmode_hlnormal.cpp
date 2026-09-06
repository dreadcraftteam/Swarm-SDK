/*//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: Draws the normal TF2 or HL2 HUD.
//
// $Workfile:     $
// $Date:         $
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "clientmode_hlnormal.h"
#include "vgui_int.h"
#include "hud.h"
#include <vgui/IInput.h>
#include <vgui/IPanel.h>
#include <vgui/ISurface.h>
#include <vgui_controls/AnimationController.h>
#include "iinput.h"
#include "ienginevgui.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

extern bool g_bRollingCredits;

//-----------------------------------------------------------------------------
// Globals
//-----------------------------------------------------------------------------
vgui::HScheme g_hVGuiCombineScheme = 0;


// Instance the singleton and expose the interface to it.
IClientMode *GetClientModeNormal()
{
	static ClientModeHLNormal g_ClientModeNormal;
	return &g_ClientModeNormal;
}



//-----------------------------------------------------------------------------
// Purpose: this is the viewport that contains all the hud elements
//-----------------------------------------------------------------------------
class CHudViewport : public CBaseViewport
{
private:
	DECLARE_CLASS_SIMPLE( CHudViewport, CBaseViewport );

protected:
	virtual void ApplySchemeSettings( vgui::IScheme *pScheme )
	{
		BaseClass::ApplySchemeSettings( pScheme );

		GetHud().InitColors(pScheme);

		SetPaintBackgroundEnabled( false );
	}

	virtual void CreateDefaultPanels( void ) { };/ don't create any panels yet/ };
};


//-----------------------------------------------------------------------------
// ClientModeHLNormal implementation
//-----------------------------------------------------------------------------
ClientModeHLNormal::ClientModeHLNormal()
{
	m_pViewport = new CHudViewport();
	m_pViewport->Start( gameuifuncs, gameeventmanager );
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
ClientModeHLNormal::~ClientModeHLNormal()
{
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void ClientModeHLNormal::Init()
{
	BaseClass::Init();

	// Load up the combine control panel scheme
	g_hVGuiCombineScheme = vgui::scheme()->LoadSchemeFromFileEx( enginevgui->GetPanel( PANEL_CLIENTDLL ), IsXbox() ? "resource/ClientScheme.res" : "resource/CombinePanelScheme.res", "CombineScheme" );
	if (!g_hVGuiCombineScheme)
	{
		Warning( "Couldn't load combine panel scheme!\n" );
	}
}

bool ClientModeHLNormal::ShouldDrawCrosshair( void )
{
	return ( g_bRollingCredits == false );
}



class FullscreenHLViewport : public CHudViewport
{
private:
	DECLARE_CLASS_SIMPLE(FullscreenHLViewport, CHudViewport);

private:
	virtual void InitViewportSingletons(void)
	{
		SetAsFullscreenViewportInterface();
	}
};

class ClientModeHLNormalFullscreen : public	ClientModeHLNormal
{
	DECLARE_CLASS_SIMPLE(ClientModeHLNormalFullscreen, ClientModeHLNormal);
public:
	virtual void InitViewport()
	{
		// Skip over BaseClass!!!
		BaseClass::ClientModeShared::InitViewport();
		m_pViewport = new FullscreenHLViewport();
		m_pViewport->Start(gameuifuncs, gameeventmanager);
	}
};

static ClientModeHLNormalFullscreen g_FullscreenClientMode;
IClientMode* GetFullscreenClientMode(void)
{
	return &g_FullscreenClientMode;
}

void ClientModeHLNormal::DoPostScreenSpaceEffects(const CViewSetup *)
{

}
*/
//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//
#include "cbase.h"
#include "ivmodemanager.h"
#include "clientmode_hlnormal.h"
#include "panelmetaclassmgr.h"
#include "c_playerresource.h"
//#include "c_HL_player.h"
//#include "clientmode_HL.h"
#include "usermessages.h"
#include "prediction.h"
#include "viewpostprocess.h"
#include "glow_outline_effect.h"
#include "ienginevgui.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

// default FOV for HL2
//ConVar default_fov("default_fov", "75", FCVAR_CHEAT);

//extern EHANDLE g_eKillTarget1;
//extern EHANDLE g_eKillTarget2;

vgui::HScheme g_hVGuiCombineScheme = 0;

#define SCREEN_FILE		"scripts/vgui_screens.txt"

//void MsgFunc_KillCam(bf_read &msg) 
//{
//	C_HL_Player *pPlayer = C_HL_Player::GetLocalHLPlayer();
//
//	if ( !pPlayer )
//		return;
//
//	g_eKillTarget1 = 0;
//	g_eKillTarget2 = 0;
//	g_nKillCamTarget1 = 0;
//	g_nKillCamTarget1 = 0;
//
//	long iEncodedEHandle = msg.ReadLong();
//
//	if( iEncodedEHandle == INVALID_NETWORKED_EHANDLE_VALUE )
//		return;	
//
//	int iEntity = iEncodedEHandle & ((1 << MAX_EDICT_BITS) - 1);
//	int iSerialNum = iEncodedEHandle >> MAX_EDICT_BITS;
//
//	EHANDLE hEnt1( iEntity, iSerialNum );
//
//	iEncodedEHandle = msg.ReadLong();
//
//	if( iEncodedEHandle == INVALID_NETWORKED_EHANDLE_VALUE )
//		return;	
//
//	iEntity = iEncodedEHandle & ((1 << MAX_EDICT_BITS) - 1);
//	iSerialNum = iEncodedEHandle >> MAX_EDICT_BITS;
//
//	EHANDLE hEnt2( iEntity, iSerialNum );
//
//	g_eKillTarget1 = hEnt1;
//	g_eKillTarget2 = hEnt2;
//
//	if ( g_eKillTarget1.Get() )
//	{
//		g_nKillCamTarget1	= g_eKillTarget1.Get()->entindex();
//	}
//
//	if ( g_eKillTarget2.Get() )
//	{
//		g_nKillCamTarget2	= g_eKillTarget2.Get()->entindex();
//	}
//}

//-----------------------------------------------------------------------------
// Purpose: this is the viewport that contains all the hud elements
//-----------------------------------------------------------------------------
class CHudViewport : public CBaseViewport
{
private:
	DECLARE_CLASS_SIMPLE(CHudViewport, CBaseViewport);

protected:
	virtual void ApplySchemeSettings(vgui::IScheme* pScheme)
	{
		BaseClass::ApplySchemeSettings(pScheme);

		GetHud().InitColors(pScheme);

		SetPaintBackgroundEnabled(false);
	}

	virtual IViewPortPanel* CreatePanelByName(const char* szPanelName);
};

IViewPortPanel* CHudViewport::CreatePanelByName(const char* szPanelName)
{
	/*IViewPortPanel* newpanel = NULL;

	if ( Q_strcmp( PANEL_SCOREBOARD, szPanelName) == 0 )
	{
		newpanel = new CHL2MPClientScoreBoardDialog( this );
		return newpanel;
	}
	else if ( Q_strcmp(PANEL_INFO, szPanelName) == 0 )
	{
		newpanel = new CHL2MPTextWindow( this );
		return newpanel;
	}*/

	return BaseClass::CreatePanelByName(szPanelName);
}


class CHLModeManager : public IVModeManager
{
public:
	CHLModeManager(void);
	virtual		~CHLModeManager(void);

	virtual void	Init(void);
	virtual void	SwitchMode(bool commander, bool force);
	virtual void	OverrideView(CViewSetup* pSetup);
	virtual void	CreateMove(float flInputSampleTime, CUserCmd* cmd);
	virtual void	LevelInit(const char* newmap);
	virtual void	LevelShutdown(void);
};

//void CHLModeManager::CreateMove(float flInputSampleTime, CUserCmd* cmd)
//{
//}
/*
void CHLModeManager::LevelInit(const char* newmap)
{
	for (int i = 0; i < MAX_SPLITSCREEN_PLAYERS; ++i)
	{
		ACTIVE_SPLITSCREEN_PLAYER_GUARD(i);
		GetClientMode()->LevelInit(newmap);
	}

	if (g_nKillCamMode > OBS_MODE_NONE)
	{
		g_bForceCLPredictOff = false;
	}

	g_nKillCamMode = OBS_MODE_NONE;
	//g_nKillCamTarget1	= 0;
	//g_nKillCamTarget2	= 0;
}

void CHLModeManager::LevelShutdown(void)
{
	for (int i = 0; i < MAX_SPLITSCREEN_PLAYERS; ++i)
	{
		ACTIVE_SPLITSCREEN_PLAYER_GUARD(i);
		GetClientMode()->LevelShutdown();
	}
}*/


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
ClientModeHLNormal::ClientModeHLNormal()
{
	m_BlurFadeScale = 0;
		// Load up the combine control panel scheme

}

//-----------------------------------------------------------------------------
// Purpose: If you don't know what a destructor is by now, you are probably going to get fired
//-----------------------------------------------------------------------------
ClientModeHLNormal::~ClientModeHLNormal()
{
}

void ClientModeHLNormal::Init()
{
	BaseClass::Init();
	g_hVGuiCombineScheme = vgui::scheme()->LoadSchemeFromFileEx(enginevgui->GetPanel(PANEL_CLIENTDLL), IsXbox() ? "resource/ClientScheme.res" : "resource/CombinePanelScheme.res", "CombineScheme");
	if (!g_hVGuiCombineScheme)
	{
		Warning("Couldn't load combine panel scheme!\n");
	}
	//usermessages->HookMessage( "KillCam", MsgFunc_KillCam );
}

void ClientModeHLNormal::InitViewport()
{
	m_pViewport = new CHudViewport();
	m_pViewport->Start(gameuifuncs, gameeventmanager);
}

void ClientModeHLNormal::LevelInit(const char* newmap)
{

	m_BlurFadeScale = 0;

	BaseClass::LevelInit(newmap);
}

void ClientModeHLNormal::SetBlurFade(float scale)
{
	m_BlurFadeScale = scale;
}

extern ConVar building_cubemaps;
void ClientModeHLNormal::DoPostScreenSpaceEffects(const CViewSetup* pSetup)
{
	if (building_cubemaps.GetBool())
		return;

	MDLCACHE_CRITICAL_SECTION();

	g_GlowObjectManager.RenderGlowEffects(pSetup, GetSplitScreenPlayerSlot());

	if (m_BlurFadeScale)
	{
		CMatRenderContextPtr pRenderContext(materials);

		int xl, yl, dest_width, dest_height;
		pRenderContext->GetViewport(xl, yl, dest_width, dest_height);

		DoBlurFade(m_BlurFadeScale, 1.0f, xl, yl, dest_width, dest_height);
	}
}

ClientModeHLNormal g_ClientModeNormal[MAX_SPLITSCREEN_PLAYERS];

IClientMode* GetClientModeNormal()
{
	ASSERT_LOCAL_PLAYER_RESOLVABLE();
	return &g_ClientModeNormal[GET_ACTIVE_SPLITSCREEN_SLOT()];
}

//--------------------------------------------------------------------------------------------------------
class FullscreenHLViewport : public CHudViewport
{
private:
	DECLARE_CLASS_SIMPLE(FullscreenHLViewport, CHudViewport);

private:
	virtual void InitViewportSingletons(void)
	{
		SetAsFullscreenViewportInterface();
	}
};

class ClientModeHLNormalFullscreen : public	ClientModeHLNormal
{
	DECLARE_CLASS_SIMPLE(ClientModeHLNormalFullscreen, ClientModeHLNormal);
public:
	virtual void InitViewport()
	{
		// Skip over BaseClass!!!
		BaseClass::ClientModeShared::InitViewport();
		m_pViewport = new FullscreenHLViewport();
		m_pViewport->Start(gameuifuncs, gameeventmanager);
	}
};

//--------------------------------------------------------------------------------------------------------
static ClientModeHLNormalFullscreen g_FullscreenClientMode;
IClientMode* GetFullscreenClientMode(void)
{
	return &g_FullscreenClientMode;
}

ClientModeHLNormal* GetClientModeHLNormal()
{
	Assert(dynamic_cast<ClientModeHLNormal*>(GetClientModeNormal()));

	return static_cast<ClientModeHLNormal*>(GetClientModeNormal());
}