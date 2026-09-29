//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Player for SDK
//
//========================================================================//

#include "cbase.h"

#include "sdk_player.h"
#include "sdk_team.h"
#include "sdk_gamerules.h"
#include "weapon_sdkbase.h"
#include "predicted_viewmodel.h"
#include "iservervehicle.h"
#include "viewport_panel_names.h"
#include "info_camera_link.h"
#include "GameStats.h"
#include "obstacle_pushaway.h"
#include "in_buttons.h"
#include "physics_prop_ragdoll.h"
#include "particle_parse.h"
#include "eventqueue.h"
#include "player_pickup.h"
#include "physics.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

extern int gEvilImpulse101;
extern ConVar flashlight;

ConVar SDK_ShowStateTransitions( "sdk_ShowStateTransitions", "-2", FCVAR_CHEAT, "sdk_ShowStateTransitions <ent index or -1 for all>. Show player state transitions." );

EHANDLE g_pLastDMSpawn;

//--------------------------------------------------------------------------------
// Player animation event.
//--------------------------------------------------------------------------------
class CTEPlayerAnimEvent : public CBaseTempEntity
{
public:
	DECLARE_CLASS( CTEPlayerAnimEvent, CBaseTempEntity );
	DECLARE_SERVERCLASS();

	CTEPlayerAnimEvent( const char *name ) : CBaseTempEntity( name )
	{
	}

	CNetworkHandle( CBasePlayer, m_hPlayer );
	CNetworkVar( int, m_iEvent );
	CNetworkVar( int, m_nData );
};

IMPLEMENT_SERVERCLASS_ST_NOBASE( CTEPlayerAnimEvent, DT_TEPlayerAnimEvent )
	SendPropEHandle( SENDINFO( m_hPlayer ) ),
	SendPropInt( SENDINFO( m_iEvent ), Q_log2( PLAYERANIMEVENT_COUNT ) + 1, SPROP_UNSIGNED ),
	SendPropInt( SENDINFO( m_nData ), 32 )
END_SEND_TABLE()

static CTEPlayerAnimEvent g_TEPlayerAnimEvent( "PlayerAnimEvent" );

void TE_PlayerAnimEvent( CBasePlayer *pPlayer, PlayerAnimEvent_t event, int nData )
{
	CPVSFilter filter( (const Vector&)pPlayer->EyePosition() );
	filter.RemoveRecipient( pPlayer );

	g_TEPlayerAnimEvent.m_hPlayer = pPlayer;
	g_TEPlayerAnimEvent.m_iEvent = event;
	g_TEPlayerAnimEvent.m_nData = nData;
	g_TEPlayerAnimEvent.Create( filter, 0 );
}

//--------------------------------------------------------------------------------
// Tables.
//--------------------------------------------------------------------------------
BEGIN_DATADESC( CSDKPlayer )
DEFINE_THINKFUNC( SDKPushawayThink ),
END_DATADESC()

LINK_ENTITY_TO_CLASS( player, CSDKPlayer );
PRECACHE_REGISTER(player);

// Specific to the local player
BEGIN_SEND_TABLE_NOBASE( CSDKPlayerShared, DT_SDKSharedLocalPlayerExclusive )

END_SEND_TABLE()

BEGIN_SEND_TABLE_NOBASE( CSDKPlayerShared, DT_SDKPlayerShared )
#if defined ( SDK_USE_STAMINA ) || defined ( SDK_USE_SPRINTING )
	SendPropFloat( SENDINFO( m_flStamina ), 0, SPROP_NOSCALE | SPROP_CHANGES_OFTEN ),
#endif

#if defined ( SDK_USE_PRONE )
	SendPropBool( SENDINFO( m_bProne ) ),
	SendPropTime( SENDINFO( m_flGoProneTime ) ),
	SendPropTime( SENDINFO( m_flUnProneTime ) ),
#endif
#if defined ( SDK_USE_SPRINTING )
	SendPropBool( SENDINFO( m_bIsSprinting ) ),
#endif
	SendPropDataTable( "sdksharedlocaldata", 0, &REFERENCE_SEND_TABLE(DT_SDKSharedLocalPlayerExclusive), SendProxy_SendLocalDataTable ),
END_SEND_TABLE()
extern void SendProxy_Origin( const SendProp *pProp, const void *pStruct, const void *pData, DVariant *pOut, int iElement, int objectID );

BEGIN_SEND_TABLE_NOBASE( CSDKPlayer, DT_SDKLocalPlayerExclusive )
	SendPropInt( SENDINFO( m_iShotsFired ), 8, SPROP_UNSIGNED ),
	SendPropVector	(SENDINFO(m_vecOrigin), -1,  SPROP_NOSCALE|SPROP_CHANGES_OFTEN, 0.0f, HIGH_DEFAULT, SendProxy_Origin ),
	SendPropFloat( SENDINFO_VECTORELEM(m_angEyeAngles, 0), 8, SPROP_CHANGES_OFTEN, -90.0f, 90.0f ),
	SendPropInt( SENDINFO( m_ArmorValue ), 8, SPROP_UNSIGNED ),
END_SEND_TABLE()

BEGIN_SEND_TABLE_NOBASE( CSDKPlayer, DT_SDKNonLocalPlayerExclusive )
	SendPropVector	(SENDINFO(m_vecOrigin), -1,  SPROP_COORD_MP_LOWPRECISION|SPROP_CHANGES_OFTEN, 0.0f, HIGH_DEFAULT, SendProxy_Origin ),
	SendPropFloat( SENDINFO_VECTORELEM(m_angEyeAngles, 0), 8, SPROP_CHANGES_OFTEN, -90.0f, 90.0f ),
	SendPropAngle( SENDINFO_VECTORELEM(m_angEyeAngles, 1), 10, SPROP_CHANGES_OFTEN ),
