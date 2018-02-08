/**
 * @file
 */

#pragma once

#include "testcore/TestApp.h"
#include "frontend/CameraFrustum.h"
#include "FrustumEntity.h"
#include <array>

/**
 * @brief Renders the view frustum of a camera
 */
class TestCamera: public TestApp {
private:
	using Super = TestApp;

	static constexpr int CAMERAS = 3;
	frontend::CameraFrustum _frustums[CAMERAS];
	// the cameras to render the frustums for
	video::Camera _renderCamera[CAMERAS];

	std::array<FrustumEntity, 25> _entities;

	int _targetCamera = 0;

	void doRender() override;
	void resetCameraPosition();
public:
	TestCamera(const metric::MetricPtr& metric, const io::FilesystemPtr& filesystem, const core::EventBusPtr& eventBus, const core::TimeProviderPtr& timeProvider);

	core::AppState onInit() override;
	core::AppState onRunning() override;
	core::AppState onCleanup() override;

	void onRenderUI() override;

	void onMouseWheel(int32_t x, int32_t y) override;
	bool onKeyPress(int32_t key, int16_t modifier) override;
};
