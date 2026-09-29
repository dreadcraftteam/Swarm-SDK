//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
//=============================================================================//

#include "cbase.h"
#include "player_pickup.h"
#include "player.h"
#include "physics.h"
#include "vphysics/friction.h"
#include "soundenvelope.h"
#include "engine/IEngineSound.h"
#include "te_effect_dispatch.h"
#include "movevars_shared.h"
#include "props.h"
#include "in_buttons.h"

#include "tier0/memdbgon.h"

static void MatrixOrthogonalize(matrix3x4_t& matrix, int column)
{
	Vector columns[3];
	int i;

	for (i = 0; i < 3; i++)
	{
		MatrixGetColumn(matrix, i, columns[i]);
	}

	int index0 = column;
	int index1 = (column + 1) % 3;
	int index2 = (column + 2) % 3;

	columns[index2] = CrossProduct(columns[index0], columns[index1]);
	columns[index1] = CrossProduct(columns[index2], columns[index0]);
	VectorNormalize(columns[index2]);
	VectorNormalize(columns[index1]);
	MatrixSetColumn(columns[index1], index1, matrix);
	MatrixSetColumn(columns[index2], index2, matrix);
}

#define SIGN(x) ( (x) < 0 ? -1 : 1 )

static QAngle AlignAngles(const QAngle& angles, float cosineAlignAngle)
{
	matrix3x4_t alignMatrix;
	AngleMatrix(angles, alignMatrix);

	// NOTE: Must align z first
	for (int j = 3; --j >= 0; )
	{
		Vector vec;
		MatrixGetColumn(alignMatrix, j, vec);
		for (int i = 0; i < 3; i++)
		{
			if (fabs(vec[i]) > cosineAlignAngle)
			{
				vec[i] = SIGN(vec[i]);
				vec[(i + 1) % 3] = 0;
				vec[(i + 2) % 3] = 0;
				MatrixSetColumn(vec, j, alignMatrix);
				MatrixOrthogonalize(alignMatrix, j);
				break;
			}
		}
	}

	QAngle out;
	MatrixAngles(alignMatrix, out);
	return out;
}

#define PLAYER_HOLD_LEVEL_EYES	-8
#define PLAYER_HOLD_DOWN_FEET	2
#define PLAYER_HOLD_UP_EYES		24
#define PLAYER_LOOK_PITCH_RANGE	30
#define PLAYER_REACH_DOWN_DISTANCE	24

static void ComputePlayerMatrix(CBasePlayer* pPlayer, matrix3x4_t& out)
{
	if (!pPlayer)
		return;

	QAngle angles = pPlayer->EyeAngles();
	Vector origin = pPlayer->EyePosition();

	angles.x = 0;

	float feet = pPlayer->GetAbsOrigin().z + pPlayer->WorldAlignMins().z;
	float eyes = origin.z;
	float zoffset = 0;

	// moving up (negative pitch is up)
	if (angles.x < 0)
	{
		zoffset = RemapVal(angles.x, 0, -PLAYER_LOOK_PITCH_RANGE, PLAYER_HOLD_LEVEL_EYES, PLAYER_HOLD_UP_EYES);
	}
	else
	{
		zoffset = RemapVal(angles.x, 0, PLAYER_LOOK_PITCH_RANGE, PLAYER_HOLD_LEVEL_EYES, PLAYER_HOLD_DOWN_FEET + (feet - eyes));
	}
	origin.z += zoffset;
	angles.x = 0;
	AngleMatrix(angles, origin, out);
}

const float DEFAULT_MAX_ANGULAR = 360.0f * 10.0f;
const float REDUCED_CARRY_MASS = 1.0f;

CGrabController::CGrabController(void)
{
	m_shadow.dampFactor = 1.0;
	m_shadow.teleportDistance = 0;
	m_errorTime = 0;
	m_error = 0;
	m_shadow.maxSpeed = 1000;
	m_shadow.maxAngular = DEFAULT_MAX_ANGULAR;
	m_shadow.maxDampSpeed = m_shadow.maxSpeed * 2;
	m_shadow.maxDampAngular = m_shadow.maxAngular;
	m_attachedEntity = NULL;
	m_vecPreferredCarryAngles = vec3_angle;
	m_bHasPreferredCarryAngles = false;
	m_angleAlignment = 0;
	m_bIgnoreRelativePitch = false;
	m_controller = NULL;
	m_flLoadWeight = 0;
	m_frameCount = 0;
	m_bCarriedEntityBlocksLOS = false;
	m_timeToArrive = 0;
	m_contactAmount = 0;
}

