/**
 * @file
 */

#include "NoiseTool.h"
#include "io/Filesystem.h"
#include "ui/NoiseToolWindow.h"
#include "ui/noisedata/NoiseDataItemWidget.h"

NoiseTool::NoiseTool(const metric::MetricPtr& metric, const io::FilesystemPtr& filesystem, const core::EventBusPtr& eventBus, const core::TimeProviderPtr& timeProvider) :
		Super(metric, filesystem, eventBus, timeProvider) {
	init(ORGANISATION, "noisetool");
}

void NoiseTool::add(uint32_t dataId, const NoiseData& data) {
	auto pair = _noiseData.insert(std::make_pair(dataId, data));
	if (pair.second) {
		const char* name = getNoiseTypeName(data.noiseType);
		_noiseItemSource->AddItem(new NoiseItem(name, dataId, data));
	}
}

void NoiseTool::remove(uint32_t dataId) {
	if (_noiseData.erase(dataId) <= 0) {
		return;
	}
	const int n = _noiseItemSource->GetNumItems();
	for (int i = 0; i < n; ++i) {
		if (_noiseItemSource->GetItemID(i) == dataId) {
			_noiseItemSource->DeleteItem(i);
			return;
		}
	}
}

core::AppState NoiseTool::onInit() {
	core::AppState state = Super::onInit();
	if (state != core::AppState::Running) {
		return state;
	}

	_noiseItemSource = new NoiseItemSource(this);

	_window = new NoiseToolWindow(this);
	if (!_window->init()) {
		return core::AppState::InitFailure;
	}

	return state;
}

core::AppState NoiseTool::onRunning() {
	core::AppState state = Super::onRunning();
	_window->update();
	return state;
}

int main(int argc, char *argv[]) {
	const core::EventBusPtr& eventBus = std::make_shared<core::EventBus>();
	const io::FilesystemPtr& filesystem = std::make_shared<io::Filesystem>();
	const core::TimeProviderPtr& timeProvider = std::make_shared<core::TimeProvider>();
	const metric::MetricPtr& metric = std::make_shared<metric::Metric>();
	NoiseTool app(metric, filesystem, eventBus, timeProvider);
	return app.startMainLoop(argc, argv);
}
