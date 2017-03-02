/**
 * @file
 */

#include "NoiseToolWindow.h"
#include "../NoiseTool.h"
#include "noisedata/NoiseDataItemWidget.h"
#include "noise/Noise.h"
#include "noise/Simplex.h"
#include "ui/UIApp.h"
#include "core/Color.h"
#include "NoiseDataNodeWindow.h"

#define IMAGE_PREFIX "2d"
#define GRAPH_PREFIX "graph"

NoiseToolWindow::NoiseToolWindow(NoiseTool* tool) :
		ui::Window(tool), _noiseTool(tool) {
	for (int i = 0; i < (int)NoiseType::Max; ++i) {
		addMenuItem(_noiseTypeSource, getNoiseTypeName((NoiseType)i));
	}
}

NoiseToolWindow::~NoiseToolWindow() {
	if (_noiseType != nullptr) {
		_noiseType->SetSource(nullptr);
	}
	delete[] _graphBufferBackground;
	_graphBufferBackground = nullptr;
}

bool NoiseToolWindow::init() {
	if (!loadResourceFile("ui/window/noisetool-main.tb.txt")) {
		Log::error("Failed to init the main window: Could not load the ui definition");
		return false;
	}
	_noiseType = getWidgetByType<tb::TBSelectDropdown>("type");
	if (_noiseType == nullptr) {
		Log::error("Failed to init the main window: No type widget found");
		return false;
	}
	_noiseType->SetSource(&_noiseTypeSource);

	_select = getWidgetByType<tb::TBSelectList>("list");
	if (_select == nullptr) {
		return false;
	}
	_select->SetSource(_noiseTool->noiseItemSource());
	_select->GetScrollContainer()->SetScrollMode(tb::SCROLL_MODE_X_AUTO_Y_AUTO);

	const tb::TBRect& rect = _select->GetPaddingRect();
	_noiseHeight = rect.h;
	_noiseWidth = rect.w - 60;
	const size_t graphBufferSize = _noiseWidth * _graphHeight * BPP;

	_graphBufferBackground = new uint8_t[graphBufferSize];
	const int graphBufOffset = index(0, int(_graphHeight / 2));
	memset(&_graphBufferBackground[graphBufOffset], core::Color::getRGBA(core::Color::Gray), _noiseWidth * BPP);

	for (int i = 0; i < _graphHeight; ++i) {
		uint8_t* gbuf = &_graphBufferBackground[index(10, i)];
		*((uint32_t*)gbuf) = core::Color::getRGBA(core::Color::Gray);
	}

	return true;
}

float NoiseToolWindow::getNoise(int x, int y, NoiseData data) {
	const NoiseType noiseType = data.noiseType;
	const glm::vec2 position(data.offset + x * data.frequency, data.offset + y * data.frequency);
	switch (noiseType) {
	case NoiseType::doubleNoise:
		return noise::doubleValueNoise(glm::ivec3(position, 0), 0);
	case NoiseType::simplexNoise:
		return noise::noise(position);
	case NoiseType::ridgedNoise:
		return noise::ridgedNoise(position);
	case NoiseType::flowNoise:
		return noise::flowNoise(position, data.millis);
	case NoiseType::fbm:
		return noise::fBm(position, data.octaves, data.lacunarity, data.gain);
	case NoiseType::fbmCascade:
		return noise::fBm(noise::fBm(position * 3.0f));
	case NoiseType::fbmAnalyticalDerivatives:
		return noise::fBm(noise::dfBm(position));
	case NoiseType::flowNoiseFbm:
		return noise::flowNoise(position + noise::fBm(glm::vec3(position, data.millis * 0.1f)), data.millis);
	case NoiseType::ridgedMFTime: {
		const glm::vec3 p3(position, data.millis * 0.1f);
		return noise::ridgedMF(p3, 1.0f, data.octaves, data.lacunarity, data.gain);
	}
	case NoiseType::ridgedMF:
		return noise::ridgedMF(position, 1.0f, data.octaves, data.lacunarity, data.gain);
	case NoiseType::ridgedMFCascade:
		return noise::ridgedMF(noise::ridgedMF(position));
	case NoiseType::iqNoise:
		return noise::iqMatfBm(position, data.octaves, glm::mat2(2.3f, -1.5f, 1.5f, 2.3f), data.gain);
	case NoiseType::iqNoiseScaled:
		return noise::iqMatfBm(position * data.frequency, data.octaves, glm::mat2(-12.5f, -0.5f, 0.5f, -12.5f), data.gain);
	case NoiseType::analyticalDerivatives: {
		const glm::vec3& n = noise::dnoise(position * 5.0f);
		return (n.y + n.z) * 0.5f;
	}
	case NoiseType::noiseCurlNoise: {
		const glm::vec2& n = noise::curlNoise(position, data.millis);
		return noise::noise(glm::vec2(position.x + n.x, position.y + n.x));
	}
	case NoiseType::voronoi:
		return noise::voronoi(glm::dvec3(position, 0.0), true, 0.0, 1.0, 0);
	case NoiseType::worleyNoise:
		return noise::worleyNoise(position);
	case NoiseType::worleyNoiseFbm:
		return noise::worleyfBm(position, data.octaves, data.lacunarity, data.gain);
	case NoiseType::swissTurbulence:
		return noise::swissTurbulence(position, 0, data.octaves, data.lacunarity, data.gain);
	case NoiseType::jordanTurbulence:
		return noise::jordanTurbulence(position, 0, data.octaves);
	case NoiseType::Max:
		break;
	}
	return 0.0f;
}