CGrabController::~CGrabController(void)
{
	DetachEntity(false);
}

void CGrabController::OnRestore()
{
	if (m_controller)
	{
		m_controller->SetEventHandler(this);
	}
}

void CGrabController::SetTargetPosition(const Vector& target, const QAngle& targetOrientation)
{
	m_shadow.targetPosition = target;
	m_shadow.targetRotation = targetOrientation;

	m_timeToArrive = gpGlobals->frametime;

	CBaseEntity* pAttached = GetAttached();
	if (pAttached)
	{
		IPhysicsObject* pObj = pAttached->VPhysicsGetObject();

		if (pObj != NULL)
		{
			pObj->Wake();
		}
		else
		{
			DetachEntity(false);
		}
	}
}

float CGrabController::ComputeError()
{
	if (m_errorTime <= 0)
		return 0;

	CBaseEntity* pAttached = GetAttached();
	if (pAttached)
	{
		Vector pos;
		IPhysicsObject* pObj = pAttached->VPhysicsGetObject();

		if (pObj)
		{
			pObj->GetShadowPosition(&pos, NULL);

			float error = (m_shadow.targetPosition - pos).Length();
			if (m_errorTime > 0)
			{
				if (m_errorTime > 1)
				{
					m_errorTime = 1;
				}
				float speed = error / m_errorTime;
				if (speed > m_shadow.maxSpeed)
				{
					error *= 0.5;
				}
				m_error = (1 - m_errorTime) * m_error + error * m_errorTime;
			}
		}
		else
		{
			DevMsg("Object attached to GrabController has no physics object\n");
			DetachEntity(false);
			return 9999;
		}
	}

	if (pAttached->IsEFlagSet(EFL_IS_BEING_LIFTED_BY_BARNACLE))
	{
		m_error *= 3.0f;
	}

	m_errorTime = 0;

	return m_error;
}

#define MASS_SPEED_SCALE	60
#define MAX_MASS			40

void CGrabController::ComputeMaxSpeed(CBaseEntity* pEntity, IPhysicsObject* pPhysics)
{
	m_shadow.maxSpeed = 1000;
	m_shadow.maxAngular = DEFAULT_MAX_ANGULAR;

	float flMass = PhysGetEntityMass(pEntity);
	float flMaxMass = 250.0f;
	if (flMass <= flMaxMass)
		return;

	float flLerpFactor = clamp(flMass, flMaxMass, 500.0f);
	flLerpFactor = SimpleSplineRemapVal(flLerpFactor, flMaxMass, 500.0f, 0.0f, 1.0f);

	float invMass = pPhysics->GetInvMass();
	float invInertia = pPhysics->GetInvInertia().Length();

	float invMaxMass = 1.0f / MAX_MASS;
	float ratio = invMaxMass / invMass;
	invMass = invMaxMass;
	invInertia *= ratio;

	float maxSpeed = invMass * MASS_SPEED_SCALE * 200;
	float maxAngular = invInertia * MASS_SPEED_SCALE * 360;

	m_shadow.maxSpeed = Lerp(flLerpFactor, m_shadow.maxSpeed, maxSpeed);
	m_shadow.maxAngular = Lerp(flLerpFactor, m_shadow.maxAngular, maxAngular);
}

QAngle CGrabController::TransformAnglesToPlayerSpace(const QAngle& anglesIn, CBasePlayer* pPlayer)
{
	if (m_bIgnoreRelativePitch)
	{
		matrix3x4_t test;
		QAngle angleTest = pPlayer->EyeAngles();
		angleTest.x = 0;
		AngleMatrix(angleTest, test);
		return TransformAnglesToLocalSpace(anglesIn, test);
	}
	return TransformAnglesToLocalSpace(anglesIn, pPlayer->EntityToWorldTransform());
}

QAngle CGrabController::TransformAnglesFromPlayerSpace(const QAngle& anglesIn, CBasePlayer* pPlayer)
{
	if (m_bIgnoreRelativePitch)
	{
		matrix3x4_t test;
		QAngle angleTest = pPlayer->EyeAngles();
		angleTest.x = 0;
		AngleMatrix(angleTest, test);
		return TransformAnglesToWorldSpace(anglesIn, test);
	}
	return TransformAnglesToWorldSpace(anglesIn, pPlayer->EntityToWorldTransform());
}

