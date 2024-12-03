#pragma once

#include "types.h"

#define VK_USE_PLATFORM_WIN32_KHR
#include <vulkan/vulkan.h>

#include <vector>
#include <span>

namespace xjar {

struct Vulkan_RenderDevice;

struct Vulkan_Pipeline {
    std::vector<VkPipelineShaderStageCreateInfo> shaderStages;
    VkPipelineInputAssemblyStateCreateInfo       inputAssembly;
    VkPipelineRasterizationStateCreateInfo       rasterizer;
    VkPipelineColorBlendAttachmentState          colorBlendAttachment;
    VkPipelineMultisampleStateCreateInfo         multisampling;
    VkPipelineLayoutCreateInfo                   pipelineLayoutInfo;
    VkPipelineVertexInputStateCreateInfo         vertexInputInfo;
    VkPipelineLayout                             pipelineLayout;
    VkPipelineDepthStencilStateCreateInfo        depthStencil;
    VkFormat                                     colorAttachmentFormat;
    VkPipeline                                   pipeline;

    Vulkan_Pipeline() {
        Reset();
    }

    void Create(Vulkan_RenderDevice *rd, VkRenderPass renderPass);
    void Destroy(VkDevice device);
    void Reset();

    void SetVertexInput(const std::vector<VkVertexInputBindingDescription> &bindingDescriptions,
            const std::vector<VkVertexInputAttributeDescription> &attributeDescriptions);
    void SetShaders(std::span<VkPipelineShaderStageCreateInfo> _shaderStages);
    void SetInputTopology(VkPrimitiveTopology topology);
    void SetPolygonMode(VkPolygonMode mode);
    void SetCullMode(VkCullModeFlags mode, VkFrontFace frontFace);
    void SetPushConstants(VkPushConstantRange range, u32 count);
    void SetDescriptorSets(VkDescriptorSetLayout *layouts, u32 layoutsNum);
    void SetMultisamplingNone();
    void DisableBlending();
    void DisableDepthtest();
    void EnableDepthtest(bool depthWriteEnable, VkCompareOp op);
    void EnableBlendingAdditive();
    void EnableBlendingAlphablend();

    void Bind(VkCommandBuffer, VkPipelineBindPoint point = VK_PIPELINE_BIND_POINT_GRAPHICS);
};

}
