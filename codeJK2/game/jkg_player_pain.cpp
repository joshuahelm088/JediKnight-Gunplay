/*
===========================================================================
JKGunplay - 3rd-person player hit reactions (DF2-style)
===========================================================================
*/

#include "g_headers.h"

#include "b_local.h"
#include "g_local.h"
#include "jkg_local.h"
#include "anims.h"
#include "wp_saber.h"
#include "../cgame/cg_local.h"

extern vmCvar_t cg_thirdPerson;

extern qboolean G_CheckForStrongAttackMomentum( gentity_t *self );
extern int G_PickPainAnim( gentity_t *self, vec3_t point, int damage, int hitLoc );
extern int PM_AnimLength( int index, animNumber_t anim );
extern int PM_PickAnim( gentity_t *self, int minAnim, int maxAnim );
extern qboolean PM_SaberInSpecialAttack( int anim );
extern qboolean PM_SpinningSaberAnim( int anim );
extern qboolean PM_SpinningAnim( int anim );
extern qboolean PM_InKnockDown( playerState_t *ps );
extern qboolean PM_CrouchAnim( int anim );
extern qboolean PM_FlippingAnim( int anim );
extern qboolean PM_RollingAnim( int anim );
extern qboolean PM_InCartwheel( int anim );
extern qboolean PM_InSpecialJump( int anim );
extern qboolean PM_RunningAnim( int anim );

static int s_playerPainChanceDamage;

void JKG_SetPlayerPainChanceDamage( int damage )
{
	s_playerPainChanceDamage = damage;
}

static qboolean JKG_PlayerThirdPersonPainView( void )
{
	if ( cg.zoomMode )
	{
		return qfalse;
	}

	if ( cg_thirdPerson.integer )
	{
		return qtrue;
	}

	return cg.renderingThirdPerson;
}

static float JKG_PlayerPainChanceScale( void )
{
	float value;

	if ( !g_jkgPlayerPainChance )
	{
		return 0.0f;
	}

	value = g_jkgPlayerPainChance->value;
	if ( value < 0.0f )
	{
		value = 0.0f;
	}

	return value;
}

void JKG_PlayerTryPainAnim( gentity_t *self, gentity_t *other, vec3_t point, int damage, int mod, int hitLoc )
{
	int			painAnim;
	int			parts;
	float		painChance;
	int			chanceDamage;

	if ( !self || !self->client )
	{
		return;
	}

	chanceDamage = damage;
	if ( s_playerPainChanceDamage > 0 )
	{
		chanceDamage = s_playerPainChanceDamage;
	}
	s_playerPainChanceDamage = 0;

	if ( damage <= 0 )
	{
		return;
	}

	if ( !JKG_PlayerThirdPersonPainView() )
	{
		return;
	}

	if ( other && other == self )
	{
		return;
	}

	if ( ( ( ( mod == MOD_SABER || mod == MOD_MELEE ) && self->client->damage_blood ) || mod == MOD_CRUSH )
		&& ( self->s.weapon == WP_SABER || self->s.weapon == WP_MELEE ) )
	{
		return;
	}

	painChance = JKG_PlayerPainChanceScale();
	if ( painChance <= 0.0f )
	{
		return;
	}

	if ( (float)chanceDamage * painChance <= Q_flrand( 0.0f, 1.0f ) )
	{
		return;
	}

	if ( self->client->ps.eFlags & EF_FORCE_GRIPPED )
	{
		return;
	}

	if ( G_CheckForStrongAttackMomentum( self )
		|| PM_SpinningAnim( self->client->ps.legsAnim )
		|| PM_SaberInSpecialAttack( self->client->ps.torsoAnim )
		|| PM_InKnockDown( &self->client->ps )
		|| PM_RollingAnim( self->client->ps.legsAnim )
		|| ( PM_FlippingAnim( self->client->ps.legsAnim ) && !PM_InCartwheel( self->client->ps.legsAnim ) ) )
	{
		return;
	}

	if ( self->s.weapon == WP_SABER )
	{
		painAnim = PM_PickAnim( self, BOTH_PAIN2, BOTH_PAIN3 );
	}
	else if ( mod != MOD_ELECTROCUTE )
	{
		painAnim = G_PickPainAnim( self, point, damage, hitLoc );
		if ( painAnim == -1 )
		{
			painAnim = PM_PickAnim( self, BOTH_PAIN1, BOTH_PAIN19 );
		}
	}
	else
	{
		painAnim = PM_PickAnim( self, BOTH_PAIN1, BOTH_PAIN19 );
	}

	parts = SETANIM_BOTH;
	if ( self->client->ps.groundEntityNum == ENTITYNUM_NONE
		|| PM_SpinningSaberAnim( self->client->ps.legsAnim )
		|| PM_FlippingAnim( self->client->ps.legsAnim )
		|| PM_InSpecialJump( self->client->ps.legsAnim )
		|| PM_RollingAnim( self->client->ps.legsAnim )
		|| PM_CrouchAnim( self->client->ps.legsAnim )
		|| PM_RunningAnim( self->client->ps.legsAnim ) )
	{
		parts = SETANIM_TORSO;
	}

	NPC_SetAnim( self, parts, painAnim, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD | SETANIM_FLAG_RESTART );
}
