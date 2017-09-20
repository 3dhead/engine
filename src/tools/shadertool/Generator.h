/**
 * @file
 */

#pragma once

#include "io/Filesystem.h"
#include "Types.h"
#include <string>

namespace shadertool {

extern bool generateSrc(const std::string& templateShader, const std::string& templateUniformBuffer, const ShaderStruct& shaderStruct,
		const io::FilesystemPtr& filesystem, const std::string& namespaceSrc, const std::string& sourceDirectory, const std::string& shaderDirectory);

}
