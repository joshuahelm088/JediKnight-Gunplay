/*
===========================================================================
JKGunplay mod layer

This header is the single entry point for the "JKGunplay" custom gameplay
layer that sits on top of the stock OpenJK (JK2 single-player) code.

Design rules (see also jkg_tuning.h):
  1. Stock OpenJK files stay as close to pristine as possible. The only edit
     to a stock file should be a small, clearly-marked "JKG HOOK" that routes
     to the custom code when the relevant toggle is on.
  2. Custom *logic* that can stand alone lives in dedicated *_JKG.cpp files.
  3. Custom logic that is woven into stock functions is wrapped in a runtime
     guard: if ( JKG_<SYSTEM> ) { custom } else { stock }.
  4. Custom numeric tuning lives in jkg_tuning.h.

Every subsystem is gated by the master toggle (g_jkgplay) AND its own
per-system toggle, so you can A/B test any subsystem independently, or flip
the whole mod off to get stock behavior. All default to ON.
===========================================================================
*/

#ifndef JKG_LOCAL_H
#define JKG_LOCAL_H

#include "statindex.h"

// Master toggle for the entire JKGunplay layer.
extern cvar_t *g_jkgplay;

// Per-system toggles.
extern cvar_t *g_jkgAI;			// custom NPC AI (stormtrooper, probe, sentry, reactions, spawn scaling)
extern cvar_t *g_jkgWeapons;	// custom weapon fire behavior + tuning
extern cvar_t *g_jkgMovement;	// NPC speed ramp; player and NPC ground anims follow speed
extern cvar_t *g_jkgNpcAccel;	// NPC speed-up ramp rate (units/sec)
extern cvar_t *g_jkgNpcDecel;	// NPC slow-down ramp rate (units/sec)
extern cvar_t *g_jkgNpcStopDecel;	// decel for goal approach cap v=sqrt(2*a*d); independent of ramp feel
extern cvar_t *g_jkgNpcTurnRate;	// max NPC moveDir heading change (deg/sec)
extern cvar_t *g_jkgNpcSpeedScale;	// multiplier on NPC desired walk/run speed
extern cvar_t *g_jkgNpcAnimMinScale;	// floor on walk/run anim playback scale; 0 = no floor
extern cvar_t *g_jkgLocomotionStandSpeed;	// horizontal speed at or below this stays on stand
extern cvar_t *g_jkgLocomotionBlend;	// ms to crossfade when the locomotion anim changes
extern cvar_t *g_jkgLocomotionRunThreshold;	// walk anim until xySpeed exceeds walk cap times this (mid-speed band)
extern cvar_t *g_jkgLocomotionRunRate;	// extra playback multiplier on run locomotion anims only
extern cvar_t *g_jkgLocomotionWalkRate;	// extra playback multiplier on walk locomotion anims only
extern cvar_t *g_jkgDebugNpcMove;	// NPC locomotion debug (0=off, 1=brake events, 2=verbose)
extern cvar_t *g_jkgDebugAimCone;	// NPC weapon spread cone (0=off, 1=draw, 2=+throttled print)
extern cvar_t *g_jkgDebugNpcState;	// NPC AI state marker (0=off, 1=draw, 2=+throttled print)
extern cvar_t *g_jkgDebugNpcSpeech;	// NPC alert-bark marker (0=off, 1=draw, 2=+print)
extern cvar_t *g_jkgNpcFirstAlert;	// 1 = each NPC plays an alert the first time they notice the player
extern cvar_t *g_jkgNoCombatPoints;	// 1 = stormtrooper commander skips point_combat selection (direct hunt/scout)
extern cvar_t *g_jkgCombatMove;		// 1 = generic range/hunt/strafe combat movement (not combat points)
extern cvar_t *g_jkgCombatIdealRangeMin;	// fallback min stand-off from enemy (class cfg can override)
extern cvar_t *g_jkgCombatIdealRangeMax;	// fallback max stand-off from enemy (class cfg can override)
extern cvar_t *g_jkgCombatRangeBand;	// extra slack when min==max (old single-range hysteresis)
extern cvar_t *g_jkgCombatMoveDelay;	// ms to wait before close/back-up starts
extern cvar_t *g_jkgCombatStepDist;	// how far each close/back-up step is placed
extern cvar_t *g_jkgCombatStrafeDist;	// lateral shuffle distance
extern cvar_t *g_jkgCombatStrafeTime;	// ms of a shuffle burst
extern cvar_t *g_jkgCombatStrafePause;	// ms of standing between shuffles
extern cvar_t *g_jkgCombatHuntCheatMs;	// ms after LOS loss to still path toward live enemy pos
extern cvar_t *g_jkgCombatAimDelay;	// fallback ms ADS windup before NPC fires (class aimDelay key)
extern cvar_t *g_jkgBowcasterAimDelaySingle;	// NPC bowcaster aim ms before 1 bolt
extern cvar_t *g_jkgBowcasterAimDelayTriple;	// NPC bowcaster aim ms before 3 bolts
extern cvar_t *g_jkgBowcasterAimDelayFive;	// NPC bowcaster aim ms before 5 bolts
extern cvar_t *g_jkgBowcasterRepause;	// NPC bowcaster ms after volley before next aim cycle
extern cvar_t *g_jkgArmor;		// separate MAX_ARMOR system (armor no longer clamped to max health)
extern cvar_t *g_jkgMaxArmor;	// player shield cap when g_jkgArmor is on (STAT_MAX_ARMOR)
extern cvar_t *g_jkgShieldStationGive;	// shield points per tick (stock 4; default 12 = 3x)
extern cvar_t *g_jkgShieldStationTickMs;	// ms between ticks (stock 100; default 33 = 3x faster)
extern cvar_t *g_jkgCombat;		// custom damage tables / pain timing / hit locations
extern cvar_t *g_jkgPlayerPainChance;	// DF2-style 3p hit-react chance scale (damage * value); 0 disables
extern cvar_t *g_jkgCamera;		// weapon-fire camera kickback
extern cvar_t *g_jkgHUD;		// custom HUD / view / weapon-draw tweaks
extern cvar_t *g_jkgHitTint;	// DF2-style full-screen hit tint (0=off)
extern cvar_t *g_jkgHitTintHealthScale;	// red channel per health damage point
extern cvar_t *g_jkgHitTintShieldScale;	// green channel per armor damage point
extern cvar_t *g_jkgHitTintDecay;	// tint units faded per second
extern cvar_t *g_jkgHitTintMax;	// max stacked tint per channel
extern cvar_t *g_jkgHitTintAlphaScale;	// overlay alpha = strength * this (DF2 ApplyTint ~0.5)
extern cvar_t *g_jkgPickupTintAlphaScale;	// pickup overlay; stacks compress above one-pickup mix
extern cvar_t *g_jkgHitTintHealthR;
extern cvar_t *g_jkgHitTintHealthG;
extern cvar_t *g_jkgHitTintHealthB;
extern cvar_t *g_jkgHitTintShieldR;
extern cvar_t *g_jkgHitTintShieldG;
extern cvar_t *g_jkgHitTintShieldB;
extern cvar_t *g_jkgDamageBlobScale;	// radius multiplier (0.34 = 66% smaller)
extern cvar_t *g_jkgDamageBlobHealthSize;
extern cvar_t *g_jkgDamageBlobShieldSize;
extern cvar_t *g_jkgDamageBlobArmorThreshold;
extern cvar_t *g_jkgDamageBlobTime;	// blob fade duration (ms)
extern cvar_t *g_jkgDamageBlobHealthR;
extern cvar_t *g_jkgDamageBlobHealthG;
extern cvar_t *g_jkgDamageBlobHealthB;
extern cvar_t *g_jkgDamageBlobShieldR;
extern cvar_t *g_jkgDamageBlobShieldG;
extern cvar_t *g_jkgDamageBlobShieldB;
extern cvar_t *g_jkgGunSwayAmount;	// first-person weapon sway strength (degrees per degree turned)
extern cvar_t *g_jkgGunSwayReturn;	// weapon sway return speed (lower = slower recenter)
extern cvar_t *g_jkgDebugProjectile;	// projectile spawn debug (0=off, 1=server, 2=+client, 3=+NPC)
extern cvar_t *g_jkgProjectileAabbHits;	// 1=player missiles use NPC shot hulls; enemy missiles and the player stay Ghoul2 mesh
extern cvar_t *g_jkgNpcHitboxScale;	// default horizontal XY bbox scale for NPCs (1.0 = stock)
extern cvar_t *g_jkgBurstPistolShots;	// shots per JKG officer pistol (WP_BLASTER_PISTOL) burst
extern cvar_t *g_jkgBurstShotDelay;	// ms between shots within an E-11 burst
extern cvar_t *g_jkgBurstPistolShotDelay;	// ms between shots within an officer pistol burst
extern cvar_t *g_jkgBurstPauseSingle;	// ms after a 1-shot E-11 burst
extern cvar_t *g_jkgBurstPauseDouble;	// ms after a 2-shot E-11 burst
extern cvar_t *g_jkgBurstPauseTriple;	// ms after a 3-shot E-11 burst (most common)
extern cvar_t *g_jkgBurstPistolPause;	// ms after a pistol burst before the next burst starts
extern cvar_t *g_jkgBryarVelocity;	// player Bryar bolt speed (DF2 +bryarbolt 4/6 of stock blaster)
extern cvar_t *g_jkgBlasterVelocity;	// player E-11 bolt speed (stock JKO 2300; DF2 +stlaser)
extern cvar_t *g_jkgBlasterNpcVelocity;	// NPC E-11 bolt speed (DF2 +elaser)
extern cvar_t *g_jkgBlasterPistolNpcVelocity;	// NPC blaster pistol (DF2 +ebolt)
extern cvar_t *g_jkgBowcasterVelocity;	// player bowcaster (DF2 +crossbowbolt)
extern cvar_t *g_jkgBowcasterNpcVelocity;	// NPC bowcaster (DF2 +ebow)
extern cvar_t *g_jkgRepeaterVelocity;	// repeater primary (DF2 +repeaterball)
extern cvar_t *g_jkgBryarDamageDecay;	// player Bryar; DF2 15 scaled to feel-tested E-11 4
extern cvar_t *g_jkgBryarMinDamage;	// player Bryar decay floor
extern cvar_t *g_jkgBlasterDamageDecay;	// player E-11; playtested 4 (DF2 rate 10)
extern cvar_t *g_jkgBlasterMinDamage;	// player E-11 decay floor
extern cvar_t *g_jkgBlasterNpcDamageDecay;	// NPC E-11; same DF2 rate as player E-11
extern cvar_t *g_jkgBlasterNpcMinDamage;	// NPC E-11 decay floor
extern cvar_t *g_jkgBlasterPistolNpcDamageDecay;	// NPC pistol; DF2 15 scaled like Bryar
extern cvar_t *g_jkgBlasterPistolNpcMinDamage;	// NPC pistol decay floor
extern cvar_t *g_jkgBlasterDamage;	// player E-11 start damage (DF2 +stlaser 30)
extern cvar_t *g_jkgDamageLog;	// 1 = print damage dealt to the player and NPCs
extern cvar_t *g_jkgBryarTapFireTime;	// min ms between player Bryar tap-fires; also uncharged alt recovery (0.5x fireTime)
extern cvar_t *g_jkgBryarChargeFireTime;	// ms after a fully charged Bryar alt (1.5x fireTime); lerps from tap time by charge level

