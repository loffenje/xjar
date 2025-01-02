#version 460

#extension GL_EXT_shader_explicit_arithmetic_types_int64: enable

layout(location = 0) out vec3 outUVW;
layout(location = 1) out flat uint outMatIndex;

struct ImDrawVert   { 
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

struct MaterialData { uint tex2D; };

layout(push_constant) uniform PushConstantData {
    mat4 model;
} push;

layout(binding = 0) uniform  UniformBuffer { 
    mat4 view;
    mat4 projection;
} ubo;
layout(binding = 1) readonly buffer SBO    { ImDrawVert data[]; } sbo;
layout(binding = 2) readonly buffer IBO    { uint   data[]; } ibo;
layout(binding = 3) readonly buffer InstanceBO { InstanceData data[]; } instanceDataBuffer;

void main()
{
	InstanceData instance = instanceDataBuffer.data[gl_BaseInstance];

	uint refIdx = instance.indexOffset + gl_VertexIndex;
	ImDrawVert v = sbo.data[ibo.data[refIdx] + instance.vertexOffset];

	gl_Position = ubo.projection * ubo.view * push.model * vec4(v.x, v.y, v.z, 1.0);
    outMatIndex = instance.material;
    outUVW = vec3(v.u, v.v, 1.0);
}