void CGrabController::AttachEntity(CBasePlayer* pPlayer, CBaseEntity* pEntity, IPhysicsObject* pPhys, bool bIsMegaPhysCannon, const Vector& vGrabPosition, bool bUseGrabPosition)
{
	Vector position;
	QAngle angles;
	pPhys->GetPosition(&position, &angles);

	// If it has a preferred orientation, use that instead.
	Pickup_GetPreferredCarryAngles(pEntity, pPlayer, pPlayer->EntityToWorldTransform(), angles);

	m_bCarriedEntityBlocksLOS = pEntity->BlocksLOS();
	pEntity->SetBlocksLOS(false);
	m_controller = physenv->CreateMotionController(this);
	m_controller->AttachObject(pPhys, true);

	pPhys->Wake();
	PhysSetGameFlags(pPhys, FVPHYSICS_PLAYER_HELD);
	SetTargetPosition(position, angles);
	m_attachedEntity = pEntity;
	IPhysicsObject* pList[VPHYSICS_MAX_OBJECT_LIST_COUNT];
	int count = pEntity->VPhysicsGetObjectList(pList, ARRAYSIZE(pList));
	m_flLoadWeight = 0;
	float damping = 10;
	float flFactor = count / 7.5f;
	if (flFactor < 1.0f)
	{
		flFactor = 1.0f;
	}
	for (int i = 0; i < count; i++)
	{
		float mass = pList[i]->GetMass();
		pList[i]->GetDamping(NULL, &m_savedRotDamping[i]);
		m_flLoadWeight += mass;
		m_savedMass[i] = mass;

		pList[i]->SetMass(REDUCED_CARRY_MASS / flFactor);
		pList[i]->SetDamping(NULL, &damping);
	}

	pPhys->SetMass(REDUCED_CARRY_MASS);
	pPhys->EnableDrag(false);

	m_errorTime = -1.0f;
	m_error = 0;
	m_contactAmount = 0;

	m_attachedAnglesPlayerSpace = TransformAnglesToPlayerSpace(angles, pPlayer);
	if (m_angleAlignment != 0)
	{
		m_attachedAnglesPlayerSpace = AlignAngles(m_attachedAnglesPlayerSpace, m_angleAlignment);
	}

	VectorITransform(pEntity->WorldSpaceCenter(), pEntity->EntityToWorldTransform(), m_attachedPositionObjectSpace);

	CPhysicsProp* pProp = dynamic_cast<CPhysicsProp*>(pEntity);
	if (pProp)
	{
		m_bHasPreferredCarryAngles = pProp->GetPropDataAngles("preferred_carryangles", m_vecPreferredCarryAngles);
	}
	else
	{
		m_bHasPreferredCarryAngles = false;
	}
}

static void ClampPhysicsVelocity(IPhysicsObject* pPhys, float linearLimit, float angularLimit)
{
	Vector vel;
	AngularImpulse angVel;
	pPhys->GetVelocity(&vel, &angVel);
	float speed = VectorNormalize(vel) - linearLimit;
	float angSpeed = VectorNormalize(angVel) - angularLimit;
	speed = speed < 0 ? 0 : -speed;
	angSpeed = angSpeed < 0 ? 0 : -angSpeed;
	vel *= speed;
	angVel *= angSpeed;
	pPhys->AddVelocity(&vel, &angVel);
}

void CGrabController::DetachEntity(bool bClearVelocity)
{
	CBaseEntity* pEntity = GetAttached();
	if (pEntity)
	{
		pEntity->SetBlocksLOS(m_bCarriedEntityBlocksLOS);
		IPhysicsObject* pList[VPHYSICS_MAX_OBJECT_LIST_COUNT];
		int count = pEntity->VPhysicsGetObjectList(pList, ARRAYSIZE(pList));

		for (int i = 0; i < count; i++)
		{
			IPhysicsObject* pPhys = pList[i];
			if (!pPhys)
				continue;

			pPhys->EnableDrag(true);
			pPhys->Wake();
			pPhys->SetMass(m_savedMass[i]);
			pPhys->SetDamping(NULL, &m_savedRotDamping[i]);
			PhysClearGameFlags(pPhys, FVPHYSICS_PLAYER_HELD);
			if (bClearVelocity)
			{
				PhysForceClearVelocity(pPhys);
			}
			else
			{
				ClampPhysicsVelocity(pPhys, 190.0f * 1.5f, 2.0f * 360.0f);
			}
		}
	}

	m_attachedEntity = NULL;
	if (physenv && m_controller)
	{
		physenv->DestroyMotionController(m_controller);
	}
	m_controller = NULL;
}

