/*
===========================================================================
JKGunplay — scorch marks on Ghoul2 models

Clips a burn decal onto the posed mesh (G2API_AddSkinGore) so the mark
skins with the character. BSP hits stay on CG_ImpactMark.
===========================================================================
*/

#include "../game/g_local.h"
#include "../game/jkg_local.h"
#include "cg_local.h"

static int JKG_CvarIntegerNonNegative( cvar_t *cv )
{
	if ( !cv || cv->integer < 0 )
	{
		return 0;
	}
	return cv->integer;
}

static float JKG_CvarFloatPositive( cvar_t *cv )
{
	if ( !cv || cv->value <= 0.0f )
	{
		return 0.0f;
	}
	return cv->value;
}

static qhandle_t JKG_Ghoul2MarkShader( void )
{
	static qhandle_t shader = 0;
	static qboolean registered = qfalse;

	if ( !registered )
	{
		shader = cgi_R_RegisterShader( "gfx/damage/burnmark1" );
		registered = qtrue;
	}
	return shader;
}

static qboolean JKG_G2MarkDebugOn( void )
{
	return ( g_jkgGhoul2MarkDebug && g_jkgGhoul2MarkDebug->integer ) ? qtrue : qfalse;
}

static int JKG_PackDebugColor( int r, int g, int b )
{
	return r | ( g << 8 ) | ( b << 16 );
}

static const char *JKG_EntLabel( const gentity_t *ent )
{
	if ( !ent )
	{
		return "none";
	}
	if ( ent->NPC_type && ent->NPC_type[0] )
	{
		return ent->NPC_type;
	}
	if ( ent->classname && ent->classname[0] )
	{
		return ent->classname;
	}
	return "unknown";
}

static const char *JKG_ModelName( const gentity_t *ent )
{
	if ( !ent || !ent->ghoul2.IsValid() || ent->ghoul2.size() <= 0 )
	{
		return "none";
	}
	if ( !ent->ghoul2[0].mFileName[0] )
	{
		return "empty";
	}
	return ent->ghoul2[0].mFileName;
}

static void JKG_DrawShotDebug( const vec3_t hullPos, const vec3_t shotEnd, const vec3_t meshPos, const vec3_t meshNormal, qboolean hit )
{
	vec3_t spike;

	if ( hit )
	{
		G_DebugLine( hullPos, meshPos, 8000, JKG_PackDebugColor( 255, 220, 0 ), qtrue );
		VectorMA( meshPos, 48.0f, meshNormal, spike );
		G_DebugLine( meshPos, spike, 8000, JKG_PackDebugColor( 0, 255, 0 ), qtrue );
	}
	else
	{
		G_DebugLine( hullPos, shotEnd, 8000, JKG_PackDebugColor( 255, 40, 40 ), qtrue );
	}
}

static qboolean JKG_EntityHasGhoul2( const gentity_t *ent )
{
	if ( !ent || !ent->inuse )
	{
		return qfalse;
	}
	if ( !ent->ghoul2.IsValid() || ent->ghoul2.size() <= 0 )
	{
		return qfalse;
	}
	return qtrue;
}

static void JKG_PoseForGhoul2( const gentity_t *ent, vec3_t angles, vec3_t origin, vec3_t scale )
{
	VectorSet( angles, 0.0f, ent->currentAngles[YAW], 0.0f );
	VectorCopy( ent->currentOrigin, origin );
	if ( ent->client )
	{
		angles[YAW] = ent->client->renderInfo.legsYaw;
		VectorCopy( ent->client->ps.origin, origin );
	}

	VectorCopy( ent->s.modelScale, scale );
	if ( !scale[0] && !scale[1] && !scale[2] )
	{
		VectorSet( scale, 1.0f, 1.0f, 1.0f );
	}
}

static qboolean JKG_BoltWorldPos( gentity_t *ent, int boltIndex, vec3_t pos )
{
	mdxaBone_t boltMatrix;
	vec3_t angles;
	vec3_t origin;
	vec3_t scale;
	vec3_t result;
	int modelIndex;

	if ( boltIndex < 0 || !gi.G2API_GetBoltMatrix || !gi.G2API_GiveMeVectorFromMatrix )
	{
		return qfalse;
	}

	modelIndex = ent->playerModel;
	if ( modelIndex < 0 || modelIndex >= ent->ghoul2.size() )
	{
		modelIndex = 0;
	}

	JKG_PoseForGhoul2( ent, angles, origin, scale );
	if ( !gi.G2API_GetBoltMatrix( ent->ghoul2, modelIndex, boltIndex, &boltMatrix, angles, origin,
		cg.time ? cg.time : level.time, NULL, scale ) )
	{
		return qfalse;
	}

	gi.G2API_GiveMeVectorFromMatrix( boltMatrix, ORIGIN, result );
	if ( VectorLength( result ) < 1.0f )
	{
		return qfalse;
	}
	VectorCopy( result, pos );
	return qtrue;
}

