/*
===========================================================================
JKGunplay - DF2-style mounted turret death spin (misc_turret / misc_ns_turret)
===========================================================================
*/
#include "g_headers.h"
#include "g_local.h"
#include "g_functions.h"
#include "jkg_local.h"

static int JKG_CvarIntegerNonNegative( cvar_t *cv )
{
	if ( !cv )
	{
		return 0;
	}
	if ( cv->integer < 0 )
	{
		return 0;
	}
	return cv->integer;
}

static float JKG_CvarFloatPositive( cvar_t *cv )
{
	if ( !cv )
	{
		return 0.0f;
	}
	if ( cv->value <= 0.0f )
	{
		return 0.0f;
	}
	return cv->value;
}

static qboolean JKG_Turret_IsMountedGun( const gentity_t *self )
{
	if ( !self || !self->classname )
	{
		return qfalse;
	}
	if ( !Q_stricmp( self->classname, "misc_turret" ) )
	{
		return qtrue;
	}
	if ( !Q_stricmp( self->classname, "misc_ns_turret" ) )
	{
		return qtrue;
	}
	return qfalse;
}

qboolean JKG_TurretDeathSpin_Active( const gentity_t *self )
{
	if ( !self )
	{
		return qfalse;
	}
	// lastImpact holds absolute level.time when we explode; non-zero = in death spin.
	return ( self->lastImpact > 0 ) ? qtrue : qfalse;
}

static void JKG_TurretDeathSpin_PickRates( gentity_t *self )
{
	const float yawBase = JKG_CvarFloatPositive( g_jkgTurretDeathYawSpeed );
	const float pitchBase = JKG_CvarFloatPositive( g_jkgTurretDeathPitchSpeed );
	const float yawScale = 0.6f + Q_flrand( 0.0f, 1.0f ) * 0.8f;
	const float pitchScale = 0.6f + Q_flrand( 0.0f, 1.0f ) * 0.8f;

	self->movedir[0] = ( Q_flrand( 0.0f, 1.0f ) > 0.5f ? 1.0f : -1.0f ) * yawBase * yawScale;
	self->movedir[1] = ( Q_flrand( 0.0f, 1.0f ) > 0.5f ? 1.0f : -1.0f ) * pitchBase * pitchScale;
	self->aimDebounceTime = level.time + 200 + (int)( Q_flrand( 0.0f, 1.0f ) * 200.0f );
}

static void JKG_TurretDeathSpin_UpdateYaw( gentity_t *self )
{
	EvaluateTrajectory( &self->s.apos, level.time, self->currentAngles );
	self->currentAngles[YAW] = AngleNormalize360( self->currentAngles[YAW] );

	VectorCopy( self->currentAngles, self->s.apos.trBase );
	self->s.apos.trType = TR_LINEAR;
	self->s.apos.trTime = level.time;
	VectorClear( self->s.apos.trDelta );
	self->s.apos.trDelta[YAW] = self->movedir[0];
}

static void JKG_TurretDeathSpin_UpdatePitch( gentity_t *self, float dt )
{
	const float pitchLimit = 45.0f;

	self->speed += self->movedir[1] * dt;

	if ( self->speed > pitchLimit )
	{
		self->speed = pitchLimit;
		if ( self->movedir[1] > 0.0f )
		{
			self->movedir[1] = -self->movedir[1];
		}
	}
	else if ( self->speed < -pitchLimit )
	{
		self->speed = -pitchLimit;
		if ( self->movedir[1] < 0.0f )
		{
			self->movedir[1] = -self->movedir[1];
		}
	}

	Turret_ApplyPitchBone( self, self->speed );
}

static void JKG_TurretDeathSpin_MaybeSpark( gentity_t *self )
{
	vec3_t point, dir;

	if ( Q_flrand( 0.0f, 1.0f ) > 0.12f )
	{
		return;
	}

	VectorSet( dir, Q_flrand( -1.0f, 1.0f ), Q_flrand( -1.0f, 1.0f ), Q_flrand( -1.0f, 1.0f ) );
	VectorNormalize( dir );
	VectorCopy( self->currentOrigin, point );
	point[0] += Q_flrand( -8.0f, 8.0f );
	point[1] += Q_flrand( -8.0f, 8.0f );
	if ( self->spawnflags & 2 )
	{
		point[2] += Q_flrand( 0.0f, 12.0f );
	}
	else
	{
		point[2] += Q_flrand( -12.0f, 0.0f );
	}
	G_PlayEffect( "spark_exp_nosnd", point, dir );
}

qboolean JKG_TurretDeathSpin_TryStart( gentity_t *self, gentity_t *inflictor, gentity_t *attacker, int damage, int meansOfDeath )
{
	const int deathMs = JKG_CvarIntegerNonNegative( g_jkgTurretDeathMs );

	(void)inflictor;
	(void)damage;
	(void)meansOfDeath;

	if ( deathMs <= 0 )
	{
		return qfalse;
	}
	if ( !JKG_Turret_IsMountedGun( self ) )
	{
		return qfalse;
	}
	if ( JKG_TurretDeathSpin_Active( self ) )
	{
		self->lastImpact = 0;
		self->s.loopSound = 0;
		Turret_FinishExplodeDeath( self, inflictor, attacker, damage, meansOfDeath );
		return qtrue;
	}

	self->enemy = NULL;
	self->e_UseFunc = useF_NULL;
	self->e_DieFunc = dieF_turret_die;
	self->takedamage = qtrue;
	self->health = 1;
	self->flags |= FL_NOTARGET;

	self->activator = attacker;
	self->lastImpact = level.time + deathMs;
	self->disconnectDebounceTime = level.time;

	EvaluateTrajectory( &self->s.apos, level.time, self->currentAngles );
	self->currentAngles[YAW] = AngleNormalize360( self->currentAngles[YAW] );

	JKG_TurretDeathSpin_PickRates( self );
	JKG_TurretDeathSpin_UpdateYaw( self );
	Turret_ApplyPitchBone( self, self->speed );

	self->pushDebounceTime = level.time;
	self->s.loopSound = G_SoundIndex( "sound/chars/turret/move.wav" );

	return qtrue;
}

void JKG_TurretDeathSpin_Think( gentity_t *self )
{
	gentity_t *attacker;
	float dt;
	int shotDelay;

	if ( self->lastImpact <= 0 )
	{
		return;
	}

	if ( level.time >= self->lastImpact )
	{
		attacker = self->activator;
		self->lastImpact = 0;
		self->s.loopSound = 0;
		Turret_FinishExplodeDeath( self, self, attacker, 0, MOD_UNKNOWN );
		return;
	}

	dt = ( level.time - self->disconnectDebounceTime ) * 0.001f;
	if ( dt <= 0.0f )
	{
		dt = FRAMETIME * 0.001f;
	}
	self->disconnectDebounceTime = level.time;

	if ( level.time >= self->aimDebounceTime )
	{
		JKG_TurretDeathSpin_PickRates( self );
	}

	JKG_TurretDeathSpin_UpdateYaw( self );
	JKG_TurretDeathSpin_UpdatePitch( self, dt );

	shotDelay = JKG_CvarIntegerNonNegative( g_jkgTurretDeathShotMs );
	if ( shotDelay > 0 && self->pushDebounceTime < level.time )
	{
		Turret_FireFromMuzzle( self );
		self->pushDebounceTime = level.time + shotDelay;
		self->fly_sound_debounce_time = level.time;
	}

	JKG_TurretDeathSpin_MaybeSpark( self );
}
