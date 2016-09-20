/**
 * @file
 */

#pragma once

#include "Shape.h"
#include "Shared_generated.h"
#include "cooldown/CooldownType.h"

namespace stock {

using ItemType = network::ItemType;

/**
 * @brief Converts a string into the enum value
 * @ingroup Stock
 */
inline ItemType getItemType(const char* name) {
	return network::getEnum<ItemType>(name, network::EnumNamesItemType());
}

/**
 * @brief Converts a string into the enum value
 * @ingroup Stock
 */
inline ItemType getItemType(const std::string& name) {
	return getItemType(name.c_str());
}

using ItemId = uint32_t;

/**
 * @brief Blueprint that describes a thing that can be managed by the Stock class.
 *
 * @note This is 'static' data - meaning that if you own 100 items of the same type, they
 * share one instance of this class.
 */
class ItemData {
protected:
	ItemId _id;
	ItemShape _shape;
	ItemType _type;
	cooldown::Type _construction = cooldown::Type::NONE;
	cooldown::Type _usage = cooldown::Type::NONE;
	cooldown::Type _regenerate = cooldown::Type::NONE;
public:
	ItemData(ItemId id, ItemType type);

	void setSize(uint8_t width, uint8_t height);

	const ItemType& type() const;

	const ItemShape& shape() const;

	const char *name() const;

	ItemId id() const;

	ItemShape& shape();

	const cooldown::Type& regenerateCooldown() const;

	const cooldown::Type& usageCooldown() const;

	const cooldown::Type& constructionCooldown() const;
};

inline ItemId ItemData::id() const {
	return _id;
}

inline const cooldown::Type& ItemData::regenerateCooldown() const {
	return _regenerate;
}

inline const cooldown::Type& ItemData::usageCooldown() const {
	return _usage;
}

inline const cooldown::Type& ItemData::constructionCooldown() const {
	return _construction;
}

inline const ItemType& ItemData::type() const {
	return _type;
}

inline ItemShape& ItemData::shape() {
	return _shape;
}

inline const ItemShape& ItemData::shape() const {
	return _shape;
}

}
