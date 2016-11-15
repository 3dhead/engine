/**
 * @file
 */

#include "VolumeRotator.h"
#include "RawVolume.h"
#include "core/AABB.h"

namespace voxel {

/**
 * @param[in] source The RawVolume to rotate
 * @param[in] angles The angles for the x, y and z axis given in degrees
 * @param[in] increaseSize If you rotate e.g. by 45 degree, the rotated volume
 * would have a bigger size as the source volume. You can define that you would
 * like to cut it to the source volume size with this flag.
 * @return A new RawVolume. It's the caller's responsibility to free this
 * memory.
 */
RawVolume* rotateVolume(const RawVolume* source, const glm::vec3& angles, const Voxel& empty, bool increaseSize) {
	const glm::quat& quat = glm::normalize(
			  glm::angleAxis(glm::radians(angles.x), glm::right)
			* glm::angleAxis(glm::radians(angles.y), glm::up)
			* glm::angleAxis(glm::radians(angles.z), glm::backward));
	const glm::mat4& rot = glm::mat4_cast(quat);

	const voxel::Region& srcRegion = source->getRegion();
	const glm::ivec3& srcCenter = srcRegion.getCentre();
	voxel::Region destRegion;

	if (increaseSize) {
		const glm::vec4 mins(glm::vec3(srcRegion.getLowerCorner()) + 0.5f, 1.0f);
		const glm::vec4 maxs(glm::vec3(srcRegion.getUpperCorner()) + 0.5f, 1.0f);
		const glm::vec4& newMins = rot * mins;
		const glm::vec4& newMaxs = rot * maxs;
		const glm::ivec3 vertices[] = { newMins.xyz(), newMaxs.xyz() };
		core::AABB<int> aabb = core::AABB<int>::construct(vertices, SDL_arraysize(vertices));
		aabb.shift(-aabb.getLowerCorner());
		destRegion = voxel::Region(aabb.getLowerCorner(), aabb.getUpperCorner());
	} else {
		destRegion = srcRegion;
	}
	voxel::RawVolume* destination = new RawVolume(destRegion);
	voxel::RawVolume::Sampler destSampler(destination);
	voxel::RawVolume::Sampler srcSampler(source);

	const int32_t depth = srcRegion.getDepthInVoxels();
	const int32_t height = srcRegion.getHeightInVoxels();
	const int32_t width = srcRegion.getWidthInVoxels();
	for (int32_t z = 0; z < depth; z++) {
		for (int32_t y = 0; y < height; y++) {
			for (int32_t x = 0; x < width; x++) {
				srcSampler.setPosition(x, y, z);
				const Voxel& v = srcSampler.getVoxel();
				if (v == empty) {
					continue;
				}
				const glm::vec4 pos(x - srcCenter.x + 0.5f, y - srcCenter.y + 0.5f, z - srcCenter.z + 0.5f, 1.0f);
				const glm::vec4 newPos = rot * pos;
				const glm::ivec3 volumePos(glm::ivec3(newPos) + srcCenter);
				if (!destRegion.containsPoint(volumePos)) {
					continue;
				}

				destSampler.setPosition(volumePos);
				if (destSampler.getVoxel() == empty) {
					destSampler.setVoxel(v);
				}
			}
		}
	}
	return destination;
}

}
