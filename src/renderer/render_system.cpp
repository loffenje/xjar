#include "pch.h"
#include "render_system.h"
#include "resource_types.h"

#include "renderer_backend.h"

#if RENDERER_BACKEND == OpenGL
#include "gl/opengl_backend.h"
#include "gl/opengl_test_feature.h"
#else
#include "vk/vulkan_backend.h"
#include "vk/vulkan_multimesh_feature.h"
#endif

#include "texture_manager.h"
#include "window.h"

namespace xjar {

static RendererBackend *g_backend = nullptr;


RenderSystem &RenderSystem::Instance() {
    static RenderSystem system;

    return system;
}

void RenderSystem::Startup() {
#if RENDERER_BACKEND == OpenGL
    g_backend = new OpenGL_Backend;
#else
    g_backend = new Vulkan_Backend;
    
#endif

    g_backend->OnInit();
}

void RenderSystem::Shutdown() {
    g_backend->OnDestroy();
    delete g_backend;
}


void RenderSystem::LoadInstanceData(const char *filename, std::vector<InstanceData> &instances) {
    FILE *file = fopen(filename, "rb");
    if (!file) {
        fprintf(stderr, "Failed to load instance %s\n", filename);
        exit(1);
    }

    fseek(file, 0, SEEK_END);
    size_t fsize = ftell(file);
    fseek(file, 0, SEEK_SET);
    u32 maxInstanceCount = static_cast<u32>(fsize / sizeof(InstanceData));

    instances.resize(maxInstanceCount);

    if (fread(instances.data(), sizeof(InstanceData), maxInstanceCount, file) != maxInstanceCount) {
        fprintf(stderr, "Unable to read instance data\n");
        exit(1);
    }

    fclose(file);
}

void RenderSystem::LoadModel(const char *meshFilename, const char *instanceFilename, const char *materialFilename, Model &model) {

    FILE *file = fopen(meshFilename, "rb");
    if (!file) {
        fprintf(stderr, "Failed to load mesh %s\n", meshFilename);
        exit(1);
    }

    MeshHdr hdr;
    if (fread(&hdr, 1, sizeof(hdr), file) != sizeof(hdr)) {
        fprintf(stderr, "Unable to read %s file\n", meshFilename);
        exit(1);
    }

    const u32 meshNum = hdr.meshNum;
    model.mesh.meshes.resize(meshNum);

    if (fread(model.mesh.meshes.data(), sizeof(Mesh), meshNum, file) != meshNum) {
        fprintf(stderr, "Unable to read meshes from %s file\n", meshFilename);
        exit(1);
    }

    const u32 indexDataSize = hdr.indexDataSize;
    const u32 vertexDataSize = hdr.vertexDataSize;
    model.mesh.indexData.resize(indexDataSize / sizeof(u32));
    model.mesh.vertexData.resize(vertexDataSize / sizeof(f32));

    if (fread(model.mesh.indexData.data(), 1, indexDataSize, file) != indexDataSize ||
        fread(model.mesh.vertexData.data(), 1, vertexDataSize, file) != vertexDataSize) {
        fprintf(stderr, "Unable to read geometry\n");
        exit(1);
    }

    std::vector<InstanceData> instances;
    LoadInstanceData(instanceFilename, instances);

    std::vector<std::string> textureFilenames;
    std::vector<MaterialDescr> materials;
    LoadMaterials(materialFilename, materials, textureFilenames);

    g_backend->CreateModel(instances, materials, textureFilenames, model);
}

void RenderSystem::LoadMaterials(const char *fileName, std::vector<MaterialDescr> &materials, std::vector<std::string> &files) {
    FILE *f = fopen(fileName, "rb");
    if (!f) {
        fprintf(stderr, "Failed to load materials %s\n", fileName);
        exit(1);
    }

    u32 size;
    fread(&size, sizeof(u32), 1, f);
    materials.resize(size);
    fread(materials.data(), sizeof(MaterialDescr), materials.size(), f);

    {
        u32 size = 0;
        fread(&size, sizeof(uint32_t), 1, f);
        files.resize(size);
    }

    std::vector<char> inBytes;
    for (auto &s : files) {
        u32 size = 0;
        fread(&size, sizeof(u32), 1, f);
        inBytes.resize(size + 1);
        fread(inBytes.data(), size + 1, 1, f);
        s = std::string(inBytes.data());
    }

    fclose(f);
}
void RenderSystem::DrawEntities(FrameStatus frame, GPU_SceneData *sceneData, std::initializer_list<Entity *> entities) {
    g_backend->DrawEntities(frame, sceneData, entities);
}

void RenderSystem::DrawGrid(FrameStatus frame, GPU_SceneData *sceneData) {
    g_backend->BeginGridPass(frame);
    g_backend->DrawGrid(frame, sceneData);
    g_backend->EndGridPass(frame);
}

void RenderSystem::CreateTexture(const void *pixels, Texture *texture) {
    g_backend->CreateTexture(pixels, texture);
}

void RenderSystem::DestroyTexture(Texture *texture) {
    g_backend->DestroyTexture(texture);
}

void RenderSystem::OnResized(u32 width, u32 height) {
    g_backend->OnResized(width, height);
}

void RenderSystem::ClearColor(FrameStatus frame, f32 r, f32 g, f32 b, f32 a) {
    g_backend->ClearColor(frame, r, g, b, a);
}

FrameStatus RenderSystem::BeginFrame() {
    return g_backend->BeginFrame();
}

void RenderSystem::EndFrame() {
    g_backend->EndFrame();
}

}
