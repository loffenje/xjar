#version 460

layout (location = 0) out vec2 uv;
layout (location = 1) out vec2 cameraPos;

// Grid position are in xy clipped space
vec3 gridPlane[6] = vec3[](
    vec3(1, 1, 0), vec3(-1, -1, 0), vec3(-1, 1, 0),
    vec3(-1, -1, 0), vec3(1, 1, 0), vec3(1, -1, 0)
);
const vec3 positions[4] = vec3[4](
    vec3(-1.0, 0.0, -1.0),
    vec3(1.0, 0.0, -1.0),
    vec3(1.0, 0.0, 1.0),
    vec3(-1.0, 0.0, 1.0));

const int indices[6] = int[6](0,1,2,2,3,0);

layout(binding = 0) uniform  UniformBuffer { 
    mat4 view;
    mat4 projection;
} ubo;

// extents of grid in world coordinates
float gridSize = 100.0;

void main() {
    float gridSize = 100.0;
    vec3 pos = positions[indices[gl_VertexIndex]] * gridSize;

    mat4 iview = inverse(ubo.view);
    cameraPos = vec2(iview[3][0], iview[3][2]);

    pos.x += cameraPos.x;
    pos.z += cameraPos.y;

    gl_Position = ubo.projection * ubo.view * mat4(1.0f) * vec4(pos, 1.0);
    uv = pos.xz;
}
