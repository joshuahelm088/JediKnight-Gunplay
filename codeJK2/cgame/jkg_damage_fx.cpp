/*
===========================================================================
JKGunplay — DF2-style full-screen hit tint (client)
===========================================================================
*/

#include "../game/g_local.h"
#include "../game/jkg_local.h"
#include "cg_local.h"

static vec3_t s_hitTint;
static vec3_t s_pickupTint;

// DF2 pow_*.cog: AddDynamicTint(player, 0.0, 0.0, 0.2);
// Decay is sithPlayer_Tick: tint -= deltaTime * 0.4 (same engine rate as hit tints, not the hit cvar).
#define JKG_DF2_PICKUP_TINT_R		0.0f
#define JKG_DF2_PICKUP_TINT_G		0.0f
#define JKG_DF2_PICKUP_TINT_B		0.2f
#define JKG_DF2_PICKUP_TINT_DECAY	0.4f
#define JKG_DF2_PICKUP_OVERLAY_ALPHA	0.1f	// one pickup: 0.2 * ApplyTint 0.5

static float JKG_ClampFloat( float value, float low, float high )
{
	if ( value < low )
	{
		return low;
	}
	if ( value > high )
	{
		return high;
	}
	return value;
}

static float JKG_CvarFloatNonNegative( cvar_t *cv )
{
	if ( !cv )
	{
		return 0.0f;
	}
	if ( cv->value < 0.0f )
	{
		return 0.0f;
	}
	return cv->value;
}

static void JKG_HitTintDecay( void )
{
	const float decay = JKG_CvarFloatNonNegative( g_jkgHitTintDecay );
	float step;

	if ( decay <= 0.0f )
	{
		return;
	}

	step = decay * ( (float)cg.frametime / 1000.0f );

	if ( s_hitTint[0] > 0.0f )
	{
		s_hitTint[0] = JKG_ClampFloat( s_hitTint[0] - step, 0.0f, 1.0f );
	}
	if ( s_hitTint[1] > 0.0f )
	{
		s_hitTint[1] = JKG_ClampFloat( s_hitTint[1] - step, 0.0f, 1.0f );
	}
}

static void JKG_PickupTintDecay( void )
{
	const float step = JKG_DF2_PICKUP_TINT_DECAY * ( (float)cg.frametime / 1000.0f );
	int i;

	for ( i = 0; i < 3; i++ )
	{
		if ( s_pickupTint[i] > 0.0f )
		{
			s_pickupTint[i] = JKG_ClampFloat( s_pickupTint[i] - step, 0.0f, 1.0f );
		}
	}
}

void JKG_PickupTintAdd( void )
{
	if ( !JKG_HUD )
	{
		return;
	}

	s_pickupTint[0] = JKG_ClampFloat( s_pickupTint[0] + JKG_DF2_PICKUP_TINT_R, 0.0f, 1.0f );
	s_pickupTint[1] = JKG_ClampFloat( s_pickupTint[1] + JKG_DF2_PICKUP_TINT_G, 0.0f, 1.0f );
	s_pickupTint[2] = JKG_ClampFloat( s_pickupTint[2] + JKG_DF2_PICKUP_TINT_B, 0.0f, 1.0f );
}

void JKG_HitTintAdd( int healthDmg, int armorDmg )
{
	float maxTint;
	float healthScale;
	float shieldScale;

	if ( !JKG_HUD || !g_jkgHitTint || !g_jkgHitTint->integer )
	{
		return;
	}

	maxTint = JKG_CvarFloatNonNegative( g_jkgHitTintMax );
	healthScale = JKG_CvarFloatNonNegative( g_jkgHitTintHealthScale );
	shieldScale = JKG_CvarFloatNonNegative( g_jkgHitTintShieldScale );

	if ( healthDmg > 0 )
	{
		s_hitTint[0] = JKG_ClampFloat( s_hitTint[0] + (float)healthDmg * healthScale, 0.0f, maxTint );
	}
	if ( armorDmg > 0 )
	{
		s_hitTint[1] = JKG_ClampFloat( s_hitTint[1] + (float)armorDmg * shieldScale, 0.0f, maxTint );
	}
}

void JKG_HitTintDraw( void )
{
	float color[4];
	qboolean drawHitTint;

	if ( !JKG_HUD )
	{
		return;
	}

	if ( cg.levelShot )
	{
		return;
	}

	JKG_HitTintDecay();
	JKG_PickupTintDecay();

	drawHitTint = ( g_jkgHitTint && g_jkgHitTint->integer ) ? qtrue : qfalse;

	if ( !drawHitTint && s_pickupTint[0] <= 0.0f && s_pickupTint[1] <= 0.0f && s_pickupTint[2] <= 0.0f )
	{
		return;
	}

	if ( drawHitTint && s_hitTint[0] > 0.0f )
	{
		color[0] = g_jkgHitTintHealthR ? g_jkgHitTintHealthR->value : 0.0f;
		color[1] = g_jkgHitTintHealthG ? g_jkgHitTintHealthG->value : 0.0f;
		color[2] = g_jkgHitTintHealthB ? g_jkgHitTintHealthB->value : 0.0f;
		color[3] = s_hitTint[0];
		CG_FillRect( 0.0f, 0.0f, 640.0f, 480.0f, color );
	}

	if ( drawHitTint && s_hitTint[1] > 0.0f )
	{
		color[0] = g_jkgHitTintShieldR ? g_jkgHitTintShieldR->value : 0.0f;
		color[1] = g_jkgHitTintShieldG ? g_jkgHitTintShieldG->value : 0.0f;
		color[2] = g_jkgHitTintShieldB ? g_jkgHitTintShieldB->value : 0.0f;
		color[3] = s_hitTint[1];
		CG_FillRect( 0.0f, 0.0f, 640.0f, 480.0f, color );
	}

	if ( s_pickupTint[0] > 0.0f || s_pickupTint[1] > 0.0f || s_pickupTint[2] > 0.0f )
	{
		float strength;
		float linear;
		float extra;

		strength = s_pickupTint[0];
		if ( s_pickupTint[1] > strength )
		{
			strength = s_pickupTint[1];
		}
		if ( s_pickupTint[2] > strength )
		{
			strength = s_pickupTint[2];
		}
		if ( strength <= 0.0f )
		{
			return;
		}

		// DF2 ApplyTint is multiplicative (dark stays dark). Linear alpha
		// (tint*0.5) matches one pickup (~0.1) but 5 stacks become 50% fog.
		linear = strength * 0.5f;
		if ( linear <= JKG_DF2_PICKUP_OVERLAY_ALPHA )
		{
			color[3] = linear;
		}
		else
		{
			extra = linear - JKG_DF2_PICKUP_OVERLAY_ALPHA;
			color[3] = JKG_DF2_PICKUP_OVERLAY_ALPHA + extra / ( 1.0f + extra * 8.0f );
		}
		color[0] = s_pickupTint[0] / strength;
		color[1] = s_pickupTint[1] / strength;
		color[2] = s_pickupTint[2] / strength;
		CG_FillRect( 0.0f, 0.0f, 640.0f, 480.0f, color );
	}
}
