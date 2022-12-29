#version 450 // version 4.5

// Final output color must also have location
layout(location = 0) in vec3 fragCol;
layout(location = 1) in vec2 fragUV;

layout(set = 1, binding = 0) uniform sampler2D textureSampler;

layout(location = 0) out vec4 outColor;

void main() {
    outColor = texture(textureSampler, fragUV);
}