END_SEND_TABLE()

IMPLEMENT_SERVERCLASS_ST( CSDKPlayer, DT_SDKPlayer )
	SendPropExclude( "DT_BaseAnimating", "m_flPoseParameter" ),
	SendPropExclude( "DT_BaseAnimating", "m_flPlaybackRate" ),	
	SendPropExclude( "DT_BaseAnimating", "m_nSequence" ),
	SendPropExclude( "DT_BaseAnimating", "m_nNewSequenceParity" ),
	SendPropExclude( "DT_BaseAnimating", "m_nResetEventsParity" ),
	SendPropExclude( "DT_BaseEntity", "m_angRotation" ),
	SendPropExclude( "DT_BaseAnimatingOverlay", "overlay_vars" ),
	SendPropExclude( "DT_BaseEntity", "m_vecOrigin" ),
	
	SendPropExclude( "DT_ServerAnimationData" , "m_flCycle" ),	
	SendPropExclude( "DT_AnimTimeMustBeFirst" , "m_flAnimTime" ),

	SendPropDataTable( SENDINFO_DT( m_Shared ), &REFERENCE_SEND_TABLE( DT_SDKPlayerShared ) ),

	SendPropDataTable( "sdklocaldata", 0, &REFERENCE_SEND_TABLE(DT_SDKLocalPlayerExclusive), SendProxy_SendLocalDataTable ),
	SendPropDataTable( "sdknonlocaldata", 0, &REFERENCE_SEND_TABLE(DT_SDKNonLocalPlayerExclusive), SendProxy_SendNonLocalDataTable ),

	SendPropEHandle( SENDINFO( m_hRagdoll ) ),

	SendPropInt( SENDINFO( m_iPlayerState ), Q_log2( NUM_PLAYER_STATES )+1, SPROP_UNSIGNED ),

	SendPropBool( SENDINFO( m_bSpawnInterpCounter ) ),
END_SEND_TABLE()

class CSDKRagdoll : public CBaseAnimatingOverlay
{
public:
	DECLARE_CLASS( CSDKRagdoll, CBaseAnimatingOverlay );
	DECLARE_SERVERCLASS();

	virtual int UpdateTransmitState()
	{
		return SetTransmitState( FL_EDICT_ALWAYS );
	}

public:
	CNetworkHandle( CBaseEntity, m_hPlayer );
	CNetworkVector( m_vecRagdollVelocity );
	CNetworkVector( m_vecRagdollOrigin );
};

LINK_ENTITY_TO_CLASS( sdk_ragdoll, CSDKRagdoll );

IMPLEMENT_SERVERCLASS_ST_NOBASE( CSDKRagdoll, DT_SDKRagdoll )
	SendPropVector( SENDINFO(m_vecRagdollOrigin), -1,  SPROP_COORD ),
	SendPropEHandle( SENDINFO( m_hPlayer ) ),
	SendPropModelIndex( SENDINFO( m_nModelIndex ) ),
	SendPropInt		( SENDINFO(m_nForceBone), 8, 0 ),
	SendPropVector	( SENDINFO(m_vecForce), -1, SPROP_NOSCALE ),
	SendPropVector( SENDINFO( m_vecRagdollVelocity ) )
END_SEND_TABLE()

void cc_CreatePredictionError_f()
{
	CBaseEntity *pEnt = CBaseEntity::Instance( 1 );
	pEnt->SetAbsOrigin( pEnt->GetAbsOrigin() + Vector( 63, 0, 0 ) );
}

ConCommand cc_CreatePredictionError( "CreatePredictionError", cc_CreatePredictionError_f, "Create a prediction error", FCVAR_CHEAT );

void CSDKPlayer::SetupVisibility( CBaseEntity *pViewEntity, unsigned char *pvs, int pvssize )
{
	BaseClass::SetupVisibility( pViewEntity, pvs, pvssize );

	int area = pViewEntity ? pViewEntity->NetworkProp()->AreaNum() : NetworkProp()->AreaNum();
	PointCameraSetupVisibility( this, area, pvs, pvssize );
}

CSDKPlayer::CSDKPlayer()
{
	m_PlayerAnimState = CreateSDKPlayerAnimState( this );
	m_iLastWeaponFireUsercmd = 0;
	
	m_Shared.Init( this );

	UseClientSideAnimation();

	m_angEyeAngles.Init();

	m_pCurStateInfo = NULL;

	m_bPlayUseDenySound = false;
	m_flTimeUseSuspended = 0.0f;
}

CSDKPlayer::~CSDKPlayer()
{
	DestroyRagdoll();
	m_PlayerAnimState->Release();
}

CSDKPlayer *CSDKPlayer::CreatePlayer( const char *className, edict_t *ed )
{
	CSDKPlayer::s_PlayerEdict = ed;
	return (CSDKPlayer*)CreateEntityByName( className );
}

void CSDKPlayer::LeaveVehicle( const Vector &vecExitPoint, const QAngle &vecExitAngles )
{
	BaseClass::LeaveVehicle( vecExitPoint, vecExitAngles );
}

void CSDKPlayer::PreThink(void)
{
	State_PreThink();

	if ( IsInAVehicle() )	
	{
		UpdateClientData();		
		CheckTimeBasedDamage();
		CheckSuitUpdate();
		WaterMove();	
		return;
	}

	BaseClass::PreThink();
}

void CSDKPlayer::PostThink()
{
	BaseClass::PostThink();

	QAngle angles = GetLocalAngles();
	angles[PITCH] = 0;
	SetLocalAngles( angles );
	
	m_angEyeAngles = EyeAngles();

	m_PlayerAnimState->Update( m_angEyeAngles[YAW], m_angEyeAngles[PITCH] );

	if (m_hUseEntity != NULL)
	{
		if (!m_hUseEntity->OnControls(this) || (GetActiveWeapon() && GetActiveWeapon()->GetActivity() == ACT_VM_HOLSTER))
		{
			ClearUseEntity();
		}
	}
}

void CSDKPlayer::Precache()
{
	int i = 0;
	while( pszPossiblePlayerModels[i] != NULL )
	{
		PrecacheModel( pszPossiblePlayerModels[i] );
		i++;
	}

	PrecacheScriptSound( "Player.Jump" );
	PrecacheScriptSound( "Player.JumpLanding" );
	PrecacheScriptSound( "Player.FlashlightOn" );
	PrecacheScriptSound( "Player.FlashlightOff" );

	PrecacheScriptSound("HL2Player.Use");
	PrecacheScriptSound("HL2Player.UseDeny");
	PrecacheScriptSound("HL2Player.TrainUse");

	BaseClass::Precache();
}

void CSDKPlayer::GiveAllItems()
{
	EquipSuit();

	CBasePlayer::GiveAmmo(30, "pistol");
	CBasePlayer::GiveAmmo(30, "mp5");
	CBasePlayer::GiveAmmo(12, "shotgun");
	CBasePlayer::GiveAmmo(5, "grenades");

	GiveNamedItem("weapon_pistol");
	GiveNamedItem("weapon_mp5");
	GiveNamedItem("weapon_shotgun");
	GiveNamedItem("weapon_crowbar");
	GiveNamedItem("weapon_grenade");
}

void CSDKPlayer::GiveDefaultItems()
{
	EquipSuit();

	CBasePlayer::GiveAmmo(30, "pistol");
	CBasePlayer::GiveAmmo(30, "mp5");
	CBasePlayer::GiveAmmo(12, "shotgun");
	CBasePlayer::GiveAmmo(5, "grenades");

	GiveNamedItem("weapon_pistol");
	GiveNamedItem("weapon_mp5");
	GiveNamedItem("weapon_shotgun");
	GiveNamedItem("weapon_crowbar");
	GiveNamedItem("weapon_grenade");
}

#define SDK_PUSHAWAY_THINK_CONTEXT	"SDKPushawayThink"

void CSDKPlayer::SDKPushawayThink(void)
{
	PerformObstaclePushaway( this );
	SetNextThink( gpGlobals->curtime + PUSHAWAY_THINK_INTERVAL, SDK_PUSHAWAY_THINK_CONTEXT );
}

void CSDKPlayer::Spawn()
{
	SetModel( SDK_PLAYER_MODEL );
	
	SetBloodColor( BLOOD_COLOR_RED );
	
	SetMoveType( MOVETYPE_WALK );
	RemoveSolidFlags( FSOLID_NOT_SOLID );

	m_hRagdoll = NULL;
	
	BaseClass::Spawn();

	m_bPlayUseDenySound = false;
	m_flTimeUseSuspended = 0.0f;
	
	m_iLastWeaponFireUsercmd = 0;

#if defined ( SDK_USE_STAMINA ) || defined ( SDK_USE_SPRINTING )
	m_Shared.SetStamina( 100 );
#endif

#if defined ( SDK_USE_PRONE )
	InitProne();
#endif

#if defined ( SDK_USE_SPRINTING )
	InitSprinting();
#endif

	m_bSpawnInterpCounter = !m_bSpawnInterpCounter;

	InitSpeeds();

	SetArmorValue(SpawnArmorValue());

	pl.deadflag = false;

	SetContextThink( &CSDKPlayer::SDKPushawayThink, gpGlobals->curtime + PUSHAWAY_THINK_INTERVAL, SDK_PUSHAWAY_THINK_CONTEXT );
	
	SetContextThink( &CSDKPlayer::GiveDefaultItems, gpGlobals->curtime + 3.0f, "GiveDefaultItems" );
}

bool CSDKPlayer::SelectSpawnSpot( const char *pEntClassName, CBaseEntity* &pSpot )
{
	pSpot = gEntList.FindEntityByClassname( pSpot, pEntClassName );

	if ( pSpot == NULL )
		pSpot = gEntList.FindEntityByClassname( pSpot, pEntClassName );

	CBaseEntity *pFirstSpot = pSpot;
	do 
	{
		if ( pSpot )
		{
			if ( g_pGameRules->IsSpawnPointValid( pSpot, this ) )
			{
				if ( pSpot->GetAbsOrigin() == Vector( 0, 0, 0 ) )
				{
					pSpot = gEntList.FindEntityByClassname( pSpot, pEntClassName );
					continue;
				}
				return true;
			}
		}
		pSpot = gEntList.FindEntityByClassname( pSpot, pEntClassName );
	} while ( pSpot != pFirstSpot );

	DevMsg("CSDKPlayer::SelectSpawnSpot: couldn't find valid spawn point.\n");

	return true;
}

void CSDKPlayer::CommitSuicide( bool bExplode /* = false */, bool bForce /*= false*/ )
{
	if (State_Get() != STATE_ACTIVE)
		return;
	
	m_iSuicideCustomKillFlags = SDK_DMG_CUSTOM_SUICIDE;

	BaseClass::CommitSuicide( bExplode, bForce );
}

void CSDKPlayer::InitialSpawn( void )
{
	BaseClass::InitialSpawn();
}

void CSDKPlayer::TraceAttack( const CTakeDamageInfo &inputInfo, const Vector &vecDir, trace_t *ptr )
{
	CDisablePredictionFiltering disabler;
	BaseClass::TraceAttack( inputInfo, vecDir, ptr );
}

int CSDKPlayer::OnTakeDamage( const CTakeDamageInfo &inputInfo )
{
	CTakeDamageInfo info = inputInfo;

	CBaseEntity *pInflictor = info.GetInflictor();

	if ( !pInflictor )
		return 0;

	if ( GetMoveType() == MOVETYPE_NOCLIP || GetMoveType() == MOVETYPE_OBSERVER )
		return 0;

	float flArmorBonus = 0.5f;
	float flArmorRatio = 0.5f;
	float flDamage = info.GetDamage();

	bool bCheckFriendlyFire = false;
	bool bFriendlyFire = friendlyfire.GetBool();
	if ( gpGlobals->teamplay )
		bCheckFriendlyFire = true;

	if ( bFriendlyFire || ( bCheckFriendlyFire && pInflictor->GetTeamNumber() != GetTeamNumber() ) || pInflictor == this ||	info.GetAttacker() == this )
	{
		if ( bFriendlyFire && (info.GetDamageType() & DMG_BLAST) == 0 )
		{
			if ( pInflictor->GetTeamNumber() == GetTeamNumber() && bCheckFriendlyFire)
			{
				flDamage *= 0.35;
			}
		}

		m_lastDamageAmount = flDamage;
		if ( ArmorValue() && !( info.GetDamageType() & (DMG_FALL | DMG_DROWN)) )
		{
			float flNew = flDamage * flArmorRatio;
			float flArmor = (flDamage - flNew) * flArmorBonus;

			if (flArmor > ArmorValue() )
			{
				flArmor = ArmorValue();
				flArmor *= (1/flArmorBonus);
				flNew = flDamage - flArmor;
				SetArmorValue( 0 );
			}
			else
			{
				int oldValue = (int)(ArmorValue());
			
				if ( flArmor < 0 )
					 flArmor = 1;

				SetArmorValue( oldValue - flArmor );
			}
			
			flDamage = flNew;
			
			info.SetDamage( flDamage );
		}

		info.SetDamage( (int)flDamage );

		if ( info.GetDamage() <= 0 )
			return 0;

		CSingleUserRecipientFilter user( this );
		user.MakeReliable();
		UserMessageBegin( user, "Damage" );
			WRITE_BYTE( (int)info.GetDamage() );
			WRITE_VEC3COORD( info.GetInflictor()->WorldSpaceCenter() );
		MessageEnd();

		if ( info.GetDamageType() & DMG_BLAST )
		{
			OnDamagedByExplosion( info );
		}

		gamestats->Event_PlayerDamage( this, info );

		return CBaseCombatCharacter::OnTakeDamage( info );
	}
	else
	{
		return 0;
	}
}

int CSDKPlayer::OnTakeDamage_Alive( const CTakeDamageInfo &info )
{
	m_bitsDamageType |= info.GetDamageType();

	if ( !CBaseCombatCharacter::OnTakeDamage_Alive( info ) )
		return 0;

	IGameEvent * event = gameeventmanager->CreateEvent( "player_hurt" );

	if ( event )
	{
		event->SetInt("userid", GetUserID() );
		event->SetInt("health", MAX(0, m_iHealth) );
		event->SetInt("armor", MAX(0, ArmorValue()) );

		if ( info.GetDamageType() & DMG_BLAST )
		{
			event->SetInt( "hitgroup", HITGROUP_GENERIC );
		}
		else
		{
			event->SetInt( "hitgroup", LastHitGroup() );
		}

		CBaseEntity * attacker = info.GetAttacker();
		const char *weaponName = "";

		if ( attacker->IsPlayer() )
		{
			CBasePlayer *player = ToBasePlayer( attacker );
			event->SetInt("attacker", player->GetUserID() );

			CBaseEntity *pInflictor = info.GetInflictor();
			if ( pInflictor )
			{
				if ( pInflictor == player )
				{
					if ( player->GetActiveWeapon() )
					{
						weaponName = player->GetActiveWeapon()->GetClassname();
					}
				}
				else
				{
					weaponName = STRING( pInflictor->m_iClassname );
				}
			}
		}
		else
		{
			event->SetInt("attacker", 0 );
		}

		if ( strncmp( weaponName, "weapon_", 7 ) == 0 )
		{
			weaponName += 7;
		}
		else if( strncmp( weaponName, "grenade", 9 ) == 0 )	
		{
			weaponName = "grenade";
		}

		event->SetString( "weapon", weaponName );
		event->SetInt( "priority", 5 );

		gameeventmanager->FireEvent( event );
	}
	
	return 1;
}

ConVar c_server_ragdoll("c_server_ragdoll", "0", FCVAR_CHEAT, "If set, players will have server ragdolls instead of clientside ones.");

bool CSDKPlayer::BecomeRagdollOnClient( const Vector &force )
{
	if ( !CanBecomeRagdoll() ) 
		return false;

	if (c_server_ragdoll.GetBool())
	{
		CTakeDamageInfo	info;

		info.SetDamageType( DMG_GENERIC );
		info.SetDamageForce( force );
		info.SetDamagePosition( WorldSpaceCenter() );		
		IPhysicsObject *pPhysics = VPhysicsGetObject();
		if ( pPhysics )
		{
			VPhysicsDestroyObject();
		}
		CBaseEntity *pRagdoll = CreateServerRagdoll( this, m_nForceBone, info, COLLISION_GROUP_INTERACTIVE_DEBRIS, true );
		FixupBurningServerRagdoll( pRagdoll );
		PhysSetEntityGameFlags( pRagdoll, FVPHYSICS_NO_SELF_COLLISIONS );

		pRagdoll->SetName( GetEntityName() );
		RemoveDeferred();

		return true;
	}

	return BaseClass::BecomeRagdollOnClient( force );
}

void CSDKPlayer::Event_Killed( const CTakeDamageInfo &info )
{
	ThrowActiveWeapon();

	if( info.GetAttacker() && info.GetAttacker()->IsPlayer() )
	{
		m_hObserverTarget.Set( info.GetAttacker() ); 
		SetFOV( this, 0 );
	}
	else
		m_hObserverTarget.Set( NULL );

	State_Transition( STATE_DEATH_ANIM );

	RemoveAllItems( true );

	FlashlightTurnOff();

	BaseClass::Event_Killed( info );
}

void CSDKPlayer::ThrowActiveWeapon(void)
{
	CWeaponSDKBase* pWeapon = (CWeaponSDKBase*)GetActiveWeapon();

	if (pWeapon && pWeapon->CanWeaponBeDropped())
	{
		Vector VecForward;

		EyeVectors(&VecForward, NULL, NULL);

		VecForward *= 300.0f;

		BaseClass::Weapon_Drop(pWeapon, NULL, &VecForward);
	}
}

void CSDKPlayer::Weapon_Equip( CBaseCombatWeapon *pWeapon )
{
	BaseClass::Weapon_Equip( pWeapon );
	dynamic_cast<CWeaponSDKBase*>(pWeapon)->SetDieThink( false );
}

void CSDKPlayer::SDKThrowWeapon( CWeaponSDKBase *pWeapon, const Vector &vecForward, const QAngle &vecAngles, float flDiameter  )
{
	Vector vecOrigin;
	CollisionProp()->RandomPointInBounds( Vector( 0.5f, 0.5f, 0.5f ), Vector( 0.5f, 0.5f, 1.0f ), &vecOrigin );

	Vector vecThrow;
	SDKThrowWeaponDir( pWeapon, vecForward, &vecThrow );

	Vector vecOffsetOrigin;
	VectorMA( vecOrigin, flDiameter, vecThrow, vecOffsetOrigin );

	trace_t	tr;
	UTIL_TraceLine( vecOrigin, vecOffsetOrigin, MASK_SOLID_BRUSHONLY, this, COLLISION_GROUP_NONE, &tr );
		
	if ( tr.startsolid || tr.allsolid || ( tr.fraction < 1.0f && tr.m_pEnt != pWeapon ) )
	{
		vecThrow.Negate();
		VectorMA( vecOrigin, flDiameter, vecThrow, vecOffsetOrigin );
	}

	vecThrow *= random->RandomFloat( 150.0f, 240.0f );

	pWeapon->SetAbsOrigin( vecOrigin );
	pWeapon->SetAbsAngles( vecAngles );
	pWeapon->Drop( vecThrow );
	pWeapon->SetRemoveable( false );
	Weapon_Detach( pWeapon );

	pWeapon->SetDieThink( true );
}

void CSDKPlayer::SDKThrowWeaponDir( CWeaponSDKBase *pWeapon, const Vector &vecForward, Vector *pVecThrowDir )
{
	VMatrix zRot;
	MatrixBuildRotateZ( zRot, random->RandomFloat( -60.0f, 60.0f ) );

	Vector vecThrow;
	Vector3DMultiply( zRot, vecForward, *pVecThrowDir );

	pVecThrowDir->z = random->RandomFloat( -0.5f, 0.5f );
	VectorNormalize( *pVecThrowDir );
}

void CSDKPlayer::PlayerDeathThink()
{
	// overridden, do nothing
}

void CSDKPlayer::CreateRagdollEntity()
{
	CSDKRagdoll *pRagdoll = dynamic_cast< CSDKRagdoll* >( m_hRagdoll.Get() );

	if( pRagdoll )
	{
		UTIL_Remove( pRagdoll );
		pRagdoll = NULL;
	}
	Assert( pRagdoll == NULL );

	if ( !pRagdoll )
	{
		pRagdoll = dynamic_cast< CSDKRagdoll* >( CreateEntityByName( "sdk_ragdoll" ) );
	}

	if ( pRagdoll )
	{
		pRagdoll->m_hPlayer = this;
		pRagdoll->m_vecRagdollOrigin = GetAbsOrigin();
		pRagdoll->m_vecRagdollVelocity = GetAbsVelocity();
		pRagdoll->m_nModelIndex = m_nModelIndex;
		pRagdoll->m_nForceBone = m_nForceBone;
		pRagdoll->m_vecForce = Vector(0,0,0);
	}

	m_hRagdoll = pRagdoll;
}

void CSDKPlayer::DestroyRagdoll( void )
{
	CSDKRagdoll *pRagdoll = dynamic_cast<CSDKRagdoll*>( m_hRagdoll.Get() );	
	if( pRagdoll )
	{
		UTIL_Remove( pRagdoll );
	}
}

void CSDKPlayer::DoAnimationEvent( PlayerAnimEvent_t event, int nData )
{
	m_PlayerAnimState->DoAnimationEvent( event, nData );
	TE_PlayerAnimEvent( this, event, nData );
}

CWeaponSDKBase* CSDKPlayer::GetActiveSDKWeapon() const
{
	return static_cast< CWeaponSDKBase* >( GetActiveWeapon() );
}

void CSDKPlayer::CreateViewModel( int index /*=0*/ )
{
	Assert( index >= 0 && index < MAX_VIEWMODELS );

	if ( GetViewModel( index ) )
		return;

	CPredictedViewModel *vm = ( CPredictedViewModel * )CreateEntityByName( "predicted_viewmodel" );
	if ( vm )
	{
		vm->SetAbsOrigin( GetAbsOrigin() );
		vm->SetOwner( this );
		vm->SetIndex( index );
		DispatchSpawn( vm );
		vm->FollowEntity( this, false );
		m_hViewModel.Set( index, vm );
	}
}

void CSDKPlayer::CheatImpulseCommands( int iImpulse )
{
	switch (iImpulse)
	{
	case 101:
	{
		if (sv_cheats->GetBool())
		{
			GiveAllItems();
		}
	}
	break;

	default:
		BaseClass::CheatImpulseCommands(iImpulse);
	}
}

void CSDKPlayer::FlashlightTurnOn( void )
{
	if( flashlight.GetInt() > 0 && IsAlive() )
	{
		AddEffects( EF_DIMLIGHT );
		EmitSound( "Player.FlashlightOn" );
	}
}

void CSDKPlayer::FlashlightTurnOff( void )
{
	RemoveEffects( EF_DIMLIGHT );
	
	if( IsAlive() )
		EmitSound( "Player.FlashlightOff" );
}

int CSDKPlayer::FlashlightIsOn( void )
{
	return IsEffectActive( EF_DIMLIGHT );
}

bool CSDKPlayer::ClientCommand( const CCommand &args )
{
	const char *pcmd = args[0];
	if ( FStrEq( pcmd, "jointeam" ) ) 
	{
		if ( args.ArgC() < 2 )
		{
			Warning( "Player sent bad jointeam syntax\n" );
		}

		int iTeam = atoi( args[1] );
		HandleCommand_JoinTeam( iTeam );
		return true;
	}
	else if( !Q_strncmp( pcmd, "cls_", 4 ) )
	{
		return true;
	}
	else if ( FStrEq( pcmd, "spectate" ) )
	{
		HandleCommand_JoinTeam( TEAM_SPECTATOR );
		return true;
	}
	else if ( FStrEq( pcmd, "joingame" ) )
	{
		if ( m_iPlayerState == STATE_WELCOME )
		{
			State_Transition( STATE_ACTIVE );
		}
		
		return true;
	}
	else if ( FStrEq( pcmd, "joinclass" ) ) 
	{
		return true;
	}
	else if ( FStrEq( pcmd, "menuopen" ) )
	{
		return true;
	}
	else if ( FStrEq( pcmd, "menuclosed" ) )
	{
		return true;
	}
	else if ( FStrEq( pcmd, "drop" ) )
	{
		ThrowActiveWeapon();
		return true;
	}

	return BaseClass::ClientCommand( args );
}

bool CSDKPlayer::HandleCommand_JoinTeam( int team )
{
	CSDKGameRules *mp = SDKGameRules();
	int iOldTeam = GetTeamNumber();
	if ( !GetGlobalTeam( team ) )
	{
		Warning( "HandleCommand_JoinTeam( %d ) - invalid team index.\n", team );
		return false;
	}

	if ( team == TEAM_UNASSIGNED )
	{
		team = mp->SelectDefaultTeam();

		if ( team == TEAM_UNASSIGNED )
		{
			ClientPrint( this, HUD_PRINTTALK, "#All_Teams_Full" );
			team = TEAM_SPECTATOR;
		}
	}

	if ( team == iOldTeam )
		return true;

	if ( team == TEAM_SPECTATOR )
	{
		if ( !mp_allowspectators.GetInt() && !IsHLTV() )
		{
			ClientPrint( this, HUD_PRINTTALK, "#Cannot_Be_Spectator" );
			ShowViewPortPanel( PANEL_TEAM );
			return false;
		}

		ChangeTeam( TEAM_SPECTATOR );

		return true;
	}
	
	ChangeTeam( team );

	return true;
}

#if defined ( SDK_USE_PRONE )
void CSDKPlayer::InitProne( void )
{
	m_Shared.SetProne( false, true );
	m_bUnProneToDuck = false;
}
#endif // SDK_USE_PRONE

#if defined ( SDK_USE_SPRINTING )
void CSDKPlayer::InitSprinting(void)
{
	m_Shared.SetSprinting(false);
}

bool CSDKPlayer::CanSprint()
{
	return (
		!(m_Local.m_bDucked && !m_Local.m_bDucking) &&
		(GetWaterLevel() != 3));
}
#endif // SDK_USE_SPRINTING

//------------------------------------------------------------------------------------------------
// Player state management.
//------------------------------------------------------------------------------------------------
void CSDKPlayer::State_Transition( SDKPlayerState newState )
{
	State_Leave();
	State_Enter( newState );
}

void CSDKPlayer::State_Enter( SDKPlayerState newState )
{
	m_iPlayerState = newState;
	m_pCurStateInfo = State_LookupInfo( newState );

	if ( SDK_ShowStateTransitions.GetInt() == -1 || SDK_ShowStateTransitions.GetInt() == entindex() )
	{
		if ( m_pCurStateInfo )
			Msg( "ShowStateTransitions: entering '%s'\n", m_pCurStateInfo->m_pStateName );
		else
			Msg( "ShowStateTransitions: entering #%d\n", newState );
	}
	
	if ( m_pCurStateInfo && m_pCurStateInfo->pfnEnterState )
		(this->*m_pCurStateInfo->pfnEnterState)();
}

void CSDKPlayer::State_Leave()
{
	if ( m_pCurStateInfo && m_pCurStateInfo->pfnLeaveState )
	{
		(this->*m_pCurStateInfo->pfnLeaveState)();
	}
}

void CSDKPlayer::State_PreThink()
{
	if ( m_pCurStateInfo && m_pCurStateInfo->pfnPreThink )
	{
		(this->*m_pCurStateInfo->pfnPreThink)();
	}
}

CSDKPlayerStateInfo* CSDKPlayer::State_LookupInfo( SDKPlayerState state )
{
	static CSDKPlayerStateInfo playerStateInfos[] =
	{
		{ STATE_ACTIVE,			"STATE_ACTIVE",			&CSDKPlayer::State_Enter_ACTIVE, NULL, &CSDKPlayer::State_PreThink_ACTIVE },
		{ STATE_WELCOME,		"STATE_WELCOME",		&CSDKPlayer::State_Enter_WELCOME, NULL, &CSDKPlayer::State_PreThink_WELCOME },
		{ STATE_DEATH_ANIM,		"STATE_DEATH_ANIM",		&CSDKPlayer::State_Enter_DEATH_ANIM,	NULL, &CSDKPlayer::State_PreThink_DEATH_ANIM },
		{ STATE_OBSERVER_MODE,	"STATE_OBSERVER_MODE",	&CSDKPlayer::State_Enter_OBSERVER_MODE,	NULL, &CSDKPlayer::State_PreThink_OBSERVER_MODE }
	};

	for ( int i=0; i < ARRAYSIZE( playerStateInfos ); i++ )
	{
		if ( playerStateInfos[i].m_iPlayerState == state )
			return &playerStateInfos[i];
	}

	return NULL;
}

