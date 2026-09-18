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
extern cvar_t *g_jkgMovement;	// custom player movement feel + weapon-fire cadence
extern cvar_t *g_jkgArmor;		// separate MAX_ARMOR system (armor no longer clamped to max health)
extern cvar_t *g_jkgCombat;		// custom damage tables / pain timing / hit locations
extern cvar_t *g_jkgCamera;		// weapon-fire camera kickback
extern cvar_t *g_jkgHUD;		// custom HUD / view / weapon-draw tweaks
extern cvar_t *g_jkgGunSwayAmount;	// first-person weapon sway strength (degrees per degree turned)
extern cvar_t *g_jkgGunSwayReturn;	// weapon sway return speed (lower = slower recenter)
extern cvar_t *g_jkgDebugProjectile;	// projectile spawn debug (0=off, 1=server, 2=+client, 3=+NPC)
extern cvar_t *g_jkgDebugHitboxes;	// draw NPC entity AABBs used for shot collision (requires cheats)

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

#endif	// JKG_LOCAL_H
