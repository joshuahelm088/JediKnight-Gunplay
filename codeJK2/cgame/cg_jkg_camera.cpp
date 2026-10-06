/*
===========================================================================
JKGunplay tight first/third-person camera

View angles stay locked to the player this frame. The third-person origin is
an over-the-shoulder arm on that same basis. First and third person blend
between those two live anchors; a mid-blend toggle keeps the current weight
and eases toward the new target.
===========================================================================
*/

#include "cg_local.h"
#include "cg_jkg_camera.h"
#include "../game/jkg_local.h"

#define JKG_CAM_SIZE				4
#define JKG_CAM_PITCH_LIMIT			89.0f
#define JKG_CAM_HIDE_BODY_T			0.85f

// Pose overrides that the stock chase cam is built to honor. Alpha is not one of them.
#define JKG_CAM_STOCK_OVERRIDES ( \
	CG_OVERRIDE_3RD_PERSON_ENT | \
	CG_OVERRIDE_3RD_PERSON_RNG | \
	CG_OVERRIDE_3RD_PERSON_ANG | \
	CG_OVERRIDE_3RD_PERSON_VOF | \
	CG_OVERRIDE_3RD_PERSON_POF | \
	CG_OVERRIDE_3RD_PERSON_CDP )

static vec3_t s_camMins = { -JKG_CAM_SIZE, -JKG_CAM_SIZE, -JKG_CAM_SIZE };
static vec3_t s_camMaxs = { JKG_CAM_SIZE, JKG_CAM_SIZE, JKG_CAM_SIZE };

static qboolean s_blendInit;
static qboolean s_toThird;
static float s_originWeight;
static float s_weight;
static float s_legT;
static int s_legStart;

static qboolean s_armInit;
static float s_armFraction;
static int s_armLastTime;

static int s_showBodyFrame;
static qboolean s_showBody;

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

static float JKG_CvarFloat( cvar_t *cv )
{
	if ( !cv )
	{
		return 0.0f;
	}

	return cv->value;
}

static float JKG_CvarFloatNonNegative( cvar_t *cv )
{
	float value;

	value = JKG_CvarFloat( cv );
	if ( value < 0.0f )
	{
		value = 0.0f;
	}

	return value;
}

static float JKG_EaseOutCubic( float t )
{
	float inv;

	if ( t <= 0.0f )
	{
		return 0.0f;
	}
	if ( t >= 1.0f )
	{
		return 1.0f;
	}

	inv = 1.0f - t;
	return 1.0f - inv * inv * inv;
}

static void JKG_ClampPitch( vec3_t angles )
{
	if ( angles[PITCH] > JKG_CAM_PITCH_LIMIT )
	{
		angles[PITCH] = JKG_CAM_PITCH_LIMIT;
	}
	else if ( angles[PITCH] < -JKG_CAM_PITCH_LIMIT )
	{
		angles[PITCH] = -JKG_CAM_PITCH_LIMIT;
	}
}

static void JKG_LerpAngles( const vec3_t from, const vec3_t to, float t, vec3_t out )
{
	int i;

	for ( i = 0; i < 3; i++ )
	{
		out[i] = from[i] + AngleSubtract( to[i], from[i] ) * t;
	}
}

static void JKG_ResetBlend( qboolean toThird )
{
	s_toThird = toThird;
	s_originWeight = toThird ? 1.0f : 0.0f;
	s_weight = s_originWeight;
	s_legT = 1.0f;
	s_legStart = cg.time;
	s_blendInit = qtrue;
	s_armInit = qfalse;
}

/*
================
JKG_UpdateBlend

weight 0 is first person, 1 is third. A toggle keeps the current weight as the
start of a new ease-out toward the other anchor.
================
*/
static void JKG_UpdateBlend( qboolean wantThird )
{
	int duration;
	float t;
	float eased;

	if ( !s_blendInit || cg.time < s_legStart )
	{
		JKG_ResetBlend( wantThird );
		return;
	}

	if ( wantThird != s_toThird )
	{
		s_originWeight = s_weight;
		s_toThird = wantThird;
		s_legStart = cg.time;
	}

	duration = JKG_CvarIntegerNonNegative( g_jkgCamBlend );
	if ( duration <= 0 )
	{
		s_weight = s_toThird ? 1.0f : 0.0f;
		s_originWeight = s_weight;
		s_legT = 1.0f;
		return;
	}

	t = (float)( cg.time - s_legStart ) / (float)duration;
	if ( t < 0.0f )
	{
		t = 0.0f;
	}
	else if ( t > 1.0f )
	{
		t = 1.0f;
	}
	s_legT = t;
	eased = JKG_EaseOutCubic( t );

	if ( s_toThird )
	{
		s_weight = s_originWeight + ( 1.0f - s_originWeight ) * eased;
	}
	else
	{
		s_weight = s_originWeight * ( 1.0f - eased );
	}

	if ( t >= 1.0f )
	{
		s_weight = s_toThird ? 1.0f : 0.0f;
		s_originWeight = s_weight;
	}
}

static void JKG_UpdateShowBody( void )
{
	if ( s_toThird )
	{
		// Visible from the first frame of a first-to-third blend.
		s_showBody = qtrue;
	}
	else if ( s_legT >= 1.0f )
	{
		s_showBody = qfalse;
	}
	else if ( s_legT >= JKG_CAM_HIDE_BODY_T )
	{
		// Hide near the end of a third-to-first blend, before the camera enters the head.
		s_showBody = qfalse;
	}
	else
	{
		s_showBody = qtrue;
	}

	s_showBodyFrame = cg.clientFrame;
}

