#version 450 // version 4.5

// Interpolated color from vertex, location must match
layout(location = 0) in vec3 fragColor;

// Final output color must also have location
layout(location = 0) out vec4 outColor;

void main() {
    outColor = vec4(fragColor, 1.0);
}