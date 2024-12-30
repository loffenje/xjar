#pragma once

#include "resource_types.h"
#include "renderer_types.h"
#include <vector>

namespace xjar {

struct Entity;

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
    void        BeginMultiMeshFeaturePass(FrameStatus frame);
    void        EndMultiMeshFeaturePass(FrameStatus frame);

    void        Shutdown();
    void        LoadModel(const char *meshFilename, const char *instanceFilename, Model &model);
    void        CreateTexture(const void *pixels, Texture *texture);
    void        DestroyTexture(Texture *texture);
    void        DrawEntities(FrameStatus frame, const GPU_SceneData &sceneData, std::initializer_list<Entity *> entities);

private:
    void LoadInstanceData(const char *filename, std::vector<InstanceData> &instances);

    RenderSystem() = default;
};

}
