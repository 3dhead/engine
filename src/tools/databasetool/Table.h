#pragma once

#include "persistence/Field.h"
#include "persistence/Structs.h"
#include <stdint.h>
#include <vector>
#include <map>
#include <set>
#include <unordered_map>
#include <string>

namespace databasetool {

// TODO: sort for insertion order - keep it stable
typedef std::map<std::string, persistence::Field> Fields;

struct Table {
	std::string name;
	std::string classname;
	std::string namespaceSrc = "backend";
	std::string schema = "public";
	Fields fields;
	persistence::Constraints constraints;
	persistence::ForeignKeys foreignKeys;
	int primaryKeys = 0;
	persistence::UniqueKeys uniqueKeys;
	std::string autoIncrementField;
	int autoIncrementStart = 1;
};

}
