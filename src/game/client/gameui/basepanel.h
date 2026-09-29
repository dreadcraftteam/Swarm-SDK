//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef BASEPANEL_H
#define BASEPANEL_H
#ifdef _WIN32
#pragma once
#endif

#ifdef SWARM_DLL
#include "swarmui/basemodpanel.h"
#elif SDK_CLIENT_DLL
#include "scratchui/basemodpanel.h"
#endif

inline BaseModUI::CBaseModPanel * BasePanel() { return &BaseModUI::CBaseModPanel::GetSingleton(); }

#endif // BASEPANEL_H
