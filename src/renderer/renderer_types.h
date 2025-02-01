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
    void *defaultPass;
    void *multimeshPass;
    u32   currentImage;
};

struct GPU_SceneData {
    alignas(16) glm::mat4 viewMat;
    alignas(16)  glm::mat4 projMat;
    alignas(16)  glm::vec3 viewPos;
};

static_assert(sizeof(GPU_SceneData) % 16 == 0, "GPU_SceneData should be padded to 16 bytes");

struct Vertex {
    glm::vec3 pos;
    glm::vec3 norm;
    glm::vec2 texcoord;

    Vertex() = default;
    Vertex (f32 x, f32 y, f32 z,
                f32 nx, f32 ny, f32 nz,
                f32 uvx, f32 uvy):
        pos{x, y, z},
        norm{nx, ny, nz},
        texcoord{uvx, uvy} {}
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

struct ModelID {
    int modelIndex;
    int instanceCount;
};
struct Model {
    Texture      texture;
    glm::mat4    localTransform;
    TriangleMesh mesh;
    void        *handle; // the actual handle to the mesh with vao, vbo, ebo
};

struct PushConstantData {
    glm::mat4 model;
};

}
