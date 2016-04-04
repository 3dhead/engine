#pragma once

#include "core/App.h"
#include "io/IEventObserver.h"
#include "io/EventHandler.h"
#include <glm/glm.hpp>

struct SDL_Window;
typedef void *SDL_GLContext;

namespace video {

class WindowedApp: public core::App, public io::IEventObserver {
protected:
	SDL_Window* _window;
	SDL_GLContext _glcontext;
	int _width;
	int _height;
	float _aspect;
	glm::vec4 _clearColor;

	WindowedApp(const io::FilesystemPtr& filesystem, const core::EventBusPtr& eventBus);

	virtual ~WindowedApp() {
	}
public:
	virtual core::AppState onRunning() override;
	virtual void onAfterRunning() override;
	virtual core::AppState onConstruct() override;
	virtual core::AppState onInit() override;
	virtual core::AppState onCleanup() override;
};

}
