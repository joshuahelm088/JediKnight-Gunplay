/*
===========================================================================
JKGunplay - NPC AI state debug marker (colored sprite above head)
===========================================================================
*/

#include "g_headers.h"

#include "ai.h"
#include "b_local.h"
#include "bstate.h"
#include "g_local.h"
#include "jkg_local.h"

/*
 * G_DebugLine / CG_TestLine color packing (see jkg_npc_aim.cpp).
 * FX_AddSprite from NPC_Think does not show reliably; lines do.
 */
static int JKG_PackDebugLineColor( int r, int g, int b )
{
	return r | ( g << 8 ) | ( b << 16 );
}

static int JKG_RgbToDebugColor( const vec3_t rgb )
{
	int r;
	int g;
	int b;

	r = (int)( rgb[0] * 255.0f );
	g = (int)( rgb[1] * 255.0f );
	b = (int)( rgb[2] * 255.0f );

	if ( r < 0 )
	{
		r = 0;
	}
	else if ( r > 255 )
	{
		r = 255;
	}

	if ( g < 0 )
	{
		g = 0;
	}
	else if ( g > 255 )
	{
		g = 255;
	}

	if ( b < 0 )
	{
		b = 0;
	}
	else if ( b > 255 )
	{
		b = 255;
	}

	return JKG_PackDebugLineColor( r, g, b );
}

static void JKG_DrawStateMarkerLines( const vec3_t head, int color )
{
	vec3_t top;
	vec3_t a;
	vec3_t b;
	const int duration = FRAMETIME * 3;

	VectorCopy( head, top );
	top[2] += 52.0f;

	G_DebugLine( head, top, duration, color, qtrue );

	VectorCopy( top, a );
	VectorCopy( top, b );
	a[0] += 10.0f;
	b[0] -= 10.0f;
	G_DebugLine( a, b, duration, color, qtrue );

	a[0] -= 10.0f;
	a[1] += 10.0f;
	b[0] += 10.0f;
	b[1] -= 10.0f;
	G_DebugLine( a, b, duration, color, qtrue );
}

static void JKG_SetRgb( vec3_t rgb, float r, float g, float b )
{
	rgb[0] = r;
	rgb[1] = g;
	rgb[2] = b;
}

static qboolean JKG_NpcIsInvestigating( const gentity_t *ent )
{
	if ( !ent || !ent->NPC )
	{
		return qfalse;
	}

	if ( ent->NPC->tempBehavior == BS_INVESTIGATE )
	{
		return qtrue;
	}

	if ( ent->NPC->behaviorState == BS_INVESTIGATE )
	{
		return qtrue;
	}

	if ( ent->NPC->investigateCount > 0
		&& ( ent->NPC->investigateDebounceTime + ent->NPC->pauseTime ) > level.time )
	{
		return qtrue;
	}

	return qfalse;
}

static void JKG_NpcStateColor( gentity_t *ent, vec3_t rgb, const char **name )
{
	const char *stateName;

	if ( !ent || !ent->NPC )
	{
		JKG_SetRgb( rgb, 0.5f, 0.5f, 0.5f );
		stateName = "invalid";
		*name = stateName;
		return;
	}

	if ( !ent->enemy )
	{
		if ( JKG_NpcIsInvestigating( ent ) )
		{
			JKG_SetRgb( rgb, 1.0f, 1.0f, 0.0f );
			stateName = "investigate";
		}
		else
		{
			JKG_SetRgb( rgb, 0.55f, 0.55f, 0.55f );
			stateName = "idle";
		}
		*name = stateName;
		return;
	}

	if ( !TIMER_Done( ent, "flee" ) )
	{
		JKG_SetRgb( rgb, 1.0f, 0.0f, 0.0f );
		stateName = "flee";
		*name = stateName;
		return;
	}

	switch ( ent->NPC->squadState )
	{
	case SQUAD_STAND_AND_SHOOT:
		JKG_SetRgb( rgb, 1.0f, 0.55f, 0.0f );
		stateName = "stand_and_shoot";
		break;
	case SQUAD_COVER:
		JKG_SetRgb( rgb, 0.0f, 0.35f, 1.0f );
		stateName = "cover";
		break;
	case SQUAD_POINT:
		JKG_SetRgb( rgb, 0.0f, 1.0f, 1.0f );
		stateName = "point";
		break;
	case SQUAD_TRANSITION:
		JKG_SetRgb( rgb, 0.0f, 1.0f, 0.0f );
		stateName = "transition";
		break;
	case SQUAD_SCOUT:
		JKG_SetRgb( rgb, 1.0f, 0.0f, 1.0f );
		stateName = "scout";
		break;
	case SQUAD_RETREAT:
		JKG_SetRgb( rgb, 1.0f, 0.0f, 0.0f );
		stateName = "retreat";
		break;
	case SQUAD_IDLE:
	default:
		JKG_SetRgb( rgb, 1.0f, 1.0f, 1.0f );
		stateName = "combat_idle";
		break;
	}

	*name = stateName;
}

void JKG_DebugDrawNpcState( gentity_t *ent )
{
	vec3_t headPos;
	vec3_t rgb;
	const char *stateName;
	static int s_lastPrintTime;

	if ( !g_jkgDebugNpcState || !g_jkgDebugNpcState->integer )
	{
		return;
	}

	if ( !ent || !ent->client || !ent->NPC || ent->s.number == 0 )
	{
		return;
	}

	if ( ent->health <= 0 )
	{
		return;
	}

	CalcEntitySpot( ent, SPOT_HEAD, headPos );
	JKG_NpcStateColor( ent, rgb, &stateName );
	JKG_DrawStateMarkerLines( headPos, JKG_RgbToDebugColor( rgb ) );

	if ( g_jkgDebugNpcState->integer >= 2 && level.time - s_lastPrintTime > 500 )
	{
		s_lastPrintTime = level.time;
		gi.Printf( "npcState %s #%d %s squad=%d enemy=%s\n",
			ent->NPC_type ? ent->NPC_type : "?",
			ent->s.number,
			stateName,
			ent->NPC->squadState,
			( ent->enemy && ent->enemy->inuse ) ? "yes" : "no" );
	}
}
