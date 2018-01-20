/**
 * @file
 */

#pragma once

#include "ui/turbobadger/Window.h"
#include "ui/turbobadger/ui_widgets.h"
#include "core/Common.h"
#include "collection/ConcurrentQueue.h"
#include <unordered_map>
#include "../NoiseData.h"

class NoiseTool;

class NoiseToolWindow: public ui::turbobadger::Window {
private:
	static constexpr int BPP = 4;
	static_assert(BPP == sizeof(uint32_t), "This code heavily relies on RGBA being 32bit");
	using Super = ui::turbobadger::Window;

	NoiseTool* _noiseTool;
	tb::TBSelectList* _select = nullptr;

	tb::TBSelectDropdown* _noiseType = nullptr;
	tb::TBSelectItemSourceList<tb::TBGenericStringItem> _noiseTypeSource;

	struct QueueData {
		NoiseData data;
		uint8_t *noiseBuffer;
		uint8_t *graphBuffer;

		inline bool operator<(const QueueData& rhs) const {
			return noiseBuffer < rhs.noiseBuffer;
		}
	};
	core::ConcurrentQueue<QueueData> _queue;

	int _noiseWidth = 768;
	int _noiseHeight = 1024;
	const int _graphHeight = 65;
	uint8_t *_graphBufferBackground = nullptr;

	/**
	 * @return the noise in the range [-1.0 - 1.0]
	 */
	float getNoise(int x, int y, NoiseData _data);
	int index(int x, int y) const;
	void generateImage();
	void updateForNoiseType(NoiseType type);
	void generateImage(NoiseType type);
	void generateAll();
public:
	NoiseToolWindow(NoiseTool* tool);
	~NoiseToolWindow();
	bool init();
	void update();

	bool OnEvent(const tb::TBWidgetEvent &ev) override;
	void OnDie() override;
};

inline int NoiseToolWindow::index(int x, int y) const {
	core_assert_msg(x >= 0, "x is smaller than 0: %i", x);
	core_assert_msg(x < _noiseWidth * BPP, "x is out of bounds: %i", x);
	core_assert_msg(y >= 0, "y is smaller than 0: %i", y);
	return x * BPP + y * _noiseWidth * BPP;
}
