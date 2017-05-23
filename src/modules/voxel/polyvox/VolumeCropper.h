#pragma once

#include "RawVolume.h"
#include "VolumeMerger.h"

namespace voxel {

/**
 * @brief Will skip air voxels on volume cropping
 */
struct CropSkipEmpty {
	inline bool operator() (const voxel::Voxel& voxel) const {
		return isAir(voxel.getMaterial());
	}
};

template<class CropSkipCondition = CropSkipEmpty>
RawVolume* cropVolume(const RawVolume* volume, const glm::ivec3& mins, const glm::ivec3& maxs, CropSkipCondition condition = CropSkipCondition()) {
	core_trace_scoped(CropRawVolume);
	const voxel::Region newRegion(glm::ivec3(), maxs - mins);
	if (!newRegion.isValid()) {
		return nullptr;
	}
	voxel::RawVolume* newVolume = new voxel::RawVolume(newRegion);
	voxel::mergeVolumes(newVolume, volume, newRegion, voxel::Region(mins, maxs));
	return newVolume;
}

template<class CropSkipCondition = CropSkipEmpty>
RawVolume* cropVolume(const RawVolume* volume, CropSkipCondition condition = CropSkipCondition()) {
	core_trace_scoped(CropRawVolume);
	const glm::ivec3& mins = volume->mins();
	const glm::ivec3& maxs = volume->maxs();
	glm::ivec3 newMins(std::numeric_limits<int>::max());
	glm::ivec3 newMaxs(std::numeric_limits<int>::min());
	voxel::RawVolume::Sampler volumeSampler(volume);
	for (int32_t z = mins.z; z <= maxs.z; ++z) {
		for (int32_t y = mins.y; y <= maxs.y; ++y) {
			for (int32_t x = mins.x; x <= maxs.x; ++x) {
				volumeSampler.setPosition(x, y, z);
				const voxel::Voxel& voxel = volumeSampler.voxel();
				if (condition(voxel)) {
					continue;
				}
				newMins.x = glm::min(newMins.x, x);
				newMins.y = glm::min(newMins.y, y);
				newMins.z = glm::min(newMins.z, z);

				newMaxs.x = glm::max(newMaxs.x, x);
				newMaxs.y = glm::max(newMaxs.y, y);
				newMaxs.z = glm::max(newMaxs.z, z);
			}
		}
	}
	if (newMaxs.z == std::numeric_limits<int>::min()) {
		return nullptr;
	}
	return cropVolume(volume, newMins, newMaxs, condition);
}

}
