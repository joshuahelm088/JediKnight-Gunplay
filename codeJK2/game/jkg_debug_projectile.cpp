/*
===========================================================================
JKGunplay - projectile spawn debug logging

Enable with: g_jkgDebugProjectile 1   (server spawn pipeline)
              g_jkgDebugProjectile 2   (+ client muzzle/render extrapolation)
              g_jkgDebugProjectile 3   (+ NPC shots)
===========================================================================
*/

#include "g_headers.h"

#include "g_local.h"
#include "jkg_local.h"
#include "w_local.h"
#include "../cgame/cg_local.h"

#define JKG_PROJ_PREFIX "^3[JKG proj]^7"

static qboolean JKG_DebugProjectile_Active( const gentity_t *ent, int minLevel )
{
	if ( !g_jkgDebugProjectile || g_jkgDebugProjectile->integer < minLevel )
	{
		return qfalse;
	}

	if ( !ent || !ent->client )
	{
		return qfalse;
	}

	if ( ent->s.number != 0 && g_jkgDebugProjectile->integer < 3 )
	{
		return qfalse;
	}

	return qtrue;
}

static float JKG_VecDist( const vec3_t a, const vec3_t b )
{
	vec3_t delta;

	VectorSubtract( a, b, delta );
	return VectorLength( delta );
}

static void JKG_LogVec( const char *label, const vec3_t v )
{
	Com_Printf( "%s %s=(%.1f %.1f %.1f)\n", JKG_PROJ_PREFIX, label, v[0], v[1], v[2] );
}

void JKG_DebugProjectile_MuzzlePoint( gentity_t *ent, const char *source, const vec3_t muzzlePoint, int cacheAge )
{
	if ( !JKG_DebugProjectile_Active( ent, 1 ) )
	{
		return;
	}

	Com_Printf( "%s CalcMuzzlePoint ent=%d weapon=%d source=%s cacheAge=%d level.time=%d mPCalcTime=%d\n",
		JKG_PROJ_PREFIX,
		ent->s.number,
		ent->s.weapon,
		source,
		cacheAge,
		level.time,
		ent->client ? ent->client->renderInfo.mPCalcTime : 0 );

	JKG_LogVec( "muzzle", muzzlePoint );
	JKG_LogVec( "playerOrigin", ent->currentOrigin );
	JKG_LogVec( "clientMuzzleCache", ent->client->renderInfo.muzzlePoint );

	Com_Printf( "%s dist origin->muzzle=%.1f origin->clientCache=%.1f muzzle->clientCache=%.1f\n",
		JKG_PROJ_PREFIX,
		JKG_VecDist( ent->currentOrigin, muzzlePoint ),
		JKG_VecDist( ent->currentOrigin, ent->client->renderInfo.muzzlePoint ),
		JKG_VecDist( muzzlePoint, ent->client->renderInfo.muzzlePoint ) );
}

void JKG_DebugProjectile_TraceSetStart( gentity_t *ent, const vec3_t before, const vec3_t after, float traceFraction )
{
	vec3_t pullback;

	if ( !JKG_DebugProjectile_Active( ent, 1 ) )
	{
		return;
	}

	VectorSubtract( before, after, pullback );
	Com_Printf( "%s WP_TraceSetStart ent=%d frac=%.3f pulledBack=%.1f\n",
		JKG_PROJ_PREFIX,
		ent->s.number,
		traceFraction,
		VectorLength( pullback ) );

	JKG_LogVec( "beforeTrace", before );
	JKG_LogVec( "afterTrace", after );
	JKG_LogVec( "traceStart", ent->currentOrigin );
}