static qboolean JKG_ConsiderAim( const vec3_t hullPos, const vec3_t candidate, const char *name, vec3_t bestPos, const char **bestName, float *bestDist )
{
	vec3_t delta;
	float dist;

	if ( !candidate || VectorLength( candidate ) < 1.0f )
	{
		return qfalse;
	}

	VectorSubtract( candidate, hullPos, delta );
	dist = VectorLength( delta );
	if ( dist < 1.0f || dist > 512.0f || dist >= *bestDist )
	{
		return qfalse;
	}

	*bestDist = dist;
	*bestName = name;
	VectorCopy( candidate, bestPos );
	return qtrue;
}

// The shot box can sit off the posed mesh (dead NPCs flatten maxs[2]). Aim at the nearest bone instead of the bolt velocity.
static qboolean JKG_ClosestBodyAim( gentity_t *ent, const vec3_t hullPos, vec3_t aim, const char **aimName )
{
	vec3_t boltPos;
	float bestDist;
	static const struct
	{
		const char *name;
		int gentity_t::*bolt;
	} bolts[] = {
		{ "head", &gentity_t::headBolt },
		{ "chest", &gentity_t::chestBolt },
		{ "gut", &gentity_t::gutBolt },
		{ "torso", &gentity_t::torsoBolt },
		{ "pelvis", &gentity_t::crotchBolt },
		{ "handL", &gentity_t::handLBolt },
		{ "handR", &gentity_t::handRBolt },
		{ "kneeL", &gentity_t::kneeLBolt },
		{ "kneeR", &gentity_t::kneeRBolt },
		{ "footL", &gentity_t::footLBolt },
		{ "footR", &gentity_t::footRBolt },
	};
	int i;

	bestDist = 512.0f;
	*aimName = "none";
	VectorClear( aim );

	if ( ent->client )
	{
		JKG_ConsiderAim( hullPos, ent->client->renderInfo.headPoint, "headPoint", aim, aimName, &bestDist );
		JKG_ConsiderAim( hullPos, ent->client->renderInfo.torsoPoint, "torsoPoint", aim, aimName, &bestDist );
		JKG_ConsiderAim( hullPos, ent->client->renderInfo.crotchPoint, "crotchPoint", aim, aimName, &bestDist );
		JKG_ConsiderAim( hullPos, ent->client->renderInfo.handLPoint, "handLPoint", aim, aimName, &bestDist );
		JKG_ConsiderAim( hullPos, ent->client->renderInfo.handRPoint, "handRPoint", aim, aimName, &bestDist );
		JKG_ConsiderAim( hullPos, ent->client->renderInfo.footLPoint, "footLPoint", aim, aimName, &bestDist );
		JKG_ConsiderAim( hullPos, ent->client->renderInfo.footRPoint, "footRPoint", aim, aimName, &bestDist );
	}

	for ( i = 0; i < (int)( sizeof( bolts ) / sizeof( bolts[0] ) ); i++ )
	{
		if ( JKG_BoltWorldPos( ent, ent->*( bolts[i].bolt ), boltPos ) )
		{
			JKG_ConsiderAim( hullPos, boltPos, bolts[i].name, aim, aimName, &bestDist );
		}
	}

	return ( *aimName && *aimName[0] && bestDist < 512.0f ) ? qtrue : qfalse;
}

static void JKG_ClearCollisionMap( CCollisionRecord *map )
{
	int i;

	memset( map, 0, sizeof( CCollisionRecord ) * MAX_G2_COLLISIONS );
	for ( i = 0; i < MAX_G2_COLLISIONS; i++ )
	{
		map[i].mEntityNum = -1;
		map[i].mDistance = 100000.0f;
	}
}

static int JKG_BestMeshRecord( const CCollisionRecord *map, int entNum )
{
	int i;
	int best;

	best = -1;
	for ( i = 0; i < MAX_G2_COLLISIONS; i++ )
	{
		if ( map[i].mEntityNum == -1 )
		{
			break;
		}
		if ( map[i].mEntityNum != entNum )
		{
			continue;
		}
		if ( map[i].mModelIndex == 0 && ( map[i].mFlags & G2_FRONTFACE ) )
		{
			return i;
		}
		if ( best < 0 )
		{
			best = i;
		}
	}
	return best;
}

