#version 450 // version 4.5

// Final output color must also have location
layout(location = 0) in vec3 fragCol;

layout(location = 0) out vec4 outColor;

void main() {
    outColor = vec4(fragCol, 1.0);
}