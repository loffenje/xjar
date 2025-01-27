#pragma once

#include "resource_types.h"
#include "renderer_types.h"
#include "material_system.h"
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
    void        ClearColor(FrameStatus frame, f32 r, f32 g, f32 b, f32 a);
    void        BeginGridPass(FrameStatus frame, GPU_SceneData *sceneData);
    void        EndGridPass(FrameStatus frame);
    void        BeginMultiMeshFeaturePass(FrameStatus frame);
    void        EndMultiMeshFeaturePass(FrameStatus frame);

    void        Shutdown();
    void        LoadModel(const char *meshFilename, const char *instanceFilename, const char *materialFilename, Model &model);
    void        CreatePlane(Texture *texture);
    void        CreateTexture(const void *pixels, Texture *texture);
    void        DestroyTexture(Texture *texture);
    void        DrawEntities(FrameStatus frame, GPU_SceneData *sceneData, std::initializer_list<Entity *> entities);
    void        DrawGrid(FrameStatus frame, GPU_SceneData *sceneData);
private:
    void LoadInstanceData(const char *filename, std::vector<InstanceData> &instances);
    void LoadMaterials(const char *fileName, std::vector<MaterialDescr> &materials, std::vector<std::string> &files);

    RenderSystem() = default;
};

}
