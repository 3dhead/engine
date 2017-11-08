/**
 * @file
 */

#pragma once

#include "Task.h"
#include "core/Common.h"
#include "backend/spawn/SpawnMgr.h"
#include "backend/entity/Npc.h"
#include "backend/world/Map.h"

using namespace ai;

namespace backend {

/**
 * @ingroup AI
 */
AI_TASK(Spawn) {
	backend::Npc& npc = chr.getNpc();
	const glm::ivec3 pos = glm::ivec3(npc.pos());
	const SpawnMgrPtr& spawnMgr = npc.map()->spawnMgr();
	if (spawnMgr->spawn(npc.entityType(), 1, &pos) == 1) {
		return FINISHED;
	}
	return FAILED;
}

}

