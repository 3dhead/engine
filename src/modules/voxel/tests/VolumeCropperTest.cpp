/**
 * @file
 */

#include "AbstractVoxelTest.h"
#include "voxel/polyvox/VolumeCropper.h"

namespace voxel {

class VolumeCropperTest: public AbstractVoxelTest {
};

TEST_F(VolumeCropperTest, testCropSmall) {
	voxel::RawVolume smallVolume(voxel::Region(0, 2));
	smallVolume.setVoxel(0, 0, 0, createVoxel(voxel::VoxelType::Grass, 0));
	RawVolume *croppedVolume = voxel::cropVolume(&smallVolume);
	ASSERT_NE(nullptr, croppedVolume) << "Expected to get the cropped raw volume";
	const voxel::Region& croppedRegion = croppedVolume->getRegion();
	EXPECT_EQ(croppedRegion.getUpperCorner(), glm::ivec3()) << croppedRegion;
	EXPECT_EQ(croppedRegion.getLowerCorner(), glm::ivec3()) << croppedRegion;
	EXPECT_EQ(croppedVolume->getVoxel(croppedRegion.getLowerCorner()), createVoxel(VoxelType::Grass, 0));
}

TEST_F(VolumeCropperTest, testCropBigger) {
	voxel::Region region = voxel::Region(0, 100);
	voxel::RawVolume smallVolume(region);
	smallVolume.setVoxel(region.getCentre(), createVoxel(voxel::VoxelType::Grass, 0));
	voxel::RawVolume *croppedVolume = voxel::cropVolume(&smallVolume);
	ASSERT_NE(nullptr, croppedVolume) << "Expected to get the cropped raw volume";
	const voxel::Region& croppedRegion = croppedVolume->getRegion();
	EXPECT_EQ(croppedRegion.getUpperCorner(), glm::ivec3()) << croppedRegion;
	EXPECT_EQ(croppedRegion.getLowerCorner(), glm::ivec3()) << croppedRegion;
	EXPECT_EQ(croppedVolume->getVoxel(croppedRegion.getLowerCorner()), createVoxel(VoxelType::Grass, 0)) << *croppedVolume;
}

TEST_F(VolumeCropperTest, testCropBiggerMultiple) {
	voxel::Region region = voxel::Region(0, 100);
	voxel::RawVolume smallVolume(region);
	smallVolume.setVoxel(region.getCentre(), createVoxel(voxel::VoxelType::Grass, 0));
	smallVolume.setVoxel(region.getUpperCorner(), createVoxel(voxel::VoxelType::Grass, 0));
	voxel::RawVolume *croppedVolume = voxel::cropVolume(&smallVolume);
	ASSERT_NE(nullptr, croppedVolume) << "Expected to get the cropped raw volume";
	const voxel::Region& croppedRegion = croppedVolume->getRegion();
	EXPECT_EQ(croppedRegion.getUpperCorner(), region.getCentre()) << croppedRegion;
	EXPECT_EQ(croppedRegion.getLowerCorner(), glm::ivec3()) << croppedRegion;
	EXPECT_EQ(croppedVolume->getVoxel(croppedRegion.getLowerCorner()), createVoxel(VoxelType::Grass, 0)) << *croppedVolume;
	EXPECT_EQ(croppedVolume->getVoxel(croppedRegion.getUpperCorner()), createVoxel(VoxelType::Grass, 0)) << *croppedVolume;
}

}
