/**
 * @file
 */

#include "core/tests/AbstractTest.h"
#include "../ui/editorscene/UndoHandler.h"
#include "voxel/polyvox/RawVolume.h"

namespace voxel {

class UndoHandlerTest: public core::AbstractTest {
protected:
	voxel::RawVolume* create(int size) const {
		const voxel::Region region(glm::ivec3(0), glm::ivec3(size - 1));
		EXPECT_EQ(size, region.getWidthInVoxels());
		return new voxel::RawVolume(region);
	}
};

TEST_F(UndoHandlerTest, testMarkUndo) {
	voxel::RawVolume* first = create(1);
	voxel::RawVolume* second = create(2);
	voxel::RawVolume* third = create(3);
	voxedit::UndoHandler undoHandler;
	EXPECT_FALSE(undoHandler.canRedo());
	EXPECT_FALSE(undoHandler.canUndo());

	undoHandler.markUndo(first);
	EXPECT_FALSE(undoHandler.canRedo()) << "Without a second entry and without undoing something before, you can't redo anything";
	EXPECT_FALSE(undoHandler.canUndo()) << "Without a second entry, you can't undo anything, because it is your initial state";
	EXPECT_EQ(1, (int)undoHandler.undoSize());
	EXPECT_EQ(0, (int)undoHandler.undoPosition());

	undoHandler.markUndo(second);
	EXPECT_FALSE(undoHandler.canRedo());
	EXPECT_TRUE(undoHandler.canUndo());
	EXPECT_EQ(2, (int)undoHandler.undoSize());
	EXPECT_EQ(1, (int)undoHandler.undoPosition());

	undoHandler.markUndo(third);
	EXPECT_FALSE(undoHandler.canRedo());
	EXPECT_TRUE(undoHandler.canUndo());
	EXPECT_EQ(3, (int)undoHandler.undoSize());
	EXPECT_EQ(2, (int)undoHandler.undoPosition());
}

TEST_F(UndoHandlerTest, testUndoRedo) {
	voxel::RawVolume* first = create(1);
	voxel::RawVolume* second = create(2);
	voxel::RawVolume* third = create(3);
	voxedit::UndoHandler undoHandler;
	undoHandler.markUndo(first);
	undoHandler.markUndo(second);
	undoHandler.markUndo(third);

	voxel::RawVolume* undoThird = undoHandler.undo();
	ASSERT_NE(nullptr, undoThird);
	EXPECT_EQ(2, undoThird->getRegion().getWidthInVoxels());
	EXPECT_TRUE(undoHandler.canRedo());
	EXPECT_TRUE(undoHandler.canUndo());
	EXPECT_EQ(1, (int)undoHandler.undoPosition());

	voxel::RawVolume* undoSecond = undoHandler.undo();
	ASSERT_NE(nullptr, undoSecond);
	EXPECT_EQ(1, undoSecond->getRegion().getWidthInVoxels());
	EXPECT_TRUE(undoHandler.canRedo());
	EXPECT_FALSE(undoHandler.canUndo());
	EXPECT_EQ(0, (int)undoHandler.undoPosition());

	voxel::RawVolume* redoSecond = undoHandler.redo();
	ASSERT_NE(nullptr, redoSecond);
	EXPECT_EQ(2, redoSecond->getRegion().getWidthInVoxels());
	EXPECT_TRUE(undoHandler.canRedo());
	EXPECT_TRUE(undoHandler.canUndo());
	EXPECT_EQ(1, (int)undoHandler.undoPosition());

	undoSecond = undoHandler.undo();
	ASSERT_NE(nullptr, undoSecond);
	EXPECT_EQ(1, undoSecond->getRegion().getWidthInVoxels());
	EXPECT_TRUE(undoHandler.canRedo());
	EXPECT_FALSE(undoHandler.canUndo());
	EXPECT_EQ(0, (int)undoHandler.undoPosition());

	voxel::RawVolume* undoNotPossible = undoHandler.undo();
	ASSERT_EQ(nullptr, undoNotPossible);
}

}
