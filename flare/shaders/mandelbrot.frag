#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) out vec4 fragColor;

layout(push_constant) uniform globalConstant {
    vec2 resolution;
	float time;
} global;

float mandelbrot(in vec2 uv) {
    const float MAX_STEPS = 256.;
    
    vec2 c = 2.3*uv - vec2(0.5, 0.0);
    vec2 z = vec2(0.);

    for (float i = 0.; i < MAX_STEPS; ++i) {
        z = vec2(z.x*z.x - z.y*z.y, 2.*z.x*z.y) + c;
        if (length(z) > 2.) return i/MAX_STEPS;
    }

    return 0.;
}

void main() {
	vec2 uv = (gl_FragCoord.xy - 0.5 * global.resolution.xy) / global.resolution.y;

    vec3 col = vec3(0);
    col += mandelbrot(uv);
	col *= sin(vec3(0.2, 0.8, 0.9) * global.time) * 0.5 + 0.5;

	fragColor = vec4(col, 1.0);
}