/**
 * @file
 */

#pragma once

#include "core/Common.h"

#include <algorithm>
#include <limits>

namespace core {

template<typename TYPE>
class AABB {
public:
	AABB(const glm::tvec3<TYPE>& mins, const glm::tvec3<TYPE>& maxs);

	/// Equality Operator.
	bool operator==(const AABB& rhs) const;
	/// Inequality Operator.
	bool operator!=(const AABB& rhs) const;

	TYPE getWidthX() const;
	TYPE getWidthY() const;
	TYPE getWidthZ() const;
	glm::tvec3<TYPE> getWidth() const;

	/// Gets the 'x' position of the center.
	TYPE getCenterX() const;
	/// Gets the 'y' position of the center.
	TYPE getCenterY() const;
	/// Gets the 'z' position of the center.
	TYPE getCenterZ() const;
	/// Gets the 'x' position of the lower corner.
	TYPE getLowerX() const;
	/// Gets the 'y' position of the lower corner.
	TYPE getLowerY() const;
	/// Gets the 'z' position of the lower corner.
	TYPE getLowerZ() const;
	/// Gets the 'x' position of the upper corner.
	TYPE getUpperX() const;
	/// Gets the 'y' position of the upper corner.
	TYPE getUpperY() const;
	/// Gets the 'z' position of the upper corner.
	TYPE getUpperZ() const;

	/// Gets the center of the AABB
	glm::tvec3<TYPE> getCenter() const;
	/// Gets the position of the lower corner.
	glm::tvec3<TYPE> getLowerCorner() const;
	/// Gets the position of the upper corner.
	glm::tvec3<TYPE> getUpperCorner() const;

	/// Sets the 'x' position of the lower corner.
	void setLowerX(TYPE iX);
	/// Sets the 'y' position of the lower corner.
	void setLowerY(TYPE iY);
	/// Sets the 'z' position of the lower corner.
	void setLowerZ(TYPE iZ);
	/// Sets the 'x' position of the upper corner.
	void setUpperX(TYPE iX);
	/// Sets the 'y' position of the upper corner.
	void setUpperY(TYPE iY);
	/// Sets the 'z' position of the upper corner.
	void setUpperZ(TYPE iZ);

	/// Sets the position of the lower corner.
	void setLowerCorner(const glm::tvec3<TYPE>& v3dLowerCorner);
	/// Sets the position of the upper corner.
	void setUpperCorner(const glm::tvec3<TYPE>& v3dUpperCorner);

	/// Tests whether the given point is contained in this AABB.
	/// Tests whether the given point is contained in this AABB.
	bool containsPoint(const glm::tvec3<TYPE>& pos, TYPE boundary = static_cast<TYPE>(0)) const;
	/// Tests whether the given point is contained in this AABB.
	bool containsPoint(TYPE iX, TYPE iY, TYPE iZ, TYPE boundary = static_cast<TYPE>(0)) const;
	/// Tests whether the given position is contained in the 'x' range of this AABB.
	bool containsPointInX(TYPE pos, TYPE boundary = static_cast<TYPE>(0)) const;
	/// Tests whether the given position is contained in the 'y' range of this AABB.
	bool containsPointInY(TYPE pos, TYPE boundary = static_cast<TYPE>(0)) const;
	/// Tests whether the given position is contained in the 'z' range of this AABB.
	bool containsPointInZ(TYPE pos, TYPE boundary = static_cast<TYPE>(0)) const;

	/// Tests whether the given AABB is contained in this AABB.
	bool containsAABB(const AABB& reg, TYPE boundary = static_cast<TYPE>(0)) const;

	/// Enlarges the AABB so that it contains the specified position.
	void accumulate(TYPE iX, TYPE iY, TYPE iZ);
	/// Enlarges the AABB so that it contains the specified position.
	void accumulate(const glm::tvec3<TYPE>& v3dPos);
	/// Enlarges the AABB so that it contains the specified AABB.
	void accumulate(const AABB& reg);

	/// Crops the extents of this AABB accoring to another AABB.
	void cropTo(const AABB& other);

