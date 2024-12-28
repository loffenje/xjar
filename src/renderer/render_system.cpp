#include "types.h"
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
    
    meshFeature = new Vulkan_MultiMeshFeature;
#endif

    g_backend->OnInit();

    meshFeature->Init(g_backend->GetRenderDevice(), g_backend->GetSwapchain());
}

void RenderSystem::Shutdown() {
    g_backend->OnDestroy();
    delete g_backend;
}


void RenderSystem::LoadModel(const char *meshFilename, const char *instanceFilename, Model &model) {

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
