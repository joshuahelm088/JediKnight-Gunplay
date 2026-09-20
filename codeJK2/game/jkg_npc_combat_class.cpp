/*
===========================================================================
JKGunplay - combat class table (ext_data/jkg_combat_classes.cfg)

NPCs.cfg sets combatClass <name>. Missing/unknown names use "default".
Any omitted key in a class falls back to the matching g_jkgCombat* cvar.
===========================================================================
*/

#include "g_headers.h"

#include "b_local.h"
#include "jkg_local.h"

#define JKG_MAX_COMBAT_CLASSES		24
#define JKG_COMBAT_CLASS_UNSET		-1

typedef struct jkgCombatClass_s {
	char	name[JKG_COMBAT_CLASS_NAME_LEN];
	int		rangeMin;
	int		rangeMax;
	int		rangeBand;
	int		stepDist;
	int		strafeDist;
	int		strafeTime;
	int		strafePause;
	int		huntCheatMs;
	int		moveDelay;
} jkgCombatClass_t;

static jkgCombatClass_t	s_classes[JKG_MAX_COMBAT_CLASSES];
static int				s_numClasses;

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

static int JKG_ResolveClassInt( int stored, cvar_t *cv )
{
	if ( stored >= 0 )
	{
		return stored;
	}
	return JKG_CvarIntegerNonNegative( cv );
}

static void JKG_ClearCombatClass( jkgCombatClass_t *cls )
{
	if ( !cls )
	{
		return;
	}

	cls->name[0] = 0;
	cls->rangeMin = JKG_COMBAT_CLASS_UNSET;
	cls->rangeMax = JKG_COMBAT_CLASS_UNSET;
	cls->rangeBand = JKG_COMBAT_CLASS_UNSET;
	cls->stepDist = JKG_COMBAT_CLASS_UNSET;
	cls->strafeDist = JKG_COMBAT_CLASS_UNSET;
	cls->strafeTime = JKG_COMBAT_CLASS_UNSET;
	cls->strafePause = JKG_COMBAT_CLASS_UNSET;
	cls->huntCheatMs = JKG_COMBAT_CLASS_UNSET;
	cls->moveDelay = JKG_COMBAT_CLASS_UNSET;
}

static jkgCombatClass_t *JKG_FindCombatClass( const char *name )
{
	int i;

	if ( !name || !name[0] )
	{
		return &s_classes[0];
	}

	for ( i = 0; i < s_numClasses; i++ )
	{
		if ( !Q_stricmp( s_classes[i].name, name ) )
		{
			return &s_classes[i];
		}
	}

	return NULL;
}

static jkgCombatClass_t *JKG_EnsureCombatClass( const char *name )
{
	jkgCombatClass_t *cls;

	cls = JKG_FindCombatClass( name );
	if ( cls )
	{
		return cls;
	}

	if ( s_numClasses >= JKG_MAX_COMBAT_CLASSES )
	{
		gi.Printf( S_COLOR_YELLOW"WARNING: too many combat classes, ignoring '%s'\n", name );
		return NULL;
	}

	cls = &s_classes[s_numClasses];
	JKG_ClearCombatClass( cls );
	Q_strncpyz( cls->name, name, sizeof( cls->name ) );
	s_numClasses++;
	return cls;
}

static void JKG_ParseCombatClassBlock( const char **p, jkgCombatClass_t *cls )
{
	const char	*token;
	int			n;

	while ( 1 )
	{
		token = COM_ParseExt( p, qtrue );
		if ( !token[0] )
		{
			gi.Printf( S_COLOR_RED"ERROR: unexpected EOF in combat class '%s'\n", cls->name );
			return;
		}

		if ( !Q_stricmp( token, "}" ) )
		{
			return;
		}

		if ( !Q_stricmp( token, "idealRangeMin" ) || !Q_stricmp( token, "rangeMin" ) )
		{
			if ( COM_ParseInt( p, &n ) )
			{
				SkipRestOfLine( p );
				continue;
			}
			cls->rangeMin = n;
			continue;
		}

		if ( !Q_stricmp( token, "idealRangeMax" ) || !Q_stricmp( token, "rangeMax" ) )
		{
			if ( COM_ParseInt( p, &n ) )
			{
				SkipRestOfLine( p );
				continue;
			}
			cls->rangeMax = n;
			continue;
		}

		if ( !Q_stricmp( token, "rangeBand" ) )
		{
			if ( COM_ParseInt( p, &n ) )
			{
				SkipRestOfLine( p );
				continue;
			}
			cls->rangeBand = n;
			continue;
		}

		if ( !Q_stricmp( token, "stepDist" ) )
		{
			if ( COM_ParseInt( p, &n ) )
			{
				SkipRestOfLine( p );
				continue;
			}
			cls->stepDist = n;
			continue;
		}

		if ( !Q_stricmp( token, "strafeDist" ) )
		{
			if ( COM_ParseInt( p, &n ) )
			{
				SkipRestOfLine( p );
				continue;
			}
			cls->strafeDist = n;
			continue;
		}

		if ( !Q_stricmp( token, "strafeTime" ) )
		{
			if ( COM_ParseInt( p, &n ) )
			{
				SkipRestOfLine( p );
				continue;
			}
			cls->strafeTime = n;
			continue;
		}

		if ( !Q_stricmp( token, "strafePause" ) )
		{
			if ( COM_ParseInt( p, &n ) )
			{
				SkipRestOfLine( p );
				continue;
			}
			cls->strafePause = n;
			continue;
		}

		if ( !Q_stricmp( token, "huntCheatMs" ) )
		{
			if ( COM_ParseInt( p, &n ) )
			{
				SkipRestOfLine( p );
				continue;
			}
			cls->huntCheatMs = n;
			continue;
		}

		if ( !Q_stricmp( token, "moveDelay" ) )
		{
			if ( COM_ParseInt( p, &n ) )
			{
				SkipRestOfLine( p );
				continue;
			}
			cls->moveDelay = n;
			continue;
		}

		gi.Printf( S_COLOR_YELLOW"WARNING: unknown combat class key '%s' in '%s'\n", token, cls->name );
		SkipRestOfLine( p );
	}
}

