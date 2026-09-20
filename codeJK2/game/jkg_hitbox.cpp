/*
===========================================================================
JKGunplay - projectile shot hulls (separate from movement AABB)
===========================================================================
*/

#include "g_headers.h"

#include "g_local.h"
#include "jkg_local.h"

static float JKG_ClampHitboxScale( float scale )
{
	if ( scale < 1.0f )
	{
		scale = 1.0f;
	}
	else if ( scale > 4.0f )
	{
		scale = 4.0f;
	}

	return scale;
}

float JKG_GetNpcHitboxScale( const gentity_t *ent )
{
	float scale;

	if ( !ent || !ent->client || !g_jkgNpcHitboxScale )
	{
		return 1.0f;
	}

	if ( ent->client->jkgHitboxScale > 0.0f )
	{
		scale = ent->client->jkgHitboxScale;
	}
	else
	{
		scale = g_jkgNpcHitboxScale->value;
	}

	return JKG_ClampHitboxScale( scale );
}

void JKG_GetNpcShotAbsBounds( const gentity_t *ent, vec3_t absmin, vec3_t absmax )
{
	float scale;
	vec3_t mins;
	vec3_t maxs;

	VectorCopy( ent->mins, mins );
	VectorCopy( ent->maxs, maxs );

	scale = JKG_GetNpcHitboxScale( ent );
	if ( scale > 1.0f )
	{
		mins[0] *= scale;
		mins[1] *= scale;
		maxs[0] *= scale;
		maxs[1] *= scale;
	}

	VectorAdd( ent->currentOrigin, mins, absmin );
	VectorAdd( ent->currentOrigin, maxs, absmax );
}

static qboolean JKG_RayHitAabb( const vec3_t start, const vec3_t end, const vec3_t absmin, const vec3_t absmax, float *outFrac, vec3_t outNormal )
{
	vec3_t dir;
	float tmin = 0.0f;
	float tmax = 1.0f;
	int i;
	int hitAxis = -1;
	float hitSign = 0.0f;

	VectorSubtract( end, start, dir );

	for ( i = 0; i < 3; i++ )
	{
		if ( dir[i] == 0.0f )
		{
			if ( start[i] < absmin[i] || start[i] > absmax[i] )
			{
				return qfalse;
			}
			continue;
		}

		const float inv = 1.0f / dir[i];
		float t0 = ( absmin[i] - start[i] ) * inv;
		float t1 = ( absmax[i] - start[i] ) * inv;
		float sign = -1.0f;

		if ( t0 > t1 )
		{
			const float tmp = t0;
			t0 = t1;
			t1 = tmp;
			sign = 1.0f;
		}

		if ( t0 > tmin )
		{
			tmin = t0;
			hitAxis = i;
			hitSign = sign;
		}
		if ( t1 < tmax )
		{
			tmax = t1;
		}
		if ( tmin > tmax )
		{
			return qfalse;
		}
	}

	if ( tmin > 1.0f )
	{
		return qfalse;
	}

	if ( tmin < 0.0f )
	{
		tmin = 0.0f;
	}

	*outFrac = tmin;
	VectorClear( outNormal );
	if ( hitAxis >= 0 )
	{
		outNormal[hitAxis] = hitSign;
	}
	else
	{
		outNormal[2] = 1.0f;
	}

	return qtrue;
}

void JKG_MissileClipToNpcShotHitboxes( gentity_t *missile, const vec3_t start, const vec3_t end, int passEntityNum, int contentmask, trace_t *tr )
{
	int i;
	gentity_t *npc;
	vec3_t absmin;
	vec3_t absmax;
	vec3_t hitNormal;
	float frac;
	float bestFrac;

	if ( !tr || !JKG_WEAPONS || !g_jkgProjectileAabbHits || !g_jkgProjectileAabbHits->integer )
	{
		return;
	}

	bestFrac = tr->fraction;

	for ( i = 1; i < ENTITYNUM_WORLD; i++ )
	{
		if ( !PInUse( i ) )
		{
			continue;
		}

		npc = &g_entities[i];
		if ( !npc->client || !npc->NPC )
		{
			continue;
		}
		if ( i == passEntityNum )
		{
			continue;
		}
		if ( missile && missile->owner && npc == missile->owner )
		{
			continue;
		}
		if ( !( npc->contents & contentmask ) )
		{
			continue;
		}
		if ( JKG_GetNpcHitboxScale( npc ) <= 1.0f )
		{
			continue;
		}

		JKG_GetNpcShotAbsBounds( npc, absmin, absmax );

		if ( missile )
		{
			absmin[0] -= missile->maxs[0];
			absmin[1] -= missile->maxs[1];
			absmin[2] -= missile->maxs[2];
			absmax[0] -= missile->mins[0];
			absmax[1] -= missile->mins[1];
			absmax[2] -= missile->mins[2];
		}

		if ( !JKG_RayHitAabb( start, end, absmin, absmax, &frac, hitNormal ) )
		{
			continue;
		}

		if ( frac >= bestFrac )
		{
			continue;
		}

		bestFrac = frac;
		tr->fraction = frac;
		tr->entityNum = i;
		tr->contents = npc->contents;
		tr->surfaceFlags = 0;
		tr->allsolid = qfalse;
		tr->startsolid = ( frac <= 0.0f ) ? qtrue : qfalse;
		VectorCopy( hitNormal, tr->plane.normal );
		tr->endpos[0] = start[0] + ( end[0] - start[0] ) * frac;
		tr->endpos[1] = start[1] + ( end[1] - start[1] ) * frac;
		tr->endpos[2] = start[2] + ( end[2] - start[2] ) * frac;
	}
}
