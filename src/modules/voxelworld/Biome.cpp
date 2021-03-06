/**
 * @file
 */
#include "Biome.h"
#include "voxel/MaterialColor.h"
#include "voxel/Constants.h"

namespace voxelworld {

Biome::Biome() :
		Biome(voxel::VoxelType::Grass, 0, voxel::MAX_MOUNTAIN_HEIGHT, 0.5f, 0.5f, false, 90) {
}

Biome::Biome(voxel::VoxelType _type, int16_t _yMin, int16_t _yMax, float _humidity, float _temperature, bool _underground, int _treeDistance) :
		indices(getMaterialIndices(_type)), yMin(_yMin), yMax(_yMax), humidity(_humidity), temperature(_temperature),
		underground(_underground), type(_type), treeDistance(_treeDistance),
		cloudDistribution(calcCloudDistribution()), plantDistribution(calcPlantDistribution()) {
	core_assert(!indices.empty());
}

Biome::~Biome() {
	for (const char *t : _treeTypes) {
		SDL_free(const_cast<char*>(t));
	}
	_treeTypes.clear();
}

void Biome::addTreeType(const char *treeType) {
	_treeTypes.push_back(SDL_strdup(treeType));
}

// TODO: move into lua script
int Biome::calcCloudDistribution() const {
	int distribution = 150;
	if (temperature > 0.7f || humidity < 0.2f) {
		distribution = 200;
	} else if (temperature > 0.9f || humidity < 0.1f) {
		distribution = 250;
	}
	return distribution;
}

// TODO: move into lua script
int Biome::calcPlantDistribution() const {
	int distribution = 30;
	if (temperature > 0.7f || humidity < 0.2f) {
		distribution = 50;
	} else if (temperature > 0.9f || humidity < 0.1f) {
		distribution = 100;
	}
	return distribution;
}

}
