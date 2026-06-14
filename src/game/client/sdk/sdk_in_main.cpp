//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: SDK specific input handling
//
//========================================================================//

#include "cbase.h"

#include "kbutton.h"
#include "input.h"

//-----------------------------------------------------------------------------
// Purpose: SDK Input interface
//-----------------------------------------------------------------------------
class CSDKInput : public CInput
{
public:
};

static CSDKInput g_Input;

// Expose this interface
IInput *input = ( IInput * )&g_Input;