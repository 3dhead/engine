#include "SelectEntitiesOfTypes.h"
#include "core/String.h"
#include "core/Common.h"
#include "backend/entity/Npc.h"
#include "backend/entity/ai/AICharacter.h"
#include "network/ProtocolEnum.h"

using namespace ai;

namespace backend {

SelectEntitiesOfTypes::SelectEntitiesOfTypes(const std::string& parameters) :
		IFilter("SelectEntitiesOfTypes", parameters) {
	std::vector<std::string> types;
	core::string::splitString(parameters, types, ",");
	for (const std::string& type : types) {
		auto entityType = network::getEnum<network::EntityType>(type.c_str(), network::EnumNamesEntityType());
		core_assert_always(entityType != network::EntityType::NONE);
		_entityTypes[std::enum_value(entityType)] = true;
	}
}

void SelectEntitiesOfTypes::filter(const AIPtr& entity) {
	FilteredEntities& entities = getFilteredEntities(entity);
	backend::Npc& chr = getNpc(entity);
	chr.visitVisible([&] (const backend::EntityPtr& e) {
		if (!_entityTypes[std::enum_value(e->entityType())]) {
			return;
		}
		entities.push_back(e->id());
	});
}

}
