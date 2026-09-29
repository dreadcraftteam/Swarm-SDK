//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: APIs for player pickup of physics objects
//
//=============================================================================//

#ifndef PLAYER_PICKUP_H
#define PLAYER_PICKUP_H
#ifdef _WIN32
#pragma once
#endif

#include "physics.h"

class CBasePlayer;
class CBaseEntity;
class IPhysicsObject;
class IPhysicsMotionController;
class IPhysicsFrictionSnapshot;

// Reasons behind a pickup
enum PhysGunPickup_t
{
	PICKED_UP_BY_CANNON,
	PUNTED_BY_CANNON,
	PICKED_UP_BY_PLAYER, // Picked up by +USE, not physgun.
};

// Reasons behind a drop
enum PhysGunDrop_t
{
	DROPPED_BY_PLAYER,
	THROWN_BY_PLAYER,
	DROPPED_BY_CANNON,
	LAUNCHED_BY_CANNON,
};

enum PhysGunForce_t
{
	PHYSGUN_FORCE_DROPPED,	// Dropped by +USE
	PHYSGUN_FORCE_THROWN,	// Thrown from +USE
	PHYSGUN_FORCE_PUNTED,	// Punted by cannon
	PHYSGUN_FORCE_LAUNCHED,	// Launched by cannon
};

abstract_class IPlayerPickupVPhysics
{
public:
	virtual bool			OnAttemptPhysGunPickup(CBasePlayer * pPhysGunUser, PhysGunPickup_t reason = PICKED_UP_BY_CANNON) = 0;
	virtual CBaseEntity* OnFailedPhysGunPickup(Vector vPhysgunPos) = 0;
	virtual void			OnPhysGunPickup(CBasePlayer* pPhysGunUser, PhysGunPickup_t reason = PICKED_UP_BY_CANNON) = 0;
	virtual void			OnPhysGunDrop(CBasePlayer* pPhysGunUser, PhysGunDrop_t Reason) = 0;
	virtual bool			HasPreferredCarryAnglesForPlayer(CBasePlayer* pPlayer = NULL) = 0;
	virtual QAngle			PreferredCarryAngles(void) = 0;
	virtual bool			ForcePhysgunOpen(CBasePlayer* pPlayer) = 0;
	virtual AngularImpulse	PhysGunLaunchAngularImpulse() = 0;
	virtual bool			ShouldPuntUseLaunchForces(PhysGunForce_t reason) = 0;
	virtual Vector			PhysGunLaunchVelocity(const Vector& vecForward, float flMass) = 0;
};

// Forward declaration (нужна для inline-метода CDefaultPlayerPickupVPhysics)
Vector Pickup_DefaultPhysGunLaunchVelocity(const Vector& vecForward, float flMass);

class CDefaultPlayerPickupVPhysics : public IPlayerPickupVPhysics
{
public:
	virtual bool			OnAttemptPhysGunPickup(CBasePlayer* pPhysGunUser, PhysGunPickup_t reason = PICKED_UP_BY_CANNON) { return true; }
	virtual CBaseEntity* OnFailedPhysGunPickup(Vector vPhysgunPos) { return NULL; }
	virtual void			OnPhysGunPickup(CBasePlayer* pPhysGunUser, PhysGunPickup_t reason = PICKED_UP_BY_CANNON) {}
	virtual void			OnPhysGunDrop(CBasePlayer* pPhysGunUser, PhysGunDrop_t reason) {}
	virtual bool			HasPreferredCarryAnglesForPlayer(CBasePlayer* pPlayer) { return false; }
	virtual QAngle			PreferredCarryAngles(void) { return vec3_angle; }
	virtual bool			ForcePhysgunOpen(CBasePlayer* pPlayer) { return false; }
	virtual AngularImpulse	PhysGunLaunchAngularImpulse() { return RandomAngularImpulse(-600, 600); }
	virtual bool			ShouldPuntUseLaunchForces(PhysGunForce_t reason) { return false; }
	virtual Vector			PhysGunLaunchVelocity(const Vector& vecForward, float flMass)
	{
		return Pickup_DefaultPhysGunLaunchVelocity(vecForward, flMass);
	}
};

void PlayerPickupObject(CBasePlayer* pPlayer, CBaseEntity* pObject);
void Pickup_ForcePlayerToDropThisObject(CBaseEntity* pTarget);

