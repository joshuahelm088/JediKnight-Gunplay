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

static float JKG_TintMaxChannel( float r, float g, float b )
{
	float strength;

	strength = r;
	if ( g > strength )
	{
		strength = g;
	}
	if ( b > strength )
	{
		strength = b;
	}
	return strength;
}

static float JKG_TintOverlayAlpha( float strength, float alphaScale )
{
	return JKG_ClampFloat( strength * alphaScale, 0.0f, 1.0f );
}

// Linear mix matches one pulse; extra stacks compress so FillRect does not fog the view.
static float JKG_TintOverlayAlphaDiminished( float strength, float alphaScale, float onePulse )
{
	const float linear = JKG_TintOverlayAlpha( strength, alphaScale );
	float extra;

	if ( linear <= onePulse )
	{
		return linear;
	}

	extra = linear - onePulse;
	return JKG_ClampFloat( onePulse + extra / ( 1.0f + extra * 8.0f ), 0.0f, 1.0f );
}

static void JKG_DrawTintOverlay( float r, float g, float b, float alpha )
{
	float color[4];
	float strength;

	if ( alpha <= 0.0f )
	{
		return;
	}

	strength = JKG_TintMaxChannel( r, g, b );
	if ( strength <= 0.0f )
	{
		return;
	}

	color[0] = r / strength;
	color[1] = g / strength;
	color[2] = b / strength;
	color[3] = alpha;
	CG_FillRect( 0.0f, 0.0f, 640.0f, 480.0f, color );
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

	if ( drawHitTint && ( s_hitTint[0] > 0.0f || s_hitTint[1] > 0.0f ) )
	{
		float rgb[3];
		float strength;
		float alphaScale;

		rgb[0] = s_hitTint[0] * ( g_jkgHitTintHealthR ? g_jkgHitTintHealthR->value : 0.0f )
			+ s_hitTint[1] * ( g_jkgHitTintShieldR ? g_jkgHitTintShieldR->value : 0.0f );
		rgb[1] = s_hitTint[0] * ( g_jkgHitTintHealthG ? g_jkgHitTintHealthG->value : 0.0f )
			+ s_hitTint[1] * ( g_jkgHitTintShieldG ? g_jkgHitTintShieldG->value : 0.0f );
		rgb[2] = s_hitTint[0] * ( g_jkgHitTintHealthB ? g_jkgHitTintHealthB->value : 0.0f )
			+ s_hitTint[1] * ( g_jkgHitTintShieldB ? g_jkgHitTintShieldB->value : 0.0f );

		strength = JKG_TintMaxChannel( rgb[0], rgb[1], rgb[2] );
		if ( strength > 0.0f )
		{
			alphaScale = JKG_CvarFloatNonNegative( g_jkgHitTintAlphaScale );
			JKG_DrawTintOverlay( rgb[0], rgb[1], rgb[2], JKG_TintOverlayAlpha( strength, alphaScale ) );
		}
	}

	if ( s_pickupTint[0] > 0.0f || s_pickupTint[1] > 0.0f || s_pickupTint[2] > 0.0f )
	{
		float strength;
		float alphaScale;
		float onePulse;

		strength = JKG_TintMaxChannel( s_pickupTint[0], s_pickupTint[1], s_pickupTint[2] );
		if ( strength <= 0.0f )
		{
			return;
		}

		alphaScale = JKG_CvarFloatNonNegative( g_jkgPickupTintAlphaScale );
		onePulse = JKG_DF2_PICKUP_TINT_B * alphaScale;
		JKG_DrawTintOverlay( s_pickupTint[0], s_pickupTint[1], s_pickupTint[2],
			JKG_TintOverlayAlphaDiminished( strength, alphaScale, onePulse ) );
	}
}
