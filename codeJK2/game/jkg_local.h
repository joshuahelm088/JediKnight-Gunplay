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
extern cvar_t *g_jkgMovement;	// NPC locomotion ramp/blend; player pmove is stock JK2
extern cvar_t *g_jkgNpcAccel;	// NPC speed-up ramp rate (units/sec)
extern cvar_t *g_jkgNpcDecel;	// NPC slow-down ramp rate (units/sec)
extern cvar_t *g_jkgNpcStopDecel;	// decel for goal approach cap v=sqrt(2*a*d); independent of ramp feel
extern cvar_t *g_jkgNpcTurnRate;	// max NPC moveDir heading change (deg/sec)
extern cvar_t *g_jkgNpcSpeedScale;	// multiplier on NPC desired walk/run speed
extern cvar_t *g_jkgNpcAnimMinScale;	// floor on NPC walk/run anim playback scale
extern cvar_t *g_jkgDebugNpcMove;	// NPC locomotion debug (0=off, 1=brake events, 2=verbose)
extern cvar_t *g_jkgDebugAimCone;	// NPC weapon spread cone (0=off, 1=draw, 2=+throttled print)
extern cvar_t *g_jkgArmor;		// separate MAX_ARMOR system (armor no longer clamped to max health)
extern cvar_t *g_jkgCombat;		// custom damage tables / pain timing / hit locations
extern cvar_t *g_jkgCamera;		// weapon-fire camera kickback
extern cvar_t *g_jkgHUD;		// custom HUD / view / weapon-draw tweaks
extern cvar_t *g_jkgGunSwayAmount;	// first-person weapon sway strength (degrees per degree turned)
extern cvar_t *g_jkgGunSwayReturn;	// weapon sway return speed (lower = slower recenter)
extern cvar_t *g_jkgDebugProjectile;	// projectile spawn debug (0=off, 1=server, 2=+client, 3=+NPC)
extern cvar_t *g_jkgProjectileAabbHits;	// 1=player missiles use NPC shot hulls; enemy missiles and the player stay Ghoul2 mesh
extern cvar_t *g_jkgNpcHitboxScale;	// default horizontal XY bbox scale for NPCs (1.0 = stock)
extern cvar_t *g_jkgBurstShots;		// shots per JKG E-11 (WP_BLASTER) burst
extern cvar_t *g_jkgBurstPistolShots;	// shots per JKG officer pistol (WP_BLASTER_PISTOL) burst
extern cvar_t *g_jkgBurstShotDelay;	// ms between shots within a burst (blaster and pistol)
extern cvar_t *g_jkgBurstPause;		// ms after an E-11 burst before the next burst starts
extern cvar_t *g_jkgBurstPistolPause;	// ms after a pistol burst before the next burst starts

typedef enum {
	JKG_FIREMODE_DEFAULT = 0,
	JKG_FIREMODE_BURST = 1,
	JKG_FIREMODE_SINGLE = 2,
} jkgFireMode_t;

float JKG_GetNpcHitboxScale( const gentity_t *ent );	// per-NPC override when client->jkgHitboxScale > 0
void JKG_GetNpcShotAbsBounds( const gentity_t *ent, vec3_t absmin, vec3_t absmax );
void JKG_MissileClipToNpcShotHitboxes( gentity_t *missile, const vec3_t start, const vec3_t end, int passEntityNum, int contentmask, trace_t *tr );

void JKG_ApplyNpcBurstFireMode( gentity_t *ent );
qboolean JKG_NpcBurstShootThink( void );

int JKG_NpcScaleDesiredSpeed( int speed );
void JKG_NPCApplyStopSlowdown( gentity_t *ent );
void JKG_NpcCombatDesiredSpeed( gentity_t *ent, usercmd_t *ucmd );
void JKG_NpcApplyMovementCoast( gentity_t *ent, usercmd_t *ucmd );
void JKG_NPCRampSpeed( gentity_t *ent, int msec );
void JKG_NpcApplyMoveDir( gentity_t *self, usercmd_t *cmd, vec3_t dir );
float JKG_NpcLocomotionAnimScale( gentity_t *ent, int anim );

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

#endif	// JKG_LOCAL_H
