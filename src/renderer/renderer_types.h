#pragma once

#include <glm/vec3.hpp>
#include <glm/vec2.hpp>
#include <glm/mat4x4.hpp>

#include <vector>   
#include "types.h"

#include "resource_types.h"

namespace xjar {

static constexpr u32 MAX_LODS = 8;
static constexpr u32 MAX_STREAMS = 8;

// universal structure to keep the relevant data for frame
struct FrameStatus {
    b32   success;

    // Vulkan stuff
    void *commandBuffer;
    u32   currentImage;
};

struct GPU_SceneData {
    glm::mat4 viewMat;
    glm::mat4 projMat;

    glm::vec4 ambientColor;
    glm::vec4 sunlightDir;
    glm::vec4 sunlightColor;
};

struct Vertex {
    glm::vec3 pos;
    glm::vec3 norm;
    glm::vec2 texcoord;
};

struct Vertex3D {
    glm::vec3 position;
    glm::vec2 texcoord;
};

struct Mesh {
    u32 lodNum;
    u32 streamNum;
    u32 materialID;
    u32 meshSize;
    u32 vertexCount;
    u32 indexOffset;
    u32 vertexOffset;
    u32 lodOffset[MAX_LODS];
    u64 streamOffset[MAX_STREAMS];
    u32 streamElementSize[MAX_STREAMS];

    inline u64 LodSize(u32 lod) const {
        return lodOffset[lod + 1] - lodOffset[lod];
    }
};

struct MeshHdr {
    u32 magicValue;
    u32 meshNum;
    u32 dataStartOffset;
    u32 indexDataSize;
    u32 vertexDataSize;
};

struct Material {
    Texture *diffuseTexture;
};

struct TriangleMesh {
    std::vector<u32> indexData;
    std::vector<f32> vertexData;
    std::vector<Mesh> meshes;
};

struct InstanceData {
    u32 meshIndex;
    u32 materialIndex;
    u32 LOD;
    u32 indexOffset;
    u32 vertexOffset;
    u32 transformIndex;
};

struct Model {
    Texture      texture;
    glm::mat4    localTransform;
    TriangleMesh mesh;
    void        *handle; // the actual handle to the mesh with vao, vbo, ebo
};

struct UniformSceneObject {
    glm::mat4 view;
    glm::mat4 projection;
};

struct PushConstantData {
    glm::mat4 model;
};

}
