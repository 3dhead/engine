/**
 * @file
 */

#pragma once

#include "video/Camera.h"
#include "core/Color.h"
#include "math/Plane.h"
#include "video/ShapeBuilder.h"
#include "render/ShapeRenderer.h"
#include "core/IComponent.h"

namespace render {

/**
 * @brief Renders a plane
 *
 * @see video::ShapeBuilder
 * @see ShapeRenderer
 */
class Plane : public core::IComponent {
private:
	video::ShapeBuilder _shapeBuilder;
	render::ShapeRenderer _shapeRenderer;
	int32_t _planeMesh = -1;
public:
	void render(const video::Camera& camera, video::Shader* shader = nullptr);

	void render(const video::Camera& camera, const glm::mat4& model, video::Shader* shader = nullptr);

	void shutdown() override;

	/**
	 * @param[in] position The offset that should be applied to the center of the plane
	 * @param[in] tesselation The amount of splits on the plane that should be made
	 * @param[in] scale The vertices are in the normalized coordinate space between -0.5 and 0.5 - we have to scale them up to the size we need
	 * @param[in] color The color of the plane.
	 */
	bool init() override;

	bool plane(const glm::vec3& position, int tesselation = 0, float scale = 100.0f, const glm::vec4& color = core::Color::White);

	bool plane(const glm::vec3& position, const math::Plane& plane, const glm::vec4& color = core::Color::White);
};

}