	/// Grows this AABB by the amount specified.
	void grow(TYPE iAmount);
	/// Grows this AABB by the amounts specified.
	void grow(TYPE iAmountX, TYPE iAmountY, TYPE iAmountZ);
	/// Grows this AABB by the amounts specified.
	void grow(const glm::tvec3<TYPE>& v3dAmount);

	/// Tests whether all components of the upper corner are at least
	/// as great as the corresponding components of the lower corner.
	bool isValid() const;

	/// Moves the AABB by the amount specified.
	void shift(TYPE iAmountX, TYPE iAmountY, TYPE iAmountZ);
	/// Moves the AABB by the amount specified.
	void shift(const glm::tvec3<TYPE>& v3dAmount);
	/// Moves the lower corner of the AABB by the amount specified.
	void shiftLowerCorner(TYPE iAmountX, TYPE iAmountY, TYPE iAmountZ);
	/// Moves the lower corner of the AABB by the amount specified.
	void shiftLowerCorner(const glm::tvec3<TYPE>& v3dAmount);
	/// Moves the upper corner of the AABB by the amount specified.
	void shiftUpperCorner(TYPE iAmountX, TYPE iAmountY, TYPE iAmountZ);
	/// Moves the upper corner of the AABB by the amount specified.
	void shiftUpperCorner(const glm::tvec3<TYPE>& v3dAmount);

