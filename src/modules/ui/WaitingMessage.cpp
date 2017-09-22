#include "WaitingMessage.h"
#include "FontUtil.h"
#include "core/Array.h"
#include "UIApp.h"

namespace ui {

WaitingMessage::WaitingMessage(UIApp* app) :
		_app(app) {
}

WaitingMessage::~WaitingMessage() {
	shutdown();
}

void WaitingMessage::setColor(const glm::vec4& color) {
	_color = tb::TBColor(color.r, color.g, color.b, color.a);
}

void WaitingMessage::init(int fontSize) {
	_font = getFont(fontSize);
}

void WaitingMessage::shutdown() {
	_font = nullptr;
}

void WaitingMessage::setTextId(const char *textId) {
	_translatedStr = tr(textId);
}

void WaitingMessage::reset() {
	_connectingStart = 0;
	_dotsIndex = 0;
}

void WaitingMessage::update(int deltaFrame) {
	_connectingStart += deltaFrame;
}

void WaitingMessage::render() {
	static const char *dotsArray[] = { ".", "..", "...", "....", "....." };
	if (_translatedStr == nullptr) {
		return;
	}
	if (_font == nullptr) {
		return;
	}
	if (_connectingStart >= 2000) {
		_dotsIndex = (_dotsIndex + 1) % lengthof(dotsArray);
		_connectingStart -= 2000;
	}
	const int y = _app->height() / 2 - _font->GetHeight() / 2;
	const int w = _font->GetStringWidth(_translatedStr);
	const int x = _app->width() / 2 - w / 2;
	_font->DrawString(x, y, _color, _translatedStr);

	const int dotX = x + w + 5;
	const char *dotsString = dotsArray[_dotsIndex];
	_font->DrawString(dotX, y, _color, dotsString);
}

}
