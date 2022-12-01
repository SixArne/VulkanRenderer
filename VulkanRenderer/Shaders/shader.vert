#version 450 // version 4.5

layout(location = 0) in vec3 pos;
layout(location = 1) in vec3 col;

layout(location = 0) out vec3 fragCol;

void main() {
    // gl_VertexIndex keeps track like a static var
    gl_Position = vec4(pos, 1.0);
    fragCol = col;
}