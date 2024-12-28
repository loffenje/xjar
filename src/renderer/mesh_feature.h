#pragma once

#include <initializer_list>
#include "renderer_types.h"
#include "camera.h"

namespace xjar {

struct Entity;

class MeshFeature {
public:
    virtual void Init(void *device, void *swapchain) = 0;
    virtual void LoadModel(const char *meshFilename, const char *instanceFilename, Model &model) {}
    virtual void DrawEntities(FrameStatus frame, const GPU_SceneData &sceneData, std::initializer_list<Entity *> entities) = 0;
    virtual void OnResize() {}

    virtual ~MeshFeature() = default;
};

}

