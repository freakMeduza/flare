#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) out vec4 fragColor;

layout(push_constant) uniform globalConstant {
    vec2 resolution;
	vec2 mouse;
	float time;
} global;

vec2 rotate(in vec2 uv, in float a) {
	float c = cos(a);
	float s = sin(a);
	return mat2(c, -s, s, c) * uv;
}

float cardioid(in vec2 uv, in float r) {
	float c = 0.;
	for (float i = 0.0; i < 60.0; ++i) {
		float f = (sin(global.time) * 0.5 + 0.5) + 0.3;
		i += f;
		float a = i / 5;
		float dx = 2 * r * cos(a) - r * cos(2.0 * a);
		float dy = 2 * r * sin(a) - r * sin(2.0 * a);
		c += 0.01 * f / length(uv - vec2(dx + 0.1, dy));
	}
	return c;
}

void main() {
	vec2 uv = (gl_FragCoord.xy - 0.5 * global.resolution.xy) / global.resolution.y;
	uv = rotate(uv, -1.0 * 3.14 / 2.0);

	vec3 col = vec3(0.0);
	col += cardioid(uv, 0.17);
	col *= sin(vec3(0.2, 0.8, 0.9) * global.time) * 0.15 + 0.25;

	fragColor = vec4(col, 1.0);
}