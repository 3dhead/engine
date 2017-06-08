#include "Model.h"
#include "voxel/polyvox/VolumeMerger.h"
#include "voxel/polyvox/VolumeCropper.h"
#include "voxel/polyvox/VolumeRotator.h"
#include "voxel/polyvox/VolumeMover.h"
#include "voxel/polyvox/VolumeRescaler.h"
#include "voxel/polyvox//RawVolumeWrapper.h"
#include "voxel/polyvox//RawVolumeMoveWrapper.h"
#include "voxel/generator/WorldGenerator.h"
#include "voxel/generator/CloudGenerator.h"
#include "voxel/generator/CactusGenerator.h"
#include "voxel/generator/BuildingGenerator.h"
#include "voxel/generator/PlantGenerator.h"
#include "voxel/model/VoxFormat.h"
#include "voxel/model/QBTFormat.h"
#include "voxel/model/QBFormat.h"
#include "video/ScopedPolygonMode.h"
#include "video/ScopedLineWidth.h"
#include "core/Random.h"
#include "core/App.h"
#include "io/Filesystem.h"
#include "voxedit-util/tool/Crop.h"
#include "voxedit-util/tool/Expand.h"
#include "voxedit-util/tool/Fill.h"
#include "voxedit-util/ImportHeightmap.h"
#include "core/GLM.h"
#include <set>

