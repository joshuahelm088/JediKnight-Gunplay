/*
===========================================================================
JKGunplay mod layer - weapon tuning overrides

Central home for JKGunplay's numeric weapon tuning. This header is included
ONLY from the very end of weapons.h (gated by the JKGUNPLAY compile define),
after all the stock #defines. It #undef's each stock value and re-#define's
the JKGunplay value, so the rest of the code transparently sees the new
numbers with weapons.h kept otherwise pristine.

Why included from exactly one place: these constants are defined only in
weapons.h, so overriding them here (immediately after their definition, once
per translation unit via the weapons.h include guard) is redefinition-safe
and ordering-safe. Non-weapon tunables (jump height, movement feel, crate
splash, g_speed, etc.) are NOT here - they live as clearly-marked guarded
edits at their own sites, because sharing one override header across multiple
stock headers is fragile w.r.t. include ordering.

Each override keeps the stock value in a trailing comment for easy diffing.
===========================================================================
*/

#ifndef JKG_TUNING_H
#define JKG_TUNING_H

// --- Bryar Pistol ---
#undef  BRYAR_PISTOL_VEL
#define BRYAR_PISTOL_VEL			3350	// stock 1800
#undef  BRYAR_PISTOL_DAMAGE
#define BRYAR_PISTOL_DAMAGE			16		// stock 14
#define BRYAR_BOLT_SIZE				3		// new (no stock value)

// --- E11 Blaster ---
#undef  BLASTER_MAIN_SPREAD
#define BLASTER_MAIN_SPREAD			2.75f	// stock 0.5f; was 2.25f JKG
#undef  BLASTER_ALT_SPREAD
#define BLASTER_ALT_SPREAD			2.75f	// stock 1.5f; was 2.25f JKG
#undef  BLASTER_VELOCITY
#define BLASTER_VELOCITY			3150	// stock 2300
#undef  BLASTER_NPC_SPREAD
#define BLASTER_NPC_SPREAD			1.0f	// stock 0.5f
#define BLASTER_NPC_AIM_SPREAD_SCALE	0.3f	// stock 0.25f per currentAim point (NPC E-11)
#define BLASTER_BOLT_SIZE			3		// new (no stock value)

// --- Wookiee Bowcaster ---
#undef  BOWCASTER_DAMAGE
#define BOWCASTER_DAMAGE			60		// stock 45
#undef  BOWCASTER_SIZE
#define BOWCASTER_SIZE				3		// stock 2; same as BLASTER_BOLT_SIZE
#undef  BOWCASTER_ALT_SPREAD
#define BOWCASTER_ALT_SPREAD		2.2f	// stock 5.0f

// --- Heavy Repeater ---
#undef  REPEATER_DAMAGE
#define REPEATER_DAMAGE				10		// stock 8
#undef  REPEATER_VELOCITY
#define REPEATER_VELOCITY			2000	// stock 1600
#define REPEATER_SIZE				3		// new (no stock value)
#undef  REPEATER_ALT_SIZE
#define REPEATER_ALT_SIZE			4		// stock 3
#undef  REPEATER_ALT_VELOCITY
#define REPEATER_ALT_VELOCITY		1800	// stock 1100

// --- DEMP2 (effectively disabled as a damage weapon) ---
#undef  DEMP2_DAMAGE
#define DEMP2_DAMAGE				1		// stock 15
#undef  DEMP2_NPC_DAMAGE_EASY
#define DEMP2_NPC_DAMAGE_EASY		1		// stock 6
#undef  DEMP2_NPC_DAMAGE_NORMAL
#define DEMP2_NPC_DAMAGE_NORMAL		1		// stock 12
#undef  DEMP2_NPC_DAMAGE_HARD
#define DEMP2_NPC_DAMAGE_HARD		1		// stock 18

// --- Golan Arms Flechette ---
#undef  FLECHETTE_SHOTS
#define FLECHETTE_SHOTS				10		// stock 6
#undef  FLECHETTE_SPREAD
#define FLECHETTE_SPREAD			2.5f	// stock 4.0f
#undef  FLECHETTE_DAMAGE
#define FLECHETTE_DAMAGE			10		// stock 15
#undef  FLECHETTE_SIZE
#define FLECHETTE_SIZE				2.1f	// stock 1

// --- Personal Rocket Launcher ---
#undef  ROCKET_VELOCITY
#define ROCKET_VELOCITY				1600	// stock 900

#endif	// JKG_TUNING_H
