#pragma once

#include "renderer_types.h"
#include "material_system.h"

namespace xjar {

struct Entity;

class RendererBackend {
public:
    virtual void        CreateTexture(const void *pixels, Texture *texture) = 0;
    virtual void        DestroyTexture(Texture *texture) = 0;
    virtual void        OnResized(u32 width, u32 height) = 0;
    virtual FrameStatus BeginFrame() = 0;
    virtual void        EndFrame() = 0;

    virtual void DrawEntities(FrameStatus frame, const GPU_SceneData &sceneData, std::initializer_list<Entity *> entities) = 0;

    virtual void CreateModel(std::vector<InstanceData> &instances,
        std::vector<MaterialDescr> &materials,
        const std::vector<std::string> &textureFilenames,
        Model &model) = 0;
    
    virtual void *GetDefaultRenderPass() {
        return nullptr;
    }
    virtual void *GetRenderDevice() {
        return nullptr;
    }
    virtual void *GetSwapchain() {
        return nullptr;
    }

    virtual void OnInit() {
    }
    virtual void OnDestroy() {
    }
    virtual void UpdateGlobalState(const GPU_SceneData &sceneData) {
    }
    virtual void ClearColor(f32 r, f32 g, f32 b, f32 a) {
    }

    virtual void BeginDefaultPass() {
    }
    virtual void EndDefaultPass() {
    }
    
    virtual void BeginMultiMeshFeaturePass(FrameStatus frame) {
    }
    
    virtual void EndMultiMeshFeaturePass(FrameStatus frame) {
    }

    virtual ~RendererBackend() = default;
};

}
