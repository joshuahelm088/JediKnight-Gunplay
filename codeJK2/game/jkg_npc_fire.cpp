/*
===========================================================================
JKGunplay mod layer - NPC blaster fire cadence (burst vs single)
===========================================================================
*/

#include "g_headers.h"

#include "b_local.h"
#include "jkg_local.h"

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

static int JKG_BurstShotsPerBurst( void )
{
	int shots;

	shots = JKG_CvarIntegerNonNegative( g_jkgBurstShots );
	if ( shots < 1 )
	{
		shots = 1;
	}
	return shots;
}

static qboolean JKG_NPCUsesBlasterBurstCadence( const gentity_t *ent )
{
	if ( !ent || !ent->NPC )
	{
		return qfalse;
	}

	if ( ent->NPC->jkgFireMode == JKG_FIREMODE_SINGLE )
	{
		return qfalse;
	}

	if ( ent->NPC->jkgFireMode == JKG_FIREMODE_BURST )
	{
		return qtrue;
	}

	// DEFAULT: burst for E-11 blaster only
	return ( ent->client && ent->client->ps.weapon == WP_BLASTER ) ? qtrue : qfalse;
}

void JKG_ApplyBlasterFireMode( gentity_t *ent )
{
	int shots;

	if ( !ent || !ent->NPC || !ent->client )
	{
		return;
	}

	if ( ent->client->ps.weapon != WP_BLASTER )
	{
		return;
	}

	if ( !JKG_NPCUsesBlasterBurstCadence( ent ) )
	{
		ent->NPC->aiFlags &= ~NPCAI_BURST_WEAPON;
		ent->NPC->burstCount = 0;
		return;
	}

	shots = JKG_BurstShotsPerBurst();

	ent->NPC->aiFlags |= NPCAI_BURST_WEAPON;
	ent->NPC->burstMin = shots;
	ent->NPC->burstMax = shots;
	ent->NPC->burstCount = 0;
}

qboolean JKG_BlasterBurstShootThink( void )
{
	int delay;
	int shots;

	if ( !NPC || !NPC->NPC || !NPC->client )
	{
		return qfalse;
	}

	if ( NPC->client->ps.weapon != WP_BLASTER )
	{
		return qfalse;
	}

	if ( !JKG_NPCUsesBlasterBurstCadence( NPC ) )
	{
		return qfalse;
	}

	ucmd.buttons |= BUTTON_ATTACK;

	NPCInfo->currentAmmo = client->ps.ammo[weaponData[client->ps.weapon].ammoIndex];

	NPC_ApplyWeaponFireDelay();

	shots = JKG_BurstShotsPerBurst();

	if ( NPCInfo->burstCount <= 0 )
	{
		NPCInfo->burstCount = shots;
	}

	NPCInfo->burstCount--;

	if ( NPCInfo->burstCount > 0 )
	{
		delay = JKG_CvarIntegerNonNegative( g_jkgBurstShotDelay );
	}
	else
	{
		delay = JKG_CvarIntegerNonNegative( g_jkgBurstPause );
	}

	NPCInfo->shotTime = level.time + delay;
	NPC->attackDebounceTime = level.time + NPC_AttackDebounceForWeapon();

	return qtrue;
}
