#pragma once

#include "types.h"

#define VK_USE_PLATFORM_WIN32_KHR
#include <vulkan/vulkan.h>

#include <vector>
#include <span>
#include "vulkan_render_device.h"

namespace xjar {

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

    void Create(Vulkan_RenderDevice &rd, VkViewport viewport, VkRenderPass renderPass);
    void Destroy(VkDevice device);
    void Reset();

    void SetVertexInput(VkVertexInputBindingDescription bindingDescription, std::span<VkVertexInputAttributeDescription> attributeDescriptions);
    void SetShaders(std::span<VkPipelineShaderStageCreateInfo> _shaderStages);
    void SetInputTopology(VkPrimitiveTopology topology);
    void SetPolygonMode(VkPolygonMode mode);
    void SetCullMode(VkCullModeFlags mode, VkFrontFace frontFace);
    void SetDescriptorSets(VkDescriptorSetLayout *layouts, u32 layoutsNum);
    void SetMultisamplingNone();
    void DisableBlending();
    void DisableDepthtest();
    void EnableDepthtest(bool depthWriteEnable, VkCompareOp op);
    void EnableBlendingAdditive();
    void EnableBlendingAlphablend();

    void Bind(VkCommandBuffer, VkPipelineBindPoint point);
};

}
