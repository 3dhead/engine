#pragma once

#include "ui/Widget.h"
#include "video/Camera.h"
#include "video/FrameBuffer.h"
#include "frontend/Axis.h"
#include "video/MeshPool.h"
#include "voxel/polyvox/RawVolume.h"
#include "voxel/generator/LSystemGenerator.h"
#include "voxel/TreeContext.h"
#include "voxel/generator/PlantType.h"
#include "voxel/generator/BuildingGeneratorContext.h"
#include "voxel/generator/NoiseGenerator.h"
#include "voxel/WorldContext.h"
#include "Action.h"
#include "Controller.h"
#include "voxedit-util/Shape.h"
#include "voxedit-util/Axis.h"
#include "voxedit-util/SelectType.h"

class EditorScene: public ui::Widget {
private:
	using Super = ui::Widget;
	frontend::Axis _axis;
	video::FrameBuffer _frameBuffer;
	tb::UIBitmapGL _bitmap;
	voxedit::Controller _controller;
	glm::ivec2 _mousePos;
	std::string _cameraMode;

	void render();

	void setKeyAction(voxedit::Action action);
	void setInternalAction(voxedit::Action action);
public:
	UIWIDGET_SUBCLASS(EditorScene, Super);

	EditorScene();
	~EditorScene();

	void update();

	video::Camera& camera();
	void resetCamera();

	bool isDirty() const;
	bool isEmpty() const;
	bool voxelizeModel(const video::MeshPtr& mesh);
	bool importHeightmap(const std::string& file);
	bool saveModel(const std::string& file);
	bool loadModel(const std::string& file);
	bool exportModel(const std::string& file);
	bool newModel(bool force);

	void place();
	void remove();
	void rotate(int angleX, int angleY, int angleZ);
	void move(int x, int y, int z);

	void unselectAll();
	void select(const glm::ivec3& pos);

	void spaceColonization();

	void noise(int octaves, float persistence, float frequency, float amplitude, voxel::noise::NoiseType type);
	void lsystem(const voxel::lsystem::LSystemContext& ctx);
	void createTree(const voxel::TreeContext& ctx);
	void createBuilding(voxel::BuildingType type, const voxel::BuildingContext& ctx);
	void createPlant(voxel::PlantType type);
	void createCloud();
	void createCactus();
	void world(const voxel::WorldContext& ctx);

	const glm::ivec3& cursorPosition() const;
	void setCursorPosition(const glm::ivec3& pos, bool force = false);

	voxedit::SelectType selectionType() const;
	void setSelectionType(voxedit::SelectType type);

	voxedit::Shape cursorShape() const;
	void setCursorShape(voxedit::Shape type);
	void scaleCursorShape(const glm::vec3& scale);

	void setActionExecutionDelay(long actionExecutionDelay);
	long actionExecutionDelay() const;

	void setAction(voxedit::Action action);

	void setVoxel(const voxel::Voxel& voxel);

	voxedit::Axis lockedAxis() const;
	void setLockedAxis(voxedit::Axis axis, bool unlock);

	voxedit::Axis mirrorAxis() const;
	void setMirrorAxis(voxedit::Axis axis, const glm::ivec3& pos);

	float cameraSpeed() const;
	void setCameraSpeed(float cameraSpeed);

	bool renderAxis() const;
	void setRenderAxis(bool renderAxis);

	bool renderLockAxis() const;
	void setRenderLockAxis(bool renderLockAxis);

	bool renderAABB() const;
	void setRenderAABB(bool renderAABB);

	bool renderGrid() const;
	void setRenderGrid(bool renderGrid);

	void copy();
	void paste();
	void cut();
	void undo();
	void redo();
	bool canUndo() const;
	bool canRedo() const;

	void crop();
	void extend(const glm::ivec3& size);
	void scale();
	void fill(int x, int y, int z);

	virtual void OnInflate(const tb::INFLATE_INFO &info) override;
	virtual void OnProcess() override;
	virtual bool OnEvent(const tb::TBWidgetEvent &ev) override;
	virtual void OnPaint(const PaintProps &paintProps) override;
	virtual void OnResized(int oldw, int oldh) override;
};

inline float EditorScene::cameraSpeed() const {
	return _controller.cameraSpeed();
}

inline void EditorScene::setCameraSpeed(float cameraSpeed) {
	_controller.setCameraSpeed(cameraSpeed);
}

inline video::Camera& EditorScene::camera() {
	return _controller.camera();
}
