#pragma once

#include <glm/mat4x4.hpp>
#include "renderer/resource_types.h"
#include "renderer/renderer_types.h"

namespace xjar {

class RendererSystem final {
public:
    static RendererSystem &Instance();

    RendererSystem(const RendererSystem &) = delete;
    RendererSystem &operator=(const RendererSystem &) = delete;

    void Startup();
    void OnResized(u32 width, u32 height);
    void DrawFrame(f32 dt);
    void Shutdown();
    void SetView(const glm::mat4 &view);
    void CreateTexture(const void *pixels, Texture *texture);
    void DestroyTexture(Texture *texture);

private:
    RendererSystem() = default;
};

}
