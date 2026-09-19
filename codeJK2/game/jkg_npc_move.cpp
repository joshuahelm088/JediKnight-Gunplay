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
