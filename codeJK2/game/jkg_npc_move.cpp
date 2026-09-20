/*
===========================================================================
JKGunplay mod layer - NPC locomotion (speed ramp + move direction blend)
===========================================================================
*/

#include "g_headers.h"

#include "b_local.h"
#include "g_local.h"
#include "jkg_local.h"

extern qboolean PM_WalkingAnim( int anim );
extern qboolean PM_RunningAnim( int anim );
extern void G_UcmdMoveForDir( gentity_t *self, usercmd_t *cmd, vec3_t dir );

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

static float JKG_CvarFloatPositive( cvar_t *cv )
{
	float value;

	if ( !cv )
	{
		return 0.0f;
	}

	value = cv->value;
	if ( value <= 0.0f )
	{
		value = 0.0f;
	}
	return value;
}

int JKG_NpcScaleDesiredSpeed( int speed )
{
	float scale;

	if ( !JKG_MOVEMENT || speed <= 0 )
	{
		return speed;
	}

	scale = JKG_CvarFloatPositive( g_jkgNpcSpeedScale );
	if ( scale <= 0.0f )
	{
		return 0;
	}

	return (int)( (float)speed * scale );
}

#define JKG_MOVE_DBG_PREFIX "^3[JKG move]^7"

static void JKG_NpcRefreshDistToGoal( gentity_t *ent )
{
	vec3_t delta;

	if ( !ent || !ent->NPC || !ent->NPC->goalEntity || !ent->NPC->goalEntity->inuse )
	{
		return;
	}

	VectorSubtract( ent->NPC->goalEntity->currentOrigin, ent->currentOrigin, delta );
	ent->NPC->distToGoal = VectorLength( delta );
}

static float JKG_NpcStopRemainingDist( gentity_t *ent )
{
	float dist;
	float stopRadius;
	float remaining;

	if ( !ent || !ent->NPC )
	{
		return 0.0f;
	}

	JKG_NpcRefreshDistToGoal( ent );
	dist = ent->NPC->distToGoal;
	stopRadius = (float)ent->NPC->goalRadius;
	if ( stopRadius > 64.0f )
	{
		stopRadius = 64.0f;
	}
	if ( stopRadius > dist )
	{
		stopRadius = dist;
	}

	remaining = dist - stopRadius;
	if ( remaining < 0.0f )
	{
		remaining = 0.0f;
	}

	return remaining;
}

static void JKG_NpcMoveDebug( gentity_t *ent, int logLevel, const char *event, int desiredBefore, int cap, float remaining )
{
	static int s_lastLogTime[256];
	int idx;
	int throttleMs;

	if ( !g_jkgDebugNpcMove || g_jkgDebugNpcMove->integer < logLevel || !ent || !ent->NPC )
	{
		return;
	}

	throttleMs = ( g_jkgDebugNpcMove->integer >= 2 ) ? 100 : 400;
	idx = ent->s.number & 255;
	if ( logLevel < 2 )
	{
		if ( level.time - s_lastLogTime[idx] < throttleMs )
		{
			return;
		}
	}
	s_lastLogTime[idx] = level.time;

	gi.Printf( "%s #%d %s combat=%d dist=%.0f goalR=%d rem=%.0f want=%d->%d cur=%d ps=%d ucmd=(%d,%d) decel=%d stopDec=%d\n",
		JKG_MOVE_DBG_PREFIX,
		ent->s.number,
		event,
		ent->NPC->combatMove,
		ent->NPC->distToGoal,
		ent->NPC->goalRadius,
		remaining,
		desiredBefore,
		ent->NPC->desiredSpeed,
		ent->NPC->currentSpeed,
		ent->client ? ent->client->ps.speed : 0,
		ent->NPC->last_ucmd.forwardmove,
		ent->NPC->last_ucmd.rightmove,
		JKG_CvarIntegerNonNegative( g_jkgNpcDecel ),
		JKG_CvarIntegerNonNegative( g_jkgNpcStopDecel ) );
}