static int JKG_CollideGhoul2Ray( gentity_t *ent, vec3_t start, vec3_t end, CCollisionRecord *map )
{
	vec3_t angles;
	vec3_t origin;
	vec3_t scale;

	JKG_ClearCollisionMap( map );
	if ( !gi.G2API_CollisionDetect )
	{
		return -1;
	}

	JKG_PoseForGhoul2( ent, angles, origin, scale );
	gi.G2API_CollisionDetect( map, ent->ghoul2, angles, origin, cg.time ? cg.time : level.time,
		ent->s.number, start, end, scale, NULL, G2_RETURNONHIT, 0, 0.0f );
	return JKG_BestMeshRecord( map, ent->s.number );
}

static void JKG_PrintMeshRay( const char *tag, gentity_t *ent, const char *how, const char *aimName, const vec3_t hullPos, const vec3_t shotDir, const vec3_t aim, const vec3_t rayEnd, const CCollisionRecord *map, int best )
{
	int i;
	int records;

	if ( !JKG_G2MarkDebugOn() )
	{
		return;
	}

	records = 0;
	for ( i = 0; i < MAX_G2_COLLISIONS; i++ )
	{
		if ( map[i].mEntityNum == -1 )
		{
			break;
		}
		records++;
	}

	gi.Printf( "JKG g2mark %s ent=%d npc=%s health=%d model=%s how=%s aim=%s\n",
		tag, ent->s.number, JKG_EntLabel( ent ), ent->health, JKG_ModelName( ent ), how, aimName ? aimName : "none" );
	gi.Printf( "  hull=%.1f %.1f %.1f shotDir=%.3f %.3f %.3f mins=%.1f %.1f %.1f maxs=%.1f %.1f %.1f\n",
		hullPos[0], hullPos[1], hullPos[2], shotDir[0], shotDir[1], shotDir[2],
		ent->mins[0], ent->mins[1], ent->mins[2], ent->maxs[0], ent->maxs[1], ent->maxs[2] );
	if ( aim )
	{
		gi.Printf( "  aimPos=%.1f %.1f %.1f rayEnd=%.1f %.1f %.1f\n",
			aim[0], aim[1], aim[2], rayEnd[0], rayEnd[1], rayEnd[2] );
	}
	gi.Printf( "  g2records=%d picked=%d\n", records, best );
	for ( i = 0; i < records; i++ )
	{
		gi.Printf( "  g2[%d] ent=%d model=%d surf=%d poly=%d flags=%d dist=%.1f pos=%.1f %.1f %.1f n=%.2f %.2f %.2f\n",
			i, map[i].mEntityNum, map[i].mModelIndex, map[i].mSurfaceIndex, map[i].mPolyIndex, map[i].mFlags, map[i].mDistance,
			map[i].mCollisionPosition[0], map[i].mCollisionPosition[1], map[i].mCollisionPosition[2],
			map[i].mCollisionNormal[0], map[i].mCollisionNormal[1], map[i].mCollisionNormal[2] );
	}
	if ( best < 0 )
	{
		gi.Printf( "  MISS no polygon on this model\n" );
	}
}

static qboolean JKG_AcceptMeshHit( gentity_t *ent, const vec3_t hullPos, const vec3_t shotDir, const CCollisionRecord *map, int best, vec3_t outPos, vec3_t outNormal )
{
	if ( best < 0 )
	{
		return qfalse;
	}

	VectorCopy( map[best].mCollisionPosition, outPos );
	VectorCopy( map[best].mCollisionNormal, outNormal );
	if ( VectorNormalize( outNormal ) < 0.1f )
	{
		VectorScale( shotDir, -1.0f, outNormal );
		VectorNormalize( outNormal );
	}
	if ( JKG_G2MarkDebugOn() )
	{
		gi.Printf( "  HIT mesh=%.1f %.1f %.1f n=%.2f %.2f %.2f\n",
			outPos[0], outPos[1], outPos[2], outNormal[0], outNormal[1], outNormal[2] );
		JKG_DrawShotDebug( hullPos, outPos, outPos, outNormal, qtrue );
	}
	return qtrue;
}

