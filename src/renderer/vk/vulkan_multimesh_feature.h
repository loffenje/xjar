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

struct ModelResources {
    std::vector<InstanceData>       m_instances;
    std::vector<VkDescriptorSet>    m_descriptorSets;

    u32 m_maxVertexBufferSize, m_maxIndexBufferSize;
    u32 m_maxInstances;
    u32 m_maxInstanceSize, m_maxMaterialSize;
    u32 m_maxInstanceCount;

    VkBuffer                        m_storageBuffer;
    VkDeviceMemory                  m_storageBufferMemory;
    VkBuffer       m_materialBuffer;
    VkDeviceMemory m_materialBufferMemory;

    // for each swapchain image
    std::vector<VkBuffer>       m_indirectBuffers;
    std::vector<VkDeviceMemory> m_indirectBuffersMemory;
    std::vector<VkBuffer>       m_instanceBuffers;
    std::vector<VkDeviceMemory> m_instanceBuffersMemory;
};

class Vulkan_MultiMeshFeature final : public MeshFeature {
public:
    void Init(void *device, void *swapchain) override;
    void LoadModel(const char *meshFilename, const char *instanceFilename, Model &model) override;
    void DrawEntities(FrameStatus frame, const GPU_SceneData &sceneData, std::initializer_list<Entity *> entities) override;
    void OnResize() override;
private:
    void  LoadMesh(const char *filename, Model &model);
    void  LoadInstanceData(const char *filename, ModelResources &res);

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

    std::vector<DescriptorAllocator> m_dsAllocators;
    VkDescriptorSetLayout           m_dsLayout;
    VkImage        m_depthImage;
    VkImageView    m_depthImageView;
    VkDeviceMemory m_depthImageMemory;

    std::vector<VkBuffer>       m_uniformBuffers;
    std::vector<VkDeviceMemory> m_uniformBuffersMemory;

    std::deque<ModelResources>  m_models;
    int                         m_modelID = 0;

};

}
