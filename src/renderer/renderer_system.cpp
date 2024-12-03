#include "types.h"
#include "renderer_system.h"
#include "resource_types.h"

#include "renderer_backend.h"

#if RENDERER_BACKEND == OpenGL
#include "gl/opengl_backend.h"
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

struct RendererState {
    glm::mat4 view;
    f32       nearClip;
    f32       farClip;
    f32       zoom;
};

static RendererState g_state;

RendererSystem &RendererSystem::Instance() {
    static RendererSystem system;

    return system;
}

void RendererSystem::Startup() {
#if RENDERER_BACKEND == OpenGL
    g_backend = new OpenGL_Backend;
#else
    g_backend = new Vulkan_Backend;
    
    testFeature = new Vulkan_TestFeature;
#endif

    g_backend->OnInit();

    testFeature->Init(g_backend->GetRenderDevice(), g_backend->GetSwapchainRenderPass());

    g_state.nearClip = 0.1f;
    g_state.farClip = 1000.0f;
    g_state.zoom = 45.0f;
}

void RendererSystem::Shutdown() {
    g_backend->OnDestroy();
    delete g_backend;
}

void RendererSystem::SetView(const glm::mat4 &view) {
    g_state.view = view;
}

void RendererSystem::LoadModel(Model &model) {
    g_backend->LoadModel(model);
}

void RendererSystem::CreateTexture(const void *pixels, Texture *texture) {
    g_backend->CreateTexture(pixels, texture);
}

void RendererSystem::DestroyTexture(Texture *texture) {
    g_backend->DestroyTexture(texture);
}

void RendererSystem::OnResized(u32 width, u32 height) {
    g_backend->OnResized(width, height);
}

void RendererSystem::BeginDefaultPass() {
    g_backend->BeginSwapchainPass();
}

void RendererSystem::EndDefaultPass() {
    g_backend->EndSwapchainPass();
}

void *RendererSystem::BeginFrame() {
    return g_backend->BeginFrame();
}

void RendererSystem::EndFrame() {
    g_backend->EndFrame();
}

}