bool NoiseToolWindow::OnEvent(const tb::TBWidgetEvent &ev) {
	const tb::TBID& id = ev.target->GetID();
	if (ev.type == tb::EVENT_TYPE_CLICK) {
		if (id == TBIDC("ok")) {
			generateImage();
			return true;
		} else if (id == TBIDC("all")) {
			generateAll();
			return true;
		} else if (id == TBIDC("quit")) {
			Close();
			return true;
		} else if (id == TBIDC("nodes")) {
			NoiseDataNodeWindow* window = new NoiseDataNodeWindow(_noiseTool);
			window->init();
			return true;
		}
	}

	if (ev.type == tb::EVENT_TYPE_CHANGED && id == TBIDC("filter") && _select != nullptr) {
		_select->SetFilter(ev.target->GetText());
		return true;
	}
	return Super::OnEvent(ev);
}

void NoiseToolWindow::generateImage() {
	const int type = getSelectedId("type");
	if (type < 0 || type >= (int)NoiseType::Max) {
		return;
	}

	generateImage((NoiseType)type);
}

void NoiseToolWindow::generateAll() {
	for (int i = 0; i < (int)NoiseType::Max; ++i) {
		NoiseType type = (NoiseType)i;
		generateImage(type);
	}
}

void NoiseToolWindow::generateImage(NoiseType type) {
	Log::info("Generate noise for %s", getNoiseTypeName(type));
	NoiseData data;
	data.offset = getFloat("offset");
	data.lacunarity = getFloat("lacunarity");
	data.octaves = getInt("octaves");
	data.gain = getFloat("gain");
	data.frequency = getFloat("frequency");
	data.noiseType = type;

	_noiseTool->threadPool().enqueue([this, type, data] () {
		const size_t noiseBufferSize = _noiseWidth * _noiseHeight * BPP;
		const size_t graphBufferSize = _noiseWidth * _graphHeight * BPP;
		QueueData qd;
		qd.data = data;;
		qd.data.millis = _noiseTool->timeProvider()->currentTime();
		qd.noiseBuffer = new uint8_t[noiseBufferSize];
		qd.graphBuffer = new uint8_t[graphBufferSize];

		uint8_t* noiseBuffer = qd.noiseBuffer;
		uint8_t* graphBuffer = qd.graphBuffer;

		memset(noiseBuffer, 255, noiseBufferSize);
		memcpy(graphBuffer, _graphBufferBackground, graphBufferSize);

		for (int y = 0; y < _noiseHeight; ++y) {
			for (int x = 0; x < _noiseWidth; ++x) {
				const float n = getNoise(x, y, qd.data);
				const float cn = noise::norm(n);
				const uint8_t c = cn * 255;
				uint8_t* buf = &noiseBuffer[index(x, y)];
				memset(buf, c, BPP - 1);
				if (y == 0 && x < _noiseWidth) {
					uint8_t* gbuf = &graphBuffer[index(x, ((_graphHeight - 1) - cn * _graphHeight) - 1)];
					*((uint32_t*)gbuf) = core::Color::getRGBA(core::Color::Red);
				}
			}
		}

		qd.data.endmillis = _noiseTool->timeProvider()->currentTime();
		this->_queue.push(qd);
	});
}

void NoiseToolWindow::update() {
	QueueData qd;
	if (!_queue.pop(qd)) {
		return;
	}
	NoiseData& data = qd.data;
	tb::TBStr idStr;
	idStr.SetFormatted(IMAGE_PREFIX "-%i-%f-%i-%f-%f-%f", (int)data.noiseType, data.offset, data.octaves, data.lacunarity, data.gain, data.frequency);
	tb::TBStr graphIdStr;
	graphIdStr.SetFormatted(GRAPH_PREFIX "-%i-%f-%i-%f-%f-%f", (int)data.noiseType, data.offset, data.octaves, data.lacunarity, data.gain, data.frequency);
	data.noise = tb::g_image_manager->GetImage(idStr.CStr(), (uint32_t*)qd.noiseBuffer, _noiseWidth, _noiseHeight);
	data.graph = tb::g_image_manager->GetImage(graphIdStr.CStr(), (uint32_t*)qd.graphBuffer, _noiseWidth, _graphHeight);
	_noiseTool->add(TBIDC(idStr), data);

	const int n = _select->GetSource()->GetNumItems();
	_select->SetValue(n - 1);
	Log::info("Generating noise for %s took %lums", getNoiseTypeName(data.noiseType), data.endmillis - data.millis);
	delete [] qd.noiseBuffer;
	delete [] qd.graphBuffer;
}

void NoiseToolWindow::OnDie() {
	if (_noiseType != nullptr) {
		_noiseType->SetSource(nullptr);
	}
	Super::OnDie();
	requestQuit();
}
