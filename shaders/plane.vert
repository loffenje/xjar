#version 460

#extension GL_EXT_shader_explicit_arithmetic_types_int64: enable

layout(location = 0) out vec3 outUVW;
layout(location = 2) out vec3 outNormal;
layout(location = 3) out vec3 outFragPos;

struct ImDrawVert   { 
    float x, y, z;
    float nx, ny, nz;
    float u, v;
};

layout(push_constant) uniform PushConstantData {
    mat4 model;
} push;

layout(binding = 0) uniform  UniformBuffer { 
    mat4 view;
    mat4 projection;
    vec3 viewPos;
} ubo;

layout(binding = 1) readonly buffer SBO    { ImDrawVert data[]; } sbo;

void main()
{
    
	ImDrawVert v = sbo.data[gl_VertexIndex];
    
    vec3 pos = vec3(v.x, v.y, v.z);

	gl_Position = ubo.projection * ubo.view * push.model * vec4(pos, 1.0);
    outUVW = vec3(v.u, v.v, 1.0);
    outNormal = mat3(transpose(inverse(push.model))) * vec3(v.nx, v.ny, v.nz);
    outFragPos = vec3(push.model * vec4(pos, 1.0f));
}

