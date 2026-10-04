/*
===========================================================================
JKGunplay — DF2-style full-screen hit tint (client)
===========================================================================
*/

#include "../game/g_local.h"
#include "../game/jkg_local.h"
#include "cg_local.h"

static vec3_t s_hitTint;

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

	if ( !JKG_HUD || !g_jkgHitTint || !g_jkgHitTint->integer )
	{
		return;
	}

	if ( cg.levelShot )
	{
		return;
	}

	JKG_HitTintDecay();

	if ( s_hitTint[0] <= 0.0f && s_hitTint[1] <= 0.0f )
	{
		return;
	}

	if ( s_hitTint[0] > 0.0f )
	{
		color[0] = g_jkgHitTintHealthR ? g_jkgHitTintHealthR->value : 0.0f;
		color[1] = g_jkgHitTintHealthG ? g_jkgHitTintHealthG->value : 0.0f;
		color[2] = g_jkgHitTintHealthB ? g_jkgHitTintHealthB->value : 0.0f;
		color[3] = s_hitTint[0];
		CG_FillRect( 0.0f, 0.0f, 640.0f, 480.0f, color );
	}

	if ( s_hitTint[1] > 0.0f )
	{
		color[0] = g_jkgHitTintShieldR ? g_jkgHitTintShieldR->value : 0.0f;
		color[1] = g_jkgHitTintShieldG ? g_jkgHitTintShieldG->value : 0.0f;
		color[2] = g_jkgHitTintShieldB ? g_jkgHitTintShieldB->value : 0.0f;
		color[3] = s_hitTint[1];
		CG_FillRect( 0.0f, 0.0f, 640.0f, 480.0f, color );
	}
}
