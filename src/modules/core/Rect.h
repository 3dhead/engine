/**
 * @file
 */

#pragma once
#include <limits>
#include <array>
#include "GLM.h"
#include "core/Common.h"

namespace core {

template<typename TYPE>
class Rect {
private:
	glm::tvec2<TYPE> _mins;
	glm::tvec2<TYPE> _maxs;
public:
	Rect() :
			_mins(glm::tvec2<TYPE>((TYPE)0)),
			_maxs(glm::tvec2<TYPE>((TYPE)0)) {
	}

	Rect(TYPE minX, TYPE minZ, TYPE maxX, TYPE maxZ) :
			_mins(minX, minZ), _maxs(maxX, maxZ) {
		core_assert(_mins.x < _maxs.x);
		core_assert(_mins.y < _maxs.y);
	}

	Rect(const glm::tvec2<TYPE>& mins, const glm::tvec2<TYPE>& maxs) :
			_mins(mins), _maxs(maxs) {
		core_assert(_mins.x < _maxs.x);
		core_assert(_mins.y < _maxs.y);
	}

	static const Rect<TYPE>& getMaxRect() {
		static constexpr TYPE lowest = std::numeric_limits<TYPE>::lowest();
		static constexpr TYPE max = std::numeric_limits<TYPE>::max();
		static const Rect<TYPE> maxRect(lowest, lowest, max, max);
		return maxRect;
	}

	inline TYPE getMaxZ() const {
		return _maxs.y;
	}

	inline TYPE getMinX() const {
		return _mins.x;
	}

	inline TYPE getMaxX() const {
		return _maxs.x;
	}

	inline TYPE getMinZ() const {
		return _mins.y;
	}

	inline bool operator==(const Rect& other) const {
		return _mins == other._mins && _maxs == other._maxs;
	}

	inline bool intersectsWith(const Rect& other) const {
		if (getMaxX() <= other.getMinX() || getMinX() >= other.getMaxX()) {
			return false;
		}
		if (getMaxZ() <= other.getMinZ() || getMinZ() >= other.getMaxZ()) {
			return false;
		}
		return true;
	}

	inline void offset(TYPE dx, TYPE dz) {
		if (getMaxRect() == *this) {
			return;
		}
		_mins.x += dx;
		_mins.y += dz;
		_maxs.x += dx;
		_maxs.y += dz;
	}

	void split(std::array<Rect<TYPE>, 4>& result) const {
		if (getMaxRect() == *this) {
			// special case because the length would exceed the max possible value of TYPE
			if (std::numeric_limits<TYPE>::is_signed) {
				static const std::array<Rect<TYPE>, 4> maxSplit = {{
					Rect<TYPE>(_mins.x, _mins.y, 0, 0),
					Rect<TYPE>(0, _mins.y, _maxs.x, 0),
					Rect<TYPE>(_mins.x, 0, 0, _maxs.y),
					Rect<TYPE>(0, 0, _maxs.x, _maxs.y)
				}};
				result = maxSplit;
				return;
			}
		}

		const TYPE lengthX = glm::abs(_maxs.x - _mins.x);
		const TYPE halfX = lengthX / 2.0;
		const TYPE lengthY = glm::abs(_maxs.y - _mins.y);
		const TYPE halfY = lengthY / 2.0;
		result[0] = Rect<TYPE>(_mins.x, _mins.y, _mins.x + halfX, _mins.y + halfY);
		result[1] = Rect<TYPE>(_mins.x + halfX, _mins.y, _maxs.x, _mins.y + halfY);
		result[2] = Rect<TYPE>(_mins.x, _mins.y + halfY, _mins.x + halfX, _maxs.y);
		result[3] = Rect<TYPE>(_mins.x + halfX, _mins.y + halfY, _maxs.x, _maxs.y);
	}

	inline bool contains(const Rect& rect) const {
		if (rect.getMaxX() > getMaxX()) {
			return false;
		}
		if (rect.getMaxZ() > getMaxZ()) {
			return false;
		}
		if (rect.getMinX() < getMinX()) {
			return false;
		}
		if (rect.getMinZ() < getMinZ()) {
			return false;
		}
		return true;
	}

	inline bool contains(const glm::tvec2<TYPE>& point) const {
		if (point.x > getMaxX()) {
			return false;
		}
		if (point.y > getMaxZ()) {
			return false;
		}
		if (point.x < getMinX()) {
			return false;
		}
		if (point.y < getMinZ()) {
			return false;
		}
		return true;
	}

	inline glm::tvec2<TYPE> maxs() const {
		return glm::tvec2<TYPE>(getMaxX(), getMaxZ());
	}

	inline glm::tvec2<TYPE> mins() const {
		return glm::tvec2<TYPE>(getMinX(), getMinZ());
	}

	inline glm::tvec2<TYPE> size() const {
		return maxs() - mins();
	}

	inline glm::tvec2<TYPE> center() const {
		return mins() + size() / (TYPE)2;
	}
};

typedef Rect<uint32_t> RectuInt;
typedef Rect<float> RectFloat;

}
