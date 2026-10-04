/*
===========================================================================
Copyright (C) 2000 - 2013, Raven Software, Inc.
Copyright (C) 2001 - 2013, Activision, Inc.
Copyright (C) 2013 - 2015, OpenJK contributors

This file is part of the OpenJK source code.

OpenJK is free software; you can redistribute it and/or modify it
under the terms of the GNU General Public License version 2 as
published by the Free Software Foundation.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, see <http://www.gnu.org/licenses/>.
===========================================================================
*/

#include "g_headers.h"

#include "b_local.h"
#include "g_local.h"
#include "wp_saber.h"
#include "w_local.h"
#include "g_functions.h"
#include "../cgame/cg_camera.h"
#include "jkg_local.h"

//---------------
//	Bryar Pistol
//---------------

//---------------------------------------------------------
void WP_FireBryarPistol_JKG( gentity_t *ent, qboolean alt_fire )
//---------------------------------------------------------
{
	vec3_t	start;
	int		damage = BRYAR_PISTOL_DAMAGE;
	float	decayRate = 0.0f;
	float	decayFloor = 0.0f;

	if ( JKG_NpcBlasterPistolBolt( ent ) )
	{
		damage = BLASTER_PISTOL_NPC_DAMAGE;
	}

	JKG_GetBryarPistolDecay( ent, &decayRate, &decayFloor );

	VectorCopy( wpMuzzle, start );
	WP_TraceSetStart( ent, start, vec3_origin, vec3_origin );//make sure our start point isn't on the other side of a wall

	if ( ent->NPC && ent->NPC->currentAim < 5 )
	{
		vec3_t	angs;

		vectoangles( wpFwd, angs );

		if ( ent->client->NPC_class == CLASS_IMPWORKER )
		{//*sigh*, hack to make impworkers less accurate without affecteing imperial officer accuracy
			angs[PITCH] += ( Q_flrand(-1.0f, 1.0f) * (BLASTER_NPC_SPREAD+(6-ent->NPC->currentAim)*0.25f));//was 0.5f
			angs[YAW]	+= ( Q_flrand(-1.0f, 1.0f) * (BLASTER_NPC_SPREAD+(6-ent->NPC->currentAim)*0.25f));//was 0.5f
		}
		else
		{
			angs[PITCH] += ( Q_flrand(-1.0f, 1.0f) * ((5-ent->NPC->currentAim)*0.25f) );
			angs[YAW]	+= ( Q_flrand(-1.0f, 1.0f) * ((5-ent->NPC->currentAim)*0.25f) );
		}

		AngleVectors( angs, wpFwd, NULL, NULL );
	}

	gentity_t	*missile = CreateMissile( start, wpFwd, JKG_BryarPistolBoltVelocityFor( ent ), 10000, ent, alt_fire );

	missile->classname = "bryar_proj";
	missile->s.weapon = WP_BRYAR_PISTOL;

	VectorSet(missile->maxs, BRYAR_BOLT_SIZE, BRYAR_BOLT_SIZE, BRYAR_BOLT_SIZE);
	VectorScale(missile->maxs, -1, missile->mins);

	if (!ent->NPC) {
		float kickIntensity = 0.35f;
		int kickDuration = 150;
		vec3_t kickDir = { -1, 0, 0 };
		VectorSet(kickDir, 1.0f, 0.0f, 0.0f);
		kickIntensity = 0.85f;
		CGCam_Kickback(0.35f, 250, kickDir);
	}

	if ( alt_fire )
	{
		int count = ( level.time - ent->client->ps.weaponChargeTime ) / BRYAR_CHARGE_UNIT;

		if ( count < 1 )
		{
			count = 1;
		}
		else if ( count > 5 )
		{
			count = 5;
		}

		damage *= count;
		missile->count = count; // this will get used in the projectile rendering code to make a beefier effect
	}

//	if ( ent->client && ent->client->ps.powerups[PW_WEAPON_OVERCHARGE] > 0 && ent->client->ps.powerups[PW_WEAPON_OVERCHARGE] > cg.time )
//	{
//		// in overcharge mode, so doing double damage
//		missile->flags |= FL_OVERCHARGED;
//		damage *= 2;
//	}

	missile->damage = damage;
	missile->dflags = DAMAGE_DEATH_KNOCKBACK;

	{
		const float stacks = ( alt_fire && missile->count > 1 ) ? (float)missile->count : 1.0f;
		JKG_ArmEnergyBoltDecay( missile, decayRate * stacks, decayFloor * stacks );
	}

	if ( alt_fire )
	{
		missile->methodOfDeath = MOD_BRYAR_ALT;
	}
	else
	{
		missile->methodOfDeath = MOD_BRYAR;
	}

	missile->clipmask = MASK_SHOT | CONTENTS_LIGHTSABER;

	// we don't want it to bounce forever
	missile->bounceCount = 8;
}

void JKG_ArmEnergyBoltDecay( gentity_t *missile, float rate, float floorDamage )
{
	if ( !missile || rate <= 0.0f )
	{
		return;
	}

	missile->wait = rate;
	missile->random = floorDamage;
}

void JKG_DecayEnergyBoltDamage( gentity_t *missile )
{
	float seconds;
	float damage;
	int startDamage;

	if ( !missile || missile->wait <= 0.0f )
	{
		return;
	}

	seconds = ( level.time - missile->s.pos.trTime ) * 0.001f;
	if ( seconds < 0.0f )
	{
		seconds = 0.0f;
	}

	startDamage = missile->damage;
	damage = (float)startDamage - missile->wait * seconds;
	if ( damage < missile->random )
	{
		damage = missile->random;
	}

	missile->damage = (int)( damage + 0.5f );

	if ( g_jkgDamageLog && g_jkgDamageLog->integer )
	{
		gi.Printf( "JKG decay: %d -> %d  t=%.2fs  rate=%.1f  floor=%.0f\n",
			startDamage, missile->damage, seconds, missile->wait, missile->random );
	}
}