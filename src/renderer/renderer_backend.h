#pragma once

#include "renderer_types.h"

namespace xjar {

class RendererBackend {
public:
    virtual void CreateTexture(const void *pixels, Texture *texture) = 0;
    virtual void DestroyTexture(Texture *texture) = 0;
    virtual void OnResized(u32 width, u32 height) = 0;
    virtual void *BeginFrame() = 0;
    virtual void EndFrame() = 0;
    virtual void LoadModel(Model &model) = 0;

    virtual void *GetSwapchainRenderPass() { return nullptr; }
    virtual void *GetRenderDevice() { return nullptr; }
    
    virtual void OnInit() {}
    virtual void OnDestroy() {}
    virtual void UpdateGlobalState(const glm::mat4 &proj, const glm::mat4 &view) {}
    virtual void DrawGeometry() {}

    virtual void ClearColor(f32 r, f32 g, f32 b, f32 a) {}

    virtual void BeginSwapchainPass() {}
    virtual void EndSwapchainPass() {}

    virtual ~RendererBackend() = default;
};

}
