/**
 * @file
 */

#pragma once

#include "TestAppMain.h"
#include "ui/imgui/IMGUIApp.h"
#include "ui/imgui/IMGUI.h"
#include "video/Mesh.h"
#include "video/Camera.h"
#include "frontend/Axis.h"
#include "frontend/Plane.h"
#include "frontend/Movement.h"

class TestApp: public ui::imgui::IMGUIApp {
private:
	using Super = ui::imgui::IMGUIApp;
protected:
	bool _cameraMotion = false;
	bool _renderPlane = false;
	bool _renderAxis = true;
	video::Camera _camera;
	frontend::Axis _axis;
	frontend::Plane _plane;
	frontend::Movement _movement;
	core::VarPtr _rotationSpeed;
	float _cameraSpeed = 0.1f;

	virtual void doRender() = 0;

	inline void setCameraSpeed(float cameraSpeed) {
		_cameraSpeed = cameraSpeed;
	}

	inline void setCameraMotion(bool cameraMotion) {
		_cameraMotion = cameraMotion;
		SDL_SetRelativeMouseMode(_cameraMotion ? SDL_TRUE : SDL_FALSE);
	}

	inline void setRenderPlane(bool renderPlane) {
		_renderPlane = renderPlane;
	}

	inline void setRenderAxis(bool renderAxis) {
		_renderAxis = renderAxis;
	}

public:
	TestApp(const metric::MetricPtr& metric, const io::FilesystemPtr& filesystem, const core::EventBusPtr& eventBus, const core::TimeProviderPtr& timeProvider);
	virtual ~TestApp();

	video::Camera& camera();

	virtual core::AppState onConstruct() override;
	virtual core::AppState onInit() override;
	virtual void beforeUI() override;
	virtual void onRenderUI() override;
	virtual core::AppState onCleanup() override;
	virtual bool onKeyPress(int32_t key, int16_t modifier) override;
	virtual void onMouseWheel(int32_t x, int32_t y) override;
	virtual void onWindowResize() override;
};

inline video::Camera& TestApp::camera() {
	return _camera;
}
