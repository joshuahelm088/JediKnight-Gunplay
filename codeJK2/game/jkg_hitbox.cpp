/*
===========================================================================
JKGunplay - NPC hitbox scaling (horizontal entity AABB)
===========================================================================
*/

#include "g_headers.h"

#include "g_local.h"
#include "jkg_local.h"

#define JKG_HITBOX_ENTITY_SLOTS	(ENTITYNUM_WORLD)

static float		jkg_hitboxAppliedScale[JKG_HITBOX_ENTITY_SLOTS];

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

void JKG_UpdateNpcHitboxes( void )
{
	int			i;
	float		scale;
	float		prevScale;
	vec3_t		wantMins;
	vec3_t		wantMaxs;
	gentity_t	*ent;

	if ( !JKG_WEAPONS || !g_jkgNpcHitboxScale )
	{
		return;
	}

	for ( i = 1; i < JKG_HITBOX_ENTITY_SLOTS; i++ )
	{
		ent = &g_entities[i];

		if ( !ent->inuse || !ent->client )
		{
			jkg_hitboxAppliedScale[i] = 0.0f;
			continue;
		}

		scale = JKG_GetNpcHitboxScale( ent );
		prevScale = jkg_hitboxAppliedScale[i];

		if ( scale <= 1.0f && prevScale <= 1.0f )
		{
			continue;
		}

		VectorCopy( ent->mins, wantMins );
		VectorCopy( ent->maxs, wantMaxs );

		// Undo last frame's XY scale so pmove's current height (Z) is preserved.
		if ( prevScale > 1.0f )
		{
			wantMins[0] /= prevScale;
			wantMins[1] /= prevScale;
			wantMaxs[0] /= prevScale;
			wantMaxs[1] /= prevScale;
		}

		if ( scale > 1.0f )
		{
			wantMins[0] *= scale;
			wantMins[1] *= scale;
			wantMaxs[0] *= scale;
			wantMaxs[1] *= scale;
		}

		jkg_hitboxAppliedScale[i] = ( scale > 1.0f ) ? scale : 0.0f;

		if ( VectorCompare( ent->mins, wantMins ) && VectorCompare( ent->maxs, wantMaxs ) )
		{
			continue;
		}

		VectorCopy( wantMins, ent->mins );
		VectorCopy( wantMaxs, ent->maxs );
		gi.linkentity( ent );
	}
}