// Shot hulls sit outside the mesh. Cast at the nearest posed bone and use the triangle it meets.
static qboolean JKG_MeshPointAlongShot( gentity_t *ent, int skipNum, const vec3_t hullPos, const vec3_t shotDir, vec3_t outPos, vec3_t outNormal, const char *tag )
{
	CCollisionRecord map[MAX_G2_COLLISIONS];
	vec3_t aim;
	vec3_t rayDir;
	vec3_t start;
	vec3_t end;
	const char *aimName;
	int best;

	(void)skipNum;

	if ( !ent || !JKG_EntityHasGhoul2( ent ) )
	{
		return qfalse;
	}

	aimName = "none";
	if ( JKG_ClosestBodyAim( ent, hullPos, aim, &aimName ) )
	{
		VectorSubtract( aim, hullPos, rayDir );
		if ( VectorNormalize( rayDir ) >= 0.1f )
		{
			VectorMA( hullPos, -8.0f, rayDir, start );
			VectorMA( aim, 48.0f, rayDir, end );
			best = JKG_CollideGhoul2Ray( ent, start, end, map );
			JKG_PrintMeshRay( tag, ent, "bone", aimName, hullPos, shotDir, aim, end, map, best );
			if ( JKG_AcceptMeshHit( ent, hullPos, shotDir, map, best, outPos, outNormal ) )
			{
				return qtrue;
			}
			if ( JKG_G2MarkDebugOn() )
			{
				JKG_DrawShotDebug( hullPos, end, NULL, NULL, qfalse );
			}
		}
	}

	if ( VectorLength( shotDir ) < 0.1f )
	{
		return qfalse;
	}

	VectorMA( hullPos, -8.0f, shotDir, start );
	VectorMA( hullPos, 160.0f, shotDir, end );
	best = JKG_CollideGhoul2Ray( ent, start, end, map );
	JKG_PrintMeshRay( tag, ent, "shotdir", "velocity", hullPos, shotDir, NULL, end, map, best );
	if ( JKG_AcceptMeshHit( ent, hullPos, shotDir, map, best, outPos, outNormal ) )
	{
		return qtrue;
	}
	if ( JKG_G2MarkDebugOn() )
	{
		JKG_DrawShotDebug( hullPos, end, NULL, NULL, qfalse );
	}
	return qfalse;
}

static void JKG_ShotDirection( gentity_t *missile, gentity_t *ent, const vec3_t hullPos, const vec3_t hullNormal, vec3_t shotDir )
{
	if ( missile )
	{
		VectorCopy( missile->s.pos.trDelta, shotDir );
		if ( VectorNormalize( shotDir ) >= 0.1f )
		{
			return;
		}
	}

	VectorScale( hullNormal, -1.0f, shotDir );
	if ( VectorNormalize( shotDir ) >= 0.1f )
	{
		return;
	}

	VectorSubtract( ent->currentOrigin, hullPos, shotDir );
	VectorNormalize( shotDir );
}

static void JKG_AddGhoul2Mark( gentity_t *ent, const vec3_t hitPos, const vec3_t hitDir, float size, int cap, int life )
{
	SSkinGoreData gore;
	vec3_t dir;
	vec3_t scale;
	vec3_t position;
	float yaw;

	if ( size <= 0.0f || !JKG_EntityHasGhoul2( ent ) )
	{
		return;
	}
	if ( !gi.G2API_GetNumGoreMarks || !gi.G2API_AddSkinGore )
	{
		return;
	}

	VectorCopy( hitDir, dir );
	if ( VectorNormalize( dir ) < 0.1f )
	{
		return;
	}

	memset( &gore, 0, sizeof( gore ) );
	{
		static int nextShotId = 1;
		gore.myIndex = nextShotId;
		nextShotId++;
		if ( nextShotId <= 0 )
		{
			nextShotId = 1;
		}
	}
	gore.maxMarks = cap;
	gore.growDuration = -1;
	gore.goreScaleStartFraction = 1.0f;
	gore.frontFaces = true;
	gore.backFaces = false;
	gore.lifeTime = life;
	gore.firstModel = 0;
	gore.currentTime = cg.time;
	gore.entNum = ent->s.number;
	gore.SSize = size;
	gore.TSize = size;
	gore.shader = JKG_Ghoul2MarkShader();
	gore.theta = Q_flrand( 0.0f, 6.28f );
	gore.depthStart = -1000.0f;
	gore.depthEnd = 1000.0f;
	gore.useTheta = true;

	VectorCopy( ent->s.modelScale, scale );
	if ( !scale[0] && !scale[1] && !scale[2] )
	{
		VectorSet( scale, 1.0f, 1.0f, 1.0f );
	}
	VectorCopy( scale, gore.scale );
	VectorCopy( dir, gore.rayDirection );
	VectorCopy( hitPos, gore.hitLocation );

	VectorCopy( ent->currentOrigin, position );
	yaw = ent->currentAngles[YAW];
	if ( ent->client )
	{
		VectorCopy( ent->client->ps.origin, position );
		yaw = ent->client->renderInfo.legsYaw;
	}
	VectorCopy( position, gore.position );
	gore.angles[YAW] = yaw;

	gi.G2API_AddSkinGore( ent->ghoul2, &gore );
	if ( JKG_G2MarkDebugOn() )
	{
		gi.Printf( "  MARK added shader=%d size=%.1f life=%d shot=%d cap=%d surfaces=%d ent=%d model=%s\n",
			gore.shader, size, life, gore.myIndex, cap, gi.G2API_GetNumGoreMarks( ent->ghoul2, 0 ), ent->s.number, JKG_ModelName( ent ) );
	}
}

