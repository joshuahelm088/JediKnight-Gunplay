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

#include "b_local.h"
#include "g_local.h"
#include "jkg_local.h"

// Master toggle for the entire JKGunplay layer.
cvar_t *g_jkgplay;

// Per-system toggles.
cvar_t *g_jkgAI;
cvar_t *g_jkgWeapons;
cvar_t *g_jkgMovement;
cvar_t *g_jkgArmor;
cvar_t *g_jkgMaxArmor;
cvar_t *g_jkgShieldStationGive;
cvar_t *g_jkgShieldStationTickMs;
cvar_t *g_jkgCombat;
cvar_t *g_jkgCamera;
cvar_t *g_jkgHUD;
cvar_t *g_jkgGunSwayAmount;
cvar_t *g_jkgGunSwayReturn;
cvar_t *g_jkgDebugProjectile;
cvar_t *g_jkgProjectileAabbHits;
cvar_t *g_jkgNpcHitboxScale;
cvar_t *g_jkgBurstPistolShots;
cvar_t *g_jkgBurstShotDelay;
cvar_t *g_jkgBurstPistolShotDelay;
cvar_t *g_jkgBurstPauseSingle;
cvar_t *g_jkgBurstPauseDouble;
cvar_t *g_jkgBurstPauseTriple;
cvar_t *g_jkgBurstPistolPause;
cvar_t *g_jkgBryarVelocity;
cvar_t *g_jkgBlasterVelocity;
cvar_t *g_jkgBlasterNpcVelocity;
cvar_t *g_jkgBlasterPistolNpcVelocity;
cvar_t *g_jkgBowcasterVelocity;
cvar_t *g_jkgBowcasterNpcVelocity;
cvar_t *g_jkgRepeaterVelocity;
cvar_t *g_jkgBryarDamageDecay;
cvar_t *g_jkgBryarMinDamage;
cvar_t *g_jkgBlasterDamageDecay;
cvar_t *g_jkgBlasterMinDamage;
cvar_t *g_jkgBlasterNpcDamageDecay;
cvar_t *g_jkgBlasterNpcMinDamage;
cvar_t *g_jkgBlasterPistolNpcDamageDecay;
cvar_t *g_jkgBlasterPistolNpcMinDamage;
cvar_t *g_jkgBlasterDamage;
cvar_t *g_jkgDamageLog;
cvar_t *g_jkgBryarTapFireTime;
cvar_t *g_jkgBryarChargeFireTime;
cvar_t *g_jkgNpcAccel;
cvar_t *g_jkgNpcDecel;
cvar_t *g_jkgNpcStopDecel;
cvar_t *g_jkgNpcTurnRate;
cvar_t *g_jkgNpcSpeedScale;
cvar_t *g_jkgNpcAnimMinScale;
cvar_t *g_jkgDebugNpcMove;
cvar_t *g_jkgDebugAimCone;
cvar_t *g_jkgDebugNpcState;
cvar_t *g_jkgDebugNpcSpeech;
cvar_t *g_jkgNpcFirstAlert;
cvar_t *g_jkgNoCombatPoints;
cvar_t *g_jkgCombatMove;
cvar_t *g_jkgCombatIdealRangeMin;
cvar_t *g_jkgCombatIdealRangeMax;
cvar_t *g_jkgCombatRangeBand;
cvar_t *g_jkgCombatMoveDelay;
cvar_t *g_jkgCombatStepDist;
cvar_t *g_jkgCombatStrafeDist;
cvar_t *g_jkgCombatStrafeTime;
cvar_t *g_jkgCombatStrafePause;
cvar_t *g_jkgCombatHuntCheatMs;
cvar_t *g_jkgCombatAimDelay;
cvar_t *g_jkgBowcasterAimDelaySingle;
cvar_t *g_jkgBowcasterAimDelayTriple;
cvar_t *g_jkgBowcasterAimDelayFive;
cvar_t *g_jkgBowcasterRepause;

static int JKG_CvarIntegerNonNegative( cvar_t *cv )
{
	int value;

	if ( !cv )
	{
		return 0;
	}

	value = cv->integer;
	if ( value < 0 )
	{
		value = 0;
	}

	return value;
}

