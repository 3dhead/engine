/**
 * @file
 */

#include "AbstractVoxelTest.h"
#include "voxel/generator/LSystemGenerator.h"
#include "voxel/polyvox/Region.h"
#include "voxel/polyvox/PagedVolume.h"
#include "voxel/polyvox/CubicSurfaceExtractor.h"
#include "voxel/IsQuadNeeded.h"
#include "voxel/model/MeshExporter.h"

namespace voxel {

class MeshExporterTest: public AbstractVoxelTest {
};

TEST_F(MeshExporterTest, testExport) {
	Mesh mesh(100, 100, true);
	extractCubicMesh(&_volData, _ctx.region, &mesh, IsQuadNeeded(false));
	ASSERT_GE(mesh.getNoOfVertices(), 8u);
	ASSERT_GE(mesh.getNoOfIndices(), 8u);
	const char *filename = "meshexportertest.obj";
	ASSERT_TRUE(exportMesh(&mesh, filename)) << "Could not export mesh to " << filename;
	ASSERT_TRUE(core::App::getInstance()->filesystem()->exists(filename));
}

}
