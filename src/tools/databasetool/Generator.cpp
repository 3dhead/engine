#include "Generator.h"
#include "Util.h"
#include "Mapping.h"
#include "Table.h"
#include "core/String.h"

namespace databasetool {

#define NAMESPACE "db"

struct Namespace {
	const Table& _table;
	std::stringstream& _src;

	Namespace(const Table& table, std::stringstream& src) : _table(table), _src(src) {
		if (!table.namespaceSrc.empty()) {
			src << "namespace " << table.namespaceSrc << " {\n\n";
		}
		src << "namespace " NAMESPACE " {\n\n";
	}
	~Namespace() {
		_src << "typedef std::shared_ptr<" << _table.classname << "> " << _table.classname << "Ptr;\n\n";
		_src << "} // namespace " NAMESPACE "\n\n";
		if (!_table.namespaceSrc.empty()) {
			_src << "} // namespace " << _table.namespaceSrc << "\n\n";
		}
	}
};

struct Class {
	const Table& _table;
	std::stringstream& _src;

	Class(const Table& table, std::stringstream& src) : _table(table), _src(src) {
		src << "/**\n * @brief Model class for table '" << table.name << "'\n";
		src << " * @note Work with this class in combination with the persistence::DBHandler\n";
		src << " */\n";
		src << "class " << table.classname << " : public persistence::Model {\n";
		src << "private:\n";
		src << "\tusing Super = persistence::Model;\n";
	}

	~Class() {
		_src << "}; // class " << _table.classname << "\n\n";
	}
};

struct MembersStruct {
	static const char *structName() {
		return "Members";
	}

	static const char *varName() {
		return "_m";
	}

	static std::string nullFieldName(const persistence::Field& f) {
		return "_isNull_" + f.name;
	}

	static std::string validFieldName(const persistence::Field& f) {
		return "_isValid_" + f.name;
	}
};

static std::string getFieldNameFunction(const persistence::Field& field) {
	return "f_" + field.name;
}

static void createMembersStruct(const Table& table, std::stringstream& src) {
	src << "\tstruct " << MembersStruct::structName() << " {\n";
	for (auto entry : table.fields) {
		const persistence::Field& f = entry.second;
		src << "\t\t/**\n";
		src << "\t\t * @brief Member for table column '" << entry.second.name << "'\n";
		src << "\t\t */\n";
		src << "\t\t";
		src << getCPPType(f.type, false);
		src << " _" << f.name;
		if (needsInitCPP(f.type)) {
			src << " = " << getCPPInit(f.type, false);
		}
		src << ";\n";
		// TODO: padding for short and boolean
	}
	for (auto entry : table.fields) {
		// TODO: use bitfield
		const persistence::Field& f = entry.second;
		if (isPointer(f)) {
			src << "\t\t/**\n";
			src << "\t\t * @brief Is the value set to null?\n";
			src << "\t\t * @c true if a value is set to null and the field should be taken into account for e.g. update statements, @c false if not\n";
			src << "\t\t */\n";
			src << "\t\tbool " << MembersStruct::nullFieldName(f) << " = false;\n";
		}
		src << "\t\t/**\n";
		src << "\t\t * @brief Is there a valid value set?\n";
		src << "\t\t * @c true if a value is set and the field should be taken into account for e.g. update statements, @c false if not\n";
		src << "\t\t */\n";
		src << "\t\tbool " << MembersStruct::validFieldName(f) << " = false;\n";
	}
	src << "\t};\n";
	src << "\tMembers " << MembersStruct::varName() << ";\n";
}

static void createMetaStruct(const Table& table, std::stringstream& src) {
	src << "\tstruct Meta {\n";
	src << "\t\tpersistence::Fields _fields;\n";
	src << "\t\tpersistence::Constraints _constraints;\n";
	src << "\t\tpersistence::UniqueKeys _uniqueKeys;\n";
	src << "\t\tpersistence::ForeignKeys _foreignKeys;\n";
	src << "\t\tpersistence::PrimaryKeys _primaryKeys;\n";
	src << "\t\tconst char* _autoIncrementField = nullptr;\n";
	src << "\t\tMeta() {\n";

	src << "\t\t\t_fields.reserve(" << table.fields.size() << ");\n";
	for (auto entry : table.fields) {
		const persistence::Field& f = entry.second;
		src << "\t\t\t_fields.emplace_back(persistence::Field{";
		src << "\"" << f.name << "\"";
		src << ", persistence::FieldType::" << persistence::toFieldType(f.type);
		src << ", persistence::Operator::" << OperatorNames[std::enum_value(f.updateOperator)];
		src << ", " << f.contraintMask;
		src << ", \"" << f.defaultVal << "\"";
		src << ", " << f.length;
		src << ", offsetof(";
		src << MembersStruct::structName() << ", _" << f.name << ")";
		if (isPointer(f)) {
			src << ", offsetof(";
			src << MembersStruct::structName() << ", " << MembersStruct::nullFieldName(f) << ")";
		} else {
			src << ", -1";
		}
		src << ", offsetof(";
		src << MembersStruct::structName() << ", " << MembersStruct::validFieldName(f) << ")";
		src << "});\n";
	}
	if (!table.constraints.empty()) {
		src << "\t\t\t_constraints.reserve(" << table.constraints.size() << ");\n";
	}
	for (auto i = table.constraints.begin(); i != table.constraints.end(); ++i) {
		const persistence::Constraint& c = i->second;
		src << "\t\t\t_constraints.insert(std::make_pair(\"" << i->first << "\", persistence::Constraint{{\"";
		src << core::string::join(c.fields.begin(), c.fields.end(), "\",\"");
		src << "\"}, " << c.types << "}));\n";
	}
	if (table.primaryKeys > 0) {
		src << "\t\t\t_primaryKeys.reserve(" << table.primaryKeys << ");\n";
		for (auto entry : table.constraints) {
			const persistence::Constraint& c = entry.second;
			if ((c.types & std::enum_value(persistence::ConstraintType::PRIMARYKEY)) == 0) {
				continue;
			}
			for (const std::string& pkfield : c.fields) {
				src << "\t\t\t_primaryKeys.emplace_back(\"" << pkfield << "\");\n";
			}
		}
	}
	for (auto entry : table.constraints) {
		const persistence::Constraint& c = entry.second;
		if ((c.types & std::enum_value(persistence::ConstraintType::AUTOINCREMENT)) == 0) {
			continue;
		}
		src << "\t\t\t_autoIncrementField = \"" << c.fields.front() << "\";\n";
	}
	if (!table.uniqueKeys.empty()) {
		src << "\t\t\t_uniqueKeys.reserve(" << table.uniqueKeys.size() << ");\n";
	}
	for (const auto& uniqueKey : table.uniqueKeys) {
		src << "\t\t\t_uniqueKeys.emplace_back(std::set<std::string>{\"";
		src << core::string::join(uniqueKey.begin(), uniqueKey.end(), "\", \"");
		src << "\"});\n";
	}
	if (!table.foreignKeys.empty()) {
		src << "\t\t\t_foreignKeys.reserve(" << table.foreignKeys.size() << ");\n";
	}
	for (const auto& foreignKeyEntry : table.foreignKeys) {
		src << "\t\t\t_foreignKeys.insert(std::make_pair(\"" << foreignKeyEntry.first << "\", persistence::ForeignKey{\"";
		src << foreignKeyEntry.second.table << "\", \"" << foreignKeyEntry.second.field;
		src << "\"}));\n";
	}

	src << "\t\t}\n";
	src << "\t};\n";
	src << "\tstatic inline Meta& meta() {\n\t\tstatic Meta _meta;\n\t\treturn _meta;\n\t}\n";
}

void createConstructor(const Table& table, std::stringstream& src) {
	src << "\t" << table.classname << "(";
	src << ") : Super(\"" << table.schema << "\", \"" << table.name << "\", &meta()._fields, &meta()._constraints, &meta()._uniqueKeys, &meta()._foreignKeys, &meta()._primaryKeys) {\n";
	src << "\t\t_membersPointer = (uint8_t*)&" << MembersStruct::varName() << ";\n";
	src << "\t\t_primaryKeyFields = " << table.primaryKeys << ";\n";
	src << "\t\t_autoIncrementField = meta()._autoIncrementField;\n";
	src << "\t\t_autoIncrementStart = " << table.autoIncrementStart << ";\n";
	src << "\t}\n\n";

	src << "\t" << table.classname << "(" << table.classname << "&& source) : Super(std::move(source._schema), std::move(source._tableName), &meta()._fields, &meta()._constraints, &meta()._uniqueKeys, &meta()._foreignKeys, &meta()._primaryKeys) {\n";
	src << "\t\t_m = std::move(source._m);\n";
	src << "\t\t_membersPointer = (uint8_t*)&_m;\n";
	src << "\t\t_primaryKeyFields = " << table.primaryKeys << ";\n";
	src << "\t\t_autoIncrementField = meta()._autoIncrementField;\n";
	src << "\t\t_autoIncrementStart = " << table.autoIncrementStart << ";\n";
	src << "\t}\n\n";
	src << "\t" << table.classname << "(const " << table.classname << "& source) : Super(source._schema, source._tableName, &meta()._fields, &meta()._constraints, &meta()._uniqueKeys, &meta()._foreignKeys, &meta()._primaryKeys) {\n";
	src << "\t\t_m = source._m;\n";
	src << "\t\t_membersPointer = (uint8_t*)&_m;\n";
	src << "\t\t_primaryKeyFields = " << table.primaryKeys << ";\n";
	src << "\t\t_autoIncrementField = meta()._autoIncrementField;\n";
	src << "\t\t_autoIncrementStart = " << table.autoIncrementStart << ";\n";
	src << "\t}\n\n";

	src << "\t" << table.classname << "& operator=(" << table.classname << "&& source) {\n";
	src << "\t\t_m = std::move(source._m);\n";
	src << "\t\t_membersPointer = (uint8_t*)&_m;\n";
	src << "\t\treturn *this;\n";
	src << "\t}\n\n";

	src << "\t" << table.classname << "& operator=(const " << table.classname << "& source) {\n";
	src << "\t\t_m = source._m;\n";
	src << "\t\t_membersPointer = (uint8_t*)&_m;\n";
	src << "\t\treturn *this;\n";
	src << "\t}\n\n";
}

static void createDBConditions(const Table& table, std::stringstream& src) {
	for (auto entry : table.fields) {
		const persistence::Field& f = entry.second;
		const std::string classname = "DBCondition" + core::string::upperCamelCase(table.classname) + core::string::upperCamelCase(f.name);
		src << "class " << classname;
		src << " : public persistence::DBCondition {\n";
		src << "private:\n";
		src << "\tusing Super = persistence::DBCondition;\n";
		src << "public:\n";
		src << "\t/**\n\t * @brief Condition for " << f.name << "\n\t * @param[in] value";
		if (f.type == persistence::FieldType::TIMESTAMP) {
			src << " UTC timestamp in seconds";
		} else if (isString(f)) {
			if (f.isLower()) {
				src << " The given value is converted to lowercase before the comparison takes place";
			}
		}
		src << "\n\t */\n\t";
		if (isString(f) && !f.isLower()) {
			src << "constexpr ";
		}
		src << classname << "(";
		if (isString(f)) {
			src << "const char *";
		} else {
			src << getCPPType(f.type, true, false);
		}
		src << " value, persistence::Comparator comp = persistence::Comparator::Equal) :\n\t\tSuper(";
		src << table.classname << "::" << getFieldNameFunction(f) << "(), persistence::FieldType::";
		src << persistence::toFieldType(f.type) << ", ";
		if (isString(f)) {
			if (f.isLower()) {
				src << "core::string::toLower(value)";
			} else {
				src << "value";
			}
		} else if (f.type == persistence::FieldType::TIMESTAMP) {
			src << "std::to_string(value.seconds())";
		} else {
			src << "std::to_string(value)";
		}
		src << ", comp) {\n\t}\n";

		if (isString(f)) {
			src << "\t" << classname << "(";
			src << "const std::string&";
			src << " value, persistence::Comparator comp = persistence::Comparator::Equal) :\n\t\tSuper(";
			src << table.classname << "::f_" << f.name << "(), persistence::FieldType::";
			src << persistence::toFieldType(f.type);
			src << ", ";
			if (f.isLower()) {
				src << "core::string::toLower(value)";
			} else {
				src << "value";
			}
			src << ", comp) {\n\t}\n";
		}

		src << "}; // class " << classname << "\n\n";
	}
}

static void createGetterAndSetter(const Table& table, std::stringstream& src) {
	for (auto entry : table.fields) {
		const persistence::Field& f = entry.second;
		const std::string& cpptypeGetter = getCPPType(f.type, true, isPointer(f));
		const std::string& getter = core::string::lowerCamelCase(f.name);
		const std::string& cpptypeSetter = getCPPType(f.type, true, false);
		const std::string& setter = core::string::upperCamelCase(f.name);

		src << "\t/**\n\t * @brief Access the value after the model was loaded\n";
		if (f.type == persistence::FieldType::TIMESTAMP) {
			src << "\t * @note The value is in seconds\n";
		}
		if (f.isAutoincrement()) {
			src << "\t * @note Auto increment\n";
		}
		if (f.isIndex()) {
			src << "\t * @note Index\n";
		}
		if (f.isNotNull()) {
			src << "\t * @note May not be null\n";
		}
		if (f.isPrimaryKey()) {
			src << "\t * @note Primary key\n";
		}
		if (f.isLower()) {
			src << "\t * @note Store as lowercase string\n";
		}
		if (f.isUnique()) {
			src << "\t * @note Unique key\n";
		}
		if (f.isForeignKey()) {
			src << "\t * @note Foreign key\n";
		}
		src << "\t */\n";

		src << "\tinline " << cpptypeGetter << " " << getter << "() const {\n";
		if (isPointer(f)) {
			src << "\t\tif (_m._isNull_" << f.name << ") {\n";
			src << "\t\t\treturn nullptr;\n";
			src << "\t\t}\n";
			if (isString(f)) {
				src << "\t\treturn _m._" << f.name << ".data();\n";
			} else {
				src << "\t\treturn &_m._" << f.name << ";\n";
			}
		} else {
			src << "\t\treturn _m._" << f.name << ";\n";
		}
		src << "\t}\n\n";

		src << "\t/**\n";
		src << "\t * @brief Set the value for '" << f.name << "' for updates and where clauses\n";
		if (f.isAutoincrement()) {
			src << "\t * @note Auto increment\n";
		}
		if (f.isIndex()) {
			src << "\t * @note Index\n";
		}
		if (f.isNotNull()) {
			src << "\t * @note May not be null\n";
		}
		if (f.isPrimaryKey()) {
			src << "\t * @note Primary key\n";
		}
		if (f.isLower()) {
			src << "\t * @param[in] " << f.name << " Store as lowercase string\n";
		}
		if (f.isUnique()) {
			src << "\t * @note Unique key\n";
		}
		if (f.isForeignKey()) {
			src << "\t * @note Foreign key\n";
		}
		src << "\t */\n";
		src << "\tinline void set" << setter << "(" << cpptypeSetter << " " << f.name << ") {\n";
		src << "\t\t_m._" << f.name << " = ";
		if (isString(f) && f.isLower()) {
			src << "core::string::toLower(" << f.name << ")";
		} else {
			src << f.name;
		}
		src << ";\n";
		src << "\t\t_m." << MembersStruct::validFieldName(f) << " = true;\n";
		if (isPointer(f)) {
			src << "\t\t_m." << MembersStruct::nullFieldName(f) << " = false;\n";
		}
		src << "\t}\n\n";

		if (f.type == persistence::FieldType::INT || f.type == persistence::FieldType::SHORT) {
			src << "\t/**\n\t * @brief Set the value for '" << f.name << "' for updates and where clauses\n\t */\n";
			src << "\ttemplate<typename T, class = typename std::enable_if<std::is_enum<T>::value>::type>\n";
			src << "\tinline void set" << setter << "(const T& " << f.name << ") {\n";
			src << "\t\tset" << setter << "(static_cast<" << cpptypeSetter << ">(static_cast<typename std::underlying_type<T>::type>(" << f.name << ")));\n";
			src << "\t}\n\n";
		}

		if (isPointer(f)) {
			src << "\t/**\n\t * @brief Set the value for '" << f.name << "' for updates and where clauses to null\n\t */\n";
			src << "\tinline void set" << setter << "(std::nullptr_t " << f.name << ") {\n";
			src << "\t\t_m." << MembersStruct::nullFieldName(f) << " = true;\n";
			src << "\t\t_m." << MembersStruct::validFieldName(f) << " = true;\n";
			src << "\t}\n\n";
		}
	}
}

void createFieldNames(const Table& table, std::stringstream& src) {
	for (auto entry : table.fields) {
		const persistence::Field& f = entry.second;
		src << "\t/**\n";
		src << "\t * @brief The column name for '" << f.name << "'\n";
		src << "\t */\n";
		src << "\tstatic constexpr const char* " << getFieldNameFunction(f) << "() {\n\t\treturn \"" << f.name << "\";\n\t}\n\n";
	}
}

bool generateClassForTable(const Table& table, std::stringstream& src) {
	src << "/**\n * @file\n */\n\n";
	src << "#pragma once\n\n";
	src << "#include \"persistence/Model.h\"\n";
	src << "#include \"persistence/DBCondition.h\"\n";
	src << "#include \"core/String.h\"\n";
	src << "#include \"core/Common.h\"\n";
	src << "\n";
	src << "#include <memory>\n";
	src << "#include <vector>\n";
	src << "#include <array>\n";
	src << "#include <string>\n\n";

	const Namespace ns(table, src);
	{
		const Class cl(table, src);

		src << "\tfriend class persistence::DBHandler;\n";
		src << "protected:\n";

		createMembersStruct(table, src);

		createMetaStruct(table, src);

		src << "public:\n";

		createConstructor(table, src);

		createGetterAndSetter(table, src);

		createFieldNames(table, src);
	}

	createDBConditions(table, src);

	return true;
}

}
