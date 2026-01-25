//=============================================================================//
//
// Blobulator surface rendering
//
// By VALVe and heavily modified by DREADCRAFT
// 
//=============================================================================//

#ifndef C_SURFACERENDER_H
#define C_SURFACERENDER_H

#include "cbase.h"

#include "view.h"
#include "view_shared.h"
#include "iviewrender.h"
#include "engine/ivdebugoverlay.h"

#include "../../common/blobulator/Implicit/ImpDefines.h"
#include "../../common/blobulator/Implicit/ImpRenderer.h"
#include "../../common/blobulator/Implicit/ImpTiler.h"
#include "../../common/blobulator/Implicit/UserFunctions.h"

#ifdef SDK_USE_BLOBULATOR

void Surface_DrawSpheres(IMaterial* pMaterial, float flRadius);
void Surface_Draw( IClientRenderable *pClientRenderable, const Vector &vecRenderOrigin, IMaterial *pMaterial, float flCubeWidth, bool bSurfaceNoParticleCull = false );
void Surface_SafeLightCubeUpdate( const Vector &vecRenderOrigin, Vector4D *cachedCubeColours );

extern CUtlVector<ImpParticleWithFourInterpolants, CUtlMemoryAligned<ImpParticleWithFourInterpolants, 16>> g_SurfaceRenderParticles;
extern const QAngle g_SurfaceRenderAnglesAngles;

#endif // SDK_USE_BLOBULATOR

#endif // C_SURFACERENDER_H