static bool InContactWithHeavyObject(IPhysicsObject* pObject, float heavyMass)
{
	bool contact = false;
	IPhysicsFrictionSnapshot* pSnapshot = pObject->CreateFrictionSnapshot();
	while (pSnapshot->IsValid())
	{
		IPhysicsObject* pOther = pSnapshot->GetObject(1);
		if (!pOther->IsMoveable() || pOther->GetMass() > heavyMass)
		{
			contact = true;
			break;
		}
		pSnapshot->NextFrictionData();
	}
	pObject->DestroyFrictionSnapshot(pSnapshot);
	return contact;
}

IMotionEvent::simresult_e CGrabController::Simulate(IPhysicsMotionController* pController, IPhysicsObject* pObject, float deltaTime, Vector& linear, AngularImpulse& angular)
{
	hlshadowcontrol_params_t shadowParams = m_shadow;
	if (InContactWithHeavyObject(pObject, GetLoadWeight()))
	{
		m_contactAmount = Approach(0.1f, m_contactAmount, deltaTime * 2.0f);
	}
	else
	{
		m_contactAmount = Approach(1.0f, m_contactAmount, deltaTime * 2.0f);
	}
	shadowParams.maxAngular = m_shadow.maxAngular * m_contactAmount * m_contactAmount * m_contactAmount;
	m_timeToArrive = pObject->ComputeShadowControl(shadowParams, m_timeToArrive, deltaTime);

	Vector velocity;
	AngularImpulse angVel;
	pObject->GetVelocity(&velocity, &angVel);
	PhysComputeSlideDirection(pObject, velocity, angVel, &velocity, &angVel, GetLoadWeight());
	pObject->SetVelocityInstantaneous(&velocity, NULL);

	linear.Init();
	angular.Init();
	m_errorTime += deltaTime;

	return SIM_LOCAL_ACCELERATION;
}

float CGrabController::GetSavedMass(IPhysicsObject* pObject)
{
	CBaseEntity* pHeld = m_attachedEntity;
	if (pHeld)
	{
		if (pObject->GetGameData() == (void*)pHeld)
		{
			IPhysicsObject* pList[VPHYSICS_MAX_OBJECT_LIST_COUNT];
			int count = pHeld->VPhysicsGetObjectList(pList, ARRAYSIZE(pList));
			for (int i = 0; i < count; i++)
			{
				if (pList[i] == pObject)
					return m_savedMass[i];
			}
		}
	}
	return 0.0f;
}

bool CGrabController::UpdateObject(CBasePlayer* pPlayer, float flError)
{
	CBaseEntity* pEntity = GetAttached();
	if (!pEntity)
		return false;
	if (ComputeError() > flError)
		return false;
	if (pPlayer->GetGroundEntity() == pEntity)
		return false;
	if (!pEntity->VPhysicsGetObject())
		return false;

	IPhysicsObject* pPhys = pEntity->VPhysicsGetObject();
	if (pPhys && pPhys->IsMoveable() == false)
	{
		return false;
	}

	if (m_frameCount == gpGlobals->framecount)
	{
		return true;
	}
	m_frameCount = gpGlobals->framecount;

	Vector forward, right, up;
	QAngle playerAngles = pPlayer->EyeAngles();

	float pitch = AngleDistance(playerAngles.x, 0);
	playerAngles.x = clamp(pitch, -75, 75);
	AngleVectors(playerAngles, &forward, &right, &up);

	Vector radial = physcollision->CollideGetExtent(pPhys->GetCollide(), vec3_origin, pEntity->GetAbsAngles(), -forward);
	Vector player2d = pPlayer->CollisionProp()->OBBMaxs();
	float playerRadius = player2d.Length2D();
	float flDot = DotProduct(forward, radial);

	float radius = playerRadius + fabs(flDot);

	float distance = 24 + (radius * 2.0f);

	Vector start = pPlayer->Weapon_ShootPosition();
	Vector end = start + (forward * distance);

	trace_t	tr;
	CTraceFilterSkipTwoEntities traceFilter(pPlayer, pEntity, COLLISION_GROUP_NONE);
	Ray_t ray;
	ray.Init(start, end);
	enginetrace->TraceRay(ray, MASK_SOLID_BRUSHONLY, &traceFilter, &tr);

	if (tr.fraction < 0.5)
	{
		end = start + forward * (radius * 0.5f);
	}
	else if (tr.fraction <= 1.0f)
	{
		end = start + forward * (distance - radius);
	}

	Vector playerMins, playerMaxs, nearest;
	pPlayer->CollisionProp()->WorldSpaceAABB(&playerMins, &playerMaxs);
	Vector playerLine = pPlayer->CollisionProp()->WorldSpaceCenter();
	CalcClosestPointOnLine(end, playerLine + Vector(0, 0, playerMins.z), playerLine + Vector(0, 0, playerMaxs.z), nearest, NULL);

	Vector delta = end - nearest;
	float len = VectorNormalize(delta);
	if (len < radius)
	{
		end = nearest + radius * delta;
	}

	QAngle angles = TransformAnglesFromPlayerSpace(m_attachedAnglesPlayerSpace, pPlayer);

	Pickup_GetPreferredCarryAngles(pEntity, pPlayer, pPlayer->EntityToWorldTransform(), angles);

	if (m_bHasPreferredCarryAngles)
	{
		matrix3x4_t tmp;
		ComputePlayerMatrix(pPlayer, tmp);
		angles = TransformAnglesToWorldSpace(m_vecPreferredCarryAngles, tmp);
	}

	matrix3x4_t attachedToWorld;
	Vector offset;
	AngleMatrix(angles, attachedToWorld);
	VectorRotate(m_attachedPositionObjectSpace, attachedToWorld, offset);

	SetTargetPosition(end - offset, angles);

	return true;
}

class CPlayerPickupController : public CBaseEntity
{
	DECLARE_CLASS(CPlayerPickupController, CBaseEntity);
public:
	void Init(CBasePlayer* pPlayer, CBaseEntity* pObject);
	void Shutdown(bool bThrown = false);
	bool OnControls(CBaseEntity* pControls) { return true; }
	void Use(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value);
	void OnRestore()
	{
		m_grabController.OnRestore();
		BaseClass::OnRestore();
	}
	void VPhysicsUpdate(IPhysicsObject* pPhysics) {}
	void VPhysicsShadowUpdate(IPhysicsObject* pPhysics) {}

	bool IsHoldingEntity(CBaseEntity* pEnt);
	CGrabController& GetGrabController() { return m_grabController; }

private:
	CGrabController		m_grabController;
	CBasePlayer* m_pPlayer;
};

LINK_ENTITY_TO_CLASS(player_pickup, CPlayerPickupController);

void CPlayerPickupController::Init(CBasePlayer* pPlayer, CBaseEntity* pObject)
{
	CBaseViewModel* pViewModel = pPlayer->GetViewModel();
	if (pViewModel)
	{
		pViewModel->AddEffects(EF_NODRAW);
	}

	pPlayer->m_Local.m_iHideHUD |= HIDEHUD_WEAPONSELECTION;

	if (pObject->GetCollisionGroup() == COLLISION_GROUP_DEBRIS)
	{
		pObject->SetCollisionGroup(COLLISION_GROUP_INTERACTIVE_DEBRIS);
	}

	SetParent(pPlayer);
	m_grabController.SetIgnorePitch(true);
	m_grabController.SetAngleAlignment(DOT_30DEGREE);
	m_pPlayer = pPlayer;
	IPhysicsObject* pPhysics = pObject->VPhysicsGetObject();
	Pickup_OnPhysGunPickup(pObject, m_pPlayer);

	m_grabController.AttachEntity(pPlayer, pObject, pPhysics, false, vec3_origin, false);

	m_pPlayer->SetUseEntity(this);
}

