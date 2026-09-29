//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Player for SDK
//
//========================================================================//

#ifndef SDK_PLAYER_H
#define SDK_PLAYER_H

#include "basemultiplayerplayer.h"
#include "server_class.h"
#include "sdk_playeranimstate.h"
#include "sdk_player_shared.h"
#include "player_pickup.h"

// Function table for each player state.
class CSDKPlayerStateInfo
{
public:
	SDKPlayerState m_iPlayerState;
	const char* m_pStateName;

	void (CSDKPlayer::* pfnEnterState)();	// Init and deinit the state.
	void (CSDKPlayer::* pfnLeaveState)();
	void (CSDKPlayer::* pfnPreThink)();	// Do a PreThink() in this state.
};

//-----------------------------------------------------------------------------
// SDK Game player
//-----------------------------------------------------------------------------
class CSDKPlayer : public CBaseMultiplayerPlayer
{
public:
	DECLARE_CLASS(CSDKPlayer, CBaseMultiplayerPlayer);
	DECLARE_SERVERCLASS();
	DECLARE_PREDICTABLE();
	DECLARE_DATADESC();

	CSDKPlayer();
	~CSDKPlayer();

	static CSDKPlayer* CreatePlayer(const char* className, edict_t* ed);
	static CSDKPlayer* Instance(int iEnt);

	// This passes the event to the client's and server's CPlayerAnimState.
	void DoAnimationEvent(PlayerAnimEvent_t event, int nData = 0);

	virtual int FlashlightIsOn(void);
	virtual void FlashlightTurnOn(void);
	virtual void FlashlightTurnOff(void);

	virtual void PreThink();
	virtual void PostThink();
	virtual void Spawn();
	virtual void InitialSpawn();

	virtual void GiveAllItems();
	virtual void GiveDefaultItems();

	// Animstate handles this.
	void SetAnimation(PLAYER_ANIM playerAnim) { return; }

	virtual void Precache();
	virtual int			OnTakeDamage(const CTakeDamageInfo& inputInfo);
	virtual int			OnTakeDamage_Alive(const CTakeDamageInfo& info);
	virtual void Event_Killed(const CTakeDamageInfo& info);
	virtual void TraceAttack(const CTakeDamageInfo& inputInfo, const Vector& vecDir, trace_t* ptr);
	virtual void LeaveVehicle(const Vector& vecExitPoint, const QAngle& vecExitAngles);

	CWeaponSDKBase* GetActiveSDKWeapon() const;
	virtual void	CreateViewModel(int viewmodelindex = 0);

	virtual void	CheatImpulseCommands(int iImpulse);

	virtual int		SpawnArmorValue(void) const { return m_iSpawnArmorValue; }
	virtual void	SetSpawnArmorValue(int i) { m_iSpawnArmorValue = i; }

	CNetworkQAngle(m_angEyeAngles);
	CNetworkVar(int, m_iShotsFired);

	// Tracks our ragdoll entity.
	CNetworkHandle(CBaseEntity, m_hRagdoll);

	void PhysObjectSleep();
	void PhysObjectWake();

	// Player avoidance
	virtual	bool		ShouldCollide(int collisionGroup, int contentsMask) const;
	void SDKPushawayThink(void);

	virtual void PlayerUse(void);
	virtual void PickupObject(CBaseEntity* pObject, bool bLimitMassAndSize = true);
	virtual bool IsHoldingEntity(CBaseEntity* pEnt);
	virtual void ForceDropOfCarriedPhysObjects(CBaseEntity* pOnlyIfHoldingThis = NULL);
	virtual float GetHeldObjectMass(IPhysicsObject* pHeldObject);
	virtual bool IsFollowingPhysics(void) { return false; }
	virtual void ItemPostFrame(void);

private:
	bool m_bPlayUseDenySound;
	float m_flTimeUseSuspended;

	// In shared code.
public:
	void FireBullet(
		Vector vecSrc,
		const QAngle& shootAngles,
		float vecSpread,
		int iDamage,
		int iBulletType,
		CBaseEntity* pevAttacker,
		bool bDoEffects,
		float x,
		float y);

	CNetworkVarEmbedded(CSDKPlayerShared, m_Shared);
	virtual void			PlayerDeathThink(void);
	virtual bool		ClientCommand(const CCommand& args);

	void IncreaseShotsFired() { m_iShotsFired++; if (m_iShotsFired > 16) m_iShotsFired = 16; }
	void DecreaseShotsFired() { m_iShotsFired--; if (m_iShotsFired < 0) m_iShotsFired = 0; }
	void ClearShotsFired() { m_iShotsFired = 0; }
	int GetShotsFired() { return m_iShotsFired; }

#if defined ( SDK_USE_SPRINTING )
	void SetSprinting(bool bIsSprinting);
#endif // SDK_USE_SPRINTING
	bool CanAttack(void);

	virtual int GetPlayerStance();

	void NoteWeaponFired();
	virtual bool WantsLagCompensationOnEntity(const CBasePlayer* pPlayer, const CUserCmd* pCmd, const CBitVec<MAX_EDICTS>* pEntityTransmitBits) const;

	//------------------------------------------------------------------------------------------------
	// Player state management.
	//------------------------------------------------------------------------------------------------
public:

	void State_Transition(SDKPlayerState newState);
	SDKPlayerState State_Get() const;

	virtual bool	ModeWantsSpectatorGUI(int iMode) { return (iMode != OBS_MODE_DEATHCAM && iMode != OBS_MODE_FREEZECAM); }

private:
	bool SelectSpawnSpot(const char* pEntClassName, CBaseEntity*& pSpot);

	void State_Enter(SDKPlayerState newState);
	void State_Leave();
	void State_PreThink();

	void State_Enter_WELCOME();
	void State_PreThink_WELCOME();

	void State_Enter_PICKINGTEAM();
	void State_Enter_PICKINGCLASS();

public:
	void MoveToNextIntroCamera();
private:

	void State_Enter_ACTIVE();
	void State_PreThink_ACTIVE();

	void State_Enter_OBSERVER_MODE();
	void State_PreThink_OBSERVER_MODE();

	void State_Enter_DEATH_ANIM();
	void State_PreThink_DEATH_ANIM();

	static CSDKPlayerStateInfo* State_LookupInfo(SDKPlayerState state);

	CNetworkVar(SDKPlayerState, m_iPlayerState);

	CSDKPlayerStateInfo* m_pCurStateInfo;
	bool HandleCommand_JoinTeam(int iTeam);

	bool BecomeRagdollOnClient(const Vector& force);

#if defined ( SDK_USE_PRONE )
	void InitProne(void);
#endif // SDK_USE_PRONE

#if defined ( SDK_USE_SPRINTING )
	void InitSprinting(void);
	bool IsSprinting(void);
	bool CanSprint(void);
#endif // SDK_USE_SPRINTING

	void InitSpeeds(void);

	void			SetupVisibility(CBaseEntity* pViewEntity, unsigned char* pvs, int pvssize);

	bool			CanMove(void) const;

	virtual void	SharedSpawn();

	virtual const Vector	GetPlayerMins(void) const;
	virtual const Vector	GetPlayerMaxs(void) const;

	virtual void		CommitSuicide(bool bExplode = false, bool bForce = false);

private:
	int m_iLastWeaponFireUsercmd;

	virtual void Weapon_Equip(CBaseCombatWeapon* pWeapon);
	virtual void ThrowActiveWeapon(void);
	virtual void SDKThrowWeapon(CWeaponSDKBase* pWeapon, const Vector& vecForward, const QAngle& vecAngles, float flDiameter);
	virtual void SDKThrowWeaponDir(CWeaponSDKBase* pWeapon, const Vector& vecForward, Vector* pVecThrowDir);

	EHANDLE m_pIntroCamera;
	float m_fIntroCamTime;

	void CreateRagdollEntity();
	void DestroyRagdoll(void);

	CSDKPlayerAnimState* m_PlayerAnimState;

	CNetworkVar(bool, m_bSpawnInterpCounter);

	int m_iSpawnArmorValue;
	IMPLEMENT_NETWORK_VAR_FOR_DERIVED(m_ArmorValue);
public:
#if defined ( SDK_USE_PRONE )
	bool m_bUnProneToDuck;
#endif // SDK_USE_PRONE

};

inline CSDKPlayer* ToSDKPlayer(CBaseEntity* pEntity)
{
	if (!pEntity || !pEntity->IsPlayer())
		return NULL;

#ifdef _DEBUG
	Assert(dynamic_cast<CSDKPlayer*>(pEntity) != 0);
#endif
	return static_cast<CSDKPlayer*>(pEntity);
}

inline SDKPlayerState CSDKPlayer::State_Get() const
{
	return m_iPlayerState;
}

#endif	// SDK_PLAYER_H