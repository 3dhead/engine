/**
 * @file
 */

#pragma once

#include "video/WindowedApp.h"
#include "Window.h"
#include "Console.h"
#include <tb_widgets_listener.h>

namespace ui {

class UIApp: public video::WindowedApp, private tb::TBWidgetListener {
private:
	using Super = video::WindowedApp;
protected:
	tb::TBWidget _root;
	Console _console;
	core::VarPtr _renderUI;
	int _lastShowTextY = -1;
	std::string _applicationSkin;

	virtual bool onKeyRelease(int32_t key) override;
	virtual bool onKeyPress(int32_t key, int16_t modifier) override;
	virtual bool onTextInput(const std::string& text) override;
	virtual void onMouseWheel(int32_t x, int32_t y) override;
	virtual void onMouseMotion(int32_t x, int32_t y, int32_t relX, int32_t relY) override;
	virtual void onMouseButtonPress(int32_t x, int32_t y, uint8_t button, uint8_t clicks) override;
	virtual void onMouseButtonRelease(int32_t x, int32_t y, uint8_t button) override;

	virtual void OnWidgetFocusChanged(tb::TBWidget *widget, bool focused) override;

	bool invokeKey(int key, tb::SPECIAL_KEY special, tb::MODIFIER_KEYS mod, bool down);
	void showStr(int x, int y, const glm::vec4& color, CORE_FORMAT_STRING const char *fmt, ...) __attribute__((format(printf, 5, 6)));
	void enqueueShowStr(int x, const glm::vec4& color, CORE_FORMAT_STRING const char *fmt, ...) __attribute__((format(printf, 4, 5)));

	tb::MODIFIER_KEYS getModifierKeys() const;
public:
	UIApp(const io::FilesystemPtr& filesystem, const core::EventBusPtr& eventBus, const core::TimeProviderPtr& timeProvider, uint16_t traceport = 17815);
	virtual ~UIApp();

	virtual void beforeUI() {
	}

	template<class T>
	T* getWidgetByType(const char *name);

	tb::TBWidget* getWidget(const char *name);
	tb::TBWidget* getWidgetAt(int x, int y, bool includeChildren = true);

	// hook that is called directory before the ui is rendered. Your last chance to let the app contribute
	// something in the ui context and the ui drawcalls (like debug text or rendering an in-game console on
	// top of the ui)
	virtual void afterRootWidget();

	void addChild(Window* window);

	void doLayout();

	virtual void onWindowResize() override;
	virtual core::AppState onConstruct() override;
	virtual core::AppState onInit() override;
	virtual core::AppState onRunning() override;
	virtual core::AppState onCleanup() override;
};

template<class T>
inline T* UIApp::getWidgetByType(const char *name) {
	return _root.GetWidgetByIDAndType<T>(tb::TBID(name));
}

}
