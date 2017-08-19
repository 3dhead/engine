/**
 * @file
 */

#pragma once

#include "core/App.h"
#include "compute/Types.h"
#include <simplecpp.h>
#include <vector>

/**
 * @brief This tool validates the compute shaders and generates c++ code for them.
 *
 * @li contains a C preprocessor (simplecpp/cppcheck).
 * @li detects the needed dimensions of the compute shader and generate worksizes with
 *  proper types to call the kernels.
 * @li converts OpenCL types into glm and stl types (basically just vector).
 * @li handles alignment and padding of types according to the OpenCL specification.
 * @li detects buffer flags like use-the-host-pointer(-luke) according to the alignment
 *  and size.
 * @li hides all the buffer creation/deletion mambo-jambo from the caller.
 * @li parses OpenCL structs and generate proper aligned C++ struct for them.
 */
class ComputeShaderTool: public core::App {
protected:
	std::string _namespaceSrc;
	std::string _sourceDirectory;
	std::string _shaderDirectory;
	std::string _computeFilename;
	std::string _shaderTemplateFile;
	std::string _name;

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

	std::vector<Kernel> _kernels;
	std::vector<Struct> _structs;

	const simplecpp::Token *parseKernel(const simplecpp::Token *tok);
	const simplecpp::Token *parseStruct(const simplecpp::Token *tok);
	bool parse(const std::string& src);
	void generateSrc();
	static bool validate(Kernel& kernel);
public:
	ComputeShaderTool(const io::FilesystemPtr& filesystem, const core::EventBusPtr& eventBus, const core::TimeProviderPtr& timeProvider);
	~ComputeShaderTool();

	core::AppState onRunning() override;
};
