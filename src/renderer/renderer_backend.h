#pragma once

#include "renderer_types.h"

namespace xjar {

class RendererBackend {
public:
    virtual void CreateTexture(const void *pixels, Texture *texture) = 0;
    virtual void DestroyTexture(Texture *texture) = 0;
    virtual void OnResized(u32 width, u32 height) = 0;
    virtual bool BeginFrame(f32 dt) = 0;
    virtual void EndFrame(f32 dt) = 0;

    virtual void OnInit() {}
    virtual void OnDestroy() {}
    virtual void UpdateGlobalState(const glm::mat4 &proj, const glm::mat4 &view) {}
    virtual void DrawGeometry() {}

    virtual void BeginSwapchainPass() {}
    virtual void EndSwapchainPass() {}

    virtual ~RendererBackend() = default;
};

}
