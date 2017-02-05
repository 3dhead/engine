#pragma once

#include "core/Common.h"
#include "core/Trace.h"
#include "Voxel.h"
#include "Picking.h"
#include "Raycast.h"

namespace voxel {

/**
 * @brief A structure containing the information about a picking operation
 */
struct PickResult {
	PickResult() {
	}

	/** Did the picking operation hit anything */
	bool didHit = false;

	bool validPreviousPosition = false;

	/** The location of the solid voxel it hit */
	glm::ivec3 hitVoxel;

	/** The location of the step before we end the trace - see @a validPreviousLocation */
	glm::ivec3 previousPosition;
};

namespace {

/**
 * @brief This is just an implementation class for the pickVoxel function
 *
 * It makes note of the sort of empty voxel you're looking for in the constructor.
 *
 * Each time the operator() is called:
 *  * if it's hit a voxel it sets up the result and returns false
 *  * otherwise it preps the result for the next iteration and returns true
 */
template<typename VolumeType>
class RaycastPickingFunctor {
public:
	RaycastPickingFunctor(const Voxel& emptyVoxelExample) :
			_emptyVoxelExample(emptyVoxelExample), _result() {
	}

	bool operator()(typename VolumeType::Sampler& sampler) {
		if (sampler.getVoxel() != _emptyVoxelExample) {
			// If we've hit something
			_result.didHit = true;
			_result.hitVoxel = sampler.getPosition();
			return false;
		}

		if (sampler.isCurrentPositionValid()) {
			_result.validPreviousPosition = true;
			_result.previousPosition = sampler.getPosition();
		}
		return true;
	}
	const Voxel& _emptyVoxelExample;
	PickResult _result;
};

}

/** Pick the first solid voxel along a vector */
template<typename VolumeType>
PickResult pickVoxel(const VolumeType* volData, const glm::vec3& v3dStart, const glm::vec3& v3dDirectionAndLength, const Voxel& emptyVoxelExample) {
	core_trace_scoped(pickVoxel);
	RaycastPickingFunctor<VolumeType> functor(emptyVoxelExample);
	raycastWithDirection(volData, v3dStart, v3dDirectionAndLength, functor);
	return functor._result;
}

}