	/// Shrinks this AABB by the amount specified.
	void shrink(TYPE iAmount);
	/// Shrinks this AABB by the amounts specified.
	void shrink(TYPE iAmountX, TYPE iAmountY, TYPE iAmountZ);
	/// Shrinks this AABB by the amounts specified.
	void shrink(const glm::tvec3<TYPE>& v3dAmount);
private:
	glm::tvec3<TYPE> _mins;
	glm::tvec3<TYPE> _maxs;
};

template<typename TYPE>
inline glm::tvec3<TYPE> AABB<TYPE>::getWidth() const {
	return _maxs - _mins;
}

template<typename TYPE>
inline TYPE AABB<TYPE>::getWidthX() const {
	return _maxs.x - _mins.x;
}

template<typename TYPE>
inline TYPE AABB<TYPE>::getWidthY() const {
	return _maxs.y - _mins.y;
}

template<typename TYPE>
inline TYPE AABB<TYPE>::getWidthZ() const {
	return _maxs.z - _mins.z;
}

/**
 * @return The 'x' position of the centre.
 */
template<typename TYPE>
inline TYPE AABB<TYPE>::getCenterX() const {
	return (_mins.x + _maxs.x) / 2;
}

/**
 * @return The 'y' position of the centre.
 */
template<typename TYPE>
inline TYPE AABB<TYPE>::getCenterY() const {
	return (_mins.y + _maxs.y) / 2;
}

/**
 * @return The 'z' position of the centre.
 */
template<typename TYPE>
inline TYPE AABB<TYPE>::getCenterZ() const {
	return (_mins.z + _maxs.z) / 2;
}

/**
 * @return The 'x' position of the lower corner.
 */
template<typename TYPE>
inline TYPE AABB<TYPE>::getLowerX() const {
	return _mins.x;
}

/**
 * @return The 'y' position of the lower corner.
 */
template<typename TYPE>
inline TYPE AABB<TYPE>::getLowerY() const {
	return _mins.y;
}

/**
 * @return The 'z' position of the lower corner.
 */
template<typename TYPE>
inline TYPE AABB<TYPE>::getLowerZ() const {
	return _mins.z;
}

/**
 * @return The 'x' position of the upper corner.
 */
template<typename TYPE>
inline TYPE AABB<TYPE>::getUpperX() const {
	return _maxs.x;
}

/**
 * @return The 'y' position of the upper corner.
 */
template<typename TYPE>
inline TYPE AABB<TYPE>::getUpperY() const {
	return _maxs.y;
}

/**
 * @return The 'z' position of the upper corner.
 */
template<typename TYPE>
inline TYPE AABB<TYPE>::getUpperZ() const {
	return _maxs.z;
}

/**
 * @return The position of the lower corner.
 */
template<typename TYPE>
inline glm::tvec3<TYPE> AABB<TYPE>::getCenter() const {
	return glm::tvec3<TYPE>(getCenterX(), getCenterY(), getCenterZ());
}

/**
 * @return The position of the lower corner.
 */
template<typename TYPE>
inline glm::tvec3<TYPE> AABB<TYPE>::getLowerCorner() const {
	return glm::tvec3<TYPE>(_mins.x, _mins.y, _mins.z);
}

/**
 * @return The position of the upper corner.
 */
template<typename TYPE>
inline glm::tvec3<TYPE> AABB<TYPE>::getUpperCorner() const {
	return glm::tvec3<TYPE>(_maxs.x, _maxs.y, _maxs.z);
}

/**
 * @param iX The new 'x' position of the lower corner.
 */
template<typename TYPE>
inline void AABB<TYPE>::setLowerX(TYPE iX) {
	_mins.x = iX;
}

/**
 * @param iY The new 'y' position of the lower corner.
 */
template<typename TYPE>
inline void AABB<TYPE>::setLowerY(TYPE iY) {
	_mins.y = iY;
}

/**
 * @param iZ The new 'z' position of the lower corner.
 */
template<typename TYPE>
inline void AABB<TYPE>::setLowerZ(TYPE iZ) {
	_mins.z = iZ;
}

/**
 * @param iX The new 'x' position of the upper corner.
 */
template<typename TYPE>
inline void AABB<TYPE>::setUpperX(TYPE iX) {
	_maxs.x = iX;
}

/**
 * @param iY The new 'y' position of the upper corner.
 */
template<typename TYPE>
inline void AABB<TYPE>::setUpperY(TYPE iY) {
	_maxs.y = iY;
}

/**
 * @param iZ The new 'z' position of the upper corner.
 */
template<typename TYPE>
inline void AABB<TYPE>::setUpperZ(TYPE iZ) {
	_maxs.z = iZ;
}

/**
 * @param v3dLowerCorner The new position of the lower corner.
 */
template<typename TYPE>
inline void AABB<TYPE>::setLowerCorner(const glm::tvec3<TYPE>& v3dLowerCorner) {
	_mins.x = v3dLowerCorner.x;
	_mins.y = v3dLowerCorner.y;
	_mins.z = v3dLowerCorner.z;
}

/**
 * @param v3dUpperCorner The new position of the upper corner.
 */
template<typename TYPE>
inline void AABB<TYPE>::setUpperCorner(const glm::tvec3<TYPE>& v3dUpperCorner) {
	_maxs.x = v3dUpperCorner.x;
	_maxs.y = v3dUpperCorner.y;
	_maxs.z = v3dUpperCorner.z;
}

/**
 * @param iX The 'x' component of the position to accumulate.
 * @param iY The 'y' component of the position to accumulate.
 * @param iZ The 'z' component of the position to accumulate.
 */
template<typename TYPE>
inline void AABB<TYPE>::accumulate(TYPE iX, TYPE iY, TYPE iZ) {
	_mins.x = std::min(_mins.x, iX);
	_mins.y = std::min(_mins.y, iY);
	_mins.z = std::min(_mins.z, iZ);
	_maxs.x = std::max(_maxs.x, iX);
	_maxs.y = std::max(_maxs.y, iY);
	_maxs.z = std::max(_maxs.z, iZ);
}

/**
 * @param v3dPos The position to accumulate.
 */
template<typename TYPE>
inline void AABB<TYPE>::accumulate(const glm::tvec3<TYPE>& v3dPos) {
	accumulate(v3dPos.x, v3dPos.y, v3dPos.z);
}

/**
 * Note that this is not the same as computing the union of two AABBs (as the result of
 * such a union may not be a shape which can be exactly represented by a AABB). Instead,
 * the result is simply big enough to contain both this AABB and the one passed as a parameter.
 * @param reg The AABB to accumulate. This must be valid as defined by the isValid() function.
 * @sa isValid()
 */
template<typename TYPE>
inline void AABB<TYPE>::accumulate(const AABB& reg) {
	if (!reg.isValid()) {
		// The result of accumulating an invalid AABB is not defined.
		core_assert_msg(false, "You cannot accumulate an invalid AABB.");
	}

	_mins.x = std::min(_mins.x, reg.getLowerX());
	_mins.y = std::min(_mins.y, reg.getLowerY());
	_mins.z = std::min(_mins.z, reg.getLowerZ());
	_maxs.x = std::max(_maxs.x, reg.getUpperX());
	_maxs.y = std::max(_maxs.y, reg.getUpperY());
	_maxs.z = std::max(_maxs.z, reg.getUpperZ());
}

/**
 * Constructs a AABB and sets the lower and upper corners to the specified values.
 * @param v3dLowerCorner The desired lower corner of the AABB.
 * @param v3dUpperCorner The desired upper corner of the AABB.
 */
template<typename TYPE>
inline AABB<TYPE>::AABB(const glm::tvec3<TYPE>& mins, const glm::tvec3<TYPE>& maxs) : _mins(mins), _maxs(maxs) {
}

/**
 * Two AABBs are considered equal if all their extents match.
 * @param rhs The AABB to compare to.
 * @return true if the AABBs match.
 * @sa operator!=
 */
template<typename TYPE>
inline bool AABB<TYPE>::operator==(const AABB& rhs) const {
	return ((_mins.x == rhs._mins.x) && (_mins.y == rhs._mins.y) && (_mins.z == rhs._mins.z) && (_maxs.x == rhs._maxs.x) && (_maxs.y == rhs._maxs.y)
			&& (_maxs.z == rhs._maxs.z));
}

/**
 * Two AABBs are considered different if any of their extents differ.
 * @param rhs The AABB to compare to.
 * @return true if the AABBs are different.
 * @sa operator==
 */
template<typename TYPE>
inline bool AABB<TYPE>::operator!=(const AABB& rhs) const {
	return !(*this == rhs);
}

/**
 * The boundary value can be used to ensure a position is only considered to be inside
 * the AABB if it is that far in in all directions. Also, the test is inclusive such
 * that positions lying exactly on the edge of the AABB are considered to be inside it.
 * @param fX The 'x' position of the point to test.
 * @param fY The 'y' position of the point to test.
 * @param fZ The 'z' position of the point to test.
 * @param boundary The desired boundary value.
 */
template<typename TYPE>
inline bool AABB<TYPE>::containsPoint(TYPE fX, TYPE fY, TYPE fZ, TYPE boundary) const {
	return (fX <= _maxs.x - boundary) && (fY <= _maxs.y - boundary) && (fZ <= _maxs.z - boundary) && (fX >= _mins.x + boundary) && (fY >= _mins.y + boundary)
			&& (fZ >= _mins.z + boundary);
}

/**
 * The boundary value can be used to ensure a position is only considered to be inside
 * the AABB if it is that far in in all directions. Also, the test is inclusive such
 * that positions lying exactly on the edge of the AABB are considered to be inside it.
 * @param pos The position to test.
 * @param boundary The desired boundary value.
 */
template<typename TYPE>
inline bool AABB<TYPE>::containsPoint(const glm::tvec3<TYPE>& pos, TYPE boundary) const {
	return containsPoint(pos.x, pos.y, pos.z, boundary);
}

/**
 * The boundary value can be used to ensure a position is only considered to be inside
 * the AABB if it is that far in in the 'x' direction. Also, the test is inclusive such
 * that positions lying exactly on the edge of the AABB are considered to be inside it.
 * @param pos The position to test.
 * @param boundary The desired boundary value.
 */
template<typename TYPE>
inline bool AABB<TYPE>::containsPointInX(TYPE pos, TYPE boundary) const {
	return (pos <= _maxs.x - boundary) && (pos >= _mins.x + boundary);
}

/**
 * The boundary value can be used to ensure a position is only considered to be inside
 * the AABB if it is that far in in the 'y' direction. Also, the test is inclusive such
 * that positions lying exactly on the edge of the AABB are considered to be inside it.
 * @param pos The position to test.
 * @param boundary The desired boundary value.
 */
template<typename TYPE>
inline bool AABB<TYPE>::containsPointInY(TYPE pos, TYPE boundary) const {
	return (pos <= _maxs.y - boundary) && (pos >= _mins.y + boundary);
}

/**
 * The boundary value can be used to ensure a position is only considered to be inside
 * the AABB if it is that far in in the 'z' direction. Also, the test is inclusive such
 * that positions lying exactly on the edge of the AABB are considered to be inside it.
 * @param pos The position to test.
 * @param boundary The desired boundary value.
 */
template<typename TYPE>
inline bool AABB<TYPE>::containsPointInZ(TYPE pos, TYPE boundary) const {
	return (pos <= _maxs.z - boundary) && (pos >= _mins.z + boundary);
}

/**
 * The boundary value can be used to ensure a AABB is only considered to be inside
 * another AABB if it is that far in in all directions. Also, the test is inclusive such
 * that a AABB is considered to be inside of itself.
 * @param reg The AABB to test.
 * @param boundary The desired boundary value.
 */
template<typename TYPE>
inline bool AABB<TYPE>::containsAABB(const AABB<TYPE>& reg, TYPE boundary) const {
	return (reg._maxs.x <= _maxs.x - boundary) && (reg._maxs.y <= _maxs.y - boundary) && (reg._maxs.z <= _maxs.z - boundary) && (reg._mins.x >= _mins.x + boundary)
			&& (reg._mins.y >= _mins.y + boundary) && (reg._mins.z >= _mins.z + boundary);
}

/**
 * After calling this functions, the extents of this AABB are given by the intersection
 * of this AABB and the one it was cropped to.
 * @param other The AABB to crop to.
 */
template<typename TYPE>
inline void AABB<TYPE>::cropTo(const AABB<TYPE>& other) {
	_mins.x = std::max(_mins.x, other._mins.x);
	_mins.y = std::max(_mins.y, other._mins.y);
	_mins.z = std::max(_mins.z, other._mins.z);
	_maxs.x = std::min(_maxs.x, other._maxs.x);
	_maxs.y = std::min(_maxs.y, other._maxs.y);
	_maxs.z = std::min(_maxs.z, other._maxs.z);
}

/**
 * The same amount of growth is applied in all directions. Negative growth
 * is possible but you should prefer the shrink() function for clarity.
 * @param iAmount The amount to grow by.
 */
template<typename TYPE>
inline void AABB<TYPE>::grow(TYPE iAmount) {
	_mins.x -= iAmount;
	_mins.y -= iAmount;
	_mins.z -= iAmount;

	_maxs.x += iAmount;
	_maxs.y += iAmount;
	_maxs.z += iAmount;
}

/**
 * The amount can be specified separately for each direction. Negative growth
 * is possible but you should prefer the shrink() function for clarity.
 * @param iAmountX The amount to grow by in 'x'.
 * @param iAmountY The amount to grow by in 'y'.
 * @param iAmountZ The amount to grow by in 'z'.
 */
template<typename TYPE>
inline void AABB<TYPE>::grow(TYPE iAmountX, TYPE iAmountY, TYPE iAmountZ) {
	_mins.x -= iAmountX;
	_mins.y -= iAmountY;
	_mins.z -= iAmountZ;

	_maxs.x += iAmountX;
	_maxs.y += iAmountY;
	_maxs.z += iAmountZ;
}

/**
 * The amount can be specified separately for each direction. Negative growth
 * is possible but you should prefer the shrink() function for clarity.
 * @param v3dAmount The amount to grow by (one component for each direction).
 */
template<typename TYPE>
inline void AABB<TYPE>::grow(const glm::tvec3<TYPE>& v3dAmount) {
	grow(v3dAmount.x, v3dAmount.y, v3dAmount.z);
}

template<typename TYPE>
inline bool AABB<TYPE>::isValid() const {
	return _maxs.x >= _mins.x && _maxs.y >= _mins.y && _maxs.z >= _mins.z;
}

/**
 * @param iAmountX The amount to move the AABB by in 'x'.
 * @param iAmountY The amount to move the AABB by in 'y'.
 * @param iAmountZ The amount to move the AABB by in 'z'.
 */
template<typename TYPE>
inline void AABB<TYPE>::shift(TYPE iAmountX, TYPE iAmountY, TYPE iAmountZ) {
	shiftLowerCorner(iAmountX, iAmountY, iAmountZ);
	shiftUpperCorner(iAmountX, iAmountY, iAmountZ);
}

/**
 * @param v3dAmount The amount to move the AABB by.
 */
template<typename TYPE>
inline void AABB<TYPE>::shift(const glm::tvec3<TYPE>& v3dAmount) {
	shiftLowerCorner(v3dAmount);
	shiftUpperCorner(v3dAmount);
}

/**
 * @param iAmountX The amount to move the lower corner by in 'x'.
 * @param iAmountY The amount to move the lower corner by in 'y'.
 * @param iAmountZ The amount to move the lower corner by in 'z'.
 */
template<typename TYPE>
inline void AABB<TYPE>::shiftLowerCorner(TYPE iAmountX, TYPE iAmountY, TYPE iAmountZ) {
	_mins.x += iAmountX;
	_mins.y += iAmountY;
	_mins.z += iAmountZ;
}

/**
 * @param v3dAmount The amount to move the lower corner by.
 */
template<typename TYPE>
inline void AABB<TYPE>::shiftLowerCorner(const glm::tvec3<TYPE>& v3dAmount) {
	shiftLowerCorner(v3dAmount.x, v3dAmount.y, v3dAmount.z);
}

/**
 * @param iAmountX The amount to move the upper corner by in 'x'.
 * @param iAmountY The amount to move the upper corner by in 'y'.
 * @param iAmountZ The amount to move the upper corner by in 'z'.
 */
template<typename TYPE>
inline void AABB<TYPE>::shiftUpperCorner(TYPE iAmountX, TYPE iAmountY, TYPE iAmountZ) {
	_maxs.x += iAmountX;
	_maxs.y += iAmountY;
	_maxs.z += iAmountZ;
}

/**
 * @param v3dAmount The amount to move the upper corner by.
 */
template<typename TYPE>
inline void AABB<TYPE>::shiftUpperCorner(const glm::tvec3<TYPE>& v3dAmount) {
	shiftUpperCorner(v3dAmount.x, v3dAmount.y, v3dAmount.z);
}

/**
 * The same amount of shrinkage is applied in all directions. Negative shrinkage
 * is possible but you should prefer the grow() function for clarity.
 * @param iAmount The amount to shrink by.
 */
template<typename TYPE>
inline void AABB<TYPE>::shrink(TYPE iAmount) {
	_mins.x += iAmount;
	_mins.y += iAmount;
	_mins.z += iAmount;

	_maxs.x -= iAmount;
	_maxs.y -= iAmount;
	_maxs.z -= iAmount;
}

/**
 * The amount can be specified seperatly for each direction. Negative shrinkage
 * is possible but you should prefer the grow() function for clarity.
 * @param iAmountX The amount to shrink by in 'x'.
 * @param iAmountY The amount to shrink by in 'y'.
 * @param iAmountZ The amount to shrink by in 'z'.
 */
template<typename TYPE>
inline void AABB<TYPE>::shrink(TYPE iAmountX, TYPE iAmountY, TYPE iAmountZ) {
	_mins.x += iAmountX;
	_mins.y += iAmountY;
	_mins.z += iAmountZ;

	_maxs.x -= iAmountX;
	_maxs.y -= iAmountY;
	_maxs.z -= iAmountZ;
}

/**
 * The amount can be specified seperatly for each direction. Negative shrinkage
 * is possible but you should prefer the grow() function for clarity.
 * @param v3dAmount The amount to shrink by (one component for each direction).
 */
template<typename TYPE>
inline void AABB<TYPE>::shrink(const glm::tvec3<TYPE>& v3dAmount) {
	shrink(v3dAmount.x, v3dAmount.y, v3dAmount.z);
}

/**
 * This function only returns true if the AABBs are really intersecting and not simply touching.
 */
template<typename TYPE>
inline bool intersects(const AABB<TYPE>& a, const AABB<TYPE>& b) {
	// No intersection if seperated along an axis.
	if (a.getUpperX() < b.getLowerX() || a.getLowerX() > b.getUpperX())
		return false;
	if (a.getUpperY() < b.getLowerY() || a.getLowerY() > b.getUpperY())
		return false;
	if (a.getUpperZ() < b.getLowerZ() || a.getLowerZ() > b.getUpperZ())
		return false;

	// Overlapping on all axes means AABBs are intersecting.
	return true;
}

}