void Pickup_OnPhysGunDrop(CBaseEntity* pDroppedObject, CBasePlayer* pPlayer, PhysGunDrop_t reason);
void Pickup_OnPhysGunPickup(CBaseEntity* pPickedUpObject, CBasePlayer* pPlayer, PhysGunPickup_t reason = PICKED_UP_BY_CANNON);
bool Pickup_OnAttemptPhysGunPickup(CBaseEntity* pPickedUpObject, CBasePlayer* pPlayer, PhysGunPickup_t reason = PICKED_UP_BY_CANNON);
bool Pickup_GetPreferredCarryAngles(CBaseEntity* pObject, CBasePlayer* pPlayer, matrix3x4_t& localToWorld, QAngle& outputAnglesWorldSpace);
bool Pickup_ForcePhysGunOpen(CBaseEntity* pObject, CBasePlayer* pPlayer);
bool Pickup_ShouldPuntUseLaunchForces(CBaseEntity* pObject, PhysGunForce_t reason);
AngularImpulse Pickup_PhysGunLaunchAngularImpulse(CBaseEntity* pObject, PhysGunForce_t reason);
Vector Pickup_PhysGunLaunchVelocity(CBaseEntity* pObject, const Vector& vecForward, PhysGunForce_t reason);
CBaseEntity* Pickup_OnFailedPhysGunPickup(CBaseEntity* pPickedUpObject, Vector vPhysgunPos);

bool PlayerPickupControllerIsHoldingEntity(CBaseEntity* pPickupControllerEntity, CBaseEntity* pHeldEntity);
float PlayerPickupGetHeldObjectMass(CBaseEntity* pPickupControllerEntity, IPhysicsObject* pHeldObject);

class CGrabController : public IMotionEvent
{
public:
	CGrabController(void);
	~CGrabController(void);
	void OnRestore();

	virtual IMotionEvent::simresult_e	Simulate(IPhysicsMotionController* pController, IPhysicsObject* pObject, float deltaTime, Vector& linear, AngularImpulse& angular);

	void	SetTargetPosition(const Vector& target, const QAngle& targetOrientation);
	float	ComputeError();
	void	ComputeMaxSpeed(CBaseEntity* pEntity, IPhysicsObject* pPhysics);
	void	AttachEntity(CBasePlayer* pPlayer, CBaseEntity* pEntity, IPhysicsObject* pPhys, bool bIsMegaPhysCannon, const Vector& vGrabPosition, bool bUseGrabPosition);
	void	DetachEntity(bool bClearVelocity);
	bool	UpdateObject(CBasePlayer* pPlayer, float flError);

	void	SetAngleAlignment(float flAngleAlignment) { m_angleAlignment = flAngleAlignment; }
	void	SetIgnorePitch(bool bIgnore) { m_bIgnoreRelativePitch = bIgnore; }
	bool	IsAttached() const { return m_attachedEntity != NULL; }
	CBaseEntity* GetAttached() { return m_attachedEntity; }

	float	GetSavedMass(IPhysicsObject* pObject);
	float	GetLoadWeight(void) { return m_flLoadWeight; }

	QAngle	TransformAnglesToPlayerSpace(const QAngle& anglesIn, CBasePlayer* pPlayer);
	QAngle	TransformAnglesFromPlayerSpace(const QAngle& anglesIn, CBasePlayer* pPlayer);

	// In object space
	Vector			m_attachedPositionObjectSpace;
	QAngle			m_attachedAnglesPlayerSpace;

private:
	hlshadowcontrol_params_t	m_shadow;
	float			m_timeToArrive;
	float			m_errorTime;
	float			m_error;
	float			m_contactAmount;
	float			m_angleAlignment;
	bool			m_bIgnoreRelativePitch;

	EHANDLE			m_attachedEntity;
	QAngle			m_vecPreferredCarryAngles;
	bool			m_bHasPreferredCarryAngles;

	IPhysicsMotionController* m_controller;
	float			m_flLoadWeight;
	float			m_savedRotDamping[VPHYSICS_MAX_OBJECT_LIST_COUNT];
	float			m_savedMass[VPHYSICS_MAX_OBJECT_LIST_COUNT];

	bool			m_bCarriedEntityBlocksLOS;
	int				m_frameCount;
};

#endif // PLAYER_PICKUP_H