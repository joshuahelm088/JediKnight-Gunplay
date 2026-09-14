#ifndef __AI_NEW__
#define __AI_NEW__

#include "g_local.h"   // for gentity_t, gNPC_t, vec3_t, etc.

// Updates group and NPC memory when enemy is visible
void UpdateEnemyLastSeen(gentity_t* NPC, gNPC_t* NPCInfo);

// Handles behavior for an unarmed NPC
void HandleUnarmed(gNPC_t* NPCInfo, qboolean& enemyCS);

// Checks if enemy is too close for explosive weapons
qboolean TooCloseForExplosives(gentity_t* NPC, float enemyDist);

// Evaluates whether the NPC has a clear shot, sets flags accordingly
void EvaluateShot(gentity_t* NPC, gNPC_t* NPCInfo,
    qboolean& enemyCS, qboolean& hitAlly,
    vec3_t impactPos);

// Main high-level function: handles LOS, memory, shooting decisions
void HandleEnemyVisibility(gentity_t* NPC, gNPC_t* NPCInfo,
    float enemyDist, qboolean enemyInFOV,
    qboolean& enemyLOS, qboolean& enemyCS,
    qboolean& hitAlly, vec3_t impactPos,
    qboolean& faceEnemy);

#endif
