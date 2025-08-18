#include "g_headers.h"

#include "b_local.h"
#include "g_nav.h"
#include "anims.h"
#include "g_navigator.h"

extern void NPC_CheckGetNewWeapon(void);

void NPC_BSST_Patrol_New(void);
void NPC_BSST_Attack_New(void);

void NPC_BSST_Default_New(void) {
	if (NPCInfo->scriptFlags & SCF_FIRE_WEAPON) {
		WeaponThink(qtrue);
	}

	if (!NPC->enemy)
	{//don't have an enemy, look for one
		NPC_BSST_Patrol_New();
	}
	else //if ( NPC->enemy )
	{//have an enemy
		NPC_CheckGetNewWeapon();
		NPC_BSST_Attack_New();
	}
}

void NPC_BSST_Patrol_New(void) {

}

void NPC_BSST_Attack_New(void) {

}
