/*
===========================================================================
JKGunplay - generic stormtrooper combat movement (range keep + hunt + strafe)

Used when not running a scripted nav task or combat-point transition.
Inspired by classic FPS loops (Quake 2 range + slide, Halo/Crysis LKP hunt):
  - no LOS: nav to last known, then to the enemy
  - LOS: keep an ideal firing range, back up if too close, shuffle in-band
===========================================================================
*/

#include "g_headers.h"

#include "b_local.h"
#include "g_nav.h"
#include "jkg_local.h"

extern void AI_GroupUpdateSquadstates( AIGroupInfo_t *group, gentity_t *member, int newSquadState );
extern qboolean G_ExpandPointToBBox( vec3_t point, const vec3_t mins, const vec3_t maxs, int ignore, int clipmask );
extern qboolean Q3_TaskIDPending( gentity_t *ent, taskID_t taskType );

qboolean JKG_ST_CombatMoveEnabled( void )
{
	return ( g_jkgCombatMove && g_jkgCombatMove->integer ) ? qtrue : qfalse;
}

static void JKG_ST_SetSquadState( int newState )
{
	if ( !NPC || !NPCInfo )
	{
		return;
	}
	if ( NPCInfo->squadState == newState )
	{
		return;
	}
	AI_GroupUpdateSquadstates( NPCInfo->group, NPC, newState );
}

static qboolean JKG_ST_CombatMoveAllowed( void )
{
	if ( !JKG_ST_CombatMoveEnabled() )
	{
		return qfalse;
	}
	if ( !NPC || !NPCInfo || !NPC->client || !NPC->enemy )
	{
		return qfalse;
	}
	if ( !( NPCInfo->scriptFlags & SCF_CHASE_ENEMIES ) )
	{
		return qfalse;
	}
	if ( Q3_TaskIDPending( NPC, TID_MOVE_NAV ) )
	{
		return qfalse;
	}
	if ( !TIMER_Done( NPC, "flee" ) )
	{
		return qfalse;
	}
	if ( NPC->s.weapon == WP_NONE )
	{
		return qfalse;
	}
	return qtrue;
}

static qboolean JKG_ST_DropCombatGoal( vec3_t dest )
{
	trace_t tr;
	vec3_t end;
	const int clipmask = ( NPC->clipmask & ~CONTENTS_BODY ) | CONTENTS_BOTCLIP;

	if ( !G_ExpandPointToBBox( dest, NPC->mins, NPC->maxs, NPC->s.number, clipmask ) )
	{
		return qfalse;
	}

	VectorCopy( dest, end );
	end[2] -= 96.0f;
	gi.trace( &tr, dest, NPC->mins, NPC->maxs, end, NPC->s.number, clipmask, G2_NOCOLLIDE, 0 );
	if ( tr.startsolid || tr.allsolid || tr.fraction >= 1.0f )
	{
		return qfalse;
	}

	VectorCopy( tr.endpos, dest );
	return qtrue;
}

static qboolean JKG_ST_OffsetGoalWalkable( const vec3_t dir, float dist, vec3_t out )
{
	vec3_t dest;

	VectorMA( NPC->currentOrigin, dist, dir, dest );
	if ( !JKG_ST_DropCombatGoal( dest ) )
	{
		return qfalse;
	}

	VectorCopy( dest, out );
	return qtrue;
}

static void JKG_ST_StopTempGoal( void )
{
	if ( NPCInfo->goalEntity && NPCInfo->goalEntity == NPCInfo->tempGoal )
	{
		NPCInfo->goalEntity = NULL;
	}
}

static void JKG_ST_GoToPoint( vec3_t dest, qboolean walk, qboolean useNav )
{
	NPC_SetMoveGoal( NPC, dest, 16, useNav );
	if ( walk )
	{
		TIMER_Set( NPC, "jkgWalk", 250 );
	}
	else
	{
		TIMER_Set( NPC, "jkgWalk", -1 );
	}
}

