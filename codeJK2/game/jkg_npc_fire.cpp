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

static int JKG_PistolBurstShots( void )
{
	int shots;

	shots = JKG_CvarIntegerNonNegative( g_jkgBurstPistolShots );
	if ( shots < 1 )
	{
		shots = 1;
	}
	return shots;
}

// Weighted 1/2/3 like JKG_PickBowcasterVolleyShots (1/3/5). Triple is most common.
static int JKG_PickBlasterBurstShots( void )
{
	const int roll = Q_irand( 0, 99 );

	if ( roll < 50 )
	{
		return 3;
	}
	if ( roll < 80 )
	{
		return 2;
	}
	return 1;
}

static int JKG_PickBurstShotsForWeapon( int weapon )
{
	if ( weapon == WP_BLASTER_PISTOL )
	{
		return JKG_PistolBurstShots();
	}
	if ( weapon == WP_BLASTER )
	{
		return JKG_PickBlasterBurstShots();
	}
	return 1;
}

static int JKG_BlasterBurstPauseMs( int burstShots )
{
	if ( burstShots >= 3 )
	{
		return JKG_CvarIntegerNonNegative( g_jkgBurstPauseTriple );
	}
	if ( burstShots >= 2 )
	{
		return JKG_CvarIntegerNonNegative( g_jkgBurstPauseDouble );
	}
	return JKG_CvarIntegerNonNegative( g_jkgBurstPauseSingle );
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

	if ( weapon == WP_BLASTER_PISTOL )
	{
		shots = JKG_PistolBurstShots();
		ent->NPC->burstMin = shots;
		ent->NPC->burstMax = shots;
	}
	else
	{
		ent->NPC->burstMin = 1;
		ent->NPC->burstMax = 3;
	}

	ent->NPC->aiFlags |= NPCAI_BURST_WEAPON;
	ent->NPC->burstCount = 0;
}

void JKG_ApplyNpcCombatClassAimDelay( gentity_t *ent )
{
	int aimMs;
	int merged;

	if ( !JKG_AI || !ent || !ent->client || !ent->NPC )
	{
		return;
	}

	if ( ent->client->ps.weapon == WP_BOWCASTER )
	{
		return;
	}

	aimMs = JKG_GetCombatAimDelay( ent );
	if ( aimMs <= 0 )
	{
		ent->client->jkgCombatAimPose = qfalse;
		return;
	}

	merged = ent->client->fireDelay;
	if ( aimMs > merged )
	{
		ent->client->fireDelay = aimMs;
	}

	if ( ent->client->fireDelay >= aimMs )
	{
		ent->client->jkgCombatAimPose = qtrue;
	}
}

void JKG_AdjustNpcShotTimeForFireDelay( gentity_t *ent )
{
	if ( !ent || !ent->NPC || !ent->client )
	{
		return;
	}

	if ( ent->client->fireDelay > 0 )
	{
		const int holdUntil = level.time + ent->client->fireDelay;
		if ( ent->NPC->shotTime < holdUntil )
		{
			ent->NPC->shotTime = holdUntil;
		}
	}
}

qboolean JKG_NpcBurstShootThink( void )
{
	int delay;
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

	if ( NPCInfo->burstCount <= 0 )
	{
		NPCInfo->burstCount = JKG_PickBurstShotsForWeapon( weapon );
		NPCInfo->burstMax = NPCInfo->burstCount;
	}

	NPCInfo->burstCount--;

	if ( NPCInfo->burstCount > 0 )
	{
		if ( weapon == WP_BLASTER_PISTOL )
		{
			delay = JKG_CvarIntegerNonNegative( g_jkgBurstPistolShotDelay );
		}
		else
		{
			delay = JKG_CvarIntegerNonNegative( g_jkgBurstShotDelay );
		}
	}
	else if ( weapon == WP_BLASTER_PISTOL )
	{
		delay = JKG_CvarIntegerNonNegative( g_jkgBurstPistolPause );
	}
	else
	{
		delay = JKG_BlasterBurstPauseMs( NPCInfo->burstMax );
	}

	NPCInfo->shotTime = level.time + delay;
	if ( JKG_AI )
	{
		JKG_AdjustNpcShotTimeForFireDelay( NPC );
	}
	NPC->attackDebounceTime = level.time + NPC_AttackDebounceForWeapon();

	return qtrue;
}

static int JKG_MeleeComboShots( void )
{
	int shots;

	shots = JKG_CvarIntegerNonNegative( g_jkgMeleeComboShots );
	if ( shots < 1 )
	{
		shots = 1;
	}
	return shots;
}

static int JKG_MeleeComboPauseMs( void )
{
	int pause;
	int jitter;

	pause = JKG_CvarIntegerNonNegative( g_jkgMeleeComboPause );
	jitter = ( pause * Q_irand( -20, 20 ) ) / 100;
	pause += jitter;
	if ( pause < 0 )
	{
		pause = 0;
	}
	return pause;
}

qboolean JKG_NpcMeleeComboShootThink( void )
{
	int delay;
	int shots;

	if ( !NPC || !NPC->NPC || !NPC->client )
	{
		return qfalse;
	}
	if ( NPC->client->NPC_class != CLASS_GRAN || NPC->client->ps.weapon != WP_MELEE )
	{
		return qfalse;
	}

	ucmd.buttons |= BUTTON_ATTACK;
	NPCInfo->currentAmmo = client->ps.ammo[weaponData[client->ps.weapon].ammoIndex];
	NPC_ApplyWeaponFireDelay();

	shots = JKG_MeleeComboShots();
	if ( NPCInfo->burstCount <= 0 )
	{
		NPCInfo->burstCount = shots;
		NPCInfo->burstMax = shots;
	}
	else if ( client->ps.weaponTime > 0 )
	{
		// The previous swing's weaponTime is the full anim length. Clear it so
		// the follow-up punch is not swallowed before it can start.
		client->ps.weaponTime = 0;
	}

	NPCInfo->burstCount--;
	if ( NPCInfo->burstCount > 0 )
	{
		delay = JKG_CvarIntegerNonNegative( g_jkgMeleePunchGap );
	}
	else
	{
		delay = JKG_MeleeComboPauseMs();
	}

	NPCInfo->shotTime = level.time + delay;
	JKG_AdjustNpcShotTimeForFireDelay( NPC );
	NPC->attackDebounceTime = level.time + NPC_AttackDebounceForWeapon();
	return qtrue;
}
