#version 460

#extension GL_EXT_shader_explicit_arithmetic_types_int64 : enable

struct ImDrawVert {
    float x, y, z;
    float u, v;
    float nx, ny, nz;
};

struct InstanceData {
    uint mesh;
    uint material;
    uint lod;
    uint indexOffset;
    uint vertexOffset;
    uint transformIndex;
};

layout(binding = 0) uniform UniformBuffer {
    mat4 depthMVP;
} ubo;

layout(binding = 1) readonly buffer SBO {
    ImDrawVert data[];
} sbo;

layout(binding = 2) readonly buffer IBO {
    uint data[];
} ibo;

layout(binding = 3) readonly buffer InstanceBO {
    InstanceData data[];
} instanceDataBuffer;


void main() {
    InstanceData instance = instanceDataBuffer.data[gl_BaseInstance];

    uint       refIdx = instance.indexOffset + gl_VertexIndex;
    ImDrawVert v = sbo.data[ibo.data[refIdx] + instance.vertexOffset];

    vec3 pos = vec3(v.x, v.y, v.z);
    
    gl_Position = ubo.depthMVP * vec4(pos, 1.0);
}