void JKG_NPCApplyStopSlowdown( gentity_t *ent )
{
	float remaining;
	float maxSpeed;
	int decel;
	int cap;
	int desiredBefore;

	if ( !JKG_MOVEMENT || !ent || !ent->NPC )
	{
		return;
	}

	if ( ent->NPC->aiFlags & NPCAI_NO_SLOWDOWN )
	{
		return;
	}

	remaining = JKG_NpcStopRemainingDist( ent );

	decel = JKG_CvarIntegerNonNegative( g_jkgNpcStopDecel );
	if ( decel <= 0 )
	{
		return;
	}

	maxSpeed = sqrt( 2.0f * (float)decel * remaining );
	cap = (int)ceil( maxSpeed );
	if ( cap < 0 )
	{
		cap = 0;
	}

	desiredBefore = ent->NPC->desiredSpeed;
	if ( ent->NPC->desiredSpeed > cap )
	{
		ent->NPC->desiredSpeed = cap;
		JKG_NpcMoveDebug( ent, 1, "approach_cap", desiredBefore, cap, remaining );
	}
	else if ( g_jkgDebugNpcMove && g_jkgDebugNpcMove->integer >= 2 )
	{
		JKG_NpcMoveDebug( ent, 2, "approach_ok", desiredBefore, cap, remaining );
	}
}

void JKG_NpcCombatDesiredSpeed( gentity_t *ent, usercmd_t *ucmd )
{
	qboolean moving;

	if ( !ent || !ent->NPC || !ucmd )
	{
		return;
	}

	moving = ( ucmd->forwardmove || ucmd->rightmove ) ? qtrue : qfalse;
	if ( !moving )
	{
		ent->NPC->desiredSpeed = 0;
	}
	else
	{
		ent->NPC->desiredSpeed = ( ucmd->buttons & BUTTON_WALKING ) ? ent->NPC->stats.walkSpeed : ent->NPC->stats.runSpeed;
	}
}

void JKG_NpcApplyMovementCoast( gentity_t *ent, usercmd_t *ucmd )
{
	float remaining;

	if ( !JKG_MOVEMENT || !ent || !ent->NPC || !ucmd )
	{
		return;
	}

	if ( ent->NPC->currentSpeed <= 0 )
	{
		return;
	}

	if ( ucmd->forwardmove || ucmd->rightmove )
	{
		return;
	}

	remaining = JKG_NpcStopRemainingDist( ent );
	if ( ent->NPC->desiredSpeed == 0 && remaining <= 0.0f )
	{
		return;
	}

	ucmd->forwardmove = ent->NPC->last_ucmd.forwardmove;
	ucmd->rightmove = ent->NPC->last_ucmd.rightmove;

	if ( !ucmd->forwardmove && !ucmd->rightmove && ent->client )
	{
		vec3_t vel;
		vec3_t dir;

		VectorCopy( ent->client->ps.velocity, vel );
		vel[2] = 0.0f;
		if ( VectorLengthSquared( vel ) > 256.0f )
		{
			VectorNormalize( vel );
			G_UcmdMoveForDir( ent, ucmd, vel );
		}
		else if ( !VectorCompare( ent->client->ps.moveDir, vec3_origin ) )
		{
			VectorCopy( ent->client->ps.moveDir, dir );
			G_UcmdMoveForDir( ent, ucmd, dir );
		}
	}

	if ( g_jkgDebugNpcMove && g_jkgDebugNpcMove->integer >= 2 )
	{
		JKG_NpcMoveDebug( ent, 2, "coast_ucmd", ent->NPC->desiredSpeed, 0, remaining );
	}
}

void JKG_NPCRampSpeed( gentity_t *ent, int msec )
{
	int accel;
	int decel;
	int step;

	if ( !ent || !ent->NPC || !ent->client )
	{
		return;
	}

	if ( msec < 1 )
	{
		msec = 1;
	}

	accel = JKG_CvarIntegerNonNegative( g_jkgNpcAccel );
	decel = JKG_CvarIntegerNonNegative( g_jkgNpcDecel );

	if ( ent->NPC->desiredSpeed > ent->NPC->currentSpeed )
	{
		if ( accel <= 0 )
		{
			ent->NPC->currentSpeed = ent->NPC->desiredSpeed;
			return;
		}

		step = (int)( ( (float)accel * (float)msec ) / 1000.0f );
		if ( step < 1 )
		{
			step = 1;
		}
		ent->NPC->currentSpeed += step;
		if ( ent->NPC->currentSpeed > ent->NPC->desiredSpeed )
		{
			ent->NPC->currentSpeed = ent->NPC->desiredSpeed;
		}
	}
	else if ( ent->NPC->desiredSpeed < ent->NPC->currentSpeed )
	{
		if ( decel <= 0 )
		{
			ent->NPC->currentSpeed = ent->NPC->desiredSpeed;
			return;
		}

		step = (int)( ( (float)decel * (float)msec ) / 1000.0f );
		if ( step < 1 )
		{
			step = 1;
		}
		ent->NPC->currentSpeed -= step;
		if ( ent->NPC->currentSpeed < ent->NPC->desiredSpeed )
		{
			ent->NPC->currentSpeed = ent->NPC->desiredSpeed;
		}

		if ( g_jkgDebugNpcMove && g_jkgDebugNpcMove->integer >= 1 )
		{
			JKG_NpcMoveDebug( ent, 1, "ramp_decel", ent->NPC->desiredSpeed, ent->NPC->currentSpeed,
				JKG_NpcStopRemainingDist( ent ) );
		}
	}
	else
	{
		ent->NPC->currentSpeed = ent->NPC->desiredSpeed;
	}
}

