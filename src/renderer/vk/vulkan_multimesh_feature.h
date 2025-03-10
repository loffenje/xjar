#pragma once

#include <initializer_list>
#include "renderer/mesh_feature.h"
#include "renderer/camera.h"
#include "vulkan_pipeline.h"
#include "vulkan_ds.h"
#include "material_descr.h"
#include "vulkan_shadow_technique.h"

namespace xjar {

struct Vulkan_Swapchain;
struct Vulkan_RenderDevice;

enum {
    DEFAULT_PASS = 0,
    SHADOW_PASS
};

struct ModelResources {
    std::vector<InstanceData>       m_instances;
    std::vector<MaterialDescr>      m_materials;
    std::vector<VkDescriptorSet>    m_descriptorSets;
    std::vector<VkDescriptorSet>    m_offscreenDescriptorSets;

    u32 m_maxVertexBufferSize, m_maxIndexBufferSize;
    u32 m_maxInstances;
    u32 m_maxInstanceSize, m_maxMaterialSize;
    u32 m_maxInstanceCount;

    VkBuffer                        m_storageBuffer;
    VkDeviceMemory                  m_storageBufferMemory;
    VkBuffer                        m_materialBuffer;
    VkDeviceMemory                  m_materialBufferMemory;

    // for each swapchain image
    std::vector<VkBuffer>       m_instanceBuffers;
    std::vector<VkDeviceMemory> m_instanceBuffersMemory;
    std::vector<std::string>    m_loadedTextures;
};

inline void DestroyModelResources(VkDevice device, ModelResources &res) {

    vkDestroyBuffer(device, res.m_storageBuffer, nullptr);
    vkFreeMemory(device, res.m_storageBufferMemory, nullptr);

    vkDestroyBuffer(device, res.m_materialBuffer, nullptr);
    vkFreeMemory(device, res.m_materialBufferMemory, nullptr);

    for (size_t i = 0; i < res.m_instanceBuffers.size(); i++) {
        vkDestroyBuffer(device, res.m_instanceBuffers[i], nullptr);
        vkFreeMemory(device, res.m_instanceBuffersMemory[i], nullptr);
    }
}

class Vulkan_MultiMeshFeature final {
public:
    void Init(Vulkan_RenderDevice *device, Vulkan_Swapchain *swapchain);
    void CreateModel(std::vector<InstanceData> &instances,
        std::vector<MaterialDescr> &materials, 
        const std::vector<std::string> &textureFilenames,
        Model &model);

    void EnableShadows();
    void DrawEntities(FrameStatus frame, GPU_SceneData *sceneData, std::initializer_list<Entity *> entities);
    void OnResize(Vulkan_Swapchain *swapchain);
    void BeginDefaultPass(FrameStatus frame);
    void EndDefaultPass(FrameStatus frame);
    void BeginShadowPass(FrameStatus frame);
    void EndShadowPass(FrameStatus frame);

    bool IsShadowsEnabled() const {
        return m_enableShadows;
    }

    VkRenderPass *GetPass() {
        return &m_renderPass;
    }

    void Destroy();

private:
    void CreatePipeline();
    void CreateColorAndDepthRenderPass();
    void CreateDepthResources();
    void CreateFramebuffers();
    void CreateDescriptorPool();
    void CreateUniformBuffers();
    void AllocateDescriptorSets(ModelResources &res);

    std::vector<VkFramebuffer>  m_framebuffers;

    Vulkan_RenderDevice *m_renderDevice;
    VkRenderPass         m_renderPass;
    Vulkan_Pipeline      m_pipeline;
    Vulkan_Swapchain    *m_swapchain;
    Vulkan_ShadowTechnique m_shadowTechnique;

    std::vector<DescriptorAllocator> m_dsAllocators;
    std::vector<DescriptorAllocator> m_offscreenDsAllocators;

    VkDescriptorSetLayout           m_dsLayout;
    VkImage        m_depthImage;
    VkImageView    m_depthImageView;
    VkDeviceMemory m_depthImageMemory;
    std::vector<VkBuffer>       m_indirectBuffers;
    std::vector<VkDeviceMemory> m_indirectBuffersMemory;

    VkDescriptorPool            m_offscreenDsPool;
    VkSampler                   m_defaultSamplerLinear;
    VkSampler                   m_defaultSamplerNearest;

    std::vector<VkBuffer>       m_uniformBuffers;
    std::vector<VkDeviceMemory> m_uniformBuffersMemory;

    std::deque<ModelResources>  m_models;
    u32                         m_modelCount = 0;
    int                         m_instanceCount = 0;
    int                         m_modelID = 0;
    int                         m_passState = DEFAULT_PASS;
    b32                         m_enableShadows = false;
};

}