void JKG_LoadCombatClasses( void )
{
	char		*buffer;
	const char	*p;
	const char	*token;
	int			len;
	jkgCombatClass_t *cls;

	memset( s_classes, 0, sizeof( s_classes ) );
	s_numClasses = 0;

	cls = JKG_EnsureCombatClass( "default" );
	if ( !cls )
	{
		return;
	}

	len = gi.FS_ReadFile( "ext_data/jkg_combat_classes.cfg", (void **)&buffer );
	if ( len <= 0 )
	{
		gi.Printf( S_COLOR_YELLOW"WARNING: ext_data/jkg_combat_classes.cfg not found; using cvar combat defaults\n" );
		return;
	}

	p = buffer;
	COM_BeginParseSession();

	while ( 1 )
	{
		token = COM_ParseExt( &p, qtrue );
		if ( !token[0] )
		{
			break;
		}

		cls = JKG_EnsureCombatClass( token );
		if ( !cls )
		{
			SkipBracedSection( &p );
			continue;
		}

		token = COM_ParseExt( &p, qtrue );
		if ( Q_stricmp( token, "{" ) )
		{
			gi.Printf( S_COLOR_YELLOW"WARNING: combat class '%s' missing '{'\n", cls->name );
			if ( token[0] )
			{
				JKG_ParseCombatClassBlock( &p, cls );
			}
			continue;
		}

		JKG_ParseCombatClassBlock( &p, cls );
	}

	COM_EndParseSession();
	gi.FS_FreeFile( buffer );
}

void JKG_GetCombatMoveParms( const gentity_t *ent, jkgCombatMoveParms_t *out )
{
	const jkgCombatClass_t *cls;
	int tmp;

	if ( !out )
	{
		return;
	}

	memset( out, 0, sizeof( *out ) );

	cls = &s_classes[0];
	if ( ent && ent->NPC && ent->NPC->jkgCombatClass[0] )
	{
		jkgCombatClass_t *found = JKG_FindCombatClass( ent->NPC->jkgCombatClass );
		if ( found )
		{
			cls = found;
		}
	}

	out->rangeMin = JKG_ResolveClassInt( cls->rangeMin, g_jkgCombatIdealRangeMin );
	out->rangeMax = JKG_ResolveClassInt( cls->rangeMax, g_jkgCombatIdealRangeMax );
	out->rangeBand = JKG_ResolveClassInt( cls->rangeBand, g_jkgCombatRangeBand );
	out->stepDist = JKG_ResolveClassInt( cls->stepDist, g_jkgCombatStepDist );
	out->strafeDist = JKG_ResolveClassInt( cls->strafeDist, g_jkgCombatStrafeDist );
	out->strafeTime = JKG_ResolveClassInt( cls->strafeTime, g_jkgCombatStrafeTime );
	out->strafePause = JKG_ResolveClassInt( cls->strafePause, g_jkgCombatStrafePause );
	out->huntCheatMs = JKG_ResolveClassInt( cls->huntCheatMs, g_jkgCombatHuntCheatMs );
	out->moveDelay = JKG_ResolveClassInt( cls->moveDelay, g_jkgCombatMoveDelay );

	if ( out->rangeMax < out->rangeMin )
	{
		tmp = out->rangeMax;
		out->rangeMax = out->rangeMin;
		out->rangeMin = tmp;
	}
	if ( out->rangeMin < 48 )
	{
		out->rangeMin = 48;
	}
	if ( out->rangeMax < out->rangeMin )
	{
		out->rangeMax = out->rangeMin;
	}
	if ( out->rangeMax == out->rangeMin && out->rangeBand > 0 )
	{
		int half = out->rangeBand;
		if ( out->rangeMin > half + 48 )
		{
			out->rangeMin -= half;
		}
		out->rangeMax += half;
	}
	if ( out->stepDist < 32 )
	{
		out->stepDist = 32;
	}
	if ( out->strafeDist < 24 )
	{
		out->strafeDist = 24;
	}
	if ( out->strafeTime < 200 )
	{
		out->strafeTime = 200;
	}
	if ( out->strafePause < 100 )
	{
		out->strafePause = 100;
	}
}
