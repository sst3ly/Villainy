#version 450

layout(location = 0) in vec2 inPos;
layout(location = 1) in vec3 inColor;

layout(location = 0) out vec3 fragColor;

layout(set = 0, binding = 0) uniform DummyUbo {
    vec4 dummy;
} ubo;

void main() {
    gl_Position = vec4(inPos, 0.0, 1.0);
    fragColor = inColor + 0.0 * ubo.dummy.xyz;
}
