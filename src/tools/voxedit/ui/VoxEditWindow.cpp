#include "VoxEditWindow.h"
#include "LSystemWindow.h"
#include "NoiseWindow.h"
#include "WorldWindow.h"
#include "TreeWindow.h"
#include "editorscene/EditorScene.h"
#include "palette/PaletteWidget.h"
#include "io/Filesystem.h"
#include "../VoxEdit.h"
#include <assimp/Exporter.hpp>
#include <assimp/Importer.hpp>
#include <assimp/importerdesc.h>
#include <set>

namespace voxedit {

static const struct {
	tb::TBID id;
	Action action;
	bool availableOnEmpty;
} actions[] = {
	{TBIDC("actionoverride"),	Action::OverrideVoxel, false},
	{TBIDC("actiondelete"),		Action::DeleteVoxel, false},
	{TBIDC("actioncopy"),		Action::CopyVoxel, false},
	{TBIDC("actionplace"),		Action::PlaceVoxel, true},
	{TBIDC("actionselect"),		Action::SelectVoxels, false}
};

static const struct {
	tb::TBID id;
	SelectType type;
} selectionmodes[] = {
	{TBIDC("actionselectsingle"),		SelectType::Single},
	{TBIDC("actionselectsame"),			SelectType::Same},
	{TBIDC("actionselecthorizontal"),	SelectType::LineHorizontal},
	{TBIDC("actionselectvertical"),		SelectType::LineVertical},
	{TBIDC("actionselectedge"),			SelectType::Edge}
};

static const struct {
	tb::TBID id;
	Shape shape;
} shapes[] = {
	{TBIDC("shapecone"),		Shape::Cone},
	{TBIDC("shapesingle"),		Shape::Single},
	{TBIDC("shapesphere"),		Shape::Sphere},
	{TBIDC("shapecircle"),		Shape::Circle},
	{TBIDC("shapedome"),		Shape::Dome},
	{TBIDC("shapetorus"),		Shape::Torus},
	{TBIDC("shapeplane"),		Shape::Plane}
};

static const struct {
	const char *name;
	const char *id;
	tb::TBID tbid;
	voxel::TreeType type;
} treeTypes[] = {
	{"Pine",				"tree_pine",				TBIDC("tree_pine"),					voxel::TreeType::Pine},
	{"Dome",				"tree_dome",				TBIDC("tree_dome"),					voxel::TreeType::Dome},
	{"Dome Hanging",		"tree_dome2",				TBIDC("tree_dome2"),				voxel::TreeType::DomeHangingLeaves},
	{"Cone",				"tree_cone",				TBIDC("tree_cone"),					voxel::TreeType::Cone},
	{"Fir",					"tree_fir",					TBIDC("tree_fir"),					voxel::TreeType::Fir},
	{"Ellipsis2",			"tree_ellipsis2",			TBIDC("tree_ellipsis2"),			voxel::TreeType::BranchesEllipsis},
	{"Ellipsis",			"tree_ellipsis",			TBIDC("tree_ellipsis"),				voxel::TreeType::Ellipsis},
	{"Cube",				"tree_cube",				TBIDC("tree_cube"),					voxel::TreeType::Cube},
	{"Cube Sides",			"tree_cube2",				TBIDC("tree_cube2"),				voxel::TreeType::CubeSideCubes},
	{"Palm",				"tree_palm",				TBIDC("tree_palm"),					voxel::TreeType::Palm},
	{"SpaceColonization",	"tree_spacecolonization",	TBIDC("tree_spacecolonization"),	voxel::TreeType::SpaceColonization}
};
static_assert((int)SDL_arraysize(treeTypes) == (int)voxel::TreeType::Max, "Missing support for tree types in the ui");

static const struct {
	const char *name;
	const char *id;
	tb::TBID tbid;
	voxel::PlantType type;
} plantTypes[] = {
	{"Flower",		"plant_flower",		TBIDC("plant_flower"),		voxel::PlantType::Flower},
	{"Grass",		"plant_grass",		TBIDC("plant_grass"),		voxel::PlantType::Grass},
	{"Mushroom",	"plant_mushroom",	TBIDC("plant_mushroom"),	voxel::PlantType::Mushroom}
};
static_assert((int)SDL_arraysize(plantTypes) == (int)voxel::PlantType::MaxPlantTypes, "Missing support for plant types in the ui");

static const struct {
	const char *name;
	const char *id;
	tb::TBID tbid;
	voxel::BuildingType type;
} buildingTypes[] = {
	{"Tower",		"building_tower",	TBIDC("building_tower"),	voxel::BuildingType::Tower},
	{"House",		"building_house",	TBIDC("building_house"),	voxel::BuildingType::House}
};
static_assert((int)SDL_arraysize(buildingTypes) == (int)voxel::BuildingType::Max, "Missing support for building types in the ui");

VoxEditWindow::VoxEditWindow(VoxEdit* tool) :
		ui::Window(tool), _scene(nullptr), _voxedit(tool), _paletteWidget(nullptr) {
	SetSettings(tb::WINDOW_SETTINGS_CAN_ACTIVATE);
	for (uint32_t i = 0; i < SDL_arraysize(treeTypes); ++i) {
		addMenuItem(_treeItems, treeTypes[i].name, treeTypes[i].id);
	}
	addMenuItem(_fileItems, "New");
	addMenuItem(_fileItems, "Load");
	addMenuItem(_fileItems, "Save");
	addMenuItem(_fileItems, "Import");
	addMenuItem(_fileItems, "Export");
	addMenuItem(_fileItems, "Heightmap");
	addMenuItem(_fileItems, "Quit");

	addMenuItem(_plantItems, "Cactus", "cactus");
	for (uint32_t i = 0; i < SDL_arraysize(plantTypes); ++i) {
		addMenuItem(_plantItems, plantTypes[i].name, plantTypes[i].id);
	}

	for (uint32_t i = 0; i < SDL_arraysize(buildingTypes); ++i) {
		addMenuItem(_buildingItems, buildingTypes[i].name, buildingTypes[i].id);
	}

	addMenuItem(_structureItems, "Trees")->sub_source = &_treeItems;
	addMenuItem(_structureItems, "Plants", "plants")->sub_source = &_plantItems;
	addMenuItem(_structureItems, "Clouds", "clouds");
	addMenuItem(_structureItems, "Buildings", "buildings")->sub_source = &_buildingItems;
}

VoxEditWindow::~VoxEditWindow() {
}

bool VoxEditWindow::init() {
	if (!loadResourceFile("ui/window/voxedit-main.tb.txt")) {
		Log::error("Failed to init the main window: Could not load the ui definition");
		return false;
	}
	_scene = getWidgetByType<EditorScene>("editorscene");
	if (_scene == nullptr) {
		Log::error("Failed to init the main window: Could not get the editor scene node with id 'editorscene'");
		return false;
	}

	_paletteWidget = getWidgetByType<PaletteWidget>("palettecontainer");
	if (_paletteWidget == nullptr) {
		Log::error("Failed to init the main window: Could not get the editor scene node with id 'palettecontainer'");
		return false;
	}
	const int8_t index = (uint8_t)_paletteWidget->GetValue();
	const voxel::Voxel voxel = voxel::createVoxel(voxel::VoxelType::Generic, index);
	_scene->setVoxel(voxel);
	_paletteWidget->markAsClean();

	_sceneTop = getWidgetByType<EditorScene>("editorscenetop");
	_sceneLeft = getWidgetByType<EditorScene>("editorsceneleft");
	_sceneFront = getWidgetByType<EditorScene>("editorscenefront");

	_fourViewAvailable = _sceneTop != nullptr && _sceneLeft != nullptr && _sceneFront != nullptr;

	tb::TBWidget* toggleViewPort = getWidget("toggleviewport");
	if (toggleViewPort != nullptr) {
		toggleViewPort->SetState(tb::WIDGET_STATE_DISABLED, !_fourViewAvailable);
		const int value = toggleViewPort->GetValue();
		setQuadViewport(value == 1);
	}
	_exportButton = getWidget("export");
	_saveButton = getWidget("save");
	_undoButton = getWidget("undo");
	_redoButton = getWidget("redo");

	_cursorX = getWidgetByType<tb::TBEditField>("cursorx");
	_cursorY = getWidgetByType<tb::TBEditField>("cursory");
	_cursorZ = getWidgetByType<tb::TBEditField>("cursorz");

	_lockedX = getWidgetByType<tb::TBCheckBox>("lockx");
	_lockedY = getWidgetByType<tb::TBCheckBox>("locky");
	_lockedZ = getWidgetByType<tb::TBCheckBox>("lockz");

	_mirrorX = getWidgetByType<tb::TBRadioButton>("mirrorx");
	_mirrorY = getWidgetByType<tb::TBRadioButton>("mirrory");
	_mirrorZ = getWidgetByType<tb::TBRadioButton>("mirrorz");

	_showAABB = getWidgetByType<tb::TBCheckBox>("optionshowaabb");
	_showGrid = getWidgetByType<tb::TBCheckBox>("optionshowgrid");
	_showAxis = getWidgetByType<tb::TBCheckBox>("optionshowaxis");
	_showLockAxis = getWidgetByType<tb::TBCheckBox>("optionshowlockaxis");
	_freeLook = getWidgetByType<tb::TBCheckBox>("optionfreelook");
	if (_showAABB == nullptr || _showGrid == nullptr || _showLockAxis == nullptr || _showAxis == nullptr || _freeLook == nullptr) {
		Log::error("Could not load all required widgets");
		return false;
	}

	_showAABB->SetValue(_scene->renderAABB() ? 1 : 0);
	_showGrid->SetValue(_scene->renderGrid() ? 1 : 0);
	_showAxis->SetValue(_scene->renderAxis() ? 1 : 0);
	_showLockAxis->SetValue(_scene->renderLockAxis() ? 1 : 0);
	_freeLook->SetValue(_scene->camera().rotationType() == video::CameraRotationType::Eye ? 1 : 0);

	Assimp::Exporter exporter;
	const size_t exporterNum = exporter.GetExportFormatCount();
	for (size_t i = 0; i < exporterNum; ++i) {
		const aiExportFormatDesc* desc = exporter.GetExportFormatDescription(i);
		_exportFilter.append(desc->fileExtension);
		if (i < exporterNum - 1) {
			_exportFilter.append(";");
		}
	}

	Assimp::Importer importer;
	const size_t importerNum = importer.GetImporterCount();
	std::set<std::string> importExtensions;
	for (size_t i = 0; i < importerNum; ++i) {
		const aiImporterDesc* desc = importer.GetImporterInfo(i);
		const char* ext = desc->mFileExtensions;
		const char* last = ext;
		do {
			if (ext[0] == '\0' || ext[0] == ' ') {
				importExtensions.insert(std::string(last, ext - last));
				last = ext;
				while (*last == ' ') {
					++last;
				}
			}
		} while (*ext++);
	}
	const int importerExtensionCount = importExtensions.size();
	int n = 0;
	for (auto i = importExtensions.begin(); i != importExtensions.end(); ++i, ++n) {
		_importFilter.append(*i);
		if (n < importerExtensionCount - 1) {
			_importFilter.append(";");
		}
	}
	Log::info("Supported import filters: %s", _importFilter.c_str());
	Log::info("Supported export filters: %s", _exportFilter.c_str());

	return true;
}

void VoxEditWindow::update() {
	_scene->update();
	_sceneTop->update();
	_sceneLeft->update();
	_sceneFront->update();
}

void VoxEditWindow::setCursorPosition(int x, int y, int z, bool relative) {
	if (relative) {
		glm::ivec3 p = _scene->cursorPosition();
		p.x += x;
		p.y += y;
		p.z += z;
		_scene->setCursorPosition(p, true);
	} else {
		_scene->setCursorPosition(glm::ivec3(x, y, z), true);
	}
}

void VoxEditWindow::place() {
	_scene->place();
}

void VoxEditWindow::remove() {
	_scene->remove();
}

void VoxEditWindow::rotate(int x, int y, int z) {
	Log::debug("execute rotate by %i:%i:%i", x, y, z);
	_scene->rotate(x, y, z);
}

void VoxEditWindow::scale(float x, float y, float z) {
	Log::debug("execute scale by %f:%f:%f", x, y, z);
	_scene->scaleCursorShape(glm::vec3(x, y, z));
}

void VoxEditWindow::move(int x, int y, int z) {
	Log::debug("execute move by %i:%i:%i", x, y, z);
	_scene->move(x, y, z);
}

void VoxEditWindow::executeMode() {
	if (_mode == ModifierMode::None) {
		_modeNumberBuf[0] = '\0';
		_lastModePress = -1l;
		_axis = core::Axis::None;
		return;
	}

	if (_modeNumberBuf[0] != '\0') {
		if (_mode == ModifierMode::Scale) {
			const float value = core::string::toFloat(_modeNumberBuf);
			glm::vec3 values(1.0f, 1.0f, 1.0f);
			if ((_axis & core::Axis::X) != core::Axis::None) {
				values.x = value;
			}
			if ((_axis & core::Axis::Y) != core::Axis::None) {
				values.y = value;
			}
			if ((_axis & core::Axis::Z) != core::Axis::None) {
				values.z = value;
			}
			scale(values.x, values.y, values.z);
		} else {
			const int value = core::string::toInt(_modeNumberBuf);
			glm::ivec3 values(0, 0, 0);
			if ((_axis & core::Axis::X) != core::Axis::None) {
				values.x = value;
			}
			if ((_axis & core::Axis::Y) != core::Axis::None) {
				values.y = value;
			}
			if ((_axis & core::Axis::Z) != core::Axis::None) {
				values.z = value;
			}

			if (_mode == ModifierMode::Rotate) {
				rotate(values.x, values.y, values.z);
			} else if (_mode == ModifierMode::Move) {
				move(values.x, values.y, values.z);
			}
		}
	}
	if (_mode == ModifierMode::Lock) {
		const core::Axis locked = _scene->lockedAxis();
#define VOXEDIT_LOCK(axis) if ((_axis & axis) != core::Axis::None) { _scene->setLockedAxis(axis, (locked & axis) != core::Axis::None); _lockedDirty = true; }
		VOXEDIT_LOCK(core::Axis::X)
		VOXEDIT_LOCK(core::Axis::Y)
		VOXEDIT_LOCK(core::Axis::Z)
#undef VOXEDIT_LOCK
	} else if (_mode == ModifierMode::Mirror) {
#define VOXEDIT_MIRROR(axis) if (_axis == axis) { _scene->setMirrorAxis(axis, _scene->referencePosition()); _mirrorDirty = true; }
		VOXEDIT_MIRROR(core::Axis::X)
		VOXEDIT_MIRROR(core::Axis::Y)
		VOXEDIT_MIRROR(core::Axis::Z)
#undef VOXEDIT_MIRROR
	}

	_modeNumberBuf[0] = '\0';
	_lastModePress = -1l;
	_axis = core::Axis::None;
	_mode = ModifierMode::None;
}

void VoxEditWindow::toggleviewport() {
	bool vis = false;
	if (_sceneTop != nullptr) {
		vis = _sceneTop->GetVisibilityCombined();
	}
	if (!vis && _sceneLeft != nullptr) {
		vis = _sceneLeft->GetVisibilityCombined();
	}
	if (!vis && _sceneFront != nullptr) {
		vis = _sceneFront->GetVisibilityCombined();
	}

	setQuadViewport(!vis);
}

void VoxEditWindow::setReferencePosition(int x, int y, int z) {
	_scene->setReferencePosition(glm::ivec3(x, y, z));
}

void VoxEditWindow::setReferencePositionToCursor() {
	_scene->setReferencePosition(_scene->cursorPosition());
}

void VoxEditWindow::unselectall() {
	_scene->unselectAll();
}

void VoxEditWindow::bezier(const glm::ivec3& start, const glm::ivec3& end, const glm::ivec3& control) {
	_scene->bezier(start, end, control);
}

void VoxEditWindow::rotatemode() {
	_mode = ModifierMode::Rotate;
	_axis = core::Axis::None;
	_modeNumberBuf[0] = '\0';
}

void VoxEditWindow::scalemode() {
	_mode = ModifierMode::Scale;
	_axis = core::Axis::None;
	_modeNumberBuf[0] = '\0';
}

void VoxEditWindow::movemode() {
	_mode = ModifierMode::Move;
	_axis = core::Axis::None;
	_modeNumberBuf[0] = '\0';
}

void VoxEditWindow::togglelockaxis() {
	_mode = ModifierMode::Lock;
	_axis = core::Axis::None;
	_modeNumberBuf[0] = '\0';
}

void VoxEditWindow::togglemirroraxis() {
	_mode = ModifierMode::Mirror;
	_axis = core::Axis::None;
	_modeNumberBuf[0] = '\0';
}

void VoxEditWindow::togglefreelook() {
	if (_freeLook == nullptr) {
		return;
	}
	const int v = _freeLook->GetValue();
	_freeLook->SetValue(v == 0 ? 1 : 0);
	video::Camera& c = _scene->camera();
	if (v == 0) {
		c.setRotationType(video::CameraRotationType::Eye);
	} else {
		c.setRotationType(video::CameraRotationType::Target);
	}
}

void VoxEditWindow::setQuadViewport(bool active) {
	const tb::WIDGET_VISIBILITY vis = active ? tb::WIDGET_VISIBILITY_VISIBLE : tb::WIDGET_VISIBILITY_GONE;
	if (_sceneTop != nullptr) {
		_sceneTop->SetVisibility(vis);
	}
	if (_sceneLeft != nullptr) {
		_sceneLeft->SetVisibility(vis);
	}
	if (_sceneFront != nullptr) {
		_sceneFront->SetVisibility(vis);
	}
	tb::TBWidget* toggleViewPort = getWidget("toggleviewport");
	if (toggleViewPort != nullptr) {
		toggleViewPort->SetValue(active ? 1 : 0);
	}
}

static inline bool isAny(const tb::TBWidgetEvent& ev, const tb::TBID& id) {
	return ev.target->GetID() == id || ev.ref_id == id;
}

bool VoxEditWindow::handleEvent(const tb::TBWidgetEvent &ev) {
	if (isAny(ev, TBIDC("resetcamera"))) {
		_scene->resetCamera();
		_sceneFront->resetCamera();
		_sceneLeft->resetCamera();
		_sceneTop->resetCamera();
		return true;
	} else if (isAny(ev, TBIDC("quit"))) {
		quit();
		return true;
	} else if (isAny(ev, TBIDC("crop"))) {
		crop();
		return true;
	} else if (isAny(ev, TBIDC("extend"))) {
		extend();
		return true;
	} else if (isAny(ev, TBIDC("fill"))) {
		const glm::ivec3& pos = _scene->cursorPosition();
		fill(pos.x, pos.y, pos.z);
		return true;
	} else if (isAny(ev, TBIDC("new"))) {
		createNew(false);
		return true;
	} else if (isAny(ev, TBIDC("load"))) {
		load("");
		return true;
	} else if (isAny(ev, TBIDC("export"))) {
		exportFile("");
		return true;
	} else if (isAny(ev, TBIDC("import"))) {
		voxelize("");
		return true;
	} else if (isAny(ev, TBIDC("spacecolonization"))) {
		_scene->spaceColonization();
		return true;
	} else if (isAny(ev, TBIDC("heightmap"))) {
		importHeightmp("");
		return true;
	} else if (isAny(ev, TBIDC("save"))) {
		save("");
		return true;
	} else if (isAny(ev, TBIDC("redo"))) {
		redo();
		return true;
	} else if (isAny(ev, TBIDC("undo"))) {
		undo();
		return true;
	} else if (isAny(ev, TBIDC("rotatex"))) {
		rotatex();
		return true;
	} else if (isAny(ev, TBIDC("rotatey"))) {
		rotatey();
		return true;
	} else if (isAny(ev, TBIDC("rotatez"))) {
		rotatez();
		return true;
	} else if (isAny(ev, TBIDC("menu_structure"))) {
		if (tb::TBMenuWindow *menu = new tb::TBMenuWindow(ev.target, TBIDC("structure_popup"))) {
			menu->Show(&_structureItems, tb::TBPopupAlignment());
		}
		return true;
	} else if (isAny(ev, TBIDC("menu_tree"))) {
		if (tb::TBMenuWindow *menu = new tb::TBMenuWindow(ev.target, TBIDC("tree_popup"))) {
			menu->Show(&_treeItems, tb::TBPopupAlignment());
		}
		return true;
	} else if (isAny(ev, TBIDC("menu_file"))) {
		if (tb::TBMenuWindow *menu = new tb::TBMenuWindow(ev.target, TBIDC("menu_file_window"))) {
			menu->Show(&_fileItems, tb::TBPopupAlignment());
		}
		return true;
	} else if (isAny(ev, TBIDC("dialog_lsystem"))) {
		new LSystemWindow(this, _scene);
		return true;
	} else if (isAny(ev, TBIDC("dialog_noise"))) {
		new NoiseWindow(this, _scene);
		return true;
	} else if (isAny(ev, TBIDC("dialog_world"))) {
		const std::string& luaString = core::App::getInstance()->filesystem()->load("world.lua");
		new WorldWindow(this, _scene, luaString);
		return true;
	} else if (isAny(ev, TBIDC("optionshowgrid"))) {
		_scene->setRenderGrid(ev.target->GetValue() == 1);
		return true;
	} else if (isAny(ev, TBIDC("optionshowaxis"))) {
		_scene->setRenderAxis(ev.target->GetValue() == 1);
		return true;
	} else if (isAny(ev, TBIDC("optionshowlockaxis"))) {
		_scene->setRenderLockAxis(ev.target->GetValue() == 1);
		return true;
	} else if (isAny(ev, TBIDC("optionshowaabb"))) {
		_scene->setRenderAABB(ev.target->GetValue() == 1);
		return true;
	} else if (isAny(ev, TBIDC("optionfreelook"))) {
		togglefreelook();
		return true;
	}
	return false;
}

bool VoxEditWindow::handleClickEvent(const tb::TBWidgetEvent &ev) {
	if (ev.target->GetID() == TBIDC("unsaved_changes_new")) {
		if (ev.ref_id == TBIDC("TBMessageWindow.yes")) {
			_scene->newModel(true);
		}
		return true;
	} else if (ev.target->GetID() == TBIDC("unsaved_changes_quit")) {
		if (ev.ref_id == TBIDC("TBMessageWindow.yes")) {
			Close();
		}
		return true;
	} else if (ev.target->GetID() == TBIDC("unsaved_changes_load")) {
		if (ev.ref_id == TBIDC("TBMessageWindow.yes")) {
			_scene->loadModel(_loadFile);
			resetcamera();
		}
		return true;
	} else if (ev.target->GetID() == TBIDC("unsaved_changes_voxelize")) {
		if (ev.ref_id == TBIDC("TBMessageWindow.yes")) {
			const video::MeshPtr& mesh = _voxedit->meshPool()->getMesh(_voxelizeFile, false);
			_scene->voxelizeModel(mesh);
		}
		return true;
	}

	if (handleEvent(ev)) {
		return true;
	}

	for (uint32_t i = 0; i < SDL_arraysize(actions); ++i) {
		if (isAny(ev, actions[i].id)) {
			_scene->setAction(actions[i].action);
			return true;
		}
	}
	for (uint32_t i = 0; i < SDL_arraysize(selectionmodes); ++i) {
		if (isAny(ev, selectionmodes[i].id)) {
			_scene->setSelectionType(selectionmodes[i].type);
			setAction(Action::SelectVoxels);
			return true;
		}
	}
	for (uint32_t i = 0; i < SDL_arraysize(shapes); ++i) {
		if (isAny(ev, shapes[i].id)) {
			_scene->setCursorShape(shapes[i].shape);
			return true;
		}
	}
	for (uint32_t i = 0; i < SDL_arraysize(treeTypes); ++i) {
		if (isAny(ev, treeTypes[i].tbid)) {
			new TreeWindow(this, _scene, treeTypes[i].type);
			return true;
		}
	}
	for (uint32_t i = 0; i < SDL_arraysize(buildingTypes); ++i) {
		if (isAny(ev, buildingTypes[i].tbid)) {
			voxel::BuildingContext ctx;
			if (buildingTypes[i].type == voxel::BuildingType::Tower) {
				ctx.floors = 3;
			}
			_scene->createBuilding(buildingTypes[i].type, ctx);
			return true;
		}
	}
	for (uint32_t i = 0; i < SDL_arraysize(plantTypes); ++i) {
		if (isAny(ev, plantTypes[i].tbid)) {
			_scene->createPlant(plantTypes[i].type);
			return true;
		}
	}
	if (isAny(ev, TBIDC("clouds"))) {
		_scene->createCloud();
		return true;
	} else if (isAny(ev, TBIDC("cactus"))) {
		_scene->createCactus();
		return true;
	}

#ifdef DEBUG
	Log::debug("Unknown event %s - %s", ev.target->GetID().debug_string.CStr(), ev.ref_id.debug_string.CStr());
#endif

	return false;
}

void VoxEditWindow::setSelectionType(SelectType type) {
	for (uint32_t i = 0; i < SDL_arraysize(selectionmodes); ++i) {
		if (selectionmodes[i].type != type) {
			continue;
		}
		tb::TBWidget* widget = GetWidgetByID(selectionmodes[i].id);
		if (widget != nullptr) {
			widget->SetValue(1);
		}
		_scene->setSelectionType(type);
		setAction(Action::SelectVoxels);
		break;
	}
}

void VoxEditWindow::setAction(Action action) {
	for (uint32_t i = 0; i < SDL_arraysize(actions); ++i) {
		if (actions[i].action != action) {
			continue;
		}
		if (_scene->isEmpty() && !actions[i].availableOnEmpty) {
			continue;
		}
		tb::TBWidget* widget = GetWidgetByID(actions[i].id);
		if (widget != nullptr) {
			widget->SetValue(1);
		}
		_scene->setAction(action);
		break;
	}
}

void VoxEditWindow::crop() {
	_scene->crop();
}

void VoxEditWindow::extend(const glm::ivec3& size) {
	_scene->extend(size);
}

void VoxEditWindow::scale() {
	_scene->scale();
}

void VoxEditWindow::fill() {
	const glm::ivec3& pos = _scene->referencePosition();
	fill(pos.x, pos.y, pos.z);
}

void VoxEditWindow::fill(int x, int y, int z) {
	_scene->fill(x, y, z);
}

bool VoxEditWindow::handleChangeEvent(const tb::TBWidgetEvent &ev) {
	if (ev.target->GetID() == TBIDC("cammode")) {
		tb::TBWidget *widget = ev.target;
		tb::TBWidget *parent = widget->GetParent();
		tb::TB_TYPE_ID typeId = GetTypeId<EditorScene>();
		if (!parent->IsOfTypeId(typeId)) {
			return false;
		}
		const int value = widget->GetValue();
		video::PolygonMode mode = video::PolygonMode::Solid;
		if (value == 1) {
			mode = video::PolygonMode::Points;
		} else if (value == 2) {
			mode = video::PolygonMode::WireFrame;
		}
		((EditorScene*)parent)->camera().setPolygonMode(mode);
		return true;
	} else if (ev.target->GetID() == TBIDC("toggleviewport")) {
		tb::TBWidget *widget = ev.target;
		const int value = widget->GetValue();
		setQuadViewport(value == 1);
		return true;
	} else if (ev.target->GetID() == TBIDC("lockx")) {
		_scene->setLockedAxis(core::Axis::X, ev.target->GetValue() != 1);
		return true;
	} else if (ev.target->GetID() == TBIDC("locky")) {
		_scene->setLockedAxis(core::Axis::Y, ev.target->GetValue() != 1);
		return true;
	} else if (ev.target->GetID() == TBIDC("lockz")) {
		_scene->setLockedAxis(core::Axis::Z, ev.target->GetValue() != 1);
		return true;
	} else if (ev.target->GetID() == TBIDC("mirrorx")) {
		_scene->setMirrorAxis(core::Axis::X, _scene->referencePosition());
		return true;
	} else if (ev.target->GetID() == TBIDC("mirrory")) {
		_scene->setMirrorAxis(core::Axis::Y, _scene->referencePosition());
		return true;
	} else if (ev.target->GetID() == TBIDC("mirrorz")) {
		_scene->setMirrorAxis(core::Axis::Z, _scene->referencePosition());
		return true;
	} else if (ev.target->GetID() == TBIDC("mirrornone")) {
		_scene->setMirrorAxis(core::Axis::None, _scene->referencePosition());
		return true;
	} else if (ev.target->GetID() == TBIDC("cursorx")) {
		const tb::TBStr& str = ev.target->GetText();
		if (str.IsEmpty()) {
			return true;
		}
		const int val = core::string::toInt(str);
		glm::ivec3 pos = _scene->cursorPosition();
		pos.x = val;
		_scene->setCursorPosition(pos, true);
		return true;
	} else if (ev.target->GetID() == TBIDC("cursory")) {
		const tb::TBStr& str = ev.target->GetText();
		if (str.IsEmpty()) {
			return true;
		}
		const int val = core::string::toInt(str);
		glm::ivec3 pos = _scene->cursorPosition();
		pos.y = val;
		_scene->setCursorPosition(pos, true);
		return true;
	} else if (ev.target->GetID() == TBIDC("cursorz")) {
		const tb::TBStr& str = ev.target->GetText();
		if (str.IsEmpty()) {
			return true;
		}
		const int val = core::string::toInt(str);
		glm::ivec3 pos = _scene->cursorPosition();
		pos.z = val;
		_scene->setCursorPosition(pos, true);
		return true;
	}

	return false;
}

void VoxEditWindow::OnProcess() {
	Super::OnProcess();

	if (_lastModePress > 0l && core::App::getInstance()->timeProvider()->tickTime() - _lastModePress > 1500l) {
		executeMode();
	}

	if (_paletteWidget->isDirty()) {
		const int8_t index = (uint8_t)_paletteWidget->GetValue();
		const voxel::Voxel voxel = voxel::createVoxel(voxel::VoxelType::Generic, index);
		_scene->setVoxel(voxel);
		_paletteWidget->markAsClean();
	}
	const bool empty = _scene->isEmpty();
	if (_exportButton != nullptr) {
		_exportButton->SetState(tb::WIDGET_STATE_DISABLED, empty);
	}
	if (_saveButton != nullptr) {
		_saveButton->SetState(tb::WIDGET_STATE_DISABLED, empty);
	}
	if (_undoButton != nullptr) {
		_undoButton->SetState(tb::WIDGET_STATE_DISABLED, !_scene->canUndo());
	}
	if (_redoButton != nullptr) {
		_redoButton->SetState(tb::WIDGET_STATE_DISABLED, !_scene->canRedo());
	}
	const glm::ivec3& pos = _scene->cursorPosition();
	if (_lastCursorPos != pos) {
		_lastCursorPos = pos;
		char buf[64];
		if (_cursorX != nullptr) {
			SDL_snprintf(buf, sizeof(buf), "%i", pos.x);
			if (strcmp(_cursorX->GetText().CStr(), buf)) {
				_cursorX->SetText(buf);
			}
		}
		if (_cursorY != nullptr) {
			SDL_snprintf(buf, sizeof(buf), "%i", pos.y);
			if (strcmp(_cursorY->GetText().CStr(), buf)) {
				_cursorY->SetText(buf);
			}
		}
		if (_cursorZ != nullptr) {
			SDL_snprintf(buf, sizeof(buf), "%i", pos.z);
			if (strcmp(_cursorZ->GetText().CStr(), buf)) {
				_cursorZ->SetText(buf);
			}
		}
	}

	if (_lockedDirty) {
		_lockedDirty = false;
		const core::Axis axis = _scene->lockedAxis();
		if (_lockedX != nullptr) {
			_lockedX->SetValue((axis & core::Axis::X) != core::Axis::None);
		}
		if (_lockedY != nullptr) {
			_lockedY->SetValue((axis & core::Axis::Y) != core::Axis::None);
		}
		if (_lockedZ != nullptr) {
			_lockedZ->SetValue((axis & core::Axis::Z) != core::Axis::None);
		}
	}

	if (_mirrorDirty) {
		_mirrorDirty = false;
		const core::Axis axis = _scene->mirrorAxis();
		if (_mirrorX != nullptr) {
			_mirrorX->SetValue(axis == core::Axis::X);
		}
		if (_mirrorY != nullptr) {
			_mirrorY->SetValue(axis == core::Axis::Y);
		}
		if (_mirrorZ != nullptr) {
			_mirrorZ->SetValue(axis == core::Axis::Z);
		}
	}

	for (uint32_t i = 0; i < SDL_arraysize(actions); ++i) {
		tb::TBWidget* w = GetWidgetByID(actions[i].id);
		if (w == nullptr) {
			continue;
		}
		if (!actions[i].availableOnEmpty && empty) {
			if (w->GetState(tb::WIDGET_STATE_SELECTED)) {
				w->SetState(tb::WIDGET_STATE_SELECTED, false);
			}
			w->SetState(tb::WIDGET_STATE_DISABLED, true);
		} else {
			w->SetState(tb::WIDGET_STATE_DISABLED, false);
		}
	}
}

static inline bool isValidNumberKey(int key) {
	if ((key >= SDLK_0 && key <= SDLK_9) || (key >= SDLK_KP_0 && key <= SDLK_KP_9)) {
		return true;
	}
	if (key == SDLK_PERIOD || key == SDLK_KP_PERIOD || key == SDLK_COMMA || key == SDLK_KP_COMMA) {
		return true;
	}
	if (key == SDLK_PLUS || key == SDLK_MINUS || key == SDLK_KP_PLUS || key == SDLK_KP_MINUS) {
		return true;
	}
	return false;
}

bool VoxEditWindow::OnEvent(const tb::TBWidgetEvent &ev) {
	if (ev.type == tb::EVENT_TYPE_CUSTOM) {
		if (handleEvent(ev)) {
			return true;
		}
	} else if (ev.type == tb::EVENT_TYPE_CLICK) {
		if (handleClickEvent(ev)) {
			return true;
		}
	} else if (ev.type == tb::EVENT_TYPE_CHANGED) {
		if (handleChangeEvent(ev)) {
			return true;
		}
	} else if (ev.type == tb::EVENT_TYPE_SHORTCUT) {
		if (ev.ref_id == TBIDC("undo")) {
			undo();
		} else if (ev.ref_id == TBIDC("redo")) {
			redo();
		} else if (ev.ref_id == TBIDC("copy")) {
			copy();
		} else if (ev.ref_id == TBIDC("paste")) {
			paste();
		} else if (ev.ref_id == TBIDC("cut")) {
			cut();
		}
	} else if (ev.type == tb::EVENT_TYPE_KEY_DOWN) {
		const int key = ev.key;
		if (_axis != core::Axis::None) {
			if (isValidNumberKey(key)) {
				int l = SDL_strlen(_modeNumberBuf);
				if (l < MODENUMBERBUFSIZE - 1) {
					_modeNumberBuf[l++] = (uint8_t)key;
					_modeNumberBuf[l] = '\0';
					_lastModePress = core::App::getInstance()->timeProvider()->tickTime();
				}
			} else if (ev.special_key == tb::TB_KEY_ENTER) {
				executeMode();
			}
		} else if (_mode != ModifierMode::None) {
			if (key == SDLK_x) {
				Log::debug("Set axis to x");
				_axis |= core::Axis::X;
			} else if (key == SDLK_y) {
				_axis |= core::Axis::Y;
				Log::debug("Set axis to y");
			} else if (key == SDLK_z) {
				_axis |= core::Axis::Z;
				Log::debug("Set axis to z");
			}
			_lastModePress = core::App::getInstance()->timeProvider()->tickTime();
		}
	}

	return ui::Window::OnEvent(ev);
}

void VoxEditWindow::OnDie() {
	Super::OnDie();
	requestQuit();
}

void VoxEditWindow::copy() {
	_scene->copy();
}

void VoxEditWindow::paste() {
	_scene->paste();
}

void VoxEditWindow::cut() {
	_scene->cut();
}

void VoxEditWindow::undo() {
	_scene->undo();
}

void VoxEditWindow::redo() {
	_scene->redo();
}

void VoxEditWindow::quit() {
	if (_scene->isDirty()) {
		popup("Unsaved Modifications",
				"There are unsaved modifications.\nDo you wish to discard them and quit?",
				ui::Window::PopupType::YesNo, "unsaved_changes_quit");
		return;
	}
	Close();
}

bool VoxEditWindow::importHeightmp(const std::string& file) {
	std::string f;
	if (file.empty()) {
		f = _voxedit->openDialog("png");
		if (f.empty()) {
			return false;
		}
	} else {
		f = file;
	}

	if (!_scene->importHeightmap(f)) {
		return false;
	}
	return true;
}

bool VoxEditWindow::save(const std::string& file) {
	std::string f;
	if (file.empty()) {
		f = _voxedit->saveDialog("vox,qbt,qb");
		if (f.empty()) {
			return false;
		}
	} else {
		f = file;
	}

	if (!_scene->saveModel(f)) {
		Log::warn("Failed to save the model");
		return false;
	}
	Log::info("Saved the model to %s", f.c_str());
	return true;
}

bool VoxEditWindow::voxelize(const std::string& file) {
	std::string f;
	if (file.empty()) {
		f = _voxedit->openDialog(_importFilter);
		if (f.empty()) {
			return false;
		}
	} else {
		f = file;
	}
	if (!_scene->isDirty()) {
		const video::MeshPtr& mesh = _voxedit->meshPool()->getMesh(f, false);
		return _scene->voxelizeModel(mesh);
	}

	_voxelizeFile = f;
	popup("Unsaved Modifications",
			"There are unsaved modifications.\nDo you wish to discard them and start the voxelize process?",
			ui::Window::PopupType::YesNo, "unsaved_changes_voxelize");
	return false;
}

bool VoxEditWindow::exportFile(const std::string& file) {
	if (_scene->isEmpty()) {
		return false;
	}

	std::string f;
	if (file.empty()) {
		if (_exportFilter.empty()) {
			return false;
		}
		f = _voxedit->saveDialog(_exportFilter);
		if (f.empty()) {
			return false;
		}
	} else {
		f = file;
	}
	return _scene->exportModel(f);
}

void VoxEditWindow::resetcamera() {
	_scene->resetCamera();
	if (_sceneTop != nullptr) {
		_sceneTop->resetCamera();
	}
	if (_sceneLeft != nullptr) {
		_sceneLeft->resetCamera();
	}
	if (_sceneFront != nullptr) {
		_sceneFront->resetCamera();
	}
}

bool VoxEditWindow::load(const std::string& file) {
	std::string f;
	if (file.empty()) {
		f = _voxedit->openDialog("vox,qbt,qb");
		if (f.empty()) {
			return false;
		}
	} else {
		f = file;
	}

	if (!_scene->isDirty()) {
		if (_scene->loadModel(f)) {
			resetcamera();
			return true;
		}
		return false;
	}

	_loadFile = f;
	popup("Unsaved Modifications",
			"There are unsaved modifications.\nDo you wish to discard them and load?",
			ui::Window::PopupType::YesNo, "unsaved_changes_load");
	return false;
}

void VoxEditWindow::selectCursor() {
	const glm::ivec3& pos = _scene->cursorPosition();
	select(pos);
}

void VoxEditWindow::select(const glm::ivec3& pos) {
	_scene->select(pos);
}

bool VoxEditWindow::createNew(bool force) {
	if (!force && _scene->isDirty()) {
		popup("Unsaved Modifications",
				"There are unsaved modifications.\nDo you wish to discard them and close?",
				ui::Window::PopupType::YesNo, "unsaved_changes_new");
	} else if (_scene->newModel(force)) {
		return true;
	}
	return false;
}

}