void CPlayerPickupController::Shutdown(bool bThrown)
{
	CBaseEntity* pObject = m_grabController.GetAttached();

	bool bClearVelocity = false;
	if (!bThrown && pObject && pObject->VPhysicsGetObject() && pObject->VPhysicsGetObject()->GetContactPoint(NULL, NULL))
	{
		bClearVelocity = true;
	}

	m_grabController.DetachEntity(bClearVelocity);

	if (pObject != NULL)
	{
		Pickup_OnPhysGunDrop(pObject, m_pPlayer, bThrown ? THROWN_BY_PLAYER : DROPPED_BY_PLAYER);
	}

	if (m_pPlayer)
	{
		m_pPlayer->SetUseEntity(NULL);
		if (m_pPlayer->GetActiveWeapon())
		{
			if (!m_pPlayer->GetActiveWeapon()->Deploy())
			{
				m_pPlayer->SwitchToNextBestWeapon(NULL);
			}
		}

		m_pPlayer->m_Local.m_iHideHUD &= ~HIDEHUD_WEAPONSELECTION;
	}

	Remove();
}

void CPlayerPickupController::Use(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value)
{
	if (ToBasePlayer(pActivator) == m_pPlayer)
	{
		CBaseEntity* pAttached = m_grabController.GetAttached();

		if (!pAttached || useType == USE_OFF || (m_pPlayer->m_nButtons & IN_ATTACK2) || m_grabController.ComputeError() > 12)
		{
			Shutdown();
			return;
		}

		IPhysicsObject* pPhys = pAttached->VPhysicsGetObject();
		if (pPhys && pPhys->IsMoveable() == false)
		{
			Shutdown();
			return;
		}

		if (m_pPlayer->m_nButtons & IN_ATTACK)
		{
			Shutdown(true);
			Vector vecLaunch;
			m_pPlayer->EyeVectors(&vecLaunch);
			float massFactor = clamp(pPhys->GetMass(), 0.5, 15);
			massFactor = RemapVal(massFactor, 0.5, 15, 0.5, 4);
			vecLaunch *= 1000.0f * massFactor;

			pPhys->ApplyForceCenter(vecLaunch);
			AngularImpulse aVel = RandomAngularImpulse(-10, 10) * massFactor;
			pPhys->ApplyTorqueCenter(aVel);
			return;
		}

		if (useType == USE_SET)
		{
			m_grabController.UpdateObject(m_pPlayer, 12);
		}
	}
}

bool CPlayerPickupController::IsHoldingEntity(CBaseEntity* pEnt)
{
	return (m_grabController.GetAttached() == pEnt);
}

void PlayerPickupObject(CBasePlayer* pPlayer, CBaseEntity* pObject)
{
	if (!pObject || !pPlayer)
		return;

	if (pObject->VPhysicsGetObject() == NULL)
		return;

	CPlayerPickupController* pController = (CPlayerPickupController*)CBaseEntity::Create("player_pickup", pObject->GetAbsOrigin(), vec3_angle, pPlayer);

	if (!pController)
		return;

	pController->Init(pPlayer, pObject);
}

void Pickup_ForcePlayerToDropThisObject(CBaseEntity* pTarget)
{
	if (pTarget == NULL)
		return;

	IPhysicsObject* pPhysics = pTarget->VPhysicsGetObject();

	if (pPhysics == NULL)
		return;

	if (pPhysics->GetGameFlags() & FVPHYSICS_PLAYER_HELD)
	{
		CBasePlayer* pPlayer = UTIL_GetLocalPlayer();
		if (pPlayer)
			pPlayer->ForceDropOfCarriedPhysObjects(pTarget);
	}
}

void Pickup_OnPhysGunDrop(CBaseEntity* pDroppedObject, CBasePlayer* pPlayer, PhysGunDrop_t Reason)
{
	IPlayerPickupVPhysics* pPickup = dynamic_cast<IPlayerPickupVPhysics*>(pDroppedObject);
	if (pPickup)
	{
		pPickup->OnPhysGunDrop(pPlayer, Reason);
	}
}

void Pickup_OnPhysGunPickup(CBaseEntity* pPickedUpObject, CBasePlayer* pPlayer, PhysGunPickup_t reason)
{
	IPlayerPickupVPhysics* pPickup = dynamic_cast<IPlayerPickupVPhysics*>(pPickedUpObject);
	if (pPickup)
	{
		pPickup->OnPhysGunPickup(pPlayer, reason);
	}
}