void JKG_DebugProjectile_CreateMissile( gentity_t *owner, gentity_t *missile, const vec3_t org, float vel )
{
	vec3_t viewPoint;

	if ( !JKG_DebugProjectile_Active( owner, 1 ) )
	{
		return;
	}

	ViewHeightFix( owner );
	VectorCopy( owner->currentOrigin, viewPoint );
	viewPoint[2] += owner->client->ps.viewheight;

	Com_Printf( "%s CreateMissile ent=%d missile=%d weapon=%d vel=%.0f trTime=%d level.time=%d age=%d\n",
		JKG_PROJ_PREFIX,
		owner->s.number,
		missile->s.number,
		owner->s.weapon,
		vel,
		missile->s.pos.trTime,
		level.time,
		level.time - missile->s.pos.trTime );

	JKG_LogVec( "trBase", org );
	JKG_LogVec( "viewPoint", viewPoint );
	JKG_LogVec( "clientMuzzleCache", owner->client->renderInfo.muzzlePoint );

	Com_Printf( "%s dist view->spawn=%.1f clientCache->spawn=%.1f origin->spawn=%.1f trDelta=(%.0f %.0f %.0f)\n",
		JKG_PROJ_PREFIX,
		JKG_VecDist( viewPoint, org ),
		JKG_VecDist( owner->client->renderInfo.muzzlePoint, org ),
		JKG_VecDist( owner->currentOrigin, org ),
		missile->s.pos.trDelta[0],
		missile->s.pos.trDelta[1],
		missile->s.pos.trDelta[2] );
}

void JKG_DebugProjectile_ClientMuzzle( gentity_t *ent, const char *source, const vec3_t muzzlePoint, const vec3_t viewOrg )
{
	if ( !JKG_DebugProjectile_Active( ent, 2 ) )
	{
		return;
	}

	if ( !( ent->s.eFlags & EF_FIRING ) )
	{
		return;
	}

	Com_Printf( "%s ClientMuzzle ent=%d source=%s cg.time=%d mPCalcTime=%d\n",
		JKG_PROJ_PREFIX,
		ent->s.number,
		source,
		cg.time,
		ent->client->renderInfo.mPCalcTime );

	JKG_LogVec( "tagFlash", muzzlePoint );
	JKG_LogVec( "vieworg", viewOrg );

	Com_Printf( "%s dist view->tagFlash=%.1f view->origin=%.1f\n",
		JKG_PROJ_PREFIX,
		JKG_VecDist( viewOrg, muzzlePoint ),
		JKG_VecDist( viewOrg, ent->currentOrigin ) );
}

void JKG_DebugProjectile_ClientRender( centity_t *cent )
{
	vec3_t rawBase;
	float extrapDist;
	int age;

	if ( !g_jkgDebugProjectile || g_jkgDebugProjectile->integer < 2 )
	{
		return;
	}

	if ( cent->currentState.eType != ET_MISSILE )
	{
		return;
	}

	if ( !cent->gent || !cent->gent->owner || !cent->gent->owner->client )
	{
		return;
	}

	if ( cent->gent->owner->s.number != 0 && g_jkgDebugProjectile->integer < 3 )
	{
		return;
	}

	age = cg.time - cent->currentState.pos.trTime;
	if ( age < 0 || age > 100 )
	{
		return;
	}

	VectorCopy( cent->currentState.pos.trBase, rawBase );
	extrapDist = JKG_VecDist( rawBase, cent->lerpOrigin );

	Com_Printf( "%s ClientRender missile=%d owner=%d weapon=%d age=%d cg.time=%d trTime=%d extrap=%.1f\n",
		JKG_PROJ_PREFIX,
		cent->currentState.number,
		cent->gent->owner->s.number,
		cent->currentState.weapon,
		age,
		cg.time,
		cent->currentState.pos.trTime,
		extrapDist );

	JKG_LogVec( "trBase", rawBase );
	JKG_LogVec( "lerpOrigin", cent->lerpOrigin );
	JKG_LogVec( "ownerClientMuzzle", cent->gent->owner->client->renderInfo.muzzlePoint );

	Com_Printf( "%s dist trBase->lerp=%.1f trBase->ownerMuzzle=%.1f lerp->ownerMuzzle=%.1f\n",
		JKG_PROJ_PREFIX,
		extrapDist,
		JKG_VecDist( rawBase, cent->gent->owner->client->renderInfo.muzzlePoint ),
		JKG_VecDist( cent->lerpOrigin, cent->gent->owner->client->renderInfo.muzzlePoint ) );
}
