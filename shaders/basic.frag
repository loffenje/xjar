#version 460

#extension GL_EXT_shader_explicit_arithmetic_types_int64 : enable
#extension GL_EXT_nonuniform_qualifier : require

struct MaterialData {
    vec4     albedoColor;
    vec4     emissiveColor;
    uint64_t diffuseMap;
    uint64_t specularMap;
    uint64_t normalMap;
    uint64_t padding;
};

layout(location = 0) in vec3 inUVW;
layout(location = 1) in flat uint inMatIndex;
layout(location = 2) in vec3 inNormal;
layout(location = 3) in vec3 inFragPos;
layout(location = 4) in vec4 inFragLightSpacePos;

layout(location = 0) out vec4 FragColor;

layout(binding = 0) uniform UniformBuffer {
    mat4 view;
    mat4 projection;
    mat4 lightSpaceMat;
    vec3 viewPos;
} ubo;

layout(binding = 4) readonly buffer MatBO {
    MaterialData data[];
} mat_bo;

layout(binding = 5) uniform sampler2D textures[];
layout(binding = 6) uniform sampler2D shadowMap;

float CalculateShadows(vec4 fragLightSpacePos) {
    // for ortho is meaningless, but for perspective is required, leave it to ensure it works for both projections
    vec3 projectedCoords = fragLightSpacePos.xyz / fragLightSpacePos.w;
    projectedCoords = projectedCoords * 0.5 + 0.5;
    float closestDepth = texture(shadowMap, projectedCoords.xy).r;
    float depth = projectedCoords.z;
    float shadow = depth > closestDepth ? 1.0 : 0.0;

    return shadow;
}

void main() {
    //TODO: move it to uniform
    const vec3 lightPos = vec3(-2.0f, 4.0f, 1.0f);

    const vec3 ambientColor = vec3(0.05f);
    const vec3 specularColor = vec3(0.3f);

    MaterialData matData = mat_bo.data[inMatIndex];
    vec4         diffuseMap = matData.albedoColor;
    vec4         specularMap = matData.albedoColor;

    const int INVALID_HANDLE = 2000;

    if (matData.diffuseMap < INVALID_HANDLE) {
        uint texIndex = uint(matData.diffuseMap);
        diffuseMap = texture(textures[nonuniformEXT(texIndex)], inUVW.xy);
    }

    if (matData.specularMap < INVALID_HANDLE) {
        uint texIndex = uint(matData.specularMap);
        specularMap = texture(textures[nonuniformEXT(texIndex)], inUVW.xy);
    }

    //ambient
    vec3 ambient = ambientColor * diffuseMap.rgb;

    // diffuse
    vec3  norm = normalize(inNormal);
    vec3  lightDir = normalize(lightPos - inFragPos);
    float diffuseStrength = max(dot(lightDir, norm), 0.0f);
    vec3  diffuse = diffuseStrength * diffuseMap.rgb;

    // specular
    const bool blin = true;
    vec3       viewDir = normalize(ubo.viewPos - inFragPos);
    float      specularIntensity = 0.0f;
    if (blin) {
        vec3 halfwayDir = normalize(viewDir + lightDir);
        specularIntensity = pow(max(dot(inNormal, halfwayDir), 0.0), 16.0);
    } else {
        vec3 reflectDir = reflect(-lightDir, norm);
        specularIntensity = pow(max(dot(viewDir, reflectDir), 0.0), 8.0);
    }

    float shadow = CalculateShadows(inFragLightSpacePos);
    vec3  specular = specularColor * specularIntensity * specularMap.rgb;

    vec3 finalLighting = (ambient + (1.0 - shadow) * (diffuse + specular));

    FragColor = vec4(finalLighting, 1.0);
}