static void JKG_ST_HuntEnemyEntity( void )
{
	TIMER_Set( NPC, "jkgWalk", -1 );
	NPC_FreeCombatPoint( NPCInfo->combatPoint );
	NPCInfo->goalEntity = NPC->enemy;
	JKG_ST_SetSquadState( SQUAD_SCOUT );
}

static void JKG_ST_GetHuntSpot( vec3_t out, int cheatMs )
{
	if ( cheatMs > 0
		&& NPCInfo->enemyLastSeenTime
		&& ( level.time - NPCInfo->enemyLastSeenTime ) <= cheatMs )
	{
		VectorCopy( NPC->enemy->currentOrigin, out );
		return;
	}

	if ( !VectorCompare( NPCInfo->enemyLastSeenLocation, vec3_origin ) )
	{
		VectorCopy( NPCInfo->enemyLastSeenLocation, out );
		return;
	}

	if ( NPCInfo->group && !VectorCompare( NPCInfo->group->enemyLastSeenPos, vec3_origin ) )
	{
		VectorCopy( NPCInfo->group->enemyLastSeenPos, out );
		return;
	}

	VectorCopy( NPC->enemy->currentOrigin, out );
}

static int JKG_ST_StrafeSign( void )
{
	return TIMER_Done( NPC, "jkgStrafeNeg" ) ? 1 : -1;
}

static void JKG_ST_FlipStrafe( void )
{
	if ( TIMER_Done( NPC, "jkgStrafeNeg" ) )
	{
		TIMER_Set( NPC, "jkgStrafeNeg", 60000 );
	}
	else
	{
		TIMER_Set( NPC, "jkgStrafeNeg", -1 );
	}
}

static qboolean JKG_ST_TryStrafe( int strafeDist )
{
	vec3_t away, right, dest;
	float dist;

	VectorSubtract( NPC->currentOrigin, NPC->enemy->currentOrigin, away );
	away[2] = 0.0f;
	if ( VectorNormalize( away ) < 1.0f )
	{
		AngleVectors( NPC->client->ps.viewangles, NULL, right, NULL );
	}
	else
	{
		right[0] = -away[1];
		right[1] = away[0];
		right[2] = 0.0f;
		VectorNormalize( right );
	}

	dist = (float)strafeDist;
	if ( dist < 24.0f )
	{
		dist = 24.0f;
	}

	if ( JKG_ST_StrafeSign() < 0 )
	{
		VectorScale( right, -1.0f, right );
	}

	if ( !JKG_ST_OffsetGoalWalkable( right, dist, dest ) )
	{
		JKG_ST_FlipStrafe();
		VectorScale( right, -1.0f, right );
		if ( !JKG_ST_OffsetGoalWalkable( right, dist, dest ) )
		{
			return qfalse;
		}
	}

	JKG_ST_GoToPoint( dest, qtrue, qfalse );
	JKG_ST_SetSquadState( SQUAD_STAND_AND_SHOOT );
	return qtrue;
}

static void JKG_ST_StartStrafeBurst( int duration, int pause )
{
	int burst = duration;
	int wait = pause;

	if ( burst < 200 )
	{
		burst = 200;
	}
	if ( wait < 100 )
	{
		wait = 100;
	}

	if ( Q_irand( 0, 1 ) )
	{
		JKG_ST_FlipStrafe();
	}

	TIMER_Set( NPC, "jkgStrafe", burst );
	TIMER_Set( NPC, "jkgStrafeWait", burst + wait );
}

