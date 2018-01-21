/**
 * @file
 */

#include "GBuffer.h"
#include "ScopedFrameBuffer.h"
#include "Types.h"

#include <stddef.h>
#include "core/Common.h"
#include "core/Assert.h"

namespace video {

GBuffer::GBuffer() :
		_fbo(InvalidId), _depthTexture(InvalidId) {
	for (std::size_t i = 0; i < SDL_arraysize(_textures); ++i) {
		_textures[i] = InvalidId;
	}
}

GBuffer::~GBuffer() {
	core_assert_msg(_fbo == InvalidId, "GBuffer was not properly shut down");
	shutdown();
}

void GBuffer::shutdown() {
	video::deleteFramebuffer(_fbo);
	const int texCount = (int)SDL_arraysize(_textures);
	video::deleteTextures(texCount, _textures);
	video::deleteTexture(_depthTexture);
}

bool GBuffer::init(const glm::ivec2& dimension) {
	_fbo = video::genFramebuffer();

	// +1 for the depth texture
	const int texCount = (int)SDL_arraysize(_textures);
	video::genTextures(texCount + 1, _textures);

	return video::setupGBuffer(_fbo, dimension, _textures, SDL_arraysize(_textures), _depthTexture);
}

void GBuffer::bindForWriting() {
	video::bindFramebuffer(FrameBufferMode::Draw, _fbo);
}

void GBuffer::bindForReading(bool gbuffer) {
	if (gbuffer) {
		bindFramebuffer(FrameBufferMode::Read, _fbo);
		return;
	}

	bindFramebuffer(FrameBufferMode::Draw, InvalidId);

	// activate the textures to read from
	const video::TextureUnit texUnits[] = { TextureUnit::Zero, TextureUnit::One, TextureUnit::Two };
	static_assert(SDL_arraysize(texUnits) == SDL_arraysize(_textures), "texunits and textures don't match");
	for (int i = 0; i < (int) SDL_arraysize(_textures); ++i) {
		video::bindTexture(texUnits[i], video::TextureType::Texture2D, _textures[i]);
	}
	video::activeTextureUnit(video::TextureUnit::Zero);
}

void GBuffer::unbind() {
	bindFramebuffer(FrameBufferMode::Default, InvalidId);
}

void GBuffer::setReadBuffer(GBufferTextureType textureType) {
	video::readBuffer(textureType);
}

}
