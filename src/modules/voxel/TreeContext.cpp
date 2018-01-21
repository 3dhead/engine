/**
 * @file
 */

#include "TreeContext.h"
#include <string.h>

namespace voxel {

static const char* TreeTypeStr[] = {
	"Dome",
	"DomeHangingLeaves",
	"Cone",
	"Ellipsis",
	"BranchesEllipsis",
	"Cube",
	"CubeSideCubes",
	"Pine",
	"Fir",
	"Palm",
	"SpaceColonization"
};
static_assert(lengthof(TreeTypeStr) == (int)TreeType::Max, "tree type string array size doesn't match the available tree types");

TreeType getTreeType(const char *str) {
	for (int j = 0; j < (int)voxel::TreeType::Max; ++j) {
		if (strcmp(TreeTypeStr[j], str) != 0) {
			continue;
		}
		return (TreeType)j;
	}
	return TreeType::Max;
}

}