static void JKG_MarkSplashGhoul2( gentity_t *missile, const vec3_t origin, gentity_t *ignore, float size, int cap, int life )
{
	int i;
	int radius;

	if ( !missile || size <= 0.0f || missile->splashDamage <= 0 || missile->splashRadius <= 0 )
	{
		return;
	}

	radius = missile->splashRadius;
	for ( i = 0; i < MAX_GENTITIES; i++ )
	{
		gentity_t *ent = &g_entities[i];
		vec3_t delta;
		vec3_t dir;
		float dist;

		if ( ent == ignore || ent == missile || !JKG_EntityHasGhoul2( ent ) )
		{
			continue;
		}

		VectorSubtract( ent->currentOrigin, origin, delta );
		dist = VectorLength( delta );
		if ( dist > (float)radius )
		{
			continue;
		}

		VectorSubtract( ent->currentOrigin, origin, dir );
		if ( VectorNormalize( dir ) < 0.1f )
		{
			continue;
		}
		if ( !JKG_MeshPointAlongShot( ent, missile->s.number, origin, dir, delta, dir, "splash" ) )
		{
			continue;
		}
		JKG_AddGhoul2Mark( ent, delta, dir, size, cap, life );
	}
}

void JKG_Ghoul2MarkMissile( centity_t *cent, gentity_t *directHit, vec3_t origin, vec3_t dir )
{
	int cap;
	int life;
	float boltSize;
	float blastSize;
	qboolean splash;

	cap = JKG_CvarIntegerNonNegative( g_jkgGhoul2Marks );
	if ( cap <= 0 || !cent || !cent->gent || !origin || !dir )
	{
		return;
	}

	life = JKG_CvarIntegerNonNegative( g_jkgGhoul2MarkLife );
	boltSize = JKG_CvarFloatPositive( g_jkgGhoul2MarkSize );
	blastSize = JKG_CvarFloatPositive( g_jkgGhoul2BlastSize );
	splash = ( cent->gent->splashDamage > 0 && cent->gent->splashRadius > 0 ) ? qtrue : qfalse;

	// A hit on a Ghoul2 prop with no client is sent as a wall miss. The victim is still otherEntityNum.
	if ( !directHit && cent->gent->s.otherEntityNum < ENTITYNUM_WORLD )
	{
		gentity_t *other = &g_entities[cent->gent->s.otherEntityNum];
		if ( JKG_EntityHasGhoul2( other ) )
		{
			directHit = other;
		}
	}
	if ( directHit && ( directHit->contents & CONTENTS_LIGHTSABER ) )
	{
		directHit = NULL;
	}

	if ( JKG_G2MarkDebugOn() && ( !directHit || !JKG_EntityHasGhoul2( directHit ) ) )
	{
		gi.Printf( "JKG g2mark direct skipped victim=%d other=%d hasG2=%d saber=%d weapon=%d origin=%.1f %.1f %.1f\n",
			directHit ? directHit->s.number : -1,
			cent->gent->s.otherEntityNum,
			( directHit && JKG_EntityHasGhoul2( directHit ) ) ? 1 : 0,
			( directHit && ( directHit->contents & CONTENTS_LIGHTSABER ) ) ? 1 : 0,
			cent->gent->s.weapon,
			origin[0], origin[1], origin[2] );
	}

	if ( directHit && JKG_EntityHasGhoul2( directHit ) )
	{
		vec3_t shotDir;
		vec3_t meshPos;
		vec3_t meshNormal;

		JKG_ShotDirection( cent->gent, directHit, origin, dir, shotDir );
		if ( JKG_MeshPointAlongShot( directHit, cent->gent->s.number, origin, shotDir, meshPos, meshNormal, "direct" ) )
		{
			JKG_AddGhoul2Mark( directHit, meshPos, meshNormal, splash ? blastSize : boltSize, cap, life );
		}
	}

	if ( splash )
	{
		JKG_MarkSplashGhoul2( cent->gent, origin, directHit, blastSize, cap, life );
	}
}
