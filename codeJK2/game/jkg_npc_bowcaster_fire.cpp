/*
===========================================================================
JKGunplay - NPC bowcaster aim volleys (single / triple / five)

Replaces stock bowcaster charging for NPCs: decide volley size, aim (ADS),
fire N bolts, then repause before the next cycle. Player bowcaster unchanged.
===========================================================================
*/

#include "g_headers.h"

#include "b_local.h"
#include "jkg_local.h"

extern void G_SoundOnEnt( gentity_t *ent, soundChannel_t channel, const char *soundPath );

static void JKG_NpcBowcasterStartAimChargeSound( gentity_t *ent, int volleyShots )
{
	if ( !ent || volleyShots < 3 )
	{
		return;
	}

	if ( weaponData[WP_BOWCASTER].chargeSnd[0] )
	{
		G_SoundOnEnt( ent, CHAN_WEAPON, weaponData[WP_BOWCASTER].chargeSnd );
	}
}

static void JKG_NpcBowcasterStopAimChargeSound( gentity_t *ent, int volleyShots )
{
	if ( !ent || volleyShots < 3 )
	{
		return;
	}

	if ( weaponData[WP_BOWCASTER].stopSnd[0] )
	{
		G_SoundOnEnt( ent, CHAN_WEAPON, weaponData[WP_BOWCASTER].stopSnd );
	}
}

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

static int JKG_BowcasterAimMsForVolley( int volleyShots )
{
	if ( volleyShots >= 5 )
	{
		return JKG_CvarIntegerNonNegative( g_jkgBowcasterAimDelayFive );
	}
	if ( volleyShots >= 3 )
	{
		return JKG_CvarIntegerNonNegative( g_jkgBowcasterAimDelayTriple );
	}
	return JKG_CvarIntegerNonNegative( g_jkgBowcasterAimDelaySingle );
}

static int JKG_PickBowcasterVolleyShots( void )
{
	const int roll = Q_irand( 0, 99 );

	if ( roll < 50 )
	{
		return 1;
	}
	if ( roll < 80 )
	{
		return 3;
	}
	return 5;
}

void JKG_NpcBowcasterMaintainAttack( gentity_t *ent, usercmd_t *ucmd )
{
	if ( !JKG_AI || !ent || !ent->NPC || !ent->client || !ucmd )
	{
		return;
	}

	if ( ent->client->ps.weapon != WP_BOWCASTER )
	{
		return;
	}

	if ( ent->client->fireDelay > 0 && ent->NPC->jkgBowcasterVolleyShots > 0 )
	{
		ucmd->buttons |= BUTTON_ATTACK;
	}
}

qboolean JKG_NpcBowcasterShootThink( void )
{
	int aimMs;
	int repauseMs;
	int volley;

	if ( !JKG_AI || !NPC || !NPC->NPC || !NPC->client )
	{
		return qfalse;
	}

	if ( NPC->client->ps.weapon != WP_BOWCASTER )
	{
		return qfalse;
	}

	ucmd.buttons |= BUTTON_ATTACK;

	if ( NPC->client->fireDelay > 0 )
	{
		return qtrue;
	}

	volley = JKG_PickBowcasterVolleyShots();
	aimMs = JKG_BowcasterAimMsForVolley( volley );
	repauseMs = JKG_CvarIntegerNonNegative( g_jkgBowcasterRepause );

	if ( aimMs < 1 )
	{
		aimMs = 1;
	}

	NPC->NPC->jkgBowcasterVolleyShots = volley;
	NPC->client->fireDelay = aimMs;
	NPC->client->jkgCombatAimPose = qtrue;
	NPC->client->ps.weaponstate = WEAPON_FIRING;
	NPC->client->ps.weaponChargeTime = level.time;
	NPC->client->ps.weaponShotCount = volley;

	JKG_NpcBowcasterStartAimChargeSound( NPC, volley );

	NPCInfo->currentAmmo = client->ps.ammo[weaponData[client->ps.weapon].ammoIndex];

	NPCInfo->shotTime = level.time + aimMs + repauseMs;
	NPC->attackDebounceTime = level.time + repauseMs;

	return qtrue;
}

int JKG_NpcBowcasterVolleyShotsForFire( gentity_t *ent )
{
	int count;

	if ( !ent || !ent->NPC || !ent->client )
	{
		return 0;
	}

	count = ent->NPC->jkgBowcasterVolleyShots;
	JKG_NpcBowcasterStopAimChargeSound( ent, count );
	ent->NPC->jkgBowcasterVolleyShots = 0;
	ent->client->jkgCombatAimPose = qfalse;
	ent->client->ps.weaponShotCount = 0;

	if ( count < 1 )
	{
		count = 1;
	}
	else if ( count > 5 )
	{
		count = 5;
	}

	if ( !( count & 1 ) )
	{
		count--;
	}

	if ( count < 1 )
	{
		count = 1;
	}

	return count;
}
