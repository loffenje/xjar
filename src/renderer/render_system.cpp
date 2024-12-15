#include "types.h"
#include "render_system.h"
#include "resource_types.h"

#include "renderer_backend.h"

#if RENDERER_BACKEND == OpenGL
#include "gl/opengl_backend.h"
#include "gl/opengl_test_feature.h"
#else
#include "vk/vulkan_backend.h"
#include "vk/vulkan_test_feature.h"
#endif

#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
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
    
    testFeature = new OpenGL_TestFeature;
#else
    g_backend = new Vulkan_Backend;
    
    testFeature = new Vulkan_TestFeature;
#endif

    g_backend->OnInit();

    testFeature->Init(g_backend->GetRenderDevice(), g_backend->GetDefaultRenderPass());
}

void RenderSystem::Shutdown() {
    g_backend->OnDestroy();
    delete g_backend;
}

void RenderSystem::LoadModel(const char *filename, Model &model) {
    FILE *file = fopen(filename, "rb");
    if (!file) {
        fprintf(stderr, "Failed to load model %s\n", filename);
        exit(1);
    }

    MeshHdr hdr;
    if (fread(&hdr, 1, sizeof(hdr), file) != sizeof(hdr)) {
        fprintf(stderr, "Unable to read %s file\n", filename);
        exit(1);
    }

    const u32 meshNum = hdr.meshNum;
    std::vector<MeshFormat> meshes;
    meshes.resize(meshNum);

    if (fread(meshes.data(), sizeof(MeshFormat), meshNum, file) != meshNum) {
        fprintf(stderr, "Unable to read meshes from %s file\n", filename);
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

    g_backend->CreateMesh(model);
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

void RenderSystem::BeginDefaultPass(const GPU_SceneData &sceneData) {
    g_backend->BeginDefaultPass();
    g_backend->UpdateGlobalState(sceneData);
}

void RenderSystem::EndDefaultPass() {
    g_backend->EndDefaultPass();
}

FrameStatus RenderSystem::BeginFrame() {
    return g_backend->BeginFrame();
}

void RenderSystem::EndFrame() {
    g_backend->EndFrame();
}

}
