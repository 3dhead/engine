/**
 * @file
 */

#include "UIApp.h"
#include "ui/TurboBadger.h"
#include "ui/FontUtil.h"

#include "io/File.h"
#include "core/Command.h"
#include "core/Color.h"
#include "core/UTF8.h"
#include "core/Common.h"
#include "ui_renderer_gl.h"
#include <stdarg.h>

namespace ui {

namespace {
static inline tb::MODIFIER_KEYS mapModifier(int32_t key, int16_t modifier) {
	tb::MODIFIER_KEYS code = tb::TB_MODIFIER_NONE;
	switch (key) {
	case SDLK_LCTRL:
	case SDLK_RCTRL:
		code |= tb::TB_CTRL;
		break;
	case SDLK_LSHIFT:
	case SDLK_RSHIFT:
		code |= tb::TB_SHIFT;
		break;
	case SDLK_LALT:
	case SDLK_RALT:
		code |= tb::TB_ALT;
		break;
	case SDLK_LGUI:
	case SDLK_RGUI:
		code |= tb::TB_SUPER;
		break;
	case SDLK_MODE:
		break;
	}

	if (modifier & KMOD_ALT)
		code |= tb::TB_ALT;
	if (modifier & KMOD_CTRL)
		code |= tb::TB_CTRL;
	if (modifier & KMOD_SHIFT)
		code |= tb::TB_SHIFT;
	if (modifier & KMOD_GUI)
		code |= tb::TB_SUPER;
	return code;
}

static tb::SPECIAL_KEY mapSpecialKey(int32_t key) {
	switch (key) {
	case SDLK_F1:
		return tb::TB_KEY_F1;
	case SDLK_F2:
		return tb::TB_KEY_F2;
	case SDLK_F3:
		return tb::TB_KEY_F3;
	case SDLK_F4:
		return tb::TB_KEY_F4;
	case SDLK_F5:
		return tb::TB_KEY_F5;
	case SDLK_F6:
		return tb::TB_KEY_F6;
	case SDLK_F7:
		return tb::TB_KEY_F7;
	case SDLK_F8:
		return tb::TB_KEY_F8;
	case SDLK_F9:
		return tb::TB_KEY_F9;
	case SDLK_F10:
		return tb::TB_KEY_F10;
	case SDLK_F11:
		return tb::TB_KEY_F11;
	case SDLK_F12:
		return tb::TB_KEY_F12;
	case SDLK_LEFT:
		return tb::TB_KEY_LEFT;
	case SDLK_UP:
		return tb::TB_KEY_UP;
	case SDLK_RIGHT:
		return tb::TB_KEY_RIGHT;
	case SDLK_DOWN:
		return tb::TB_KEY_DOWN;
	case SDLK_PAGEUP:
		return tb::TB_KEY_PAGE_UP;
	case SDLK_PAGEDOWN:
		return tb::TB_KEY_PAGE_DOWN;
	case SDLK_HOME:
		return tb::TB_KEY_HOME;
	case SDLK_END:
		return tb::TB_KEY_END;
	case SDLK_INSERT:
		return tb::TB_KEY_INSERT;
	case SDLK_TAB:
		return tb::TB_KEY_TAB;
	case SDLK_DELETE:
		return tb::TB_KEY_DELETE;
	case SDLK_BACKSPACE:
		return tb::TB_KEY_BACKSPACE;
	case SDLK_RETURN:
	case SDLK_KP_ENTER:
		return tb::TB_KEY_ENTER;
	case SDLK_ESCAPE:
		return tb::TB_KEY_ESC;
	}
	return tb::TB_KEY_UNDEFINED;
}

static inline int mapKey(int32_t key) {
	switch (key) {
	case SDLK_LCTRL:
	case SDLK_LSHIFT:
	case SDLK_LALT:
	case SDLK_LGUI:
	case SDLK_RCTRL:
	case SDLK_RSHIFT:
	case SDLK_RALT:
	case SDLK_RGUI:
	case SDLK_MODE:
		break;
	default:
		if (mapSpecialKey(key) == tb::TB_KEY_UNDEFINED) {
			return key;
		}
	}
	return 0;
}

}

tb::UIRendererGL _renderer;

UIApp::UIApp(const io::FilesystemPtr& filesystem, const core::EventBusPtr& eventBus, const core::TimeProviderPtr& timeProvider, uint16_t traceport) :
		Super(filesystem, eventBus, timeProvider, traceport), _quit(false) {
}

UIApp::~UIApp() {
}

bool UIApp::invokeKey(int key, tb::SPECIAL_KEY special, tb::MODIFIER_KEYS mod, bool down) {
#ifdef MACOSX
	bool shortcutKey = (mod & tb::TB_SUPER) ? true : false;
#else
	bool shortcutKey = (mod & tb::TB_CTRL) ? true : false;
#endif
	if (tb::TBWidget::focused_widget && down && shortcutKey && key != 0) {
		bool reverseKey = (mod & tb::TB_SHIFT) ? true : false;
		if (key >= 'a' && key <= 'z') {
			key += 'A' - 'a';
		}
		tb::TBID id;
		if (key == 'X') {
			id = TBIDC("cut");
		} else if (key == 'C' || special == tb::TB_KEY_INSERT) {
			id = TBIDC("copy");
		} else if (key == 'V' || (special == tb::TB_KEY_INSERT && reverseKey)) {
			id = TBIDC("paste");
		} else if (key == 'A') {
			id = TBIDC("selectall");
		} else if (key == 'Z' || key == 'Y') {
			bool undo = key == 'Z';
			if (reverseKey) {
				undo = !undo;
			}
			id = undo ? TBIDC("undo") : TBIDC("redo");
		} else if (key == 'N') {
			id = TBIDC("new");
		} else if (key == 'O') {
			id = TBIDC("open");
		} else if (key == 'S') {
			id = TBIDC("save");
		} else if (key == 'W') {
			id = TBIDC("close");
		} else if (special == tb::TB_KEY_PAGE_UP) {
			id = TBIDC("prev_doc");
		} else if (special == tb::TB_KEY_PAGE_DOWN) {
			id = TBIDC("next_doc");
		} else {
			return false;
		}

		tb::TBWidgetEvent ev(tb::EVENT_TYPE_SHORTCUT);
		ev.modifierkeys = mod;
		ev.ref_id = id;
		return tb::TBWidget::focused_widget->InvokeEvent(ev);
	}

	if (special == tb::TB_KEY_UNDEFINED && SDL_IsTextInputActive()) {
		return true;
	}

	if (_root.GetVisibility() != tb::WIDGET_VISIBILITY_VISIBLE) {
		return false;
	}
	return _root.InvokeKey(key, special, mod, down);
}

void UIApp::showStr(int x, int y, const glm::vec4& color, const char *fmt, ...) {
	static char buf[1024];
	va_list ap;
	va_start(ap, fmt);
	SDL_vsnprintf(buf, sizeof(buf), fmt, ap);
	buf[sizeof(buf) - 1] = '\0';
	_root.GetFont()->DrawString(x, y, tb::TBColor(color.r * 255.0f, color.g * 255.0f, color.b * 255.0f, color.a * 255.0f), buf);
	va_end(ap);
}

void UIApp::enqueueShowStr(int x, const glm::vec4& color, const char *fmt, ...) {
	static char buf[1024];
	va_list ap;
	va_start(ap, fmt);
	SDL_vsnprintf(buf, sizeof(buf), fmt, ap);
	buf[sizeof(buf) - 1] = '\0';
	tb::TBFontFace* font = _root.GetFont();
	font->DrawString(x, _lastShowTextY, tb::TBColor(color.r * 255.0f, color.g * 255.0f, color.b * 255.0f, color.a * 255.0f), buf);
	_lastShowTextY += _root.GetFont()->GetHeight() + 5;
	va_end(ap);
}

void UIApp::onMouseWheel(int32_t x, int32_t y) {
	if (_console.onMouseWheel(x, y)) {
		return;
	}
	int posX, posY;
	SDL_GetMouseState(&posX, &posY);
	_root.InvokeWheel(posX, posY, x, -y, getModifierKeys());
}

void UIApp::onMouseMotion(int32_t x, int32_t y, int32_t relX, int32_t relY) {
	if (_console.isActive()) {
		return;
	}
	_root.InvokePointerMove(x, y, getModifierKeys(), false);
}

void UIApp::onMouseButtonPress(int32_t x, int32_t y, uint8_t button, uint8_t clicks) {
	if (_console.onMouseButtonPress(x, y, button)) {
		return;
	}
	if (button != SDL_BUTTON_LEFT) {
		return;
	}

	const tb::MODIFIER_KEYS modKeys = getModifierKeys();
	_root.InvokePointerDown(x, y, clicks, modKeys, false);
}

tb::MODIFIER_KEYS UIApp::getModifierKeys() const {
	return mapModifier(0, SDL_GetModState());
}

void UIApp::onMouseButtonRelease(int32_t x, int32_t y, uint8_t button) {
	if (_console.isActive()) {
		return;
	}
	const tb::MODIFIER_KEYS modKeys = getModifierKeys();
	if (button == SDL_BUTTON_RIGHT) {
		_root.InvokePointerMove(x, y, modKeys, false);
		tb::TBWidget* hover = tb::TBWidget::hovered_widget;
		if (hover != nullptr) {
			hover->ConvertFromRoot(x, y);
			tb::TBWidgetEvent ev(tb::EVENT_TYPE_CONTEXT_MENU, x, y, false, modKeys);
			hover->InvokeEvent(ev);
		} else {
			_root.InvokePointerUp(x, y, modKeys, false);
		}
	} else {
		_root.InvokePointerUp(x, y, modKeys, false);
	}
}

bool UIApp::onTextInput(const std::string& text) {
	if (_console.onTextInput(text)) {
		return true;
	}
	const char *c = text.c_str();
	for (;;) {
		const int key = core::utf8::next(&c);
		if (key == -1) {
			return true;
		}
		_root.InvokeKey(key, tb::TB_KEY_UNDEFINED, tb::TB_MODIFIER_NONE, true);
		_root.InvokeKey(key, tb::TB_KEY_UNDEFINED, tb::TB_MODIFIER_NONE, false);
	}
	return true;
}

bool UIApp::onKeyPress(int32_t key, int16_t modifier) {
	if (_console.onKeyPress(key, modifier)) {
		return true;
	}

	if (Super::onKeyPress(key, modifier)) {
		return true;
	}

	return invokeKey(mapKey(key), mapSpecialKey(key), mapModifier(key, modifier), true);
}

bool UIApp::onKeyRelease(int32_t key) {
	if (_console.isActive()) {
		return true;
	}
	Super::onKeyRelease(key);
	tb::MODIFIER_KEYS mod = getModifierKeys();
	mod |= mapModifier(key, 0);
	if (key == SDLK_MENU && tb::TBWidget::focused_widget) {
		tb::TBWidgetEvent ev(tb::EVENT_TYPE_CONTEXT_MENU);
		ev.modifierkeys = mod;
		tb::TBWidget::focused_widget->InvokeEvent(ev);
		return true;
	}
	return invokeKey(mapKey(key), mapSpecialKey(key), mod, false);
}

void UIApp::onWindowResize() {
	Super::onWindowResize();
	// TODO: event for ui
}

core::AppState UIApp::onConstruct() {
	const core::AppState state = Super::onConstruct();
	core::Command::registerCommand("cl_ui_debug", [&] (const core::CmdArgs& args) {
#ifdef DEBUG
		tb::ShowDebugInfoSettingsWindow(&_root);
#endif
	}).setHelp("Show ui debug information - only available in debug builds");

	core::Command::registerCommand("quit", [&] (const core::CmdArgs& args) {_quit = true;}).setHelp("Quit the application");

	core::Command::registerCommand("bindlist", [this] (const core::CmdArgs& args) {
		for (util::BindMap::const_iterator i = _bindings.begin(); i != _bindings.end(); ++i) {
			const int32_t key = i->first;
			const util::CommandModifierPair& pair = i->second;
			const char* keyName = SDL_GetKeyName(key);
			const int16_t modifier = pair.second;
			std::string modifierKey;
			if (modifier & KMOD_ALT) {
				modifierKey += "ALT ";
			}
			if (modifier & KMOD_SHIFT) {
				modifierKey += "SHIFT ";
			}
			if (modifier & KMOD_CTRL) {
				modifierKey += "CTRL ";
			}
			const std::string& command = pair.first;
			Log::info("%-15s %-10s %s", modifierKey.c_str(), keyName, command.c_str());
		}
	}).setHelp("Show all known key bindings");

	core::Command::registerCommand("bind", [this] (const core::CmdArgs& args) {
		if (args.size() != 2) {
			Log::error("Expected parameters: key+modifier command - got %i parameters", (int)args.size());
			return;
		}

		util::KeybindingParser p(args[0], args[1]);
		const util::BindMap& bindings = p.getBindings();
		for (util::BindMap::const_iterator i = bindings.begin(); i != bindings.end(); ++i) {
			const uint32_t key = i->first;
			const util::CommandModifierPair& pair = i->second;
			auto range = _bindings.equal_range(key);
			bool found = false;
			for (auto it = range.first; it != range.second; ++it) {
				if (it->second.second == pair.second) {
					it->second.first = pair.first;
					found = true;
					Log::info("Updated binding for key %s", args[0].c_str());
					break;
				}
			}
			if (!found) {
				_bindings.insert(std::make_pair(key, pair));
				Log::info("Added binding for key %s", args[0].c_str());
			}
		}
	}).setHelp("Bind a command to a key");

	return state;
}

void UIApp::OnWidgetFocusChanged(tb::TBWidget *widget, bool focused) {
	if (focused && widget->IsOfType<tb::TBEditField>()) {
		SDL_StartTextInput();
	} else {
		SDL_StopTextInput();
	}
}

void UIApp::afterRootWidget() {
	const tb::TBRect rect(0, 0, _dimension.x, _dimension.y);
	_console.render(rect, _deltaFrame);
}

core::AppState UIApp::onInit() {
	const core::AppState state = Super::onInit();
	if (!tb::tb_core_init(&_renderer)) {
		Log::error("failed to initialize the ui");
		return core::AppState::Cleanup;
	}

	tb::TBWidgetListener::AddGlobalListener(this);

	if (!tb::g_tb_lng->Load("ui/lang/en.tb.txt")) {
		Log::warn("could not load the translation");
	}

	if (_applicationSkin.empty()) {
		const std::string skin = "ui/skin/" + _appname + "-skin.tb.txt";
		if (filesystem()->exists(skin)) {
			_applicationSkin = skin;
		}
	}
	if (!tb::g_tb_skin->Load("ui/skin/skin.tb.txt", _applicationSkin.empty() ? nullptr : _applicationSkin.c_str())) {
		Log::error("could not load the skin");
		return core::AppState::Cleanup;
	}

	if (!_renderer.init(dimension())) {
		Log::error("could not init ui renderer");
		return core::AppState::Cleanup;
	}

	tb::TBWidgetsAnimationManager::Init();

	initFonts();
	tb::TBFontFace *font = getFont(14, true);
	if (font == nullptr) {
		Log::error("could not create the font face");
		return core::AppState::Cleanup;
	}

	_root.SetRect(tb::TBRect(0, 0, _dimension.x, _dimension.y));
	_root.SetSkinBg(TBIDC("background"));
	_root.SetGravity(tb::WIDGET_GRAVITY_ALL);

	_console.init();

	_renderUI = core::Var::get(cfg::ClientRenderUI, "true");

	return state;
}

void UIApp::addChild(Window* window) {
	_root.AddChild(window);
}

tb::TBWidget* UIApp::getWidget(const char *name) {
	return _root.GetWidgetByID(tb::TBID(name));
}

tb::TBWidget* UIApp::getWidgetAt(int x, int y, bool includeChildren) {
	return _root.GetWidgetAt(x, y, includeChildren);
}

void UIApp::doLayout() {
	_root.InvalidateLayout(tb::TBWidget::INVALIDATE_LAYOUT_RECURSIVE);
}

core::AppState UIApp::onRunning() {
	if (_quit) {
		return core::AppState::Cleanup;
	}
	core::AppState state = Super::onRunning();

	_lastShowTextY = 5;

	const bool running = state == core::AppState::Running;
	if (running) {
		{
			core_trace_scoped(UIAppBeforeUI);
			beforeUI();
		}

		++_frameCounter;

		double time = _now;
		if (time > _frameCounterResetRime + 1000) {
			_fps = (int) round((_frameCounter / (time - _frameCounterResetRime)) * 1000);
			_frameCounterResetRime = time;
			_frameCounter = 0;
		}

		const bool renderUI = _renderUI->boolVal();
		if (renderUI) {
			core_trace_scoped(UIAppUpdateUI);
			tb::TBAnimationManager::Update();
			_root.InvokeProcessStates();
			_root.InvokeProcess();

			_renderer.BeginPaint(_dimension.x, _dimension.y);
			_root.InvokePaint(tb::TBWidget::PaintProps());

			enqueueShowStr(5, core::Color::White, "FPS: %d", _fps);
		}
		{
			core_trace_scoped(UIAppAfterUI);
			afterRootWidget();
		}
		if (renderUI) {
			core_trace_scoped(UIAppEndPaint);
			_renderer.EndPaint();
			// If animations are running, reinvalidate immediately
			if (tb::TBAnimationManager::HasAnimationsRunning())
				_root.Invalidate();
		}
	}
	return state;
}

core::AppState UIApp::onCleanup() {
	tb::TBAnimationManager::AbortAllAnimations();
	tb::TBWidgetListener::RemoveGlobalListener(this);

	tb::TBWidgetsAnimationManager::Shutdown();
	tb::tb_core_shutdown();

	_root.DeleteAllChildren();

	_console.shutdown();
	_renderer.shutdown();

	return Super::onCleanup();
}

}
