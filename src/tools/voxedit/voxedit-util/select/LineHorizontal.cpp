#include "LineHorizontal.h"

namespace voxedit {
namespace selections {

int LineHorizontal::execute(voxel::RawVolume::Sampler& model, voxel::RawVolume::Sampler& selection) const {
	int cnt = 0;
	if (selection.setVoxel(model.voxel())) {
		++cnt;
	}
	const glm::ivec3& pos = model.position();
	goLeft(model, selection, cnt);

	selection.setPosition(pos);
	model.setPosition(pos);
	goRight(model, selection, cnt);

	selection.setPosition(pos);
	model.setPosition(pos);
	return cnt;
}

}
}
