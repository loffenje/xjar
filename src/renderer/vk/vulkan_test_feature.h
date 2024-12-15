#pragma once

#include <initializer_list>
#include "renderer/test_feature.h"
#include "renderer/camera.h"
#include "vulkan_render_device.h"
#include "vulkan_pipeline.h"

namespace xjar {

class Vulkan_TestFeature final : public TestFeature {
public:
    void Init(void *device, void *renderPass) override;
    void DrawEntities(void *cmdbuf, const GPU_SceneData &sceneData, std::initializer_list<Entity *> entities) override;
private:
    void CreatePipeline();

    Vulkan_RenderDevice *m_renderDevice;
    VkRenderPass         m_renderPass;
    Vulkan_Pipeline      m_pipeline;
};

}
