//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: NPCs demonstrating the Blobulator technology
// 
//========================================================================//

#include "cbase.h"
#include "dt_utlvector_recv.h"
#include "bone_setup.h"
#include "c_ai_basenpc.h"
#include "engine/IVDebugOverlay.h"
#include "view.h"
#include "view_shared.h"
#include "iviewrender.h"

#include "IVRenderView.h"

#include "tier0/vprof.h"
#include "soundinfo.h"

#include "c_surfacerender.h"

#include "materialsystem/imaterial.h"
#include "materialsystem/imaterialvar.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define	MAX_SURFACE_ELEMENTS 1000 // 200
#define MAX_EXTRA_ELEMENTS 400

static ConVar npc_surface_testshape("npc_surface_testshape", "0", FCVAR_NONE, "Use a test shape instead of the hydra");
static ConVar npc_surface_center("npc_surface_center", "0", FCVAR_NONE, "Adjust render center");
static ConVar npc_surface_fountain("npc_surface_fountain", "0", FCVAR_NONE, "Turns on settings for rendering the fountain");

//-----------------------------------------------------------------------------
// Class C_NPC_Surface
//-----------------------------------------------------------------------------
class C_NPC_Surface : public C_AI_BaseNPC
{
public:
	DECLARE_CLASS(C_NPC_Surface, C_AI_BaseNPC);
	DECLARE_CLIENTCLASS();
	DECLARE_INTERPOLATION();

	C_NPC_Surface();
	~C_NPC_Surface();

	// model specific
	virtual void GetRenderBounds(Vector& theMins, Vector& theMaxs);
	virtual bool IsTransparent(void);
	ShadowType_t	ShadowCastType() { return SHADOWS_NONE; }
	bool UsesPowerOfTwoFrameBufferTexture(void);
	RenderableTranslucencyType_t ComputeTranslucencyType();
	bool UsesFullFrameBufferTexture(void);
	int DrawModel(int flags, const RenderableInstance_t& instance) override;

	virtual bool	GetSoundSpatialization(SpatializationInfo_t& info);

	IMaterial* m_pMaterial;
	IMaterial* m_pArmatureMaterialPrePass;
	IMaterial* m_pArmatureMaterialMain;

	CUtlVector< Vector	> m_vecSurfacePos;
	CUtlVector< CInterpolatedVar< Vector > > m_iv_vecSurfacePos;

	CUtlVector< float > m_flSurfaceV;
	CUtlVector< CInterpolatedVar< float > > m_iv_flSurfaceV;

	CUtlVector< float > m_flSurfaceR;
	CUtlVector< CInterpolatedVar< float > > m_iv_flSurfaceR;

	int m_nActiveParticles;
	float m_flRadius;

private:
	C_NPC_Surface(const C_NPC_Surface&);
};


//-----------------------------------------------------------------------------
// Purpose: setup network receive table
//-----------------------------------------------------------------------------
IMPLEMENT_CLIENTCLASS_DT(C_NPC_Surface, DT_NPC_Surface, CNPC_Surface)

RecvPropFloat(RECVINFO(m_flRadius)),
RecvPropInt(RECVINFO(m_nActiveParticles)),
RecvPropUtlVector(RECVINFO_UTLVECTOR(m_vecSurfacePos), MAX_SURFACE_ELEMENTS, RecvPropVector(NULL, 0, sizeof(Vector))),
RecvPropUtlVector(RECVINFO_UTLVECTOR(m_flSurfaceV), MAX_SURFACE_ELEMENTS, RecvPropFloat(NULL, 0, sizeof(float))),
RecvPropUtlVector(RECVINFO_UTLVECTOR(m_flSurfaceR), MAX_SURFACE_ELEMENTS, RecvPropFloat(NULL, 0, sizeof(float))),

END_RECV_TABLE()

//-----------------------------------------------------------------------------
// Purpose: link networked elements to local data
//-----------------------------------------------------------------------------
C_NPC_Surface::C_NPC_Surface()
{
	m_pMaterial = NULL;
	m_pArmatureMaterialPrePass = NULL;
	m_pArmatureMaterialMain = NULL;

	m_vecSurfacePos.EnsureCount(MAX_SURFACE_ELEMENTS);
	m_iv_vecSurfacePos.EnsureCount(MAX_SURFACE_ELEMENTS);

	m_flSurfaceV.EnsureCount(MAX_SURFACE_ELEMENTS);
	m_iv_flSurfaceV.EnsureCount(MAX_SURFACE_ELEMENTS);

	m_flSurfaceR.EnsureCount(MAX_SURFACE_ELEMENTS);
	m_iv_flSurfaceR.EnsureCount(MAX_SURFACE_ELEMENTS);

	for (int i = 0; i < MAX_SURFACE_ELEMENTS; i++)
	{
		IInterpolatedVar* pWatcher = &m_iv_vecSurfacePos.Element(i);
		pWatcher->SetDebugName("m_iv_vecSurfacePos");
		AddVar(&m_vecSurfacePos.Element(i), pWatcher, LATCH_ANIMATION_VAR);

		pWatcher = &m_iv_flSurfaceV.Element(i);
		pWatcher->SetDebugName("m_iv_flSurfaceV");
		AddVar(&m_flSurfaceV.Element(i), pWatcher, LATCH_ANIMATION_VAR);

		pWatcher = &m_iv_flSurfaceR.Element(i);
		pWatcher->SetDebugName("m_iv_flSurfaceR");
		AddVar(&m_flSurfaceR.Element(i), pWatcher, LATCH_ANIMATION_VAR);
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
C_NPC_Surface::~C_NPC_Surface()
{
}

//-----------------------------------------------------------------------------
// Purpose: Custom model rendering
//-----------------------------------------------------------------------------
extern ConVar	mat_wireframe;

void C_NPC_Surface::GetRenderBounds(Vector& theMins, Vector& theMaxs)
{
	if (npc_surface_testshape.GetBool())
	{
		theMins.Init(-2048, -2048, -2048);
		theMaxs.Init(2048, 2048, 2048);
	}
	else
	{
		theMins = m_vecSurfacePos[0];
		theMaxs = m_vecSurfacePos[0];
		float surfaceRadius = m_flRadius * 3.0f;
		for (int i = 0; i < m_nActiveParticles; i++)
		{
			VectorMin(m_vecSurfacePos[i] - Vector(surfaceRadius, surfaceRadius, surfaceRadius), theMins, theMins);
			VectorMax(m_vecSurfacePos[i] + Vector(surfaceRadius, surfaceRadius, surfaceRadius), theMaxs, theMaxs);
		}
	}

	theMins -= GetRenderOrigin();
	theMaxs -= GetRenderOrigin();
}

bool C_NPC_Surface::IsTransparent()
{
	// TODO: Fix this
	return true;
}

//-----------------------------------------------------------------------------
// Yes we bloody are
//-----------------------------------------------------------------------------
RenderableTranslucencyType_t C_NPC_Surface::ComputeTranslucencyType()
{
	return RENDERABLE_IS_TRANSLUCENT;
}

bool C_NPC_Surface::UsesPowerOfTwoFrameBufferTexture()
{
	if (!m_pMaterial)
		return false;

	return m_pMaterial->NeedsPowerOfTwoFrameBufferTexture();
}

bool C_NPC_Surface::UsesFullFrameBufferTexture()
{
	if (!m_pMaterial)
		return false;

	return m_pMaterial->NeedsFullFrameBufferTexture();
}

__forceinline float sqr(float a) { return a * a; }

Vector lastPoint0Pos;

//-----------------------------------------------------------------------------
// Purpose: move the sound source to a point near the player
//-----------------------------------------------------------------------------
bool C_NPC_Surface::GetSoundSpatialization(SpatializationInfo_t& info)
{
	bool bret = BaseClass::GetSoundSpatialization(info);

	if (bret) //TODO
	{
	}

	return bret;
}

#ifdef USE_BLOBULATOR
int C_NPC_Surface::DrawModel(int flags, const RenderableInstance_t& instance)
{
	Vector fountainOrigin(-1980, -1792, 1);

	if (npc_surface_fountain.GetBool())
	{
		modelrender->SetupLighting(fountainOrigin);
	}
	else
	{
		modelrender->SetupLighting(GetRenderOrigin());
	}

	g_SurfaceRenderParticles.SetCount(MAX_SURFACE_ELEMENTS + MAX_EXTRA_ELEMENTS);

	int n_particles = 0;

	if (npc_surface_testshape.GetBool())
	{
		for (int i = -10; i <= 10; i++)
		{
			ImpParticle* imp_particle = &g_SurfaceRenderParticles[i + 10];
			imp_particle->center.set(i * 2.0f * m_flRadius, 0.0f, 0.0f);
			n_particles++;
		}
	}
	else
	{
		for (int i = 0; i < m_nActiveParticles; i++)
		{
			ImpParticleWithOneInterpolant* imp_particle = &g_SurfaceRenderParticles[i];
			imp_particle->center = m_vecSurfacePos[i];
			imp_particle->setFieldScale(m_flSurfaceR[i]);
			imp_particle->interpolants1[3] = m_flSurfaceV[i];
			n_particles++;
		}

		// This code adds a water surface to the fountain trough
		// using particles that oscillate up and down
		if (npc_surface_fountain.GetBool())
		{
			static float time = 0.0f;

			bool paused = m_vecSurfacePos[0] == lastPoint0Pos;
			lastPoint0Pos = m_vecSurfacePos[0];
			if (!paused) time += 0.1f;

			for (int i = -7; i <= 7; i++)
				for (int j = -7; j <= 7; j++)
				{
					ImpParticleWithOneInterpolant* imp_particle = &g_SurfaceRenderParticles[n_particles++];
					imp_particle->center = fountainOrigin + Vector(i * 2.0f * m_flRadius, j * 2.0f * m_flRadius, 15.0f);
					float dist = sqrtf(sqr(imp_particle->center[0]) + sqr(imp_particle->center[1]));

					imp_particle->center[2] += 2.0f * sin(2.0f * time + 2.0f * dist);
					imp_particle->setFieldScale(1.0f);
					imp_particle->interpolants1[3] = 0.0f;
				}
			for (int i = -2; i <= 2; i++)
				for (int j = -2; j <= 2; j++)
				{
					ImpParticleWithOneInterpolant* imp_particle = &g_SurfaceRenderParticles[n_particles++];
					imp_particle->center = fountainOrigin + Vector(i * 2.0f * m_flRadius, j * 2.0f * m_flRadius, 15.0f - 2.0f * m_flRadius);
					imp_particle->setFieldScale(1.0f);
					imp_particle->interpolants1[3] = 0.0f;
				}

			ImpParticleWithOneInterpolant* imp_particle = &g_SurfaceRenderParticles[n_particles++];
			imp_particle->center = fountainOrigin + Vector(0.0f, 0.0f, 15.0f - 4.0f * m_flRadius);
			imp_particle->setFieldScale(1.0f);
			imp_particle->interpolants1[3] = 0.0f;
		}
	}

	g_SurfaceRenderParticles.SetCountNonDestructively(n_particles);

	Vector center;
	if (npc_surface_testshape.GetBool())
	{
		center.Init();
	}
	else if (npc_surface_fountain.GetBool())
	{
		center = fountainOrigin;
	}
	else
	{
		center.Init();

		if (npc_surface_center.GetBool())
		{
			for (int i = 0; i < m_nActiveParticles; i++)
			{
				center += m_vecSurfacePos[i];
			}

			center /= m_nActiveParticles;
		}
	}

	// Load the surface material
	m_pMaterial = materials->FindMaterial("npc/blob/blobsurface", TEXTURE_GROUP_OTHER, true);
	Surface_Draw(GetClientRenderable(), center, m_pMaterial, 8.5f);

	return 1;
}
#endif // USE_BLOBULATOR