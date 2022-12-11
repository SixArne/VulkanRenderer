#version 450 // version 4.5

layout(location = 0) in vec3 pos;
layout(location = 1) in vec3 col;

layout(binding = 0) uniform UboViewProjection {
    mat4 projection;
    mat4 view;
} uboViewProjection;

layout(binding = 1) uniform UboModel {
    mat4 model;
} uboModel;

layout(location = 0) out vec3 fragCol;

void main() {
    // gl_VertexIndex keeps track like a static var
    gl_Position = uboViewProjection.projection * uboViewProjection.view * uboModel.model * vec4(pos, 1.0);
    fragCol = col;
}