static float JKG_CvarFloatNonNegative( cvar_t *cv )
{
	float value;

	if ( !cv )
	{
		return 0.0f;
	}

	value = cv->value;
	if ( value < 0.0f )
	{
		value = 0.0f;
	}

	return value;
}

void JKG_ApplyMaxArmor( gclient_t *client )
{
	if ( !client )
	{
		return;
	}

	if ( !JKG_ARMOR )
	{
		return;
	}

	client->ps.stats[STAT_MAX_ARMOR] = JKG_CvarIntegerNonNegative( g_jkgMaxArmor );
}

int JKG_MaxArmorCap( void )
{
	return JKG_CvarIntegerNonNegative( g_jkgMaxArmor );
}

int JKG_ShieldStationGivePerTick( void )
{
	return JKG_CvarIntegerNonNegative( g_jkgShieldStationGive );
}

int JKG_BlasterBoltVelocity( void )
{
	return JKG_CvarIntegerNonNegative( g_jkgBlasterVelocity );
}

int JKG_BlasterNpcBoltVelocity( void )
{
	return JKG_CvarIntegerNonNegative( g_jkgBlasterNpcVelocity );
}

int JKG_BryarBoltVelocity( void )
{
	return JKG_CvarIntegerNonNegative( g_jkgBryarVelocity );
}

int JKG_BlasterPistolNpcBoltVelocity( void )
{
	return JKG_CvarIntegerNonNegative( g_jkgBlasterPistolNpcVelocity );
}

int JKG_BowcasterBoltVelocity( void )
{
	return JKG_CvarIntegerNonNegative( g_jkgBowcasterVelocity );
}

int JKG_BowcasterNpcBoltVelocity( void )
{
	return JKG_CvarIntegerNonNegative( g_jkgBowcasterNpcVelocity );
}

int JKG_RepeaterBoltVelocity( void )
{
	return JKG_CvarIntegerNonNegative( g_jkgRepeaterVelocity );
}

int JKG_BlasterBoltVelocityFor( const gentity_t *ent )
{
	if ( JKG_NpcBlasterBolt( ent ) )
	{
		return JKG_BlasterNpcBoltVelocity();
	}

	return JKG_BlasterBoltVelocity();
}

int JKG_BryarPistolBoltVelocityFor( const gentity_t *ent )
{
	if ( JKG_NpcBlasterPistolBolt( ent ) )
	{
		return JKG_BlasterPistolNpcBoltVelocity();
	}

	return JKG_BryarBoltVelocity();
}

int JKG_BowcasterBoltVelocityFor( const gentity_t *ent )
{
	if ( ent && ent->NPC )
	{
		return JKG_BowcasterNpcBoltVelocity();
	}

	return JKG_BowcasterBoltVelocity();
}

void JKG_GetBlasterDecay( const gentity_t *ent, float *rate, float *floorDamage )
{
	if ( JKG_NpcBlasterBolt( ent ) )
	{
		if ( rate )
		{
			*rate = JKG_CvarFloatNonNegative( g_jkgBlasterNpcDamageDecay );
		}
		if ( floorDamage )
		{
			*floorDamage = JKG_CvarFloatNonNegative( g_jkgBlasterNpcMinDamage );
		}
		return;
	}

	if ( rate )
	{
		*rate = JKG_CvarFloatNonNegative( g_jkgBlasterDamageDecay );
	}
	if ( floorDamage )
	{
		*floorDamage = JKG_CvarFloatNonNegative( g_jkgBlasterMinDamage );
	}
}

int JKG_BlasterDamage( void )
{
	return JKG_CvarIntegerNonNegative( g_jkgBlasterDamage );
}

void JKG_GetBryarPistolDecay( const gentity_t *ent, float *rate, float *floorDamage )
{
	if ( JKG_NpcBlasterPistolBolt( ent ) )
	{
		if ( rate )
		{
			*rate = JKG_CvarFloatNonNegative( g_jkgBlasterPistolNpcDamageDecay );
		}
		if ( floorDamage )
		{
			*floorDamage = JKG_CvarFloatNonNegative( g_jkgBlasterPistolNpcMinDamage );
		}
		return;
	}

	if ( rate )
	{
		*rate = JKG_CvarFloatNonNegative( g_jkgBryarDamageDecay );
	}
	if ( floorDamage )
	{
		*floorDamage = JKG_CvarFloatNonNegative( g_jkgBryarMinDamage );
	}
}

int JKG_BryarTapFireTime( void )
{
	return JKG_CvarIntegerNonNegative( g_jkgBryarTapFireTime );
}

int JKG_BryarChargeFireTime( void )
{
	return JKG_CvarIntegerNonNegative( g_jkgBryarChargeFireTime );
}

int JKG_BryarChargeRecoveryTime( int chargeCount )
{
	const int minDelay = JKG_BryarTapFireTime();
	const int maxDelay = JKG_BryarChargeFireTime();
	int t;

	if ( chargeCount < 1 )
	{
		chargeCount = 1;
	}
	else if ( chargeCount > 5 )
	{
		chargeCount = 5;
	}

	t = chargeCount - 1;
	return minDelay + ( maxDelay - minDelay ) * t / 4;
}

qboolean JKG_BryarChargeLocked( const gentity_t *ent )
{
	if ( !ent || !ent->client )
	{
		return qfalse;
	}

	return ( ent->client->jkgBryarChargeLockTime > level.time ) ? qtrue : qfalse;
}

void JKG_BryarArmChargeLock( gentity_t *ent, int recovery )
{
	if ( !ent || !ent->client || recovery <= 0 )
	{
		return;
	}

	ent->client->jkgBryarChargeLockTime = level.time + recovery;
}

int JKG_ShieldStationTickMs( void )
{
	const int tickMs = JKG_CvarIntegerNonNegative( g_jkgShieldStationTickMs );

	return ( tickMs > 0 ) ? tickMs : 1;
}

qboolean JKG_NpcBlasterBolt( const gentity_t *ent )
{
	if ( !ent || !ent->NPC || !ent->client )
	{
		return qfalse;
	}
	if ( ent->client->ps.weapon != WP_BLASTER )
	{
		return qfalse;
	}
	if ( ent->client->NPC_class == CLASS_STORMTROOPER
		|| ent->client->NPC_class == CLASS_SWAMPTROOPER
		|| ent->client->NPC_class == CLASS_SHADOWTROOPER )
	{
		return qtrue;
	}
	return qfalse;
}

qboolean JKG_NpcBlasterPistolBolt( const gentity_t *ent )
{
	if ( !ent || !ent->NPC || !ent->client )
	{
		return qfalse;
	}
	if ( ent->client->ps.weapon != WP_BLASTER_PISTOL )
	{
		return qfalse;
	}
	if ( ent->client->NPC_class == CLASS_IMPERIAL )
	{
		return qtrue;
	}
	return qfalse;
}

void JKG_RegisterCvars( void )
{
	g_jkgplay    = gi.cvar( "g_jkgplay",    "1", CVAR_ARCHIVE );

	g_jkgAI      = gi.cvar( "g_jkgAI",      "1", CVAR_ARCHIVE );
	g_jkgWeapons = gi.cvar( "g_jkgWeapons", "1", CVAR_ARCHIVE );
	g_jkgMovement= gi.cvar( "g_jkgMovement","1", CVAR_ARCHIVE );
	g_jkgArmor   = gi.cvar( "g_jkgArmor",   "1", CVAR_ARCHIVE );
	g_jkgMaxArmor = gi.cvar( "g_jkgMaxArmor", "200", CVAR_ARCHIVE );
	g_jkgShieldStationGive = gi.cvar( "g_jkgShieldStationGive", "4", CVAR_ARCHIVE );
	g_jkgShieldStationTickMs = gi.cvar( "g_jkgShieldStationTickMs", "33", CVAR_ARCHIVE );
	g_jkgCombat  = gi.cvar( "g_jkgCombat",  "1", CVAR_ARCHIVE );
	g_jkgCamera  = gi.cvar( "g_jkgCamera",  "1", CVAR_ARCHIVE );
	g_jkgHUD     = gi.cvar( "g_jkgHUD",     "1", CVAR_ARCHIVE );
	g_jkgGunSwayAmount = gi.cvar( "g_jkgGunSwayAmount", "3.5", CVAR_ARCHIVE );
	g_jkgGunSwayReturn = gi.cvar( "g_jkgGunSwayReturn", "15", CVAR_ARCHIVE );
	g_jkgDebugProjectile = gi.cvar( "g_jkgDebugProjectile", "0", CVAR_CHEAT );
	g_jkgProjectileAabbHits = gi.cvar( "g_jkgProjectileAabbHits", "1", CVAR_ARCHIVE );
	g_jkgNpcHitboxScale = gi.cvar( "g_jkgNpcHitboxScale", "1", CVAR_ARCHIVE );
	g_jkgBurstPistolShots = gi.cvar( "g_jkgBurstPistolShots", "2", CVAR_ARCHIVE );
	g_jkgBurstShotDelay = gi.cvar( "g_jkgBurstShotDelay", "1500", CVAR_ARCHIVE );
	g_jkgBurstPistolShotDelay = gi.cvar( "g_jkgBurstPistolShotDelay", "1500", CVAR_ARCHIVE );
	g_jkgBurstPauseSingle = gi.cvar( "g_jkgBurstPauseSingle", "500", CVAR_ARCHIVE );
	g_jkgBurstPauseDouble = gi.cvar( "g_jkgBurstPauseDouble", "1000", CVAR_ARCHIVE );
	g_jkgBurstPauseTriple = gi.cvar( "g_jkgBurstPauseTriple", "1500", CVAR_ARCHIVE );
	g_jkgBurstPistolPause = gi.cvar( "g_jkgBurstPistolPause", "2000", CVAR_ARCHIVE );
	g_jkgBryarVelocity = gi.cvar( "g_jkgBryarVelocity", "1533", CVAR_ARCHIVE );
	g_jkgBlasterVelocity = gi.cvar( "g_jkgBlasterVelocity", "2300", CVAR_ARCHIVE );
	g_jkgBlasterNpcVelocity = gi.cvar( "g_jkgBlasterNpcVelocity", "1533", CVAR_ARCHIVE );
	g_jkgBlasterPistolNpcVelocity = gi.cvar( "g_jkgBlasterPistolNpcVelocity", "1342", CVAR_ARCHIVE );
	g_jkgBowcasterVelocity = gi.cvar( "g_jkgBowcasterVelocity", "1917", CVAR_ARCHIVE );
	g_jkgBowcasterNpcVelocity = gi.cvar( "g_jkgBowcasterNpcVelocity", "1533", CVAR_ARCHIVE );
	g_jkgRepeaterVelocity = gi.cvar( "g_jkgRepeaterVelocity", "2300", CVAR_ARCHIVE );
	g_jkgBryarDamageDecay = gi.cvar( "g_jkgBryarDamageDecay", "6", CVAR_ARCHIVE );
	g_jkgBryarMinDamage = gi.cvar( "g_jkgBryarMinDamage", "10", CVAR_ARCHIVE );
	g_jkgBlasterDamageDecay = gi.cvar( "g_jkgBlasterDamageDecay", "4", CVAR_ARCHIVE );
	g_jkgBlasterMinDamage = gi.cvar( "g_jkgBlasterMinDamage", "10", CVAR_ARCHIVE );
	g_jkgBlasterNpcDamageDecay = gi.cvar( "g_jkgBlasterNpcDamageDecay", "4", CVAR_ARCHIVE );
	g_jkgBlasterNpcMinDamage = gi.cvar( "g_jkgBlasterNpcMinDamage", "5", CVAR_ARCHIVE );
	g_jkgBlasterPistolNpcDamageDecay = gi.cvar( "g_jkgBlasterPistolNpcDamageDecay", "6", CVAR_ARCHIVE );
	g_jkgBlasterPistolNpcMinDamage = gi.cvar( "g_jkgBlasterPistolNpcMinDamage", "5", CVAR_ARCHIVE );
	g_jkgBlasterDamage = gi.cvar( "g_jkgBlasterDamage", "30", CVAR_ARCHIVE );
	g_jkgDamageLog = gi.cvar( "g_jkgDamageLog", "1", CVAR_ARCHIVE );
	g_jkgBryarTapFireTime = gi.cvar( "g_jkgBryarTapFireTime", "200", CVAR_ARCHIVE );
	g_jkgBryarChargeFireTime = gi.cvar( "g_jkgBryarChargeFireTime", "600", CVAR_ARCHIVE );
	g_jkgNpcAccel = gi.cvar( "g_jkgNpcAccel", "150", CVAR_ARCHIVE );
	g_jkgNpcDecel = gi.cvar( "g_jkgNpcDecel", "200", CVAR_ARCHIVE );
	g_jkgNpcStopDecel = gi.cvar( "g_jkgNpcStopDecel", "200", CVAR_ARCHIVE );
	g_jkgNpcTurnRate = gi.cvar( "g_jkgNpcTurnRate", "180", CVAR_ARCHIVE );
	g_jkgNpcSpeedScale = gi.cvar( "g_jkgNpcSpeedScale", "1", CVAR_ARCHIVE );
	g_jkgNpcAnimMinScale = gi.cvar( "g_jkgNpcAnimMinScale", "0.25", CVAR_ARCHIVE );
	g_jkgDebugNpcMove = gi.cvar( "g_jkgDebugNpcMove", "0", CVAR_CHEAT );
	g_jkgDebugAimCone = gi.cvar( "g_jkgDebugAimCone", "0", CVAR_CHEAT );
	g_jkgDebugNpcState = gi.cvar( "g_jkgDebugNpcState", "0", CVAR_CHEAT );
	g_jkgDebugNpcSpeech = gi.cvar( "g_jkgDebugNpcSpeech", "0", CVAR_CHEAT );
	g_jkgNpcFirstAlert = gi.cvar( "g_jkgNpcFirstAlert", "1", CVAR_ARCHIVE );
	g_jkgNoCombatPoints = gi.cvar( "g_jkgNoCombatPoints", "1", CVAR_CHEAT );
	g_jkgCombatMove = gi.cvar( "g_jkgCombatMove", "1", CVAR_ARCHIVE );
	g_jkgCombatIdealRangeMin = gi.cvar( "g_jkgCombatIdealRangeMin", "192", CVAR_ARCHIVE );
	g_jkgCombatIdealRangeMax = gi.cvar( "g_jkgCombatIdealRangeMax", "320", CVAR_ARCHIVE );
	g_jkgCombatRangeBand = gi.cvar( "g_jkgCombatRangeBand", "32", CVAR_ARCHIVE );
	g_jkgCombatMoveDelay = gi.cvar( "g_jkgCombatMoveDelay", "700", CVAR_ARCHIVE );
	g_jkgCombatStepDist = gi.cvar( "g_jkgCombatStepDist", "80", CVAR_ARCHIVE );
	g_jkgCombatStrafeDist = gi.cvar( "g_jkgCombatStrafeDist", "64", CVAR_ARCHIVE );
	g_jkgCombatStrafeTime = gi.cvar( "g_jkgCombatStrafeTime", "900", CVAR_ARCHIVE );
	g_jkgCombatStrafePause = gi.cvar( "g_jkgCombatStrafePause", "700", CVAR_ARCHIVE );
	g_jkgCombatHuntCheatMs = gi.cvar( "g_jkgCombatHuntCheatMs", "2500", CVAR_ARCHIVE );
	g_jkgCombatAimDelay = gi.cvar( "g_jkgCombatAimDelay", "0", CVAR_ARCHIVE );
	g_jkgBowcasterAimDelaySingle = gi.cvar( "g_jkgBowcasterAimDelaySingle", "1000", CVAR_ARCHIVE );
	g_jkgBowcasterAimDelayTriple = gi.cvar( "g_jkgBowcasterAimDelayTriple", "2000", CVAR_ARCHIVE );
	g_jkgBowcasterAimDelayFive = gi.cvar( "g_jkgBowcasterAimDelayFive", "4000", CVAR_ARCHIVE );
	g_jkgBowcasterRepause = gi.cvar( "g_jkgBowcasterRepause", "1000", CVAR_ARCHIVE );
}