bool Pickup_OnAttemptPhysGunPickup(CBaseEntity* pPickedUpObject, CBasePlayer* pPlayer, PhysGunPickup_t reason)
{
	IPlayerPickupVPhysics* pPickup = dynamic_cast<IPlayerPickupVPhysics*>(pPickedUpObject);
	if (pPickup)
	{
		return pPickup->OnAttemptPhysGunPickup(pPlayer, reason);
	}
	return true;
}

CBaseEntity* Pickup_OnFailedPhysGunPickup(CBaseEntity* pPickedUpObject, Vector vPhysgunPos)
{
	IPlayerPickupVPhysics* pPickup = dynamic_cast<IPlayerPickupVPhysics*>(pPickedUpObject);
	if (pPickup)
	{
		return pPickup->OnFailedPhysGunPickup(vPhysgunPos);
	}
	return NULL;
}

bool Pickup_GetPreferredCarryAngles(CBaseEntity* pObject, CBasePlayer* pPlayer, matrix3x4_t& localToWorld, QAngle& outputAnglesWorldSpace)
{
	IPlayerPickupVPhysics* pPickup = dynamic_cast<IPlayerPickupVPhysics*>(pObject);
	if (pPickup)
	{
		if (pPickup->HasPreferredCarryAnglesForPlayer(pPlayer))
		{
			outputAnglesWorldSpace = TransformAnglesToWorldSpace(pPickup->PreferredCarryAngles(), localToWorld);
			return true;
		}
	}
	return false;
}

bool Pickup_ForcePhysGunOpen(CBaseEntity* pObject, CBasePlayer* pPlayer)
{
	IPlayerPickupVPhysics* pPickup = dynamic_cast<IPlayerPickupVPhysics*>(pObject);
	if (pPickup)
	{
		return pPickup->ForcePhysgunOpen(pPlayer);
	}
	return false;
}

AngularImpulse Pickup_PhysGunLaunchAngularImpulse(CBaseEntity* pObject, PhysGunForce_t reason)
{
	IPlayerPickupVPhysics* pPickup = dynamic_cast<IPlayerPickupVPhysics*>(pObject);
	if (pPickup != NULL && pPickup->ShouldPuntUseLaunchForces(reason))
	{
		return pPickup->PhysGunLaunchAngularImpulse();
	}
	return RandomAngularImpulse(-600, 600);
}

Vector Pickup_DefaultPhysGunLaunchVelocity(const Vector& vecForward, float flMass)
{
	return (vecForward * flMass);
}

Vector Pickup_PhysGunLaunchVelocity(CBaseEntity* pObject, const Vector& vecForward, PhysGunForce_t reason)
{
	if (pObject == NULL)
	{
		Assert(0);
		return vec3_origin;
	}

	IPhysicsObject* pPhysicsObject = pObject->VPhysicsGetObject();
	if (pPhysicsObject == NULL)
	{
		Assert(0);
		return vec3_origin;
	}

	IPlayerPickupVPhysics* pPickup = dynamic_cast<IPlayerPickupVPhysics*>(pObject);
	if (pPickup != NULL && pPickup->ShouldPuntUseLaunchForces(reason))
		return pPickup->PhysGunLaunchVelocity(vecForward, pPhysicsObject->GetMass());

	return Pickup_DefaultPhysGunLaunchVelocity(vecForward, pPhysicsObject->GetMass());
}

bool Pickup_ShouldPuntUseLaunchForces(CBaseEntity* pObject, PhysGunForce_t reason)
{
	IPlayerPickupVPhysics* pPickup = dynamic_cast<IPlayerPickupVPhysics*>(pObject);
	if (pPickup)
	{
		return pPickup->ShouldPuntUseLaunchForces(reason);
	}
	return false;
}

bool PlayerPickupControllerIsHoldingEntity(CBaseEntity* pPickupControllerEntity, CBaseEntity* pHeldEntity)
{
	CPlayerPickupController* pController = dynamic_cast<CPlayerPickupController*>(pPickupControllerEntity);

	return pController ? pController->IsHoldingEntity(pHeldEntity) : false;
}

float PlayerPickupGetHeldObjectMass(CBaseEntity* pPickupControllerEntity, IPhysicsObject* pHeldObject)
{
	float mass = 0.0f;
	CPlayerPickupController* pController = dynamic_cast<CPlayerPickupController*>(pPickupControllerEntity);
	if (pController)
	{
		CGrabController& grab = pController->GetGrabController();
		mass = grab.GetSavedMass(pHeldObject);
	}
	return mass;
}