void CSDKPlayer::PhysObjectSleep()
{
	IPhysicsObject *pObj = VPhysicsGetObject();
	if ( pObj )
		pObj->Sleep();
}

void CSDKPlayer::PhysObjectWake()
{
	IPhysicsObject *pObj = VPhysicsGetObject();
	if ( pObj )
		pObj->Wake();
}

void CSDKPlayer::State_Enter_WELCOME()
{
	SetMoveType( MOVETYPE_NONE );
	AddSolidFlags( FSOLID_NOT_SOLID );

	PhysObjectSleep();

	if ( IsBot() )
	{
		CCommand args;
		args.Tokenize( "joingame" );
		ClientCommand( args );
	}
	else
	{
		const ConVar *hostname = cvar->FindVar( "hostname" );
		const char *title = (hostname) ? hostname->GetString() : "MESSAGE OF THE DAY";

		KeyValues *data = new KeyValues("data");
		data->SetString( "title", title );
		data->SetString( "type", "1" );
		data->SetString( "msg",	"motd" );
		data->SetString( "cmd", "joingame" );

		ShowViewPortPanel( PANEL_INFO, true, data );

		data->deleteThis();
	}	
}

void CSDKPlayer::MoveToNextIntroCamera()
{
	m_pIntroCamera = gEntList.FindEntityByClassname( m_pIntroCamera, "point_viewcontrol" );

	if(!m_pIntroCamera)
		m_pIntroCamera = gEntList.FindEntityByClassname(m_pIntroCamera, "point_viewcontrol");

	CBaseEntity *Target = NULL;
	
	if( m_pIntroCamera )
	{
		Target = gEntList.FindEntityByName( NULL, STRING(m_pIntroCamera->m_target) );
	}

	if(!m_pIntroCamera)
		m_pIntroCamera = gEntList.FindEntityByClassname(m_pIntroCamera, "info_player_terrorist");

	SetViewOffset( vec3_origin );
	UTIL_SetSize( this, vec3_origin, vec3_origin );

	if( !Target )
	{
		if ( m_pIntroCamera.IsValid() )
			SetAbsOrigin( m_pIntroCamera->GetAbsOrigin() + VEC_VIEW );

		SetAbsAngles( QAngle( 0, 0, 0 ) );
		
		m_pIntroCamera = NULL;
		return;
	}
	
	Vector vCamera = Target->GetAbsOrigin() - m_pIntroCamera->GetAbsOrigin();
	Vector vIntroCamera = m_pIntroCamera->GetAbsOrigin();
	
	VectorNormalize( vCamera );
		
	QAngle CamAngles;
	VectorAngles( vCamera, CamAngles );

	SetAbsOrigin( vIntroCamera );
	SetAbsAngles( CamAngles );
	SnapEyeAngles( CamAngles );
	m_fIntroCamTime = gpGlobals->curtime + 6;
}

void CSDKPlayer::State_PreThink_WELCOME()
{
	if( m_pIntroCamera && (gpGlobals->curtime >= m_fIntroCamTime) )
	{
		MoveToNextIntroCamera();
	}
}

void CSDKPlayer::State_Enter_DEATH_ANIM()
{
	if ( HasWeapons() )
	{
		PackDeadPlayerItems();
	}

	m_flDeathTime = gpGlobals->curtime;

	StartObserverMode( OBS_MODE_DEATHCAM );

	RemoveEffects( EF_NODRAW );
}

void CSDKPlayer::State_PreThink_DEATH_ANIM()
{
	if ( GetFlags() & FL_ONGROUND )
	{
		float flForward = GetAbsVelocity().Length() - 20;
		if (flForward <= 0)
		{
			SetAbsVelocity( vec3_origin );
		}
		else
		{
			Vector vAbsVel = GetAbsVelocity();
			VectorNormalize( vAbsVel );
			vAbsVel *= flForward;
			SetAbsVelocity( vAbsVel );
		}
	}

	if ( gpGlobals->curtime >= (m_flDeathTime + SDK_PLAYER_DEATH_TIME ) )
	{
		m_lifeState = LIFE_DEAD;
		StopAnimation();
		AddEffects( EF_NOINTERP );

		if ( GetMoveType() != MOVETYPE_NONE && (GetFlags() & FL_ONGROUND) )
			SetMoveType( MOVETYPE_NONE );
	}

	if ( m_lifeState == LIFE_DEAD )
	{
		State_Transition( STATE_ACTIVE );
	}
}

void CSDKPlayer::State_Enter_OBSERVER_MODE()
{
	m_iObserverLastMode = OBS_MODE_ROAMING;

	if( m_hObserverTarget == NULL )
	{
		CheckObserverSettings();
	}

	CTeam *pTeam = GetGlobalTeam( GetTeamNumber() );

	CBasePlayer *pPlayer;
	Vector localOrigin = GetAbsOrigin();
	Vector targetOrigin;
	float flMinDist = FLT_MAX;
	float flDist;

	for ( int i=0;i<pTeam->GetNumPlayers();i++ )
	{
		pPlayer = pTeam->GetPlayer(i);

		if ( !pPlayer )
			continue;

		if ( !IsValidObserverTarget(pPlayer) )
			continue;

		targetOrigin = pPlayer->GetAbsOrigin();

		flDist = ( targetOrigin - localOrigin ).Length();

		if ( flDist < flMinDist )
		{
			m_hObserverTarget.Set( pPlayer );
			flMinDist = flDist;
		}
	}

	StartObserverMode( m_iObserverLastMode );
	PhysObjectSleep();
}