/*
================
JKG_PullArm

Blocked traces shorten the arm immediately. Clearing traces ease back out so
the camera does not pop away from a wall.
================
*/
static void JKG_PullArm( const vec3_t pivot, const vec3_t ideal, vec3_t out )
{
	trace_t tr;
	vec3_t delta;
	float desired;
	int recover;
	int dt;
	float step;

	CG_Trace( &tr, pivot, s_camMins, s_camMaxs, ideal, cg.predicted_player_state.clientNum, MASK_SOLID );
	desired = tr.fraction;
	if ( desired < 0.0f )
	{
		desired = 0.0f;
	}
	else if ( desired > 1.0f )
	{
		desired = 1.0f;
	}

	if ( !s_armInit || cg.time < s_armLastTime )
	{
		s_armFraction = desired;
		s_armInit = qtrue;
	}
	else if ( desired <= s_armFraction )
	{
		s_armFraction = desired;
	}
	else
	{
		recover = JKG_CvarIntegerNonNegative( g_jkgCamRecover );
		dt = cg.time - s_armLastTime;
		if ( recover <= 0 || dt >= recover )
		{
			s_armFraction = desired;
		}
		else if ( dt > 0 )
		{
			step = JKG_EaseOutCubic( (float)dt / (float)recover );
			s_armFraction += ( desired - s_armFraction ) * step;
			if ( s_armFraction > desired )
			{
				s_armFraction = desired;
			}
		}
	}

	s_armLastTime = cg.time;

	VectorSubtract( ideal, pivot, delta );
	VectorMA( pivot, s_armFraction, delta, out );
}

static void JKG_CalcThirdPerson( const vec3_t origin, const vec3_t angles, vec3_t outOrg, vec3_t outAng )
{
	vec3_t pivot;
	vec3_t forward;
	vec3_t right;
	vec3_t up;
	vec3_t ideal;
	float range;
	float shoulder;
	float height;

	VectorCopy( angles, outAng );
	JKG_ClampPitch( outAng );

	VectorCopy( origin, pivot );
	pivot[2] += cg.predicted_player_state.viewheight;

	range = JKG_CvarFloatNonNegative( g_jkgCamRange );
	shoulder = JKG_CvarFloat( g_jkgCamShoulder );
	height = JKG_CvarFloat( g_jkgCamHeight );

	AngleVectors( outAng, forward, right, up );
	VectorCopy( pivot, ideal );
	VectorMA( ideal, -range, forward, ideal );
	VectorMA( ideal, shoulder, right, ideal );
	VectorMA( ideal, height, up, ideal );

	JKG_PullArm( pivot, ideal, outOrg );
}

qboolean JKG_TightCamActive( void )
{
	if ( !JKG_TIGHT_CAM )
	{
		return qfalse;
	}
	if ( !cg.snap )
	{
		return qfalse;
	}
	if ( cg.zoomMode )
	{
		return qfalse;
	}
	if ( in_camera )
	{
		return qfalse;
	}
	if ( cg.snap->ps.stats[STAT_HEALTH] <= 0 )
	{
		return qfalse;
	}
	if ( g_entities[0].client && g_entities[0].client->NPC_class == CLASS_ATST )
	{
		return qfalse;
	}
	if ( cg.snap->ps.viewEntity > 0 && cg.snap->ps.viewEntity < ENTITYNUM_WORLD )
	{
		return qfalse;
	}
	if ( cg.overrides.active & JKG_CAM_STOCK_OVERRIDES )
	{
		return qfalse;
	}
	// Stock first-person saber view (raised eye, short pull-back) stays on the old path.
	if ( !cg_thirdPerson.integer
		&& ( cg.snap->ps.weapon == WP_SABER || cg.snap->ps.weapon == WP_MELEE ) )
	{
		return qfalse;
	}

	return qtrue;
}

void JKG_OffsetTightCamera( void )
{
	vec3_t savedOrg;
	vec3_t savedAng;
	vec3_t p1;
	vec3_t a1;
	vec3_t p3;
	vec3_t a3;
	vec3_t blendedOrg;
	int i;

	VectorCopy( cg.refdef.vieworg, savedOrg );
	VectorCopy( cg.refdefViewAngles, savedAng );

	CG_OffsetFirstPersonView( qfalse );
	VectorCopy( cg.refdef.vieworg, p1 );
	VectorCopy( cg.refdefViewAngles, a1 );

	JKG_CalcThirdPerson( savedOrg, savedAng, p3, a3 );
	JKG_UpdateBlend( cg_thirdPerson.integer ? qtrue : qfalse );
	JKG_UpdateShowBody();

	for ( i = 0; i < 3; i++ )
	{
		blendedOrg[i] = p1[i] + ( p3[i] - p1[i] ) * s_weight;
	}
	VectorCopy( blendedOrg, cg.refdef.vieworg );
	JKG_LerpAngles( a1, a3, s_weight, cg.refdefViewAngles );
}

qboolean JKG_ShowPlayerBody( void )
{
	if ( s_showBodyFrame != cg.clientFrame )
	{
		return qfalse;
	}

	return s_showBody;
}

qboolean JKG_ViewWeaponReady( void )
{
	if ( s_showBodyFrame != cg.clientFrame )
	{
		return qtrue;
	}

	// Hide the first-person gun until a blend into first person has finished.
	if ( s_toThird || s_legT < 1.0f )
	{
		return qfalse;
	}

	return qtrue;
}
