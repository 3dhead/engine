/**
 * @file
 */

#pragma once

#include "Types.h"
#include <string>

namespace util {

/**
 * @brief convert the given input string into lower- or upper-camel-case
 * @param in The string to convert
 * @param firstUpper Convert to upper camel case
 * @return The camel case string
 */
extern std::string convertName(const std::string& in, bool firstUpper);

extern std::string uniformSetterPostfix(const Variable::Type type, int amount);

/**
 * @return @c video::ImageFormat::Max if no mapping was found
 */
extern video::ImageFormat getImageFormat(const std::string& type, int line);

/**
 * @return @c nullptr if no mapping was found
 */
extern const char* getImageFormatGLType(video::ImageFormat format);

extern int getComponents(const Variable::Type type);

extern Variable::Type getType(const std::string& type, int line);

extern std::string std140Align(const Variable& v);

extern std::string std140Padding(const Variable& v, int& padding);

extern size_t std140Size(const Variable& v);

extern std::string std430Align(const Variable& v);

extern size_t std430Size(const Variable& v);

extern std::string std430Padding(const Variable& v, int& padding);

extern const Types& resolveTypes(Variable::Type type);

}
