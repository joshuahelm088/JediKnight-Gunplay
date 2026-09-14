/*
===========================================================================
JKGunplay mod layer - cvar registration

Defines and registers the g_jkg* toggle cvars that gate the JKGunplay custom
gameplay layer. See jkg_local.h for the design rules and the JKG_* helper
macros used throughout the stock code to branch on these toggles.

All toggles default to "1" (ON) and are CVAR_ARCHIVE so they persist. Set any
of them to "0" at the console to fall back to stock OpenJK behavior for that
subsystem; set g_jkgplay 0 to disable the entire layer at once.
===========================================================================
*/

#include "g_headers.h"

#include "g_local.h"
#include "jkg_local.h"

// Master toggle for the entire JKGunplay layer.
cvar_t *g_jkgplay;

// Per-system toggles.
cvar_t *g_jkgAI;
cvar_t *g_jkgWeapons;
cvar_t *g_jkgMovement;
cvar_t *g_jkgArmor;
cvar_t *g_jkgCombat;
cvar_t *g_jkgCamera;
cvar_t *g_jkgHUD;

void JKG_RegisterCvars( void )
{
	g_jkgplay    = gi.cvar( "g_jkgplay",    "1", CVAR_ARCHIVE );

	g_jkgAI      = gi.cvar( "g_jkgAI",      "1", CVAR_ARCHIVE );
	g_jkgWeapons = gi.cvar( "g_jkgWeapons", "1", CVAR_ARCHIVE );
	g_jkgMovement= gi.cvar( "g_jkgMovement","1", CVAR_ARCHIVE );
	g_jkgArmor   = gi.cvar( "g_jkgArmor",   "1", CVAR_ARCHIVE );
	g_jkgCombat  = gi.cvar( "g_jkgCombat",  "1", CVAR_ARCHIVE );
	g_jkgCamera  = gi.cvar( "g_jkgCamera",  "1", CVAR_ARCHIVE );
	g_jkgHUD     = gi.cvar( "g_jkgHUD",     "1", CVAR_ARCHIVE );
}
