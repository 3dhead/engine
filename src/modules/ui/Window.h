/**
 * @file
 */

#pragma once

#include "TurboBadger.h"
#include "io/IEventObserver.h"
#include <cstdint>
#include <memory>

namespace core {
class Var;
typedef std::shared_ptr<Var> VarPtr;
}

namespace ui {

class UIApp;

#define FIELD(name, type, structtarget, structmember) name, type, offsetof(structtarget, structmember)
#define INT_FIELD(name, structtarget, structmember) FIELD(name, ui::Window::T_INT, structtarget, structmember)
#define FLOAT_FIELD(name, structtarget, structmember) FIELD(name, ui::Window::T_FLOAT, structtarget, structmember)
#define IVEC2_FIELD(name, structtarget, structmember) FIELD(name, ui::Window::T_IVEC2, structtarget, structmember)
#define VEC2_FIELD(name, structtarget, structmember) FIELD(name, ui::Window::T_VEC2, structtarget, structmember)
#define tr(id) ui::Window::getTranslation(tb::TBID(id))

class Window: public tb::TBWindow, public io::IEventObserver {
protected:
	using Super = tb::TBWindow;
	UIApp* _app;
	float _percentWidth = 0.0f;
	float _percentHeight = 0.0f;

public:
	static inline const char *getTranslation(const tb::TBID& id) {
		return tb::g_tb_lng->GetString(id);
	}

	enum FieldType {
		T_INT,
		T_FLOAT,
		T_IVEC2,
		T_VEC2
	};

	struct Field {
		const char *name;
		FieldType type;
		size_t offset;
	};

	void fillFields(const Field* fields, int fieldAmount, void* basePtr);
	void fillWidgets(const Field* fields, int fieldAmount, void* basePtr);

	Window(UIApp* app);
	Window(Window* parent);
	virtual ~Window();

	enum class PopupType {
		Ok = tb::TB_MSG_OK,
		OkCancel = tb::TB_MSG_OK_CANCEL,
		YesNo = tb::TB_MSG_YES_NO
	};
	void popup(const std::string& title, const std::string& str, PopupType type = PopupType::Ok, const char *id = "");

	Window* getParent() const;
	UIApp* getApp() const;

	std::string getStr(const char* nodeId);
	float getFloat(const char *nodeId);
	int getInt(const char *nodeId);
	bool isToggled(const char *checkBoxNodeId);
	void setText(const char *nodeId, const std::string& text);
	void toggleViaVar(const char *checkBoxNodeId, const core::VarPtr& var);
	void toggle(const char *checkBoxNodeId, bool state);

	bool loadResourceFile(const char *filename);
	bool loadResourceData(const char *data);
	bool loadResource(tb::TBNode &node);

	template<class T>
	T* getWidgetByType(const char *name);

	tb::TBWidget* getWidget(const char *name);
	tb::TBWidget* getWidgetAt(int x, int y, bool includeChildren = true);

	virtual void OnDie() override;
	virtual bool OnEvent(const tb::TBWidgetEvent &ev) override;

	virtual void onWindowResize() override;
};

template<class T>
inline T* Window::getWidgetByType(const char *name) {
	return GetWidgetByIDAndType<T>(tb::TBID(name));
}

}