qboolean JKG_ST_CombatMoveThink( qboolean canSee, float distSq )
{
	jkgCombatMoveParms_t parms;
	vec3_t dest, dir, huntSpot;
	float dist;
	float closeDist;
	float farDist;
	float mid;
	float step;

	if ( !JKG_ST_CombatMoveAllowed() )
	{
		return qfalse;
	}

	if ( NPCInfo->combatPoint != -1 )
	{
		NPC_FreeCombatPoint( NPCInfo->combatPoint );
		NPCInfo->combatPoint = -1;
	}

	JKG_GetCombatMoveParms( NPC, &parms );
	closeDist = (float)parms.rangeMin;
	farDist = (float)parms.rangeMax;
	mid = ( closeDist + farDist ) * 0.5f;
	step = (float)parms.stepDist;

	if ( !canSee )
	{
		JKG_ST_GetHuntSpot( huntSpot, parms.huntCheatMs );
		if ( DistanceSquared( NPC->currentOrigin, huntSpot ) > 48.0f * 48.0f
			&& DistanceSquared( huntSpot, NPC->enemy->currentOrigin ) > 32.0f * 32.0f )
		{
			if ( JKG_ST_DropCombatGoal( huntSpot ) )
			{
				JKG_ST_GoToPoint( huntSpot, qfalse, qtrue );
				JKG_ST_SetSquadState( SQUAD_SCOUT );
				return qtrue;
			}
		}
		JKG_ST_HuntEnemyEntity();
		return qtrue;
	}

	dist = ( distSq > 0.0f ) ? (float)sqrt( (double)distSq ) : Distance( NPC->currentOrigin, NPC->enemy->currentOrigin );

	VectorSubtract( NPC->currentOrigin, NPC->enemy->currentOrigin, dir );
	dir[2] = 0.0f;
	if ( VectorNormalize( dir ) < 0.1f )
	{
		AngleVectors( NPC->client->ps.viewangles, dir, NULL, NULL );
		dir[2] = 0.0f;
		VectorNormalize( dir );
		VectorScale( dir, -1.0f, dir );
	}

	if ( dist < closeDist )
	{
		float want = mid - dist;
		if ( want > step )
		{
			want = step;
		}
		if ( want < 32.0f )
		{
			want = 32.0f;
		}
		if ( JKG_ST_OffsetGoalWalkable( dir, want, dest ) )
		{
			JKG_ST_GoToPoint( dest, qfalse, qfalse );
			JKG_ST_SetSquadState( SQUAD_STAND_AND_SHOOT );
			return qtrue;
		}
		if ( JKG_ST_TryStrafe( parms.strafeDist ) )
		{
			return qtrue;
		}
		JKG_ST_StopTempGoal();
		return qfalse;
	}

	if ( dist > farDist )
	{
		vec3_t toward;
		float want = dist - mid;

		VectorScale( dir, -1.0f, toward );
		if ( want > step )
		{
			want = step;
		}
		if ( want < 32.0f )
		{
			want = 32.0f;
		}
		if ( JKG_ST_OffsetGoalWalkable( toward, want, dest ) )
		{
			JKG_ST_GoToPoint( dest, qfalse, qfalse );
			JKG_ST_SetSquadState( SQUAD_SCOUT );
			return qtrue;
		}
		JKG_ST_HuntEnemyEntity();
		return qtrue;
	}

	if ( TIMER_Done( NPC, "jkgStrafeWait" ) )
	{
		JKG_ST_StartStrafeBurst( parms.strafeTime, parms.strafePause );
	}

	if ( !TIMER_Done( NPC, "jkgStrafe" ) )
	{
		if ( JKG_ST_TryStrafe( parms.strafeDist ) )
		{
			return qtrue;
		}
		TIMER_Set( NPC, "jkgStrafe", -1 );
		JKG_ST_FlipStrafe();
	}

	JKG_ST_StopTempGoal();
	return qfalse;
}

void JKG_ST_ApplyCombatWalk( void )
{
	if ( !NPC || !NPCInfo )
	{
		return;
	}
	if ( TIMER_Done( NPC, "jkgWalk" ) )
	{
		return;
	}
	ucmd.buttons |= BUTTON_WALKING;
}
