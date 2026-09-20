/*
===========================================================================
JKGunplay - NPC weapon spread helper and aim-cone debug draw
===========================================================================
*/

#include "g_headers.h"

#include "b_local.h"
#include "g_local.h"
#include "jkg_local.h"
#include "w_local.h"

extern cvar_t *g_spskill;

/*
 * Matches NPC_AimAdjust on a clear shot: +2 each time aimDebounce fires, with
 * TIMER_Set( NPC, "aimDebounce", Q_irand( debounce, debounce + 1000 ) ) and
 * debounce = 500 + (3 - g_spskill) * 100. Average interval is debounce + 500 ms.
 */
static int JKG_NpcAimBuildPerSecond( void )
{
	int debounce;
	int avgIntervalMs;
	int gain;

	if ( g_spskill )
	{
		debounce = 500 + ( 3 - g_spskill->integer ) * 100;
	}
	else
	{
		debounce = 600;
	}

	avgIntervalMs = debounce + 500;
	gain = ( 2 * 1000 + avgIntervalMs / 2 ) / avgIntervalMs;
	if ( gain < 1 )
	{
		gain = 1;
	}

	return gain;
}

static float JKG_BlasterNpcSpreadDegrees( int currentAim )
{
	return BLASTER_NPC_SPREAD + ( 6.0f - (float)currentAim ) * BLASTER_NPC_AIM_SPREAD_SCALE;
}

