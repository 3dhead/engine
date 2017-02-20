/**
 * @file
 */

#include "Color.h"
#include "core/GLM.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace core {

const glm::vec4 Color::Clear        = glm::vec4(  0.f,   0,   0,   0) / glm::vec4(Color::magnitude);
const glm::vec4 Color::White        = glm::vec4(255.f, 255, 255, 255) / glm::vec4(Color::magnitude);
const glm::vec4 Color::Black        = glm::vec4(  0.f,   0,   0, 255) / glm::vec4(Color::magnitude);
const glm::vec4 Color::Lime         = glm::vec4(109.f, 198,   2, 255) / glm::vec4(Color::magnitude);
const glm::vec4 Color::Pink         = glm::vec4(248.f,   4,  62, 255) / glm::vec4(Color::magnitude);
const glm::vec4 Color::LightBlue    = glm::vec4(  0.f, 153, 203, 255) / glm::vec4(Color::magnitude);
const glm::vec4 Color::DarkBlue     = glm::vec4( 55.f, 116, 145, 255) / glm::vec4(Color::magnitude);
const glm::vec4 Color::Orange       = glm::vec4(252.f, 167,   0, 255) / glm::vec4(Color::magnitude);
const glm::vec4 Color::Yellow       = glm::vec4(255.f, 255,   0, 255) / glm::vec4(Color::magnitude);
const glm::vec4 Color::Sandy        = glm::vec4(237.f, 232, 160, 255) / glm::vec4(Color::magnitude);
const glm::vec4 Color::LightGray    = glm::vec4(192.f, 192, 192, 255) / glm::vec4(Color::magnitude);
const glm::vec4 Color::Gray         = glm::vec4(128.f, 128, 128, 255) / glm::vec4(Color::magnitude);
const glm::vec4 Color::DarkGray     = glm::vec4( 84.f,  84,  84, 255) / glm::vec4(Color::magnitude);
const glm::vec4 Color::LightRed     = glm::vec4(255.f,  96,  96, 255) / glm::vec4(Color::magnitude);
const glm::vec4 Color::Red          = glm::vec4(255.f,   0,   0, 255) / glm::vec4(Color::magnitude);
const glm::vec4 Color::DarkRed      = glm::vec4(128.f,   0,   0, 255) / glm::vec4(Color::magnitude);
const glm::vec4 Color::LightGreen   = glm::vec4( 96.f, 255,  96, 255) / glm::vec4(Color::magnitude);
const glm::vec4 Color::Green        = glm::vec4(  0.f, 255,   0, 255) / glm::vec4(Color::magnitude);
const glm::vec4 Color::DarkGreen    = glm::vec4(  0.f, 128,   0, 255) / glm::vec4(Color::magnitude);
const glm::vec4 Color::Blue         = glm::vec4(  0.f,   0, 255, 255) / glm::vec4(Color::magnitude);
const glm::vec4 Color::SteelBlue    = glm::vec4( 35.f, 107, 142, 255) / glm::vec4(Color::magnitude);
const glm::vec4 Color::Olive        = glm::vec4(128.f, 128,   0, 255) / glm::vec4(Color::magnitude);
const glm::vec4 Color::Purple       = glm::vec4(128.f,   0, 128, 255) / glm::vec4(Color::magnitude);
const glm::vec4 Color::Cyan         = glm::vec4(  0.f, 255, 255, 255) / glm::vec4(Color::magnitude);
const glm::vec4 Color::Brown        = glm::vec4(107.f,  66,  38, 255) / glm::vec4(Color::magnitude);
const glm::vec4 Color::LightBrown   = glm::vec4(150.f, 107,  72, 255) / glm::vec4(Color::magnitude);
const glm::vec4 Color::DarkBrown    = glm::vec4( 82.f,  43,  26, 255) / glm::vec4(Color::magnitude);

/**
 * @brief Get the nearest matching color index from the list
 * @param color The color to find the closest match to in the given @c colors array
 * @return index in the colors vector or the first entry if non was found
 */
int Color::getClosestMatch(const glm::vec4& color, const std::vector<glm::vec4>& colors) {
	const float weightHue = 0.8f;
	const float weightSaturation = 0.1f;
	const float weightValue = 0.1f;

	float minDistance = std::numeric_limits<float>::max();

	int minIndex = 0;

	float hue;
	float saturation;
	float brightness;
	core::Color::getHSB(color, hue, saturation, brightness);

	for (size_t i = 0; i < colors.size(); ++i) {
		float chue;
		float csaturation;
		float cbrightness;
		core::Color::getHSB(colors[i], chue, csaturation, cbrightness);

		const float dH = chue - hue;
		const float dS = csaturation - saturation;
		const float dV = cbrightness - brightness;
		const float val = weightHue * glm::pow(dH, 2) +
				weightValue * glm::pow(dV, 2) +
				weightSaturation * glm::pow(dS, 2);
		const float curDistance = sqrtf(val);
		if (curDistance < minDistance) {
			minDistance = curDistance;
			minIndex = i;
		}
	}
	return minIndex;
}

