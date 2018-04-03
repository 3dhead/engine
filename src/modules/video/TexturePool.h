/**
 * @file
 */

#pragma once

#include "core/IComponent.h"
#include "io/Filesystem.h"
#include "Texture.h"
#include <memory>
#include <string>
#include <unordered_map>

namespace video {

/**
 * @ingroup Video
 */
class TexturePool : public core::IComponent {
private:
	io::FilesystemPtr _filesystem;
	std::unordered_map<std::string, TexturePtr> _cache;
public:
	TexturePool(const io::FilesystemPtr& filesystem);

	video::TexturePtr load(const std::string& name);

	bool init() override;
	void shutdown() override;
};

typedef std::shared_ptr<TexturePool> TexturePoolPtr;

}