static qboolean JKG_NpcIsTrooperSpreadClass( const gentity_t *ent )
{
	if ( !ent || !ent->client )
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

static float JKG_TrooperAimSpread( int currentAim, float baseSpread )
{
	return baseSpread + ( 6.0f - (float)currentAim ) * 0.25f;
}

void JKG_NpcPenalizeAimOnHit( gentity_t *self, int damage, gentity_t *attacker )
{
	if ( !JKG_AI )
	{
		return;
	}

	if ( !self || !self->NPC || !self->client || damage <= 0 )
	{
		return;
	}

	if ( attacker && attacker->client && self->client->playerTeam
		&& attacker->client->playerTeam == self->client->playerTeam )
	{
		return;
	}

	self->NPC->currentAim -= JKG_NpcAimBuildPerSecond();
	if ( self->NPC->currentAim > self->NPC->stats.aim )
	{
		self->NPC->currentAim = self->NPC->stats.aim;
	}
	else if ( self->NPC->currentAim < -30 )
	{
		self->NPC->currentAim = -30;
	}
}

float JKG_GetNpcWeaponSpreadDegrees( const gentity_t *ent )
{
	int weapon;
	int currentAim;

	if ( !ent || !ent->NPC || !ent->client )
	{
		return 0.0f;
	}

	weapon = ent->client->ps.weapon;
	currentAim = ent->NPC->currentAim;

	switch ( weapon )
	{
	case WP_BLASTER:
		return JKG_BlasterNpcSpreadDegrees( currentAim );

	case WP_BRYAR_PISTOL:
	case WP_BLASTER_PISTOL:
		if ( currentAim >= 5 )
		{
			return 0.0f;
		}
		if ( ent->client->NPC_class == CLASS_IMPWORKER )
		{
			return JKG_TrooperAimSpread( currentAim, BLASTER_NPC_SPREAD );
		}
		return ( 5.0f - (float)currentAim ) * 0.25f;

	case WP_REPEATER:
		if ( JKG_NpcIsTrooperSpreadClass( ent ) )
		{
			return JKG_TrooperAimSpread( currentAim, REPEATER_NPC_SPREAD );
		}
		if ( ent->client->ps.weaponShotCount > 1 )
		{
			return REPEATER_SPREAD;
		}
		return 0.0f;

	case WP_BOWCASTER:
		return JKG_TrooperAimSpread( currentAim, BLASTER_NPC_SPREAD );

	default:
		return 0.0f;
	}
}

/*
 * CG_TestLine unpacks little-endian RGB into shaderRGBA:
 *   [0] = color & 0xff          (red)
 *   [1] = (color >> 8) & 0xff   (green)
 *   [2] = (color >> 16) & 0xff  (blue)
 * Packing 0xRRGGBB swaps red/blue and made worst-aim look cyan/blue.
 */
static int JKG_PackDebugLineColor( int r, int g, int b )
{
	return r | ( g << 8 ) | ( b << 16 );
}

/*
 * Base currentAim is 0 (stock default). Combat raises currentAim toward stats.aim;
 * anger / pain can push it below 0 (floor -30 in NPC_AimAdjust).
 *
 * Colors: worst red -> base white -> cap green.
 * White->green keeps blue at 0 so the lerp does not pass through cyan.
 */
static int JKG_AimConeDebugColor( int currentAim, int capAim )
{
	const int baseAim = 0;
	const int worstAim = -30;
	float t;
	int r;
	int g;
	int b;

	if ( capAim < baseAim )
	{
		capAim = baseAim;
	}

	if ( currentAim >= capAim )
	{
		return JKG_PackDebugLineColor( 0, 255, 0 );
	}

	if ( currentAim == baseAim )
	{
		return JKG_PackDebugLineColor( 255, 255, 255 );
	}

	if ( currentAim > baseAim )
	{
		if ( capAim <= baseAim )
		{
			return JKG_PackDebugLineColor( 255, 255, 255 );
		}

		t = ( (float)currentAim - (float)baseAim ) / ( (float)capAim - (float)baseAim );
		if ( t < 0.0f )
		{
			t = 0.0f;
		}
		else if ( t > 1.0f )
		{
			t = 1.0f;
		}

		r = (int)( 255.0f * ( 1.0f - t ) );
		g = 255;
		b = 0;

		return JKG_PackDebugLineColor( r, g, b );
	}

	t = ( (float)currentAim - (float)worstAim ) / ( (float)baseAim - (float)worstAim );
	if ( t < 0.0f )
	{
		t = 0.0f;
	}
	else if ( t > 1.0f )
	{
		t = 1.0f;
	}

	r = 255;
	g = (int)( 255.0f * t );
	b = (int)( 255.0f * t );

	return JKG_PackDebugLineColor( r, g, b );
}

static void JKG_DrawAimCone( const vec3_t apex, const vec3_t forward, const vec3_t right, const vec3_t up,
	float spreadDeg, float length, int color )
{
	vec3_t muzzle;
	vec3_t fwd;
	vec3_t rgt;
	vec3_t upDir;
	vec3_t ring[8];
	vec3_t edge;
	float cs;
	float sn;
	int i;
	int next;

	VectorCopy( apex, muzzle );
	VectorCopy( forward, fwd );
	VectorCopy( right, rgt );
	VectorCopy( up, upDir );

	if ( spreadDeg <= 0.0f || length <= 0.0f )
	{
		vec3_t end;
		VectorMA( muzzle, length, fwd, end );
		G_DebugLine( muzzle, end, FRAMETIME, color, qtrue );
		return;
	}

	cs = cos( DEG2RAD( spreadDeg ) );
	sn = sin( DEG2RAD( spreadDeg ) );

	for ( i = 0; i < 8; i++ )
	{
		float tangentAngle;
		vec3_t tangent;

		tangentAngle = (float)i * 45.0f;
		VectorScale( rgt, cos( DEG2RAD( tangentAngle ) ), tangent );
		VectorMA( tangent, sin( DEG2RAD( tangentAngle ) ), upDir, tangent );

		VectorScale( fwd, cs, edge );
		VectorMA( edge, sn, tangent, edge );
		VectorNormalize( edge );
		VectorMA( muzzle, length, edge, ring[i] );

		G_DebugLine( muzzle, ring[i], FRAMETIME, color, qtrue );
	}

	for ( i = 0; i < 8; i++ )
	{
		next = ( i + 1 ) % 8;
		G_DebugLine( ring[i], ring[next], FRAMETIME, color, qtrue );
	}
}

void JKG_DebugDrawNpcAimCone( gentity_t *ent )
{
	vec3_t muzzle;
	vec3_t forward;
	vec3_t right;
	vec3_t up;
	float spread;
	float length;
	int color;
	static int s_lastPrintTime;

	if ( !g_jkgDebugAimCone || !g_jkgDebugAimCone->integer )
	{
		return;
	}

	if ( !ent || !ent->client || !ent->NPC || ent->s.number == 0 )
	{
		return;
	}

	if ( ent->client->ps.weapon == WP_NONE || ent->client->ps.weapon == WP_SABER )
	{
		return;
	}

	spread = JKG_GetNpcWeaponSpreadDegrees( ent );

	if ( ent->client->renderInfo.mPCalcTime >= level.time - FRAMETIME * 2 )
	{
		VectorCopy( ent->client->renderInfo.muzzlePoint, muzzle );
	}
	else
	{
		CalcEntitySpot( ent, SPOT_WEAPON, muzzle );
	}

	AngleVectors( ent->client->ps.viewangles, forward, right, up );

	if ( ent->enemy && ent->enemy->inuse )
	{
		length = Distance( muzzle, ent->enemy->currentOrigin );
		if ( length < 64.0f )
		{
			length = 64.0f;
		}
	}
	else
	{
		length = 256.0f;
	}

	color = JKG_AimConeDebugColor( ent->NPC->currentAim, ent->NPC->stats.aim );
	JKG_DrawAimCone( muzzle, forward, right, up, spread, length, color );

	if ( g_jkgDebugAimCone->integer >= 2 && level.time - s_lastPrintTime > 500 )
	{
		s_lastPrintTime = level.time;
		gi.Printf( "aimCone %s currentAim=%d stats.aim=%d S=%.2f\n",
			ent->NPC_type, ent->NPC->currentAim, ent->NPC->stats.aim, spread );
	}
}
