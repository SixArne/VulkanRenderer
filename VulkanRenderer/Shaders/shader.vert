#version 450 // version 4.5

// Output color for vertex, location is required
layout(location = 0) out vec3 fragColor;

// Triangle vertex positions will put in vertex buffer later
vec3 positions[3] = vec3[] (
    vec3(0.0, -0.4, 0.0),
    vec3(0.4, 0.4, 0.0),
    vec3(-0.0, 0.4, 0.0)
);

// Triangle vertex color
vec3 colors[3] = vec3[] (
    vec3(1.0, 0.0, 0.0),
    vec3(0.0, 1.0, 0.0),
    vec3(0.0, 0.0, 1.0)
);

void main() {
    // gl_VertexIndex keeps track like a static var
    gl_Position = vec4(positions[gl_VertexIndex], 1.0);
    fragColor = colors[gl_VertexIndex];
}