glm::vec4 Color::fromRGB(const unsigned int rgbInt, const float a) {
	return glm::vec4(static_cast<float>(rgbInt >> 16 & 0xFF) / Color::magnitude, static_cast<float>(rgbInt >> 8 & 0xFF) / Color::magnitude,
			static_cast<float>(rgbInt & 0xFF) / Color::magnitude, a);
}

glm::vec4 Color::fromRGBA(const unsigned int color) {
	const uint8_t a = (color >> 24) & 0xFF;
	const uint8_t b = (color >> 16) & 0xFF;
	const uint8_t g = (color >> 8) & 0xFF;
	const uint8_t r = (color >> 0) & 0xFF;
	return fromRGBA(r, g, b, a);
}

glm::vec4 Color::fromRGBA(uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
	return glm::vec4(static_cast<float>(r) / Color::magnitude, static_cast<float>(g) / Color::magnitude,
			static_cast<float>(b) / Color::magnitude, static_cast<float>(a) / Color::magnitude);
}

glm::vec4 Color::fromHSB(const float hue, const float saturation, const float brightness, const float alpha) {
	if (std::numeric_limits<float>::epsilon() > brightness) {
		return glm::vec4(0.f, 0.f, 0.f, alpha);
	}
	if (std::numeric_limits<float>::epsilon() > saturation) {
		return glm::vec4(brightness, brightness, brightness, alpha);
	}
	glm::vec4 color;
	color.a = alpha;
	float h = (hue - std::floor(hue)) * 6.f;
	float f = h - std::floor(h);
	float p = brightness * (1.f - saturation);
	float q = brightness * (1.f - saturation * f);
	float t = brightness * (1.f - (saturation * (1.f - f)));
	switch (static_cast<int>(h)) {
	case 0:
		color.r = brightness;
		color.g = t;
		color.b = p;
		break;
	case 1:
		color.r = q;
		color.g = brightness;
		color.b = p;
		break;
	case 2:
		color.r = p;
		color.g = brightness;
		color.b = t;
		break;
	case 3:
		color.r = p;
		color.g = q;
		color.b = brightness;
		break;
	case 4:
		color.r = t;
		color.g = p;
		color.b = brightness;
		break;
	case 5:
		color.r = brightness;
		color.g = p;
		color.b = q;
		break;
	}
	return color;
}

unsigned int Color::getRGB(const glm::vec4& color) {
	return static_cast<int>(color.g * magnitude) << 16 | static_cast<int>(color.b * magnitude) << 8 | static_cast<int>(color.r * magnitude);
}

unsigned int Color::getRGBA(const glm::vec4& color) {
	return static_cast<int>(color.a * magnitude) << 24 | static_cast<int>(color.b * magnitude) << 16 | static_cast<int>(color.g * magnitude) << 8
			| static_cast<int>(color.r * magnitude);
}

void Color::getHSB(const glm::vec4& color, float& chue, float& csaturation, float& cbrightness) {
	cbrightness = brightness(color);
	const float minBrightness = std::min(color.r, std::min(color.g, color.b));
	if (std::fabs(cbrightness - minBrightness) < std::numeric_limits<float>::epsilon()) {
		chue = 0.f;
		csaturation = 0.f;
		return;
	}
	const float r = (cbrightness - color.r) / (cbrightness - minBrightness);
	const float g = (cbrightness - color.g) / (cbrightness - minBrightness);
	const float b = (cbrightness - color.b) / (cbrightness - minBrightness);
	if (std::fabs(color.r - cbrightness) < std::numeric_limits<float>::epsilon()) {
		chue = b - g;
	} else if (std::fabs(color.g - cbrightness) < std::numeric_limits<float>::epsilon()) {
		chue = 2.f + r - b;
	} else {
		chue = 4.f + g - r;
	}
	chue /= 6.f;
	if (chue < 0.f) {
		chue += 1.f;
	}
	csaturation = (cbrightness - minBrightness) / cbrightness;
}

glm::vec4 Color::alpha(const glm::vec4& c, float alpha) {
	return glm::vec4(c.r, c.g, c.b, alpha);
}

float Color::brightness(const glm::vec4& color) {
	return std::max(color.r, std::max(color.g, color.b));
}

float Color::intensity(const glm::vec4& color) {
	return (color.r + color.g + color.b) / 3.f;
}

glm::vec4 Color::darker(const glm::vec4& color, float f) {
	f = std::pow(scaleFactor, f);
	return glm::vec4(glm::clamp(glm::vec3(color) * f, 0.0f, 1.0f), color.a);
}

glm::vec4 Color::brighter(const glm::vec4& color, float f) {
	static float min = 21.f / magnitude;
	glm::vec3 result = glm::vec3(color);
	f = std::pow(scaleFactor, f);
	if (glm::all(glm::epsilonEqual(glm::vec3(), result, 0.00001f))) {
		return glm::vec4(min / f, min / f, min / f, color.a);
	}
	if (result.r > 0.f && result.r < min) {
		result.r = min;
	}
	if (result.g > 0.f && result.g < min) {
		result.g = min;
	}
	if (result.b > 0.f && result.b < min) {
		result.b = min;
	}
	return glm::vec4(glm::clamp(result / f, 0.f, 1.f), color.a);
}

}
