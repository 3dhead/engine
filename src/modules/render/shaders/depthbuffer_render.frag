layout(location = 0) $out vec4 o_color;
$in vec2 v_texcoord;

uniform float u_near;
uniform float u_far;
uniform sampler2D u_depthbuffer;

float linearizedDepth(float depth) {
	float ndcz = depth * 2.0 - 1.0;
	return (2.0 * u_near * u_far) / (u_far + u_near - ndcz * (u_far - u_near));
}

void main() {
	float depth = $texture2D(u_depthbuffer, vec2(v_texcoord)).r;
	o_color = vec4(vec3(linearizedDepth(depth) / u_far), 1.0);
}
