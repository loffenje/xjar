#version 460

#extension GL_EXT_shader_explicit_arithmetic_types_int64: enable
#extension GL_EXT_nonuniform_qualifier : require

struct MaterialData
{
    vec4 albedoColor;
    vec4 emissiveColor;
    uint64_t albedoMap;
    uint64_t normalMap;
};

layout(location = 0) in vec3 uvw;
layout(location = 1) in flat uint matIndex;

layout(location = 0) out vec4 FragColor;

layout(binding = 4) readonly buffer MatBO { MaterialData   data[]; } mat_bo;
layout(binding = 5) uniform sampler2D textures[];


void main()
{
    MaterialData matData = mat_bo.data[matIndex];
    vec4 albedo = matData.albedoColor;

    const int INVALID_HANDLE = 2000;

    if (matData.albedoMap < INVALID_HANDLE) {
        uint texIndex = uint(matData.albedoMap);
        albedo = texture(textures[nonuniformEXT(texIndex)], uvw.xy);
    }

    FragColor = vec4(albedo.rgb, 1.0);
}

