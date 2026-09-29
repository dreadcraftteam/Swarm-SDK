//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
//========================================================================//

#include "cbase.h"

#include "ai_hull.h"
#include "saverestore_utlvector.h"
#include "dt_utlvector_send.h"
#include "physics_saverestore.h"
#include "vphysics/constraints.h"
#include "vcollide_parse.h"
#include "ragdoll_shared.h"
#include "physics_prop_ragdoll.h"
#include "collisionutils.h"
#include "te_effect_dispatch.h"
#include "AI_BaseNPC.h"
#include "soundenvelope.h"
#include "player_pickup.h"

#ifdef USE_BLOBULATOR
#include "../common/blobulator/Physics/PhysParticleCache.h"
#include "../common/blobulator/Physics/PhysTiler.h"
#endif // USE_BLOBULATOR

#include "npc_surface.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

static ConVar npc_surface_tension("npc_surface_tension", "5", 0, "How strong the surface tries to keep its shape");
static ConVar npc_surface_radius("npc_surface_radius", "8", 0, "Radius of each sphere");
static ConVar npc_surface_ideal("npc_surface_ideal", "2", 0, "ideal distance (N * radius) between each sphere");
static ConVar npc_surface_nearby("npc_surface_nearby", "3", 0, "acceptable distance (N * radius) between each and still be considered touching");
static ConVar npc_surface_scale("npc_surface_scale", "0.5");
static ConVar npc_lj_strength("lj_strength", "1", 0);
static ConVar lj_InteractionRadius("lj_InteractionRadius", "3", 0);
static ConVar lj_SurfaceTension("lj_SurfaceTension", "1", 0);
static ConVar lj_Repulsion("lj_Repulsion", "0.1", 0);
static ConVar lj_Attraction("lj_Attraction", "0.1", 0);
static ConVar lj_MaxRepulsion("lj_MaxRepulsion", "1", 0);
static ConVar lj_MaxAttraction("lj_MaxAttraction", "1", 0);
ConVar npc_surface_debug("npc_surface_debug", "0", FCVAR_CHEAT, "Turns on debug messages of the npc_surface");
ConVar npc_surface_depth_strength("npc_surface_depth_strength", "0");

//---------------------------------------------------------
// Purpose:
//---------------------------------------------------------
IMPLEMENT_SERVERCLASS_ST(CNPC_Surface, DT_NPC_Surface)

SendPropInt(SENDINFO(m_nActiveParticles), 12, SPROP_UNSIGNED),
SendPropFloat(SENDINFO(m_flRadius), 12, 0, 0.0, 100.0),
SendPropUtlVector(SENDINFO_UTLVECTOR(m_vecSurfacePos), MAX_SURFACE_ELEMENTS, SendPropVector(NULL, 0, sizeof(Vector), -1, SPROP_COORD)),
SendPropUtlVector(SENDINFO_UTLVECTOR(m_flSurfaceV), MAX_SURFACE_ELEMENTS, SendPropFloat(NULL, 0, sizeof(float), 6, 0, 0.0, 1.0)),
SendPropUtlVector(SENDINFO_UTLVECTOR(m_flSurfaceR), MAX_SURFACE_ELEMENTS, SendPropFloat(NULL, 0, sizeof(float), 6, 0, 0.0, 2.0)),

END_SEND_TABLE()

//---------------------------------------------------------
// Save/Restore
//---------------------------------------------------------
BEGIN_DATADESC(CNPC_Surface)

DEFINE_UTLVECTOR(m_vecSurfacePos, FIELD_POSITION_VECTOR),
DEFINE_UTLVECTOR(m_flSurfaceV, FIELD_FLOAT),
DEFINE_UTLVECTOR(m_flSurfaceR, FIELD_FLOAT),
DEFINE_FIELD(m_flRadius, FIELD_FLOAT),

DEFINE_AUTO_ARRAY(m_bContact, FIELD_BOOLEAN),

END_DATADESC()

//-----------------------------------------------------------------------------
// Purpose: Precaching
//-----------------------------------------------------------------------------
void CNPC_Surface::Precache()
{
	PrecacheModel("models/Hydra.mdl");
	BaseClass::Precache();
}

void CNPC_Surface::Activate(void)
{
	BaseClass::Activate();
}

//-----------------------------------------------------------------------------
// Purpose: Returns this monster's place in the relationship table.
//-----------------------------------------------------------------------------
Class_T	CNPC_Surface::Classify(void)
{
	return CLASS_BARNACLE;
}

void CNPC_Surface::Spawn()
{
	Precache();

	BaseClass::Spawn();

	SetModel("models/Hydra.mdl");

	SetHullType(HULL_SMALL_CENTERED);
	SetHullSizeNormal();

	// Setup model
	SetSolid(SOLID_BBOX);
	AddSolidFlags(FSOLID_FORCE_WORLD_ALIGNED | FSOLID_NOT_STANDABLE);
	SetCollisionBounds(-Vector(400, 400, 100), Vector(400, 400, 400));
	SetMoveType(MOVETYPE_VPHYSICS);

	AddSolidFlags(FSOLID_CUSTOMRAYTEST | FSOLID_CUSTOMBOXTEST);

	SetCollisionGroup(COLLISION_GROUP_NPC);

	AddEFlags(EFL_NO_DISSOLVE);
	SetBloodColor(BLOOD_COLOR_YELLOW);
	ClearEffects();
	m_iHealth = 200;
	m_flFieldOfView = -1.0;
	m_NPCState = NPC_STATE_NONE;

	SetAbsAngles(QAngle(0, 0, 0));

	m_vecSurfacePos.EnsureCount(MAX_SURFACE_ELEMENTS);
	m_flSurfaceV.EnsureCount(MAX_SURFACE_ELEMENTS);
	m_flSurfaceR.EnsureCount(MAX_SURFACE_ELEMENTS);

	m_vecStart = GetAbsOrigin();

	m_vecSurfacePos[0] = m_vecStart;
	m_flSurfaceV[0] = 0.0;
	m_flSurfaceR[0] = 1.0;

	m_flRadius = npc_surface_radius.GetFloat();

	for (int i = 1; i < MAX_SURFACE_ELEMENTS; i++)
	{
		m_vecSurfacePos[i] = m_vecSurfacePos[i - 1];
		m_flSurfaceV[i] = m_flSurfaceV[i - 1];
		m_flSurfaceR[i] = m_flSurfaceR[i - 1];
	}

	NPCInit();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_Surface::VPhysicsCollision(int index, gamevcollisionevent_t* pEvent)
{
	BaseClass::VPhysicsCollision(index, pEvent);

	int nSphere = pEvent->pObjects[index]->GetGameIndex();

	m_bContact[nSphere] = true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_Surface::OnSave(IEntitySaveUtils* pUtils)
{
	BaseClass::OnSave(pUtils);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_Surface::OnRestore(void)
{
	BaseClass::OnRestore();

	CreateVPhysics(true);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CNPC_Surface::CreateVPhysics()
{
	return CreateVPhysics(false);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CNPC_Surface::CreateVPhysics(bool bFromRestore)
{
	// setup individual spheres
	objectparams_t params = g_PhysDefaultObjectParams;
	params.pGameData = static_cast<void*>(this);

	int nMaterialIndex = physprops->GetSurfaceIndex("water");

	int i;

	for (i = 0; i < m_nActiveParticles; i++)
	{
		if (!bFromRestore)
		{
			m_vecSurfacePos[i] = GetAbsOrigin() + Vector(RandomFloat(-10, 10), RandomFloat(-10, 10), RandomFloat(0, 2)) * m_flRadius;
		}

		m_pSpheres[i] = physenv->CreateSphereObject(m_flRadius, nMaterialIndex, m_vecSurfacePos[i], GetAbsAngles(), &params, false);
		if (m_pSpheres[i])
		{
			Vector vVelocity = Vector(RandomFloat(-1, 1), RandomFloat(-1, 1), RandomFloat(1, 2)) * 10.0f;
			m_pSpheres[i]->SetVelocity(&vVelocity, NULL);

			PhysSetGameFlags(m_pSpheres[i], FVPHYSICS_NO_SELF_COLLISIONS | FVPHYSICS_MULTIOBJECT_ENTITY); // call collisionruleschanged if this changes dynamically
			m_pSpheres[i]->SetGameIndex(i);

			m_pSpheres[i]->SetMass(10.0f);
			m_pSpheres[i]->EnableGravity(true);
			m_pSpheres[i]->EnableDrag(true);

			float flDamping = 0.5f;
			float flAngDamping = 0.5f;
			m_pSpheres[i]->SetDamping(&flDamping, &flAngDamping);
		}
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: return a list of all the physics objects
//-----------------------------------------------------------------------------
int CNPC_Surface::VPhysicsGetObjectList(IPhysicsObject** pList, int listMax)
{
	int count = 0;

	for (int i = 0; i < m_nActiveParticles && i < MAX_SURFACE_ELEMENTS && count < listMax; i++)
	{
		if (i < MAX_SURFACE_ELEMENTS && m_flSurfaceR[i] > 0.0f && m_pSpheres[i] != NULL)
		{
			pList[count++] = m_pSpheres[i];
		}
	}
	return count;
}

bool CNPC_Surface::VPhysicsIsFlesh(void)
{
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_Surface::OnPhysGunPickup(CBasePlayer* pPhysGunUser, PhysGunPickup_t reason)
{
	CDefaultPlayerPickupVPhysics::OnPhysGunPickup(pPhysGunUser, reason);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_Surface::OnPhysGunDrop(CBasePlayer* pPhysGunUser, PhysGunDrop_t Reason)
{
	CDefaultPlayerPickupVPhysics::OnPhysGunDrop(pPhysGunUser, Reason);
}

//-----------------------------------------------------------------------------
// Purpose: Detect that the physgun is trying to punt us. Currently guess about damage
//-----------------------------------------------------------------------------
bool CNPC_Surface::OnAttemptPhysGunPickup(CBasePlayer* pPhysGunUser, PhysGunPickup_t reason)
{
	if (reason == PUNTED_BY_CANNON)
	{
		Vector forward;
		pPhysGunUser->EyeVectors(&forward);

		Vector start, end;
		start = pPhysGunUser->Weapon_ShootPosition();

		float d1, d2;
		for (int i = 0; i < m_nActiveParticles; i++)
		{
			if (m_flSurfaceR[i] > 0.0f && IntersectInfiniteRayWithSphere(start, forward, m_vecSurfacePos[i], m_flRadius * 3, &d1, &d2))
			{
				Vector p1 = start + d1 * forward;

				// no idea what sort of forces to use when punting
				// also, forceoffset just applies a spin, it doesn't act like being hit with a larger sphere
				m_pSpheres[i]->ApplyForceOffset(forward * 1000.0f, p1);
			}
		}

		return false;
	}
	return CDefaultPlayerPickupVPhysics::OnAttemptPhysGunPickup(pPhysGunUser, reason);
}

//-----------------------------------------------------------------------------
// Purpose: Apply collisions to multiple spheres instead of just the one hit
//-----------------------------------------------------------------------------
void CNPC_Surface::ApplyDamageForce(const CTakeDamageInfo& info)
{
	float flMinDist2 = (10 * m_flRadius);
	flMinDist2 = flMinDist2 * flMinDist2;

	// FIXME: this needs a better algorithm for radiating the force
	float flForce = info.GetDamageForce().Length();

	flMinDist2 *= MAX(1.0, sqrt(flForce / 1000));

	if (!(info.GetDamageType() & DMG_BLAST))
	{
		// apply non-blast damage in the direction of the force
		for (int i = 0; i < m_nActiveParticles; i++)
		{
			if (m_flSurfaceR[i] > 0.0f)
			{
				float flDist2 = (m_vecSurfacePos[i] - info.GetDamagePosition()).LengthSqr();
				if (flDist2 < flMinDist2)
				{
					m_pSpheres[i]->ApplyForceOffset(info.GetDamageForce() * (1.0 - flDist2 / flMinDist2), info.GetDamagePosition());
				}
			}
		}
	}
	else
	{
		// blast damage goes out from a point
		for (int i = 0; i < m_nActiveParticles; i++)
		{
			if (m_flSurfaceR[i] > 0.0f)
			{
				float flDist2 = (m_vecSurfacePos[i] - info.GetDamagePosition()).LengthSqr();
				if (flDist2 < flMinDist2)
				{
					Vector dir = (m_vecSurfacePos[i] - info.GetDamagePosition()) / sqrt(flDist2);
					m_pSpheres[i]->ApplyForceCenter(dir * flForce * (1.0 - flDist2 / flMinDist2));
				}
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CNPC_Surface::OnTakeDamage_Alive(const CTakeDamageInfo& info)
{
	CTakeDamageInfo tdInfo(info);

	// don't ever actually take damage
	tdInfo.SetDamage(0);

	if (tdInfo.GetDamageType() & (DMG_BULLET | DMG_CLUB))
	{
		ApplyDamageForce(tdInfo);
	}

	if (info.GetDamageType() & DMG_BLAST)
	{
		ApplyDamageForce(tdInfo);
	}

	return BaseClass::OnTakeDamage_Alive(tdInfo);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CNPC_Surface::TestCollision(const Ray_t& ray, unsigned int fContentsMask, trace_t& tr)
{
	int nLastHit = -1;

	if (ray.m_IsRay)
	{
		float d1, d2;

		for (int i = 0; i < m_nActiveParticles; i++)
		{
			if (m_flSurfaceR[i] > 0.0f && IntersectRayWithSphere(ray.m_Start, ray.m_Delta, m_vecSurfacePos[i], m_flRadius, &d1, &d2))
			{
				if (d1 < tr.fraction)
				{
					nLastHit = i;
					tr.fraction = d1;
				}
			}
		}
		if (nLastHit != -1)
		{
			tr.m_pEnt = this;
			tr.startpos = ray.m_Start;
			tr.endpos = ray.m_Start + ray.m_Delta * tr.fraction;
			tr.contents = CONTENTS_SOLID;
			tr.hitbox = nLastHit;
			tr.hitgroup = HITGROUP_GENERIC;
			tr.plane.dist = tr.endpos.Length();
			tr.plane.normal = (tr.endpos - m_vecSurfacePos[nLastHit]) * (1 / m_flRadius);
			tr.plane.type = 0;
			tr.physicsbone = nLastHit;
		}
	}
	else
	{
		// FIXME: This isn't a valid test, Jay needs to make it real
		Vector vecMin = Vector(-m_flRadius, -m_flRadius, -m_flRadius) - ray.m_Extents;
		Vector vecMax = Vector(m_flRadius, m_flRadius, m_flRadius) + ray.m_Extents;

		trace_t boxtrace;

		tr.fraction = 1.0;

		for (int i = 0; i < m_nActiveParticles; i++)
		{
			if (m_flSurfaceR[i] > 0.0f && IntersectRayWithBox(m_vecSurfacePos[i] - ray.m_Start, -ray.m_Delta, vecMin, vecMax, 0.0, &boxtrace))
			{
				if (boxtrace.fraction < tr.fraction)
				{
					if (tr.startsolid && !IsBoxIntersectingSphere(ray.m_Start - ray.m_Extents, ray.m_Start + ray.m_Extents, m_vecSurfacePos[i], m_flRadius))
					{
						tr.startsolid = false;
						tr.allsolid = false;
					}
					else if (tr.allsolid && !IsBoxIntersectingSphere(ray.m_Start + ray.m_Delta - ray.m_Extents, ray.m_Start + ray.m_Delta + ray.m_Extents, m_vecSurfacePos[i], m_flRadius))
					{
						tr.allsolid = false;
					}

					tr = boxtrace;
					tr.startpos = ray.m_Start;
					tr.endpos = ray.m_Start + tr.fraction * ray.m_Delta;
					nLastHit = i;
				}
			}
		}

		if (tr.fraction < 1.0)
		{
			tr.contents = CONTENTS_SOLID;
			tr.m_pEnt = this;
			tr.hitbox = nLastHit;
			tr.hitgroup = HITGROUP_GENERIC;
			tr.physicsbone = nLastHit;
		}
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_Surface::RunAI(void)
{
	BaseClass::RunAI();

	SetNextThink(gpGlobals->curtime + SURFACE_THINK_INTERVAL);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
float CNPC_Surface::MaxYawSpeed()
{
	return 180;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CNPC_Surface::TranslateSchedule(int scheduleType)
{
	return BaseClass::TranslateSchedule(scheduleType);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_Surface::HandleAnimEvent(animevent_t* pEvent)
{
	BaseClass::HandleAnimEvent(pEvent);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_Surface::PrescheduleThink()
{
	BaseClass::PrescheduleThink();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CNPC_Surface::SelectSchedule()
{
	return BaseClass::SelectSchedule();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_Surface::StartTask(const Task_t* pTask)
{
	BaseClass::StartTask(pTask);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_Surface::RunTask(const Task_t* pTask)
{
	BaseClass::RunTask(pTask);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
Vector CNPC_Surface::EyePosition()
{
	return GetAbsOrigin();
}

const QAngle& CNPC_Surface::EyeAngles()
{
	return GetAbsAngles();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
Vector CNPC_Surface::BodyTarget(const Vector& posSrc, bool bNoisy)
{
	int iShortest = 0;
	float flShortestDist = (posSrc - m_vecSurfacePos[iShortest]).LengthSqr();
	for (int i = 1; i < m_nActiveParticles; i++)
	{
		if (m_flSurfaceR[i] > 0.0f)
		{
			float flDist = (posSrc - m_vecSurfacePos[i]).LengthSqr();
			if (flDist < flShortestDist)
			{
				iShortest = i;
				flShortestDist = flDist;
			}
		}
	}

	return m_vecSurfacePos[iShortest];
}

#ifdef USE_BLOBULATOR
//-----------------------------------------------------------------------------
// Purpose: Contructor
//-----------------------------------------------------------------------------
CLennardJonesForce::CLennardJonesForce()
{
	m_fInteractionRadius = lj_InteractionRadius.GetFloat();
	m_fSurfaceTension = lj_SurfaceTension.GetFloat();
	m_fLennardJonesRepulsion = lj_Repulsion.GetFloat();
	m_fLennardJonesAttraction = lj_Attraction.GetFloat();
	m_fMaxRepulsion = lj_MaxRepulsion.GetFloat();
	m_fMaxAttraction = lj_MaxAttraction.GetFloat();
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
CLennardJonesForce::~CLennardJonesForce()
{

}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CLennardJonesForce::addParticleForce(PhysParticle* a, PhysParticle* b, float distSq, float flStrength, float ts) const
{
	float d = sqrtf(distSq);

	// based on equation of force between two molecules which is
	// factor * ((distance/bond_length)^-7 - (distance/bond_length)^-13)
	float f;
	if (a->group == b->group) // In the same group
	{
		float p = a->radius * 2.0f / (d + FLT_EPSILON);
		float p2 = p * p;
		float p4 = p2 * p2;

		float surface_tension_modifier = ((24.0f * m_fSurfaceTension) / (a->neighbor_count + b->neighbor_count + 0.1f)) + 1.0f;

		float lennard_jones_force = m_fLennardJonesAttraction * p2 - m_fLennardJonesRepulsion * p4;
		f = surface_tension_modifier * lennard_jones_force;
	}
	else
	{
		// This was 3.5 ... made 3.0 so particles get closer when they collide
		if (d > a->radius * 3.0f) return;

		float p = a->radius * 4.0f / d;
		f = -1.0f * p * p;
	}

	// These checks are great to have, but are they really necessary?
	// It might also be good to have a limit on velocity

	// Attraction is a positive value.
	// Repulsion is negative.
	if (f < -m_fMaxRepulsion) f = -m_fMaxRepulsion;
	if (f > m_fMaxAttraction) f = m_fMaxAttraction;

	Point3D scaledr = (b->center - a->center) * (f / (d + FLT_EPSILON)) * flStrength; // Dividing by d scales distance down to a unit vector
	a->force = a->force + scaledr;
	b->force = b->force - scaledr;
}

void CLennardJonesForce::AddForces(IPhysicsObject** pObject, int nObjects, float flRadius, float flStrength, Vector* pForces)
{
	int nParticles = nObjects;

	// hack: copy cvars into settings so it can be edited live
	m_fInteractionRadius = lj_InteractionRadius.GetFloat();
	m_fSurfaceTension = lj_SurfaceTension.GetFloat();
	m_fLennardJonesRepulsion = lj_Repulsion.GetFloat();
	m_fLennardJonesAttraction = lj_Attraction.GetFloat();
	m_fMaxRepulsion = lj_MaxRepulsion.GetFloat();
	m_fMaxAttraction = lj_MaxAttraction.GetFloat();

	// FIXME: this isn't thread safe
	static SmartArray<PhysParticle> imp_particles_sa; // This doesn't specify alignment, might have problems with SSE
	while (imp_particles_sa.size < nObjects)
	{
		imp_particles_sa.pushAutoSize(PhysParticle());
	}
	// Should this be a class variable still?
	PhysTiler* m_pPhysTiler = PhysTilerFactory::factory->getTiler();

	m_pPhysTiler->setCacheParams(m_fInteractionRadius, 0.0f);

	// Centered and scaled?
	m_pPhysTiler->beginFrame(Point3D(0.0f, 0.0f, 0.0f));

	// Move the spheres into particles
	for (int i = 0; i < nObjects; i++)
	{
		PhysParticle* particle = &(imp_particles_sa[i]);
		particle->force.set(0, 0, 0);

		Vector pos;
		QAngle ang;
		pObject[i]->GetPosition(&pos, &ang);

		particle->center = pos * (1.0 / flRadius);
		particle->group = i / 20;
		particle->neighbor_count = 0;
		m_pPhysTiler->insertParticle(particle);
	}

	m_pPhysTiler->processTiles();

	float timeStep = 1.0f; // This should be customizable
	float nearNeighborInteractionRadius = 2.3f;
	float nearNeighborInteractionRadiusSq = nearNeighborInteractionRadius * nearNeighborInteractionRadius;

	PhysParticleCache* pCache = m_pPhysTiler->getParticleCache();

	// Calculate number of near neighbors for each particle
	for (int i = 0; i < nParticles; i++)
	{
		PhysParticle* b1 = &(imp_particles_sa[i]);

		PhysParticleAndDist* node = pCache->get(b1);

		while (node->particle != NULL)
		{
			PhysParticle* b2 = node->particle;

			// Compare addresses of the two particles. This makes sure we apply a force only once between a pair of particles.
			if (b1 < b2 && node->distSq < nearNeighborInteractionRadiusSq)
			{
				b1->neighbor_count++;
				b2->neighbor_count++;
			}

			node++;
		}
	}

	// Calculate forces on particles due to other particles
	for (int i = 0; i < nParticles; i++)
	{
		PhysParticle* b1 = &(imp_particles_sa[i]);

		PhysParticleAndDist* node = pCache->get(b1);

		while (node->particle != NULL)
		{
			PhysParticle* b2 = node->particle;

			// Compare addresses of the two particles. This makes sure we apply a force only once between a pair of particles.
			if (b1 < b2)
			{
				addParticleForce(b1, b2, node->distSq, flStrength, timeStep);
			}

			node++;
		}
	}
	m_pPhysTiler->endFrame();
	PhysTilerFactory::factory->returnTiler(m_pPhysTiler); // Should this be a class variable still?

	// forces into output array
	for (int i = 0; i < nObjects; i++)
	{
		pForces[i] = imp_particles_sa[i].force.AsVector() * flRadius;
	}
}

//---------------------------------------------------------
// Save/Restore
//---------------------------------------------------------
BEGIN_DATADESC(CNPC_BlobArmTest)

DEFINE_AUTO_ARRAY(m_nOwnedSlot, FIELD_INTEGER),
DEFINE_AUTO_ARRAY(m_nTargetSlot, FIELD_INTEGER),
DEFINE_AUTO_ARRAY(m_bFloat, FIELD_BOOLEAN),
DEFINE_FIELD(m_bDoArms, FIELD_BOOLEAN),
DEFINE_FIELD(m_hTarget, FIELD_EHANDLE),
DEFINE_FIELD(m_flSimTime, FIELD_TIME),

END_DATADESC()


LINK_ENTITY_TO_CLASS(npc_surface, CNPC_BlobArmTest);

void CNPC_BlobArmTest::Spawn(void)
{
	VPROF("CNPC_BlobArmTest::Spawn");

	BaseClass::Spawn();

	m_flSimTime = 0;
	m_bDoArms = false;

}

bool CNPC_BlobArmTest::CreateVPhysics(bool bFromRestore)
{
	VPROF("CNPC_BlobArmTest::CreateVPhysics");

	// FIXME: don't hardcode the number of particles
	m_nActiveParticles = 50;

	bool result = BaseClass::CreateVPhysics(bFromRestore);

	return result;
}

void CNPC_BlobArmTest::RunAI(void)
{
	VPROF("CNPC_BlobArmTest::RunAI");

	Vector vel;
	m_pSpheres[0]->GetVelocity(&vel, NULL);

	float dt = gpGlobals->frametime;

	Vector newPos = GetLocalOrigin() + vel * dt;
	SetLocalOrigin(newPos);

	float flIdealDistance = m_flRadius * npc_surface_ideal.GetFloat();
	float flNearbyDistance = m_flRadius * npc_surface_nearby.GetFloat();

	int i, j;

	// copy physics positions into networked array
	for (i = 0; i < m_nActiveParticles; i++)
	{
		Vector pos;
		QAngle ang;
		m_pSpheres[i]->GetPosition(&pos, &ang);
		m_vecSurfacePos[i] = pos;
	}

	int nArmLength = MoveTowardsGoal();

	// FIXME: this isn't thread safe
	static SmartArray<PhysParticle> imp_particles_sa; // This doesn't specify alignment, might have problems with SSE

	while (imp_particles_sa.size < m_nActiveParticles)
	{
		imp_particles_sa.pushAutoSize(PhysParticle());
	}

	PhysTiler* m_pPhysTiler = PhysTilerFactory::factory->getTiler();
	m_pPhysTiler->setCacheParams(npc_surface_nearby.GetFloat(), 0.0f);//[EP3T]

	// centered and scaled?
	m_pPhysTiler->beginFrame(Point3D(0.0f, 0.0f, 0.0f));

	int nParticles = 0;

	// Move the spheres into particles
	float projection = 0.5;

	for (i = 0; i < m_nActiveParticles; i++)
	{
		if (m_flSurfaceR[i] > 0.0f)
		{
			PhysParticle* particle = &(imp_particles_sa[nParticles++]);

			Vector vecVel;
			m_pSpheres[i]->GetVelocity(&vecVel, NULL);
			Vector estPos = m_vecSurfacePos[i] + vecVel * projection;

			particle->center = estPos * (1.0 / m_flRadius);
			particle->group = 0;
			particle->neighbor_count = 0;
			particle->temp1 = i;
			m_pPhysTiler->insertParticle(particle);
		}
	}

	m_pPhysTiler->processTiles();

	m_pPhysTiler->beginIteration();

	PhysParticle** neigbours;

	int numNeighbours;

	PhysParticle* p1 = m_pPhysTiler->getNextParticleAndNeighbors(&neigbours, &numNeighbours);

	while (p1 != NULL)
	{
		i = p1->temp1;

		Vector estPos = p1->center.AsVector() * m_flRadius;
		Vector delta(0, 0, 0);

		// push against nearby spheres
		float flIdealDist2 = (flIdealDistance * flIdealDistance);
		float flNearbyDist2 = (flNearbyDistance * flNearbyDistance);
		bool bFloat = false;
		float flDist2;
		Vector dir;

		for (int i2 = 0; i2 < numNeighbours; i2++)
		{
			PhysParticle* b2 = neigbours[i2];
			if (b2 == p1)
			{
				continue;
			}
			j = b2->temp1;

			int bSameArm = (m_nArm[i] == m_nArm[j]) && (m_nArm[i] > -1);

			Vector estEffectorPos = b2->center.AsVector() * m_flRadius;
			flDist2 = (estPos - estEffectorPos).LengthSqr();

			flNearbyDist2 = (m_flSurfaceR[i] + m_flSurfaceR[j]) * 0.5 * flNearbyDistance;
			flNearbyDist2 = flNearbyDist2 * flNearbyDist2;

			if (!bSameArm && m_nTargetSlot[j] != 0)
			{
				if (flDist2 < flIdealDist2)
				{
					dir = (estPos - estEffectorPos);
					VectorNormalize(dir);
					delta += dir * MIN((flIdealDist2 - flDist2), 100);
				}
			}

			if (!bFloat && flDist2 < flNearbyDist2 && ((m_vecSurfacePos[j].z <= m_vecSurfacePos[i].z) || (bSameArm && ((j % nArmLength) < (i % nArmLength)) && m_bFloat[j])))
			{
				bFloat = true;
			}
		}

		m_pSpheres[i]->EnableGravity(!bFloat && !m_bContact[i]);
		m_bFloat[i] = bFloat;
		m_bContact[i] = false;

		// apply the force
		m_pSpheres[i]->ApplyForceCenter(delta);

		p1 = m_pPhysTiler->getNextParticleAndNeighbors(&neigbours, &numNeighbours);
	}

	m_pPhysTiler->endIteration();
	m_pPhysTiler->endFrame();

	PhysTilerFactory::factory->returnTiler(m_pPhysTiler);

	static float lastNetUpdate = 0.0f;

	float flNetDt = gpGlobals->curtime - lastNetUpdate;

	if (flNetDt > 0.1f)
	{
		if (npc_surface_debug.GetBool())
		{
		}

		NetworkProp()->NetworkStateForceUpdate();
		lastNetUpdate = gpGlobals->curtime;
	}

	if (npc_surface_debug.GetBool())
	{
		static float s_flLastThink = 0.0f;
		float dt = gpGlobals->curtime - s_flLastThink;

		if (dt > 0.05f)
		{
		}

		s_flLastThink = gpGlobals->curtime;
	}

	Vector vecVel;
	m_pSpheres[i]->GetVelocity(&vecVel, NULL);

	if (npc_surface_debug.GetBool() && m_bContact[i])
	{
	}

	if (npc_surface_debug.GetBool())
	{
		Vector pos, vel;
		m_pSpheres[0]->GetPosition(&pos, NULL);
		m_pSpheres[0]->GetVelocity(&vel, NULL);

	}

	static float s_flLastThink = 0.0f;

	if (s_flLastThink > 0 && gpGlobals->curtime - s_flLastThink > 0.5f)
	{
		if (npc_surface_debug.GetBool())
		{
		}
	}

	s_flLastThink = gpGlobals->curtime;

	SetNextThink(gpGlobals->curtime + SURFACE_THINK_INTERVAL);
}

int CNPC_BlobArmTest::MoveTowardsGoal(void)
{
	VPROF("CNPC_BlobArmTest::MoveTowardsGoal");

	int i;

	// alternate between arm mode and walk mode
	bool bDoArms = ((int)(gpGlobals->curtime / 17.0) % 2) == 1;

	if (bDoArms != m_bDoArms)
	{
		for (i = 0; i < m_nActiveParticles; i++)
		{
			m_nOwnedSlot[i] = 0;
			m_nTargetSlot[i] = 0;
		}
	}

	m_bDoArms = bDoArms;

	int nArmLength = 1;

	if (bDoArms)
	{
		nArmLength = 8;
	}
	else
	{
		m_flSimTime += 0.1;
	}
	Vector vecGoal(0, sin(m_flSimTime * 0.2) * 250, 0);

	vecGoal = vecGoal + m_vecStart;
	vecGoal.z -= npc_surface_depth_strength.GetFloat();

	float tension = npc_surface_tension.GetFloat();
	float projection = 0.5;

	// push spheres around to meet position targets
	for (i = 0; i < m_nActiveParticles; i++)
	{
		Vector vecVel;
		m_pSpheres[i]->GetVelocity(&vecVel, NULL);
		Vector estPos = m_vecSurfacePos[i] + vecVel * projection;
		Vector delta(0, 0, 0);
		float dist(0.0f);

		m_nArm[i] = (i / nArmLength);

		if (m_bFloat[i] || m_bContact[i] || fabs(vecVel.z) < 1.0)
		{
			int k = (i % nArmLength);
			int j = i - k;

			if (!bDoArms || i > m_nActiveParticles / 2)
			{
				m_nArm[i] = -1;
				delta = vecGoal - estPos;
				dist = VectorNormalize(delta);
				delta = delta * MIN(MAX(dist - m_flRadius * (bDoArms ? 8 : 10), 0), 500) * tension;
				m_nOwnedSlot[i] = 0;
				m_nTargetSlot[i] = 1;
				m_flSurfaceV[i] = Approach(0.0f, m_flSurfaceV[i], 0.2f);
			}
			else if (k == 0)
			{
				// move sphere towards center target
				delta = vecGoal - estPos;
				dist = VectorNormalize(delta);
				delta = delta * MIN(MAX(dist - m_flRadius * (bDoArms ? 8 : 10), 0), 500) * tension;
				m_nOwnedSlot[i] = 0;
				m_nTargetSlot[i] = 1;
				m_flSurfaceV[i] = Approach(0.0f, m_flSurfaceV[i], 0.2f);
			}
			else
			{
				Vector out = m_vecSurfacePos[j] - vecGoal;
				VectorNormalize(out);

				if (m_nTargetSlot[i] > m_nOwnedSlot[j] + 1)
				{
					m_nTargetSlot[i] = m_nOwnedSlot[j] + 1;
				}

				Vector target = m_vecSurfacePos[j];

				if (i != j)
				{
					Vector vecTargetVel;
					m_pSpheres[i - 1]->GetVelocity(&vecTargetVel, NULL);
					target = m_vecSurfacePos[i - 1] + vecTargetVel * 0.1 + out * m_flRadius * (m_flSurfaceR[i - 1] + m_flSurfaceR[i]);
				}

				delta = target - estPos;
				dist = VectorNormalize(delta);

				if (dist < m_flRadius * m_flSurfaceR[i])
				{
					if (m_nTargetSlot[i] == k)
					{
						m_nOwnedSlot[j] = MAX(m_nOwnedSlot[j], k);
					}
					else
					{
						m_nTargetSlot[i]++;
					}
				}
				else if (dist >= m_flRadius * m_flSurfaceR[i] * 2 && m_nTargetSlot[i] > 0)
				{
					m_nTargetSlot[i]--;
					if (m_nOwnedSlot[j] > m_nTargetSlot[i])
					{
						m_nOwnedSlot[j] = m_nTargetSlot[i];
					}
				}

				delta = delta * MIN(MAX(dist - 0, 0), 250) * m_pSpheres[i]->GetMass();

				m_flSurfaceV[i] = Approach((m_nTargetSlot[i]) / (float)nArmLength, m_flSurfaceV[i], 0.2f);
				m_flSurfaceV[i] = clamp(m_flSurfaceV[i], 0.0f, 1.0f);
			}
		}

		float a = npc_surface_scale.GetFloat();
		if (m_nTargetSlot[i] <= 1) a = 1.0;
		if (nArmLength > 1 && m_nTargetSlot[i] >= nArmLength - 1) a = (1.0 + npc_surface_scale.GetFloat()) * 0.5;
		float b = m_flSurfaceR[i];
		float c = Approach(a, b, 0.2f);
		m_flSurfaceR[i] = clamp(c, 0.0f, 1.0f);

		if (!m_bFloat[i])
		{
			delta.z = 0;
		}

		m_pSpheres[i]->ApplyForceCenter(delta);
	}

	return nArmLength;
}

LINK_ENTITY_TO_CLASS(npc_surface_fountain, CNPC_BlobFountain);

IMotionEvent::simresult_e CBlobFountainController::Simulate(IPhysicsMotionController* pController, IPhysicsObject* pObject, float deltaTime, Vector& linear, AngularImpulse& angular)
{
	if (CAI_BaseNPC::m_nDebugBits & bits_debugDisableAI)
	{
		pObject->EnableMotion(false);
		return IMotionEvent::SIM_NOTHING;
	}


	if (m_pOwner->m_bPause) return IMotionEvent::SIM_NOTHING;
	if (CAI_BaseNPC::m_nDebugBits & bits_debugDisableAI) return IMotionEvent::SIM_NOTHING;

	bool newPause = CAI_BaseNPC::m_nDebugBits & bits_debugDisableAI;

	if (newPause != m_pOwner->m_bPause)
	{
		if (newPause)
		{
			for (int i = 0; i < m_pOwner->m_nActiveParticles; i++)
			{
				m_pOwner->m_pSpheres[i]->EnableMotion(false);
			}
		}
		else
		{
			for (int i = 0; i < m_pOwner->m_nActiveParticles; i++)
			{
				m_pOwner->m_pSpheres[i]->EnableMotion(true);
			}
		}
		m_pOwner->m_bPause = newPause;
	}

	int nSphere = pObject->GetGameIndex();

	if (m_pOwner && m_flLennardJonesTime != gpGlobals->curtime)
	{
		m_pOwner->Simulate(pController, pObject, deltaTime, linear, angular);
		m_flLennardJonesTime = gpGlobals->curtime;
	}

	linear += m_vecLennardJonesForce[nSphere] * (1.0f / deltaTime);

	Vector pos, vecVel;
	QAngle ang;
	pObject->GetPosition(&pos, NULL);
	pObject->GetVelocity(&vecVel, NULL);
	float d = npc_surface_radius.GetFloat();
	Vector nozzle = m_pOwner->GetNozzle();
	Vector start = nozzle + Vector(0.0f, 0.0f, d);
	pos = pos - nozzle;
	float dist = pos.Length();

	if (m_pOwner->m_iMode[nSphere] == 0)
	{
		if (dist < 10.0f)
		{
			if (vecVel.z < 0.1f && abs(vecVel.x) < 0.05f && abs(vecVel.y) < 0.05f)
			{
				m_pOwner->m_iContactTime[nSphere] = 0;
				m_pOwner->m_fRadius[nSphere] = 0.5f;
				m_pOwner->m_iMode[nSphere] = 1;
				m_pOwner->m_bContact[nSphere] = false;
				linear.z += 10000 * (1.0f / deltaTime);
				linear.z = 500000.0f;
				linear = -vecVel / deltaTime;

				float v = 6000.0f / deltaTime;
				float azimuth = (2.0f * M_PI * (float(rand()) / float(RAND_MAX)));
				float polar = 0.04f * M_PI + (0.01f * (float(rand()) / float(RAND_MAX)));

				linear.x = v * cos(azimuth) * sin(polar);
				linear.y = v * sin(azimuth) * sin(polar);
				linear.z = v * cos(polar);
			}
		}
	}
	else if (m_pOwner->m_iMode[nSphere] == 1)
	{
		m_pOwner->m_fRadius[nSphere] = MIN(1.0f, m_pOwner->m_fRadius[nSphere] + 3.0f * deltaTime);
		if (m_pOwner->m_bContact[nSphere])
		{
			if (dist < 80.0f)
			{
				m_pOwner->m_fRadius[nSphere] = 0.0f;
				pObject->SetPosition(nozzle, QAngle(0.0f, 0.0f, 0.0f), true);
				Vector zero(0.0f, 0.0f, 0.0f);
				pObject->SetVelocity(&zero, NULL);
				m_pOwner->m_iMode[nSphere] = 0;
				m_pOwner->m_iContactTime[nSphere] = 0;
			}
			else
			{
				m_pOwner->m_fRadius[nSphere] = 0.0f;
				m_pOwner->m_iContactTime[nSphere] += deltaTime;
				if (m_pOwner->m_iContactTime[nSphere] >= 1.0f)
				{
					m_pOwner->m_iMode[nSphere] = 2;
					m_pOwner->m_iContactTime[nSphere] = 0;
					m_pOwner->m_bContact[nSphere] = false;
				}
			}
		}
	}
	else if (m_pOwner->m_iMode[nSphere] == 2)
	{
		pObject->SetPosition(nozzle, QAngle(0.0f, 0.0f, 0.0f), true);
		Vector zero(0.0f, 0.0f, 0.0f);
		pObject->SetVelocity(&zero, NULL);

		m_pOwner->m_iContactTime[nSphere] += deltaTime;
		if (m_pOwner->m_iContactTime[nSphere] >= 1.0f)
		{
			m_pOwner->m_iMode[nSphere] = 0;
			m_pOwner->m_iContactTime[nSphere] = 0;
		}
	}

	if (m_pOwner->m_bContact[nSphere])
	{
		m_pOwner->m_flSurfaceR[nSphere] = 0.0f;
		m_pOwner->m_iContactTime[nSphere]++;

		if (m_pOwner->m_iContactTime[nSphere] >= 10)
		{
			m_pOwner->m_bContact[nSphere] = false;

			if (dist > 10.0f)
			{
				pObject->SetPosition(nozzle, QAngle(0.0f, 0.0f, 0.0f), true);
				Vector vVelocity = Vector(RandomFloat(-1.0f, 1.0f), RandomFloat(-1.0f, 1.0f), RandomFloat(-1.0f, 1.0f)) * 0.2f;
				pObject->SetVelocity(&vVelocity, NULL);
			}
		}
	}
	else
	{
		if (dist < 10.0f)
		{
			if (vecVel.z < 0.1f && abs(vecVel.x) < 0.1f && abs(vecVel.y) < 0.1f)
			{
				m_pOwner->m_iContactTime[nSphere] = 0;
				m_pOwner->m_flSurfaceR[nSphere] = 1.0f;

				float v = 6000.0f / deltaTime;
				float azimuth = (2.0f * M_PI * (float(rand()) / float(RAND_MAX)));
				float polar = 0.03f * M_PI;

				linear.x = v * cos(azimuth) * sin(polar);
				linear.y = v * sin(azimuth) * sin(polar);
				linear.z = v * cos(polar);
			}
		}
	}

	return IMotionEvent::SIM_GLOBAL_FORCE;
}

void CNPC_BlobFountain::Simulate(IPhysicsMotionController* pController, IPhysicsObject* pObject, float deltaTime, Vector& linear, AngularImpulse& angular)
{
	m_force.AddForces(m_pSpheres, m_nActiveParticles, m_flRadius, npc_lj_strength.GetFloat(), m_pBlobFountainController->m_vecLennardJonesForce);
}

bool CNPC_BlobFountain::CreateVPhysics(bool bFromRestore)
{
	objectparams_t params = g_PhysDefaultObjectParams;
	params.pGameData = static_cast<void*>(this);

	int nMaterialIndex = physprops->GetSurfaceIndex("water");

	// FIXME: don't hardcode the number of particles
	m_nActiveParticles = 500;

	m_pBlobFountainController = new CBlobFountainController(this);
	m_pMotionController = physenv->CreateMotionController(m_pBlobFountainController);

	trace_t	tr;
	UTIL_TraceLine(m_vecStart + Vector(0, 0, 1), m_vecStart - Vector(0, 0, 64), MASK_SOLID_BRUSHONLY | CONTENTS_PLAYERCLIP | CONTENTS_MONSTERCLIP, this, COLLISION_GROUP_NONE, &tr);
	m_vecStart = tr.endpos;

	int i;
	for (i = 0; i < m_nActiveParticles; i++)
	{
		m_vecSurfacePos[i] = m_vecStart + Vector(0, 0, 10.0f);
		m_pSpheres[i] = physenv->CreateSphereObject(m_flRadius, nMaterialIndex, m_vecSurfacePos[i], GetAbsAngles(), &params, false);

		if (m_pSpheres[i])
		{
			Vector vVelocity = Vector(RandomFloat(-1, 1), RandomFloat(-1, 1), RandomFloat(-1, 1)) * 10.0f;
			m_pSpheres[i]->SetVelocity(&vVelocity, NULL);
			PhysSetGameFlags(m_pSpheres[i], FVPHYSICS_NO_SELF_COLLISIONS | FVPHYSICS_MULTIOBJECT_ENTITY); // call collisionruleschanged if this changes dynamically
			m_pSpheres[i]->SetGameIndex(i);

			m_pSpheres[i]->SetMass(10.0f);
			m_pSpheres[i]->EnableGravity(true);
			m_pSpheres[i]->EnableDrag(true);

			float flDamping = 0.5f;
			float flAngDamping = 0.5f;
			m_pSpheres[i]->SetDamping(&flDamping, &flAngDamping);
			m_pSpheres[i]->EnableGravity(true);
			m_pMotionController->AttachObject(m_pSpheres[i], true);
			m_iContactTime[i] = 0;
			m_iMode[i] = 0;
		}
	}

	return true;
}

void CNPC_BlobFountain::RunAI(void)
{
	Vector vel;
	m_pSpheres[0]->GetVelocity(&vel, NULL);

	float dt = gpGlobals->frametime;

	Vector newPos = GetLocalOrigin() + vel * dt;
	SetLocalOrigin(newPos);

	m_pMotionController->WakeObjects();

	for (int i = 0; i < m_nActiveParticles; i++)
	{
		m_pSpheres[i]->EnableMotion(true);
		{
			Vector pos;
			m_pSpheres[i]->GetPosition(&pos, NULL);
			m_vecSurfacePos[i] = pos;
			m_flSurfaceR[i] = m_fRadius[i];
		}
	}

	static float lastNetUpdate = 0.0f;

	float flNetDt = gpGlobals->curtime - lastNetUpdate;

	if (flNetDt > 0.1f)
	{
		if (npc_surface_debug.GetBool())
		{
		}

		NetworkProp()->NetworkStateForceUpdate();
		lastNetUpdate = gpGlobals->curtime;
	}

	if (npc_surface_debug.GetBool())
	{
		static float s_flLastThink = 0.0f;
		float dt = gpGlobals->curtime - s_flLastThink;

		if (dt > 0.05f)
		{
		}

		s_flLastThink = gpGlobals->curtime;
	}

	if (npc_surface_debug.GetBool())
	{
		Vector pos, vel;
		m_pSpheres[0]->GetPosition(&pos, NULL);
		m_pSpheres[0]->GetVelocity(&vel, NULL);

	}

	static float s_flLastThink = 0.0f;

	if (s_flLastThink > 0 && gpGlobals->curtime - s_flLastThink > 0.5f)
	{
		if (npc_surface_debug.GetBool())
		{
		}
	}

	s_flLastThink = gpGlobals->curtime;

	SetNextThink(gpGlobals->curtime + SURFACE_THINK_INTERVAL);
}
#endif // USE_BLOBULATOR