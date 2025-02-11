#pragma once

#include "vulkan_pipeline.h"

namespace xjar {

struct Vulkan_RenderDevice;
struct Vulkan_Swapchain;

struct GPU_ShadowDepth {
    alignas(16) glm::mat4 m_depthMVP;
};

struct Vulkan_ShadowTechnique {
	
	int                             m_width;
    int                             m_height;
    b32                             m_quadDebug;
	VkRenderPass	                m_renderPass;
    VkFramebuffer	                m_framebuffer;
    VkImage                         m_depthImage;
    VkImageView                     m_depthImageView;
    VkDeviceMemory                  m_depthImageMemory;
    VkSampler                       m_depthSampler;
    VkDescriptorPool                m_dsPool;
    VkDescriptorSetLayout           m_dsLayout;
    Vulkan_Pipeline                 m_offscreenPipeline;

    std::vector<VkFramebuffer>      m_framebuffers;
    std::vector<VkBuffer>           m_uniformsDepth;
    std::vector<VkDeviceMemory>     m_uniformsMemoryDepth;

    glm::mat4                       m_lightSpaceMatrix;


    void Destroy(Vulkan_RenderDevice *rd, u32 imageCount);
    void Update(Vulkan_RenderDevice *rd, int currentImage, const glm::vec3 &lightPos);
	void Create(Vulkan_RenderDevice *rd, Vulkan_Swapchain *swapchain);
    void BeginPass(VkCommandBuffer cmdbuf, VkExtent2D extent, int currentImage);
    void EndPass(VkCommandBuffer cmdbuf);
    void SetupUniforms(Vulkan_RenderDevice *rd, u32 imageCount);
    void SetupDescriptorLayout(Vulkan_RenderDevice *rd);
    void CreateShadowDepthPipeline(Vulkan_RenderDevice *rd, u32 imageCount);
    void CreateFramebuffers(Vulkan_RenderDevice *rd, u32 imageCount);
    void CreateShadowMap(Vulkan_RenderDevice *rd, u32 imageCount);
};


}
