/**
 * @file
 */

#pragma once

#include "voxel/polyvox/RawVolume.h"

namespace voxel {

/**
 * @brief A wrapper for a RawVolume that performs a sanity check for the setVoxel call.
 */
class RawVolumeWrapper {
private:
	RawVolume* _volume;
	const Region& _region;

public:
	class Sampler : public RawVolume::Sampler {
	private:
		using Super = RawVolume::Sampler;
	public:
		Sampler(const RawVolumeWrapper* volume) : Super(volume->volume()) {}

		Sampler(const RawVolumeWrapper& volume) : Super(volume.volume()) {};
	};

	RawVolumeWrapper(voxel::RawVolume* volume) :
			_volume(volume), _region(volume->region()) {
	}

	inline operator RawVolume& () const {
		return *_volume;
	}

	inline operator const RawVolume& () const {
		return *_volume;
	}

	inline operator RawVolume* () const {
		return _volume;
	}

	inline operator const RawVolume* () const {
		return _volume;
	}

	inline RawVolume* volume() const {
		return _volume;
	}

	inline const Region& region() const {
		return _region;
	}

	inline const Voxel& voxel(const glm::ivec3& pos) const {
		return _volume->voxel(pos.x, pos.y, pos.z);
	}

	inline const Voxel& voxel(int x, int y, int z) const {
		return _volume->voxel(x, y, z);
	}

	inline bool setVoxel(const glm::ivec3& pos, const Voxel& voxel) {
		return setVoxel(pos.x, pos.y, pos.z, voxel);
	}

	/**
	 * @return @c false if the voxel was not placed because the given position is outside of the valid region, @c
	 * true if the voxel was placed in the region.
	 * @note The return values have a different meaning as for the wrapped RawVolume.
	 */
	inline bool setVoxel(int x, int y, int z, const Voxel& voxel) {
		if (!_region.containsPoint(x, y, z)) {
			return false;
		}
		_volume->setVoxel(x, y, z, voxel);
		return true;
	}

	inline bool setVoxels(int x, int z, const Voxel* voxels, int amount) {
		for (int y = 0; y < amount; ++y) {
			setVoxel(x, y, z, voxels[y]);
		}
		return true;
	}

	inline bool setVoxels(int x, int y, int z, int nx, int nz, const Voxel* voxels, int amount) {
		for (int j = 0; j < nx; ++j) {
			for (int k = 0; k < nz; ++k) {
				for (int ny = y; ny < amount; ++ny) {
					setVoxel(x + j, ny, z + k, voxels[ny]);
				}
			}
		}
		return true;
	}
};

}
