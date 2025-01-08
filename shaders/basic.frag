#version 460

#extension GL_EXT_shader_explicit_arithmetic_types_int64: enable
#extension GL_EXT_nonuniform_qualifier : require

struct MaterialData
{
    vec4 albedoColor;
    vec4 emissiveColor;
    uint64_t diffuseMap;
    uint64_t normalMap;
    uint64_t specularMap;
};

layout(location = 0) in vec3 uvw;
layout(location = 1) in flat uint matIndex;
layout(location = 2) in vec3 normal;
layout(location = 3) in vec3 fragPos;

layout(location = 0) out vec4 FragColor;

layout(binding = 4) readonly buffer MatBO { MaterialData   data[]; } mat_bo;
layout(binding = 5) uniform sampler2D textures[];


void main()
{

    const vec3 lightPos = vec3(1.0f, 1.2f, 2.0f);
    const vec3 lightColor = vec3(1.0f, 1.0f, 1.0f);
    
    const vec3 ambientColor = vec3(0.2f, 0.2f, 0.2f);
    const vec3 diffuseColor = vec3(0.5f, 0.5f, 0.5f);
    const vec3 specularColor = vec3(1.0f, 1.0f, 1.0f);


    MaterialData matData = mat_bo.data[matIndex];
    vec4 albedo = matData.albedoColor;
    vec4 specular = vec4(0,0,0,0);

    const int INVALID_HANDLE = 2000;

    if (matData.diffuseMap < INVALID_HANDLE) {
        uint texIndex = uint(matData.diffuseMap);
        albedo = texture(textures[nonuniformEXT(texIndex)], uvw.xy);
    }
    
    if (matData.specularMap < INVALID_HANDLE) {
        uint texIndex = uint(matData.specularMap);
        albedo = texture(textures[nonuniformEXT(texIndex)], uvw.xy);
    }

    //ambient
    vec3 ambient = ambientColor * albedo.rgb;

    // diffuse
    vec3 norm = normalize(normal);
    vec3 lightDir = normalize(lightPos - fragPos);
    float diffuseStrength = max(dot(lightDir, norm), 0.0f);
    vec3 diffuse = diffuseStrength * diffuseColor * albedo.rgb;

    // specular

    FragColor = vec4(ambient + diffuse, 1.0);
}

