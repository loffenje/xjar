#pragma once

#include "resource_types.h"
#include "renderer_types.h"

namespace xjar {

class TestFeature;

class RendererSystem final {
public:
    static RendererSystem &Instance();

    RendererSystem(const RendererSystem &) = delete;
    RendererSystem &operator=(const RendererSystem &) = delete;

    void Startup();
    void OnResized(u32 width, u32 height);
    void *BeginFrame();
    void EndFrame();
    void BeginDefaultPass();
    void EndDefaultPass();
    void Shutdown();
    void LoadModel(Model &model);
    void CreateTexture(const void *pixels, Texture *texture);
    void DestroyTexture(Texture *texture);

    TestFeature *testFeature;
private:
    RendererSystem() = default;

};

}
