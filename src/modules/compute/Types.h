/**
 * @file
 */

#pragma once

#include "core/Common.h"

namespace compute {

/**
 * @brief Compute shader buffer flags
 */
enum class BufferFlag {
	None = 0,
	/** default */
	ReadWrite = 1,
	WriteOnly = 2,
	ReadOnly = 4,
	/**
	 * Use this when a buffer is already allocated as page-aligned with
	 * Shader::bufferAlloc() instead of @c malloc/new and a size that is a multiple of 64 bytes
	 * Also make sure to use Shader::bufferFree() instead of @c free/delete[]
	 */
	UseHostPointer = 8,
	/**
	 * when the data will be generated on the device but may be read back on the host.
	 * In this case leverage this flag to create the data
	 */
	AllocHostPointer = 16,
	CopyHostPointer = 32,

	Max = 7
};
CORE_ENUM_BIT_OPERATIONS(BufferFlag)

}
