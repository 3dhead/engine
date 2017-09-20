/**
 * @file
 */

#pragma once

#include "compute/Types.h"
#include <string>

namespace computeshadertool {

struct Parameter {
	std::string qualifier;
	std::string type;
	std::string name;
	std::string comment;
	bool byReference = false;
	compute::BufferFlag flags = compute::BufferFlag::ReadWrite;
};

struct ReturnValue {
	std::string type;
};

struct Kernel {
	std::string name;
	std::vector<Parameter> parameters;
	int workDimension = 1;
	ReturnValue returnValue;
};

struct Struct {
	std::string comment;
	std::string name;
	std::vector<Parameter> parameters;
};

}
