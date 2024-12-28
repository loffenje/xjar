#pragma once

#include "resource_types.h"
#include "renderer_types.h"

namespace xjar {

class MeshFeature;

class RenderSystem final {
public:
    static RenderSystem &Instance();

    RenderSystem(const RenderSystem &) = delete;
    RenderSystem &operator=(const RenderSystem &) = delete;

    void        Startup();
    void        OnResized(u32 width, u32 height);
    FrameStatus BeginFrame();
    void        EndFrame();
    void        BeginDefaultPass(const GPU_SceneData &sceneData);
    void        EndDefaultPass();
    void        Shutdown();
    void        LoadModel(const char *meshFilename, const char *instanceFilename, Model &model);
    void        CreateTexture(const void *pixels, Texture *texture);
    void        DestroyTexture(Texture *texture);

    MeshFeature *meshFeature;

private:
    void LoadMesh(const char *filename, Model &model);
    void LoadInstanceData(const char *filename, Model &model);

    RenderSystem() = default;
};

}