void JKG_NpcApplyMoveDir( gentity_t *self, usercmd_t *cmd, vec3_t dir )
{
	vec3_t	blended;
	vec3_t	forward, right;
	vec3_t	oldDir;
	vec3_t	angles;
	float	yawOld;
	float	yawNew;
	float	yawBlend;
	float	maxDelta;
	float	dot;
	float	turnRate;
	float	fDot;
	float	rDot;

	if ( !self || !self->client || !self->NPC || !cmd )
	{
		return;
	}

	dir[2] = 0.0f;
	if ( VectorNormalize( dir ) == 0.0f )
	{
		VectorClear( self->client->ps.moveDir );
		cmd->forwardmove = 0;
		cmd->rightmove = 0;
		return;
	}

	VectorCopy( self->client->ps.moveDir, oldDir );
	oldDir[2] = 0.0f;

	if ( VectorLengthSquared( oldDir ) < 0.01f )
	{
		VectorCopy( dir, blended );
	}
	else
	{
		VectorNormalize( oldDir );
		dot = DotProduct( oldDir, dir );
		if ( dot < 0.0f )
		{
			self->NPC->currentSpeed = 0;
		}

		vectoangles( oldDir, angles );
		yawOld = angles[YAW];
		vectoangles( dir, angles );
		yawNew = angles[YAW];

		turnRate = JKG_CvarFloatPositive( g_jkgNpcTurnRate );
		maxDelta = turnRate * ( FRAMETIME * 0.001f );
		yawBlend = yawOld + Com_Clamp( -maxDelta, maxDelta, AngleDelta( yawNew, yawOld ) );

		angles[PITCH] = 0.0f;
		angles[YAW] = yawBlend;
		angles[ROLL] = 0.0f;
		AngleVectors( angles, forward, NULL, NULL );
		blended[0] = forward[0];
		blended[1] = forward[1];
		blended[2] = 0.0f;
		VectorNormalize( blended );
	}

	VectorCopy( blended, self->client->ps.moveDir );

	AngleVectors( self->currentAngles, forward, right, NULL );

	fDot = DotProduct( forward, blended ) * 127.0f;
	rDot = DotProduct( right, blended ) * 127.0f;

	if ( fDot > 127.0f )
	{
		fDot = 127.0f;
	}
	if ( fDot < -127.0f )
	{
		fDot = -127.0f;
	}
	if ( rDot > 127.0f )
	{
		rDot = 127.0f;
	}
	if ( rDot < -127.0f )
	{
		rDot = -127.0f;
	}

	cmd->forwardmove = floor( fDot );
	cmd->rightmove = floor( rDot );
}

float JKG_NpcLocomotionAnimScale( gentity_t *ent, int anim )
{
	int walkNominal;
	int runNominal;
	float scale;
	float minScale;

	if ( !JKG_MOVEMENT || !ent || !ent->NPC || !ent->client )
	{
		return 1.0f;
	}

	if ( !PM_WalkingAnim( anim ) && !PM_RunningAnim( anim ) )
	{
		return 1.0f;
	}

	walkNominal = ent->NPC->stats.walkSpeed;
	runNominal = ent->NPC->stats.runSpeed;
	walkNominal = JKG_NpcScaleDesiredSpeed( walkNominal );
	runNominal = JKG_NpcScaleDesiredSpeed( runNominal );

	if ( PM_WalkingAnim( anim ) )
	{
		if ( walkNominal <= 0 )
		{
			return 1.0f;
		}
		scale = (float)ent->NPC->currentSpeed / (float)walkNominal;
	}
	else
	{
		if ( runNominal <= 0 )
		{
			return 1.0f;
		}
		scale = (float)ent->NPC->currentSpeed / (float)runNominal;
	}

	minScale = JKG_CvarFloatPositive( g_jkgNpcAnimMinScale );
	if ( minScale > 0.0f && scale < minScale )
	{
		scale = minScale;
	}
	if ( scale > 1.0f )
	{
		scale = 1.0f;
	}

	return scale;
}