void CSDKPlayer::State_PreThink_OBSERVER_MODE()
{
	if (  m_iObserverMode == OBS_MODE_IN_EYE || m_iObserverMode == OBS_MODE_CHASE )
	{
		if ( GetTeamNumber() != TEAM_SPECTATOR )
			ValidateCurrentObserverTarget();
		else
		{
			if ( !IsValidObserverTarget( m_hObserverTarget.Get() ) )
			{
				CBaseEntity * target = FindNextObserverTarget( false );
				if ( target )
				{
					SetObserverTarget( target );	
				}
				else
				{
					ForceObserverMode( OBS_MODE_ROAMING );
				}
			}
		}
	}
}

void CSDKPlayer::State_Enter_ACTIVE()
{
	SetMoveType( MOVETYPE_WALK );
	RemoveSolidFlags( FSOLID_NOT_SOLID );
    m_Local.m_iHideHUD = 0;
	PhysObjectWake();

	Spawn();
}

void CSDKPlayer::State_PreThink_ACTIVE()
{
}

int CSDKPlayer::GetPlayerStance()
{
#if defined ( SDK_USE_PRONE )
	if (m_Shared.IsProne() || ( m_Shared.IsGoingProne() || m_Shared.IsGettingUpFromProne() ))
		return PINFO_STANCE_PRONE;
#endif

#if defined ( SDK_USE_SPRINTING )
	if (IsSprinting())
		return PINFO_STANCE_SPRINTING;
#endif
	if (m_Local.m_bDucking)
		return PINFO_STANCE_DUCKING;
	else
		return PINFO_STANCE_STANDING;
}

void CSDKPlayer::NoteWeaponFired( void )
{
	Assert( m_pCurrentCommand );
	if( m_pCurrentCommand )
	{
		m_iLastWeaponFireUsercmd = m_pCurrentCommand->command_number;
	}
}

bool CSDKPlayer::WantsLagCompensationOnEntity( const CBasePlayer *pPlayer, const CUserCmd *pCmd, const CBitVec<MAX_EDICTS> *pEntityTransmitBits ) const
{
	if ( !( pCmd->buttons & IN_ATTACK ) && (pCmd->command_number - m_iLastWeaponFireUsercmd > 5) )
		return false;

	return BaseClass::WantsLagCompensationOnEntity( pPlayer, pCmd, pEntityTransmitBits );
}

//-----------------------------------------------------------------------------
// PlayerUse - handles +USE, finds entity, calls AcceptInput("Use")
//-----------------------------------------------------------------------------
void CSDKPlayer::PlayerUse(void)
{
	// Was use pressed or released?
	if (!((m_nButtons | m_afButtonPressed | m_afButtonReleased) & IN_USE))
		return;

	// Tracker 3926:  We can't +USE something if we're climbing a ladder
	if (GetMoveType() == MOVETYPE_LADDER)
	{
		return;
	}

	if (m_flTimeUseSuspended > gpGlobals->curtime)
	{
		// Something has temporarily stopped us being able to USE things.
		// Obviously, this should be used very carefully.(sjb)
		return;
	}

	if (m_afButtonPressed & IN_USE)
	{
		// Signal that we want to play the deny sound, unless the user is +USEing on a ladder!
		// The sound is emitted in ItemPostFrame, since that occurs after GameMovement::ProcessMove which
		// lets the ladder code unset this flag.
		m_bPlayUseDenySound = true;
	}

	BaseClass::PlayerUse();
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CSDKPlayer::PickupObject(CBaseEntity* pObject, bool bLimitMassAndSize)
{
	if (!pObject)
		return;

	if (GetGroundEntity() == pObject)
		return;

	IPhysicsObject* pPhysics = pObject->VPhysicsGetObject();
	if (!pPhysics)
		return;

	if (bLimitMassAndSize)
	{
		float flMass = pPhysics->GetMass();
		if (flMass > 35.0f)
			return;

		EmitSound("HL2Player.Use");

		const Vector& size = pObject->CollisionProp()->OBBSize();
		if (size.x > 128.0f || size.y > 128.0f || size.z > 128.0f)
			return;
	}

	if (pObject->HasNPCsOnIt())
		return;

	PlayerPickupObject(this, pObject);
}

//-----------------------------------------------------------------------------
bool CSDKPlayer::IsHoldingEntity(CBaseEntity* pEnt)
{
	return PlayerPickupControllerIsHoldingEntity(m_hUseEntity, pEnt);
}

//-----------------------------------------------------------------------------
float CSDKPlayer::GetHeldObjectMass(IPhysicsObject* pHeldObject)
{
	return PlayerPickupGetHeldObjectMass(m_hUseEntity, pHeldObject);
}

//-----------------------------------------------------------------------------
void CSDKPlayer::ForceDropOfCarriedPhysObjects(CBaseEntity* pOnlyIfHoldingThis)
{
	if (PhysIsInCallback())
	{
		variant_t value;
		g_EventQueue.AddEvent(this, "ForceDropPhysObjects", value, 0.01f, pOnlyIfHoldingThis, this);
		return;
	}

	ClearUseEntity();
}

void CSDKPlayer::ItemPostFrame(void)
{
	if (m_hUseEntity == NULL)
	{
		BaseClass::ItemPostFrame();
	}
	else
	{
		CalcViewModelView(EyePosition(), EyeAngles());

		CBaseViewModel* pVM = GetViewModel();
		if (pVM)
		{
			pVM->StudioFrameAdvance();
			pVM->DispatchAnimEvents(pVM->GetOwningWeapon());
		}

		m_hUseEntity->Use(this, this, USE_SET, 2);
	}

	if (m_bPlayUseDenySound)
	{
		m_bPlayUseDenySound = false;
	}
}