namespace voxedit {

Model::Model() :
		_gridRenderer(true, true) {
}

Model::~Model() {
	shutdown();
}

bool Model::importHeightmap(const std::string& file) {
	voxel::RawVolume* v = modelVolume();
	if (v == nullptr) {
		return false;
	}
	const image::ImagePtr& img = image::loadImage(file, false);
	if (!img->isLoaded()) {
		return false;
	}
	voxedit::importHeightmap(*v, img);
	modified(v->region());
	return true;
}

bool Model::save(const std::string& file) {
	if (modelVolume() == nullptr) {
		return false;
	}
	const io::FilePtr& filePtr = core::App::getInstance()->filesystem()->open(std::string(file), io::FileMode::Write);
	if (filePtr->extension() == "qbt") {
		voxel::QBTFormat f;
		if (f.save(modelVolume(), filePtr)) {
			_dirty = false;
			return true;
		}
	} else if (filePtr->extension() == "vox") {
		voxel::VoxFormat f;
		if (f.save(modelVolume(), filePtr)) {
			_dirty = false;
			return true;
		}
	} else if (filePtr->extension() == "qb") {
		voxel::QBFormat f;
		if (f.save(modelVolume(), filePtr)) {
			_dirty = false;
			return true;
		}
	}
	return false;
}

bool Model::load(const std::string& file) {
	const io::FilePtr& filePtr = core::App::getInstance()->filesystem()->open(file);
	if (!(bool)filePtr) {
		Log::error("Failed to open model file %s", file.data());
		return false;
	}
	voxel::RawVolume* newVolume;

	if (filePtr->extension() == "qbt") {
		voxel::QBTFormat f;
		newVolume = f.load(filePtr);
	} else if (filePtr->extension() == "vox") {
		voxel::VoxFormat f;
		newVolume = f.load(filePtr);
	} else if (filePtr->extension() == "qb") {
		voxel::QBFormat f;
		newVolume = f.load(filePtr);
	} else {
		newVolume = nullptr;
	}
	if (newVolume == nullptr) {
		Log::error("Failed to load model file %s", file.c_str());
		return false;
	}
	Log::info("Loaded model file %s", file.c_str());
	undoHandler().clearUndoStates();
	setNewVolume(newVolume);
	modified(newVolume->region());
	_dirty = false;
	return true;
}

void Model::select(const glm::ivec3& pos) {
	voxel::RawVolume* selectionVolume = _rawVolumeSelectionRenderer.volume(SelectionVolumeIndex);
	_extractSelection |= _selectionHandler.select(modelVolume(), selectionVolume, pos);
}

void Model::unselectAll() {
	_selectionHandler.unselectAll();
	_rawVolumeSelectionRenderer.volume(SelectionVolumeIndex)->clear();
	_extractSelection = true;
}

void Model::setMousePos(int x, int y) {
	_mouseX = x;
	_mouseY = y;
}

void Model::modified(const voxel::Region& modifiedRegion, bool markUndo) {
	if (markUndo) {
		undoHandler().markUndo(modelVolume());
	}
	// TODO: handle the region
	_dirty = true;
	markExtract();
}

void Model::crop() {
	if (_empty) {
		Log::info("Empty volumes can't be cropped");
		return;
	}
	voxel::RawVolume* newVolume = voxedit::tool::crop(modelVolume());
	if (newVolume == nullptr) {
		return;
	}
	setNewVolume(newVolume);
	modified(newVolume->region());
}

void Model::extend(const glm::ivec3& size) {
	voxel::RawVolume* newVolume = voxedit::tool::expand(modelVolume(), size);
	if (newVolume == nullptr) {
		return;
	}
	setNewVolume(newVolume);
	modified(newVolume->region());
}

void Model::scale() {
	// TODO: check that src region boundaries are even
	const voxel::Region& srcRegion = modelVolume()->region();
	const int w = srcRegion.getWidthInVoxels();
	const int h = srcRegion.getHeightInVoxels();
	const int d = srcRegion.getDepthInVoxels();
	const glm::ivec3 maxs(w / 2, h / 2, d / 2);
	voxel::Region region(glm::zero<glm::ivec3>(), maxs);
	voxel::RawVolume* newVolume = new voxel::RawVolume(region);
	voxel::RawVolumeWrapper wrapper(newVolume);
	voxel::rescaleVolume(*modelVolume(), *newVolume);
	setNewVolume(newVolume);
	modified(newVolume->region());
}

void Model::fill(int x, int y, int z) {
	const bool overwrite = evalAction() == Action::OverrideVoxel;
	voxel::Region modifiedRegion;
	if (voxedit::tool::fill(*modelVolume(), glm::ivec3(x, y, z), _lockedAxis, _shapeHandler.currentVoxel(), overwrite, &modifiedRegion)) {
		modified(modifiedRegion);
	}
}

bool Model::place() {
	voxel::Region modifiedRegion;
	const bool extract = placeCursor(&modifiedRegion);
	if (extract) {
		modified(modifiedRegion);
	}
	return extract;
}

// TODO: delete selected volume from model volume
bool Model::remove() {
	const bool extract = setVoxel(_cursorPos, voxel::Voxel());
	if (extract) {
		const voxel::Region modifiedRegion(_cursorPos, _cursorPos);
		modified(modifiedRegion);
	}
	return extract;
}

void Model::vertices(float* vertices, size_t vertexSize, size_t verticesSize, uint32_t* indices, size_t indicesSize) {
	glm::ivec3 mins = glm::vec3(std::numeric_limits<float>::max());
	glm::ivec3 maxs = glm::vec3(std::numeric_limits<float>::min());
	// TODO: add uv support and apply colored voxels from the texture
	const voxel::Voxel& voxel = voxel::createColorVoxel(voxel::VoxelType::Generic, 0);

	for (size_t idx = 0u; idx < indicesSize; ++idx) {
		const uint32_t vertexIndex = indices[idx];
		const float* vertex = &vertices[vertexIndex * vertexSize];
		const glm::ivec3 pos(_cursorPos.x + vertex[0], _cursorPos.y + vertex[1], _cursorPos.z + vertex[2]);
		setVoxel(pos, voxel);
		mins.x = std::min(mins.x, pos.x);
		mins.y = std::min(mins.y, pos.y);
		mins.z = std::min(mins.z, pos.z);
		maxs.x = std::max(maxs.x, pos.x);
		maxs.y = std::max(maxs.y, pos.y);
		maxs.z = std::max(maxs.z, pos.z);
	}
	const voxel::Region modifiedRegion(mins, maxs);
	modified(modifiedRegion);
}

void Model::executeAction(long now) {
	const Action execAction = evalAction();
	if (execAction == Action::None) {
		Log::warn("Nothing to execute");
		return;
	}

	core_trace_scoped(EditorSceneExecuteAction);
	if (_lastAction == execAction) {
		if (now - _lastActionExecution < _actionExecutionDelay) {
			return;
		}
	}
	_lastAction = execAction;
	_lastActionExecution = now;

	bool extract = false;
	const bool didHit = _result.didHit;
	voxel::Region modifiedRegion;
	if (didHit && execAction == Action::CopyVoxel) {
		shapeHandler().setVoxel(getVoxel(_cursorPos));
	} else if (didHit && execAction == Action::SelectVoxels) {
		select(_cursorPos);
	} else if (didHit && execAction == Action::OverrideVoxel) {
		extract = placeCursor(&modifiedRegion);
	} else if (didHit && execAction == Action::DeleteVoxel) {
		extract = setVoxel(_cursorPos, voxel::Voxel());
		if (extract) {
			modifiedRegion.setLowerCorner(_cursorPos);
			modifiedRegion.setUpperCorner(_cursorPos);
		}
	} else if (_result.validPreviousPosition && execAction == Action::PlaceVoxel) {
		extract = placeCursor(&modifiedRegion);
	} else if (didHit && execAction == Action::PlaceVoxel) {
		extract = placeCursor(&modifiedRegion);
	}

	if (!extract) {
		return;
	}
	resetLastTrace();
	modified(modifiedRegion);
}

void Model::undo() {
	voxel::RawVolume* v = undoHandler().undo();
	if (v == nullptr) {
		return;
	}
	setNewVolume(v);
	modified(v->region(), false);
}

void Model::redo() {
	voxel::RawVolume* v = undoHandler().redo();
	if (v == nullptr) {
		return;
	}
	setNewVolume(v);
	modified(v->region(), false);
}

bool Model::placeCursor(voxel::Region* modifiedRegion) {
	const glm::ivec3& pos = _cursorPos;
	const voxel::RawVolume* cursorVolume = cursorPositionVolume();
	const voxel::Region& cursorRegion = cursorVolume->region();
	const glm::ivec3 mins = -cursorRegion.getCentre() + pos;
	const glm::ivec3 maxs = mins + cursorRegion.getDimensionsInCells();
	const voxel::Region destReg(mins, maxs);

	int cnt = 0;
	for (int32_t z = cursorRegion.getLowerZ(); z <= cursorRegion.getUpperZ(); ++z) {
		const int destZ = destReg.getLowerZ() + z - cursorRegion.getLowerZ();
		for (int32_t y = cursorRegion.getLowerY(); y <= cursorRegion.getUpperY(); ++y) {
			const int destY = destReg.getLowerY() + y - cursorRegion.getLowerY();
			for (int32_t x = cursorRegion.getLowerX(); x <= cursorRegion.getUpperX(); ++x) {
				const voxel::Voxel& voxel = cursorVolume->voxel(x, y, z);
				if (isAir(voxel.getMaterial())) {
					continue;
				}
				const int destX = destReg.getLowerX() + x - cursorRegion.getLowerX();
				if (setVoxel(glm::ivec3(destX, destY, destZ), voxel)) {
					++cnt;
				}
			}
		}
	}

	if (cnt <= 0) {
		return false;
	}
	if (modifiedRegion != nullptr) {
		*modifiedRegion = destReg;
	}
	return true;
}

void Model::resetLastTrace() {
	_lastRaytraceX = _lastRaytraceY = -1;
}

void Model::setNewVolume(voxel::RawVolume* volume) {
	const voxel::Region& region = volume->region();

	delete _rawVolumeSelectionRenderer.setVolume(SelectionVolumeIndex, new voxel::RawVolume(region));
	delete _rawVolumeRenderer.setVolume(ModelVolumeIndex, volume);
	delete _rawVolumeRenderer.setVolume(CursorVolumeIndex, new voxel::RawVolume(region));

	if (_spaceColonizationTree != nullptr) {
		delete _spaceColonizationTree;
		_spaceColonizationTree = nullptr;
	}

	if (volume != nullptr) {
		const voxel::Region& region = volume->region();
		_gridRenderer.update(region);
	} else {
		_gridRenderer.clear();
	}

	setCursorShape(_shapeHandler.cursorShape());

	_dirty = false;
	_lastPlacement = glm::ivec3(-1);
	_result = voxel::PickResult();
	const glm::ivec3 pos = _cursorPos;
	_cursorPos = pos * 10 + 10;
	setCursorPosition(pos);
	const glm::ivec3 refPos(region.getCentreX(), 0, region.getCentreZ());
	setReferencePosition(refPos);
	resetLastTrace();
}

bool Model::newVolume(bool force) {
	if (dirty() && !force) {
		return false;
	}
	const voxel::Region region(glm::ivec3(0), glm::ivec3(size() - 1));
	undoHandler().clearUndoStates();
	setNewVolume(new voxel::RawVolume(region));
	modified(region);
	_dirty = false;
	return true;
}

void Model::rotate(int angleX, int angleY, int angleZ) {
	const voxel::RawVolume* model = modelVolume();
	voxel::RawVolume* newVolume = voxel::rotateVolume(model, glm::vec3(angleX, angleY, angleZ), voxel::Voxel(), false);
	setNewVolume(newVolume);
	modified(newVolume->region());
}

void Model::move(int x, int y, int z) {
	const voxel::RawVolume* model = modelVolume();
	voxel::RawVolume* newVolume = new voxel::RawVolume(model->region());
	voxel::RawVolumeMoveWrapper wrapper(newVolume);
	voxel::moveVolume(&wrapper, model, glm::ivec3(x, y, z), voxel::Voxel());
	setNewVolume(newVolume);
	modified(newVolume->region());
}

const voxel::Voxel& Model::getVoxel(const glm::ivec3& pos) const {
	return modelVolume()->voxel(pos);
}

bool Model::setVoxel(const glm::ivec3& pos, const voxel::Voxel& voxel) {
	voxel::RawVolumeWrapper wrapper(modelVolume());
	const bool placed = wrapper.setVoxel(pos, voxel);
	if (!placed) {
		return false;
	}
	_lastPlacement = pos;

	if (_mirrorAxis == core::Axis::None) {
		return true;
	}
	const int index = getIndexForMirrorAxis(_mirrorAxis);
	const int delta = _mirrorPos[index] - pos[index] - 1;
	if (delta == 0) {
		return true;
	}
	glm::ivec3 mirror(glm::uninitialize);
	for (int i = 0; i < 3; ++i) {
		if (i == index) {
			mirror[i] = _mirrorPos[i] + delta;
		} else {
			mirror[i] = pos[i];
		}
	}
	wrapper.setVoxel(mirror, voxel);
	return true;
}

void Model::copy() {
	voxel::mergeRawVolumesSameDimension(cursorPositionVolume(), _rawVolumeSelectionRenderer.volume());
	markCursorExtract();
}

void Model::paste() {
	voxel::RawVolume* cursorVolume = cursorPositionVolume();
	const voxel::Region& srcRegion = cursorVolume->region();
	const voxel::Region destRegion = srcRegion + _cursorPos;
	voxel::RawVolumeWrapper wrapper(modelVolume());
	voxel::mergeVolumes(&wrapper, cursorVolume, destRegion, srcRegion);
}

void Model::cut() {
	voxel::RawVolume* cursorVolume = cursorPositionVolume();
	voxel::mergeRawVolumesSameDimension(cursorVolume, _rawVolumeSelectionRenderer.volume(SelectionVolumeIndex));
	// TODO: see remove
	// TODO: delete selected volume from model volume
}

void Model::render(const video::Camera& camera) {
	const voxel::Mesh* mesh = _rawVolumeRenderer.mesh(ModelVolumeIndex);
	_empty = mesh != nullptr ? mesh->getNoOfIndices() == 0 : true;
	_gridRenderer.render(camera, modelVolume()->region());
	_rawVolumeRenderer.render(camera);
	// TODO: render error if rendered last - but be before grid renderer to get transparency.
	if (_renderLockAxis) {
		for (int i = 0; i < (int)SDL_arraysize(_planeMeshIndex); ++i) {
			if (_planeMeshIndex[i] == -1) {
				continue;
			}
			_shapeRenderer.render(_planeMeshIndex[i], camera);
		}
	}
	if (_mirrorMeshIndex != -1) {
		_shapeRenderer.render(_mirrorMeshIndex, camera);
	}
	renderSelection(camera);
}

void Model::renderSelection(const video::Camera& camera) {
	const voxel::Mesh* mesh = _rawVolumeSelectionRenderer.mesh(SelectionVolumeIndex);
	if (mesh == nullptr || mesh->getNoOfIndices() == 0) {
		return;
	}
	video::ScopedPolygonMode polygonMode(video::PolygonMode::WireFrame, glm::vec2(-2.0f));
	video::ScopedLineWidth lineWidth(3.0f);
	video::enable(video::State::Blend);
	video::blendFunc(video::BlendMode::One, video::BlendMode::One);
	_rawVolumeSelectionRenderer.render(camera);
	video::blendFunc(video::BlendMode::SourceAlpha, video::BlendMode::OneMinusSourceAlpha);
}

void Model::onResize(const glm::ivec2& size) {
	_rawVolumeRenderer.onResize(glm::ivec2(), size);
	_rawVolumeSelectionRenderer.onResize(glm::ivec2(), size);
}

void Model::init() {
	if (_initialized > 0) {
		return;
	}
	++_initialized;
	_rawVolumeRenderer.init();
	_rawVolumeSelectionRenderer.init();
	_shapeRenderer.init();
	_gridRenderer.init();

	_mirrorMeshIndex = -1;
	for (int i = 0; i < (int)SDL_arraysize(_planeMeshIndex); ++i) {
		_planeMeshIndex[i] = -1;
	}

	_lastAction = Action::None;
	_action = Action::None;

	_lockedAxis = core::Axis::None;
	_mirrorAxis = core::Axis::None;
}

void Model::update() {
	const long ms = core::App::getInstance()->currentMillis();
	if (_spaceColonizationTree != nullptr && ms - _lastGrow > 1000L) {
		const bool growing = _spaceColonizationTree->grow();
		_lastGrow = ms;
		voxel::RawVolumeWrapper wrapper(modelVolume());
		core::Random random;
		const voxel::RandomVoxel woodRandomVoxel(voxel::VoxelType::Wood, random);
		_spaceColonizationTree->generate(wrapper, woodRandomVoxel);
		modified(modelVolume()->region());
		if (!growing) {
			const voxel::RandomVoxel leavesRandomVoxel(voxel::VoxelType::Leaf, random);
			_spaceColonizationTree->generateLeaves(wrapper, leavesRandomVoxel, glm::ivec3(12));
			delete _spaceColonizationTree;
			_spaceColonizationTree = nullptr;
		}
	}

	extractVolume();
	extractCursorVolume();
	extractSelectionVolume();
}

void Model::shutdown() {
	--_initialized;
	if (_initialized > 0) {
		return;
	} else if (_initialized < 0) {
		_initialized = 0;
		return;
	}
	_initialized = 0;
	{
		const std::vector<voxel::RawVolume*>& old = _rawVolumeRenderer.shutdown();
		for (voxel::RawVolume* v : old) {
			delete v;
		}
	}
	{
		const std::vector<voxel::RawVolume*>& old = _rawVolumeSelectionRenderer.shutdown();
		for (voxel::RawVolume* v : old) {
			delete v;
		}
	}

	if (_spaceColonizationTree != nullptr) {
		delete _spaceColonizationTree;
		_spaceColonizationTree = nullptr;
	}

	_shapeRenderer.shutdown();
	_shapeBuilder.shutdown();
	_gridRenderer.shutdown();
	undoHandler().clearUndoStates();
}

bool Model::extractSelectionVolume() {
	if (_extractSelection) {
		_extractSelection = false;
		_rawVolumeSelectionRenderer.extract(SelectionVolumeIndex);
		return true;
	}
	return false;
}

bool Model::extractVolume() {
	if (_extract) {
		_extract = false;
		_rawVolumeRenderer.extract(ModelVolumeIndex);
		return true;
	}
	return false;
}

bool Model::extractCursorVolume() {
	if (_extractCursor) {
		_extractCursor = false;
		_rawVolumeRenderer.extract(CursorVolumeIndex);
		return true;
	}
	return false;
}

void Model::noise(int octaves, float lacunarity, float frequency, float gain, voxel::noise::NoiseType type) {
	core::Random random;
	voxel::RawVolumeWrapper wrapper(modelVolume());
	voxel::noise::generate(wrapper, octaves, lacunarity, frequency, gain, type, random);
	modified(modelVolume()->region());
}

void Model::spaceColonization() {
	const voxel::Region& region = modelVolume()->region();
	const core::AABB<int>& aabb = region.aabb();
	const int trunkHeight = aabb.getWidthY() / 4;
	_lastGrow = core::App::getInstance()->currentMillis();

	_spaceColonizationTree = new voxel::tree::Tree(referencePosition(), trunkHeight, 6,
			aabb.getWidthX(), aabb.getWidthY() - trunkHeight, aabb.getWidthZ(), 4.0f, _lastGrow);
}

void Model::lsystem(const voxel::lsystem::LSystemContext& lsystemCtx) {
	core::Random random;
	voxel::RawVolumeWrapper wrapper(modelVolume());
	if (voxel::lsystem::generate(wrapper, lsystemCtx, random)) {
		modified(modelVolume()->region());
	}
}

void Model::world(const voxel::WorldContext& ctx) {
	const voxel::Region region(glm::ivec3(0), glm::ivec3(127, 63, 127));
	setNewVolume(new voxel::RawVolume(region));
	voxel::BiomeManager mgr;
	const io::FilesystemPtr& filesystem = core::App::getInstance()->filesystem();
	mgr.init(filesystem->load("biomes.lua"));
	voxel::RawVolumeWrapper wrapper(modelVolume());
	voxel::world::WorldGenerator gen(mgr, 1L);
	gen.createWorld(ctx, wrapper, 0.0f, 0.0f);
	voxel::cloud::CloudContext cloudCtx;
	gen.createClouds(wrapper, cloudCtx);
	gen.createTrees(wrapper);
	modified(modelVolume()->region());
}

void Model::createCactus() {
	core::Random random;
	voxel::RawVolumeWrapper wrapper(modelVolume());
	voxel::cactus::createCactus(wrapper, _referencePos, 18, 2, random);
	modified(modelVolume()->region());
}

void Model::createCloud() {
	voxel::RawVolumeWrapper wrapper(modelVolume());
	struct HasClouds {
		glm::vec2 pos;
		void getCloudPositions(const voxel::Region& region, std::vector<glm::vec2>& positions, core::Random& random, int border) const {
			positions.push_back(pos);
		}
	};
	HasClouds hasClouds;
	hasClouds.pos = glm::vec2(_referencePos.x, _referencePos.z);
	voxel::cloud::CloudContext cloudCtx;
	cloudCtx.amount = 1;
	if (voxel::cloud::createClouds(wrapper, wrapper.region(), hasClouds, cloudCtx)) {
		modified(modelVolume()->region());
	}
}

void Model::createPlant(voxel::PlantType type) {
	voxel::PlantGenerator g;
	voxel::RawVolumeWrapper wrapper(modelVolume());
	if (type == voxel::PlantType::Flower) {
		g.createFlower(5, _referencePos, wrapper);
	} else if (type == voxel::PlantType::Grass) {
		g.createGrass(10, _referencePos, wrapper);
	} else if (type == voxel::PlantType::Mushroom) {
		g.createMushroom(7, _referencePos, wrapper);
	}
	g.shutdown();
	modified(modelVolume()->region());
}

void Model::createBuilding(voxel::BuildingType type, const voxel::BuildingContext& ctx) {
	voxel::RawVolumeWrapper wrapper(modelVolume());
	voxel::building::createBuilding(wrapper, _referencePos, type);
	modified(modelVolume()->region());
}

void Model::createTree(voxel::TreeContext ctx) {
	core::Random random;
	voxel::RawVolumeWrapper wrapper(modelVolume());
	ctx.pos = _referencePos;
	voxel::tree::createTree(wrapper, ctx, random);
	modified(modelVolume()->region());
}

void Model::setReferencePosition(const glm::ivec3& pos) {
	_referencePos = pos;
}

void Model::setCursorPosition(glm::ivec3 pos, bool force) {
	if (!force) {
		if ((_lockedAxis & core::Axis::X) != core::Axis::None) {
			pos.x = _cursorPos.x;
		}
		if ((_lockedAxis & core::Axis::Y) != core::Axis::None) {
			pos.y = _cursorPos.y;
		}
		if ((_lockedAxis & core::Axis::Z) != core::Axis::None) {
			pos.z = _cursorPos.z;
		}
	}

	const voxel::Region& region = modelVolume()->region();
	if (!region.containsPoint(pos)) {
		pos = region.moveInto(pos.x, pos.y, pos.z);
	}
	if (_cursorPos == pos) {
		return;
	}
	_cursorPos = pos;
	const voxel::Region& cursorRegion = cursorPositionVolume()->region();
	_rawVolumeRenderer.setOffset(CursorVolumeIndex, -cursorRegion.getCentre() + _cursorPos);

	updateLockedPlane(core::Axis::X);
	updateLockedPlane(core::Axis::Y);
	updateLockedPlane(core::Axis::Z);
}

void Model::markCursorExtract() {
	_extractCursor = true;
}

void Model::markExtract() {
	_extract = true;
}

bool Model::trace(const video::Camera& camera) {
	if (modelVolume() == nullptr) {
		return false;
	}

	if (_lastRaytraceX != _mouseX || _lastRaytraceY != _mouseY) {
		core_trace_scoped(EditorSceneOnProcessUpdateRay);
		_lastRaytraceX = _mouseX;
		_lastRaytraceY = _mouseY;

		const video::Ray& ray = camera.mouseRay(glm::ivec2(_mouseX, _mouseY));
		const glm::vec3& dirWithLength = ray.direction * camera.farPlane();
		static constexpr voxel::Voxel air;
		_result = voxel::pickVoxel(modelVolume(), ray.origin, dirWithLength, air);

		if (actionRequiresExistingVoxel(evalAction())) {
			if (_result.didHit) {
				setCursorPosition(_result.hitVoxel);
			} else if (_result.validPreviousPosition) {
				setCursorPosition(_result.previousPosition);
			}
		} else if (_result.validPreviousPosition) {
			setCursorPosition(_result.previousPosition);
		}
	}

	return true;
}

int Model::getIndexForAxis(core::Axis axis) const {
	if (axis == core::Axis::X) {
		return 0;
	} else if (axis == core::Axis::Y) {
		return 1;
	}
	return 2;
}

int Model::getIndexForMirrorAxis(core::Axis axis) const {
	if (axis == core::Axis::X) {
		return 2;
	} else if (axis == core::Axis::Y) {
		return 1;
	}
	return 0;
}

void Model::updateShapeBuilderForPlane(bool mirror, const glm::ivec3& pos, core::Axis axis, const glm::vec4& color) {
	const voxel::Region& region = modelVolume()->region();
	const int index = mirror ? getIndexForMirrorAxis(axis) : getIndexForAxis(axis);
	glm::vec3 mins = region.getLowerCorner();
	glm::vec3 maxs = region.getUpperCorner();
	mins[index] = maxs[index] = pos[index];
	const glm::vec3& ll = mins;
	const glm::vec3& ur = maxs;
	glm::vec3 ul(glm::uninitialize);
	glm::vec3 lr(glm::uninitialize);
	if (axis == core::Axis::Y) {
		ul = glm::vec3(mins.x, mins.y, maxs.z);
		lr = glm::vec3(maxs.x, maxs.y, mins.z);
	} else {
		ul = glm::vec3(mins.x, maxs.y, mins.z);
		lr = glm::vec3(maxs.x, mins.y, maxs.z);
	}
	std::vector<glm::vec3> vecs({ll, ul, ur, lr});
	// lower left (0), upper left (1), upper right (2)
	// lower left (0), upper right (2), lower right (3)
	const std::vector<uint32_t> indices { 0, 1, 2, 0, 2, 3, 2, 1, 0, 3, 2, 0 };
	_shapeBuilder.clear();
	_shapeBuilder.setColor(color);
	_shapeBuilder.geom(vecs, indices);
}

void Model::updateLockedPlane(core::Axis axis) {
	if (axis == core::Axis::None) {
		return;
	}
	const int index = getIndexForAxis(axis);
	int32_t& meshIndex = _planeMeshIndex[index];
	if ((_lockedAxis & axis) == core::Axis::None) {
		if (meshIndex != -1) {
			_shapeRenderer.deleteMesh(meshIndex);
			meshIndex = -1;
		}
		return;
	}

	const glm::vec4 colors[] = {
		core::Color::LightRed,
		core::Color::LightGreen,
		core::Color::LightBlue
	};
	updateShapeBuilderForPlane(false, _cursorPos, axis, core::Color::alpha(colors[index], 0.3f));
	_shapeRenderer.createOrUpdate(meshIndex, _shapeBuilder);
}

core::Axis Model::mirrorAxis() const {
	return _mirrorAxis;
}

void Model::setMirrorAxis(core::Axis axis, const glm::ivec3& mirrorPos) {
	if (_mirrorAxis == axis) {
		if (_mirrorPos != mirrorPos) {
			_mirrorPos = mirrorPos;
			updateMirrorPlane();
		}
		return;
	}
	_mirrorPos = mirrorPos;
	_mirrorAxis = axis;
	updateMirrorPlane();
}

void Model::updateMirrorPlane() {
	if (_mirrorAxis == core::Axis::None) {
		if (_mirrorMeshIndex != -1) {
			_shapeRenderer.deleteMesh(_mirrorMeshIndex);
			_mirrorMeshIndex = -1;
		}
		return;
	}

	updateShapeBuilderForPlane(true, _mirrorPos, _mirrorAxis, core::Color::alpha(core::Color::LightGray, 0.1f));
	_shapeRenderer.createOrUpdate(_mirrorMeshIndex, _shapeBuilder);
}

void Model::setLockedAxis(core::Axis axis, bool unlock) {
	if (unlock) {
		_lockedAxis &= ~axis;
	} else {
		_lockedAxis |= axis;
	}
	updateLockedPlane(core::Axis::X);
	updateLockedPlane(core::Axis::Y);
	updateLockedPlane(core::Axis::Z);
}

}