typedef enum {
	JKG_FIREMODE_DEFAULT = 0,
	JKG_FIREMODE_BURST = 1,
	JKG_FIREMODE_SINGLE = 2,
} jkgFireMode_t;

float JKG_GetNpcHitboxScale( const gentity_t *ent );	// per-NPC override when client->jkgHitboxScale > 0
void JKG_GetNpcShotAbsBounds( const gentity_t *ent, vec3_t absmin, vec3_t absmax );
void JKG_MissileClipToNpcShotHitboxes( gentity_t *missile, const vec3_t start, const vec3_t end, int passEntityNum, int contentmask, trace_t *tr );

void JKG_SetPlayerPainChanceDamage( int damage );
void JKG_PlayerTryPainAnim( gentity_t *self, gentity_t *other, vec3_t point, int damage, int mod, int hitLoc );

void JKG_ApplyNpcBurstFireMode( gentity_t *ent );
qboolean JKG_NpcBurstShootThink( void );
qboolean JKG_NpcBowcasterShootThink( void );
void JKG_NpcBowcasterMaintainAttack( gentity_t *ent, usercmd_t *ucmd );
int JKG_NpcBowcasterVolleyShotsForFire( gentity_t *ent );

int JKG_NpcScaleDesiredSpeed( int speed );
void JKG_NPCApplyStopSlowdown( gentity_t *ent );
void JKG_NpcCombatDesiredSpeed( gentity_t *ent, usercmd_t *ucmd );
void JKG_NpcApplyMovementCoast( gentity_t *ent, usercmd_t *ucmd );
void JKG_NPCRampSpeed( gentity_t *ent, int msec );
void JKG_NpcApplyMoveDir( gentity_t *self, usercmd_t *cmd, vec3_t dir );
float JKG_NpcLocomotionAnimScale( gentity_t *ent, int anim );
qboolean JKG_ShouldCoastLocomotion( gentity_t *ent, playerState_t *ps, float xySpeed );
qboolean JKG_LocomotionUseWalkAnim( gentity_t *ent, const usercmd_t *cmd, float xySpeed );
int JKG_LocomotionBlendTime( void );

struct centity_s;

// A subsystem is active only if the master toggle AND its own toggle are on.
// Pointers are null-checked so this is safe to call before G_InitCvars runs
// (returns false -> stock behavior until the cvars are registered).
#define JKG_ON(sys)		( g_jkgplay && g_jkgplay->integer && (sys) && (sys)->integer )

#define JKG_AI			JKG_ON( g_jkgAI )
#define JKG_WEAPONS		JKG_ON( g_jkgWeapons )
#define JKG_MOVEMENT	JKG_ON( g_jkgMovement )
#define JKG_ARMOR		JKG_ON( g_jkgArmor )
#define JKG_COMBAT		JKG_ON( g_jkgCombat )
#define JKG_CAMERA		JKG_ON( g_jkgCamera )
#define JKG_HUD			JKG_ON( g_jkgHUD )

// Armor clamp limit: separate max-armor stat when JKG_ARMOR is on, else stock max-health cap.
#define JKG_PS_MAX_ARMOR(ps)	((ps)->stats[(JKG_ARMOR) ? STAT_MAX_ARMOR : STAT_MAX_HEALTH])

// Registers all g_jkg* cvars. Called from G_InitCvars().
void JKG_RegisterCvars( void );

// Client hit feedback (cgame); gated by JKG_HUD / g_jkgHitTint.
void JKG_HitTintAdd( int healthDmg, int armorDmg );
void JKG_HitTintDraw( void );
void JKG_PickupTintAdd( void );

#define JKG_COMBAT_CLASS_NAME_LEN	32

typedef struct jkgCombatMoveParms_s {
	int	rangeMin;
	int	rangeMax;
	int	rangeBand;
	int	stepDist;
	int	strafeDist;
	int	strafeTime;
	int	strafePause;
	int	huntCheatMs;
	int	moveDelay;
} jkgCombatMoveParms_t;

void JKG_LoadCombatClasses( void );
void JKG_GetCombatMoveParms( const gentity_t *ent, jkgCombatMoveParms_t *out );
int JKG_GetCombatAimDelay( const gentity_t *ent );
void JKG_ApplyNpcCombatClassAimDelay( gentity_t *ent );
void JKG_AdjustNpcShotTimeForFireDelay( gentity_t *ent );

// Sets ps.stats[STAT_MAX_ARMOR] from g_jkgMaxArmor when JKG_ARMOR is on.
void JKG_ApplyMaxArmor( gclient_t *client );

// g_jkgMaxArmor value clamped to non-negative (0 if cvar not registered).
int JKG_MaxArmorCap( void );

// Shield power converter recharge timing (stock: 4 points every 100 ms).
int JKG_ShieldStationGivePerTick( void );
int JKG_ShieldStationTickMs( void );

// Min delay between Bryar tap-fires (0 = no extra cap).
int JKG_BryarBoltVelocity( void );
int JKG_BlasterBoltVelocity( void );
int JKG_BlasterNpcBoltVelocity( void );
int JKG_BlasterPistolNpcBoltVelocity( void );
int JKG_BowcasterBoltVelocity( void );
int JKG_BowcasterNpcBoltVelocity( void );
int JKG_RepeaterBoltVelocity( void );
int JKG_BlasterBoltVelocityFor( const gentity_t *ent );
int JKG_BryarPistolBoltVelocityFor( const gentity_t *ent );
int JKG_BowcasterBoltVelocityFor( const gentity_t *ent );
void JKG_GetBlasterDecay( const gentity_t *ent, float *rate, float *floorDamage );
void JKG_GetBryarPistolDecay( const gentity_t *ent, float *rate, float *floorDamage );
int JKG_BlasterDamage( void );
int JKG_BryarTapFireTime( void );

// Recovery after Bryar alt: tap-fire time at charge 1, charge-fire time at charge 5.
int JKG_BryarChargeFireTime( void );
int JKG_BryarChargeRecoveryTime( int chargeCount );
qboolean JKG_BryarChargeLocked( const gentity_t *ent );
void JKG_BryarArmChargeLock( gentity_t *ent, int recovery );

// DF2-style time decay for Bryar and E-11 bolts fired by the JKG weapon code.
// rate and floor are stored on the missile (wait / random). Other bryar_proj
// shots (probe, sentry) leave those at 0 and are left alone.
void JKG_ArmEnergyBoltDecay( gentity_t *missile, float rate, float floorDamage );
void JKG_DecayEnergyBoltDamage( gentity_t *missile );

qboolean JKG_NpcBlasterBolt( const gentity_t *ent );
qboolean JKG_NpcBlasterPistolBolt( const gentity_t *ent );

// Weapon fire dispatch (implemented in wp_*_JKG.cpp).
void WP_FireBryarPistol_JKG( gentity_t *ent, qboolean alt_fire );
void WP_FireBlaster_JKG( gentity_t *ent, qboolean alt_fire );
void WP_FireRepeater_JKG( gentity_t *ent, qboolean alt_fire );
void WP_FireBowcaster_JKG( gentity_t *ent, qboolean alt_fire );
gentity_t *WP_FireThermalDetonator_JKG( gentity_t *ent, qboolean alt_fire );

// Projectile spawn debug logging (jkg_debug_projectile.cpp).
void JKG_DebugProjectile_MuzzlePoint( gentity_t *ent, const char *source, const vec3_t muzzlePoint, int cacheAge );
void JKG_DebugProjectile_TraceSetStart( gentity_t *ent, const vec3_t before, const vec3_t after, float traceFraction );
void JKG_DebugProjectile_CreateMissile( gentity_t *owner, gentity_t *missile, const vec3_t org, float vel );
void JKG_DebugProjectile_ClientMuzzle( gentity_t *ent, const char *source, const vec3_t muzzlePoint, const vec3_t viewOrg );
void JKG_DebugProjectile_ClientRender( struct centity_s *cent );

float JKG_GetNpcWeaponSpreadDegrees( const gentity_t *ent );
void JKG_NpcPenalizeAimOnHit( gentity_t *self, int damage, gentity_t *attacker );
void JKG_DebugDrawNpcAimCone( gentity_t *ent );
void JKG_DebugDrawNpcState( gentity_t *ent );
void JKG_DebugDrawNpcSpeech( gentity_t *ent );
void JKG_NpcFirstAlert( gentity_t *self, gentity_t *enemy );
void JKG_NpcClearFirstAlert( gentity_t *self );

qboolean JKG_ST_CombatMoveEnabled( void );
qboolean JKG_ST_CombatMoveThink( qboolean canSee, float distSq );
void JKG_ST_ApplyCombatWalk( void );

#endif	// JKG_LOCAL_H
