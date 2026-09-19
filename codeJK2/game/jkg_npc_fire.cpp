/*
===========================================================================
JKGunplay mod layer - NPC blaster / pistol burst fire cadence
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

static qboolean JKG_IsJkgBurstWeapon( int weapon )
{
	if ( weapon == WP_BLASTER || weapon == WP_BLASTER_PISTOL )
	{
		return qtrue;
	}
	return qfalse;
}

static int JKG_BurstShotsForWeapon( int weapon )
{
	int shots;

	if ( weapon == WP_BLASTER_PISTOL )
	{
		shots = JKG_CvarIntegerNonNegative( g_jkgBurstPistolShots );
	}
	else if ( weapon == WP_BLASTER )
	{
		shots = JKG_CvarIntegerNonNegative( g_jkgBurstShots );
	}
	else
	{
		shots = 0;
	}

	if ( shots < 1 )
	{
		shots = 1;
	}
	return shots;
}

static qboolean JKG_NPCUsesBurstCadence( const gentity_t *ent )
{
	int weapon;

	if ( !ent || !ent->NPC || !ent->client )
	{
		return qfalse;
	}

	weapon = ent->client->ps.weapon;
	if ( !JKG_IsJkgBurstWeapon( weapon ) )
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

	// DEFAULT: burst for E-11 and officer blaster pistol
	return qtrue;
}

void JKG_ApplyNpcBurstFireMode( gentity_t *ent )
{
	int weapon;
	int shots;

	if ( !ent || !ent->NPC || !ent->client )
	{
		return;
	}

	weapon = ent->client->ps.weapon;
	if ( !JKG_IsJkgBurstWeapon( weapon ) )
	{
		return;
	}

	if ( !JKG_NPCUsesBurstCadence( ent ) )
	{
		ent->NPC->aiFlags &= ~NPCAI_BURST_WEAPON;
		ent->NPC->burstCount = 0;
		return;
	}

	shots = JKG_BurstShotsForWeapon( weapon );

	ent->NPC->aiFlags |= NPCAI_BURST_WEAPON;
	ent->NPC->burstMin = shots;
	ent->NPC->burstMax = shots;
	ent->NPC->burstCount = 0;
}

qboolean JKG_NpcBurstShootThink( void )
{
	int delay;
	int shots;
	int weapon;

	if ( !NPC || !NPC->NPC || !NPC->client )
	{
		return qfalse;
	}

	weapon = NPC->client->ps.weapon;
	if ( !JKG_IsJkgBurstWeapon( weapon ) )
	{
		return qfalse;
	}

	if ( !JKG_NPCUsesBurstCadence( NPC ) )
	{
		return qfalse;
	}

	ucmd.buttons |= BUTTON_ATTACK;

	NPCInfo->currentAmmo = client->ps.ammo[weaponData[client->ps.weapon].ammoIndex];

	NPC_ApplyWeaponFireDelay();

	shots = JKG_BurstShotsForWeapon( weapon );

	if ( NPCInfo->burstCount <= 0 )
	{
		NPCInfo->burstCount = shots;
	}

	NPCInfo->burstCount--;

	if ( NPCInfo->burstCount > 0 )
	{
		delay = JKG_CvarIntegerNonNegative( g_jkgBurstShotDelay );
	}
	else if ( weapon == WP_BLASTER_PISTOL )
	{
		delay = JKG_CvarIntegerNonNegative( g_jkgBurstPistolPause );
	}
	else
	{
		delay = JKG_CvarIntegerNonNegative( g_jkgBurstPause );
	}

	NPCInfo->shotTime = level.time + delay;
	NPC->attackDebounceTime = level.time + NPC_AttackDebounceForWeapon();

	return qtrue;
}
