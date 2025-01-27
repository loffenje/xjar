#pragma once

#include <initializer_list>
#include "renderer/mesh_feature.h"
#include "renderer/camera.h"
#include "vulkan_pipeline.h"
#include "vulkan_ds.h"
#include <unordered_set>

namespace xjar {

struct Vulkan_Swapchain;
struct Vulkan_RenderDevice;
struct GPU_SceneData;

class Vulkan_GridFeature final {
public:
    void Init(Vulkan_RenderDevice *device, Vulkan_Swapchain *swapchain);
    void Destroy();
    void Draw(FrameStatus frame, GPU_SceneData *sceneData);
    void OnResize(Vulkan_Swapchain *swapchain);
    void BeginPass(FrameStatus frame);

private:
    void CreateColorRenderPass();
    void CreateFramebuffers();
    void CreatePipeline();
    void CreateDescriptorPool();
    void CreateUniformBuffers();
    void AllocateDescriptorSets();

    std::vector<VkFramebuffer> m_framebuffers;

    Vulkan_RenderDevice *m_renderDevice;
    Vulkan_Pipeline      m_pipeline;
    Vulkan_Swapchain    *m_swapchain;

    VkDescriptorPool                 m_dsPool;
    VkDescriptorSetLayout            m_dsLayout;
    VkRenderPass                     m_renderPass;
    std::vector<VkBuffer>       m_uniformBuffers;
    std::vector<VkDeviceMemory> m_uniformBuffersMemory;
    std::vector<VkDescriptorSet> m_descriptorSets;
};

}
