#pragma once
#include "pch.h"
#include "vulkan_swapchain.h"
#include "vulkan_grid_feature.h"
#include "vulkan_render_device.h"
#include "vulkan_pipeline.h"
#include "vulkan_ds.h"
#include "vulkan_texture.h"

#include "io.h"
#include "world.h"
#include "window.h"
#include "material_descr.h"
#include "texture_manager.h"

namespace xjar {


struct GPU_Grid {
    alignas(16) glm::mat4 view;
    alignas(16) glm::mat4 projection;
};

static_assert(sizeof(GPU_Grid) % 16 == 0, "GPU_Grid should be padded to 16 bytes");

void Vulkan_GridFeature::Init(Vulkan_RenderDevice *device, Vulkan_Swapchain *swapchain) {
    m_renderDevice = device;
    m_swapchain = swapchain;

    CreateColorRenderPass();
    CreateFramebuffers();
    CreateUniformBuffers();
    CreateDescriptorPool();
    AllocateDescriptorSets();
    CreatePipeline();
}

void Vulkan_GridFeature::BeginPass(FrameStatus frame) {
    VkCommandBuffer *vkcmdbuf = (VkCommandBuffer *)frame.commandBuffer;

    std::array<VkClearValue, 2> clearValues {};
    clearValues[0].color = {0.1f, 0.1f, 0.1f, 1.0f};
    clearValues[1].depthStencil = {1.0f, 0};

    VkRenderPassBeginInfo passInfo {};
    passInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    passInfo.renderPass = m_renderPass;
    passInfo.framebuffer = m_framebuffers[frame.currentImage];
    passInfo.renderArea.offset = {0, 0};
    passInfo.renderArea.extent = m_swapchain->swapchainExtent;
    passInfo.clearValueCount = static_cast<u32>(clearValues.size());
    passInfo.pClearValues = clearValues.data();

    vkCmdBeginRenderPass(*vkcmdbuf, &passInfo, VK_SUBPASS_CONTENTS_INLINE);

    VkViewport viewport {};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<f32>(m_swapchain->swapchainExtent.width);
    viewport.height = static_cast<f32>(m_swapchain->swapchainExtent.height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    VkRect2D scissor {{0, 0}, m_swapchain->swapchainExtent};

    vkCmdSetViewport(*vkcmdbuf, 0, 1, &viewport);
    vkCmdSetScissor(*vkcmdbuf, 0, 1, &scissor);

    m_pipeline.Bind(*vkcmdbuf);
}


void Vulkan_GridFeature::CreateFramebuffers() {
    auto window = GetWindow();

    size_t imageCount = m_swapchain->images.size();
    m_framebuffers.resize(imageCount);
    for (size_t i = 0; i < imageCount; i++) {
        std::array<VkImageView, 2> attachments = {m_swapchain->imageViews[i], m_swapchain->depthImageViews[i]};

        VkFramebufferCreateInfo framebufferInfo = {};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = m_renderPass;
        framebufferInfo.attachmentCount = 1;
        framebufferInfo.pAttachments = attachments.data();
        framebufferInfo.width = window.width;
        framebufferInfo.height = window.height;
        framebufferInfo.layers = 1;

        if (vkCreateFramebuffer(
                m_renderDevice->device,
                &framebufferInfo,
                nullptr,
                &m_framebuffers[i]) != VK_SUCCESS) {
            fprintf(stderr, "Failed to create framebuffer\n");
            exit(1);
        }
    }
}

void Vulkan_GridFeature::CreateColorRenderPass() {
    VkAttachmentDescription depthAttachment {};
    depthAttachment.format = FindDepthFormat(m_renderDevice->physicalDevice);
    depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
    depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentReference depthAttachmentRef {};
    depthAttachmentRef.attachment = 1;
    depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentReference colorAttachmentRef = {};
    colorAttachmentRef.attachment = 0;

    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    VkAttachmentDescription colorAttachment = {};
    colorAttachment.format = m_renderDevice->swapchainImageFormat;
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass = {};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;
    subpass.pDepthStencilAttachment = nullptr;

    VkSubpassDependency dependency = {};
    dependency.dstSubpass = 0;
    dependency.dstAccessMask =
        VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    dependency.dstStageMask =
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.srcAccessMask = 0;
    dependency.srcStageMask =
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;

    std::array<VkAttachmentDescription, 2> attachments = {colorAttachment, depthAttachment};

    VkRenderPassCreateInfo renderPassInfo = {};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = 1; // don't use depth
    renderPassInfo.pAttachments = attachments.data();
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &dependency;

    if (vkCreateRenderPass(m_renderDevice->device, &renderPassInfo, nullptr, &m_renderPass) != VK_SUCCESS) {
        fprintf(stderr, "Failed to create render pass\n");
        exit(1);
    }
}
void Vulkan_GridFeature::Destroy() {
    vkDestroyRenderPass(m_renderDevice->device, m_renderPass, nullptr);

    size_t imageCount = m_swapchain->images.size();

    vkDestroyDescriptorPool(m_renderDevice->device, m_dsPool, nullptr);

    for (u32 i = 0; i < imageCount; i++) {
        vkDestroyFramebuffer(m_renderDevice->device, m_framebuffers[i], nullptr);

        vkDestroyBuffer(m_renderDevice->device, m_uniformBuffers[i], nullptr);
        vkFreeMemory(m_renderDevice->device, m_uniformBuffersMemory[i], nullptr);
    }

    vkDestroyDescriptorSetLayout(m_renderDevice->device, m_dsLayout, nullptr);
    m_pipeline.Destroy(m_renderDevice->device);
}

void Vulkan_GridFeature::AllocateDescriptorSets() {
    const u32 imageCount = static_cast<u32>(m_swapchain->images.size());
    m_descriptorSets.resize(imageCount);

    for (u32 i = 0; i < imageCount; i++) {
        VkDescriptorSetAllocateInfo        allocInfo {};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = m_dsPool;
        allocInfo.descriptorSetCount = 1;
        allocInfo.pSetLayouts = &m_dsLayout;

        if (vkAllocateDescriptorSets(m_renderDevice->device, &allocInfo, &m_descriptorSets[i]) != VK_SUCCESS) {
            fprintf(stderr, "Failed to allocate descriptor sets\n");
            exit(1);
        }

        DescriptorWriter writer;
        writer.WriteBuffer(0, m_uniformBuffers[i], sizeof(GPU_Grid), 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);

        writer.UpdateSet(m_renderDevice->device, m_descriptorSets[i]);
    }
}

void Vulkan_GridFeature::Draw(FrameStatus frame, GPU_SceneData *sceneData) {
    VkCommandBuffer *vkcmdbuf = (VkCommandBuffer *)frame.commandBuffer;

    GPU_Grid gridData {};
    gridData.view = sceneData->viewMat;
    gridData.projection = sceneData->projMat;

    gridData.projection[1][1] *= -1;

    UploadBufferData(m_renderDevice, m_uniformBuffersMemory[frame.currentImage], 0, &gridData, sizeof(GPU_Grid));

    vkCmdBindDescriptorSets(*vkcmdbuf, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline.pipelineLayout, 0, 1, &m_descriptorSets[frame.currentImage], 0, nullptr);

    vkCmdDraw(*vkcmdbuf, 6, 1, 0, 0);
}

void Vulkan_GridFeature::OnResize(Vulkan_Swapchain *swapchain) {
    m_swapchain = swapchain;

    size_t imageCount = m_swapchain->images.size();
    for (size_t i = 0; i < imageCount; i++) {
        vkDestroyFramebuffer(m_renderDevice->device, m_framebuffers[i], nullptr);
    }

    CreateFramebuffers();
}

void Vulkan_GridFeature::CreateDescriptorPool() {
    const u32 imageCount = static_cast<u32>(m_swapchain->images.size());

    VkDescriptorPoolSize poolSize {};
    poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSize.descriptorCount = imageCount;

    VkDescriptorPoolCreateInfo poolInfo {};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = 1;
    poolInfo.pPoolSizes = &poolSize;
    poolInfo.maxSets = imageCount;

    if (vkCreateDescriptorPool(m_renderDevice->device, &poolInfo, nullptr, &m_dsPool) != VK_SUCCESS) {
        fprintf(stderr, "Failed to create descriptor pool\n");
        exit(1);
    }

    DescriptorLayoutBuilder dsBindings;
    dsBindings.AddBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT);

    m_dsLayout = dsBindings.Build(m_renderDevice->device);
}

void Vulkan_GridFeature::CreateUniformBuffers() {
    const u32 imageCount = static_cast<u32>(m_swapchain->images.size());

    m_uniformBuffers.resize(imageCount);
    m_uniformBuffersMemory.resize(imageCount);

    for (u32 i = 0; i < imageCount; i++) {
        CreateBuffer(m_renderDevice, sizeof(GPU_Grid),
                     VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                     m_uniformBuffers[i], m_uniformBuffersMemory[i]);
    }
}

void Vulkan_GridFeature::CreatePipeline() {
    auto vertShaderCode = ReadFile("shaders/grid.vert.spv");
    auto fragShaderCode = ReadFile("shaders/grid.frag.spv");

    VkShaderModule vertShaderModule = CreateShaderModule(m_renderDevice, vertShaderCode);
    VkShaderModule fragShaderModule = CreateShaderModule(m_renderDevice, fragShaderCode);

    VkPipelineShaderStageCreateInfo vertShaderStageInfo {};
    vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertShaderStageInfo.module = vertShaderModule;
    vertShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo fragShaderStageInfo {};
    fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragShaderStageInfo.module = fragShaderModule;
    fragShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo shaderStages[] = {
        vertShaderStageInfo,
        fragShaderStageInfo};

    m_pipeline.SetInputTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
    m_pipeline.SetPolygonMode(VK_POLYGON_MODE_FILL);
    m_pipeline.SetCullMode(VK_CULL_MODE_NONE, VK_FRONT_FACE_CLOCKWISE);
    m_pipeline.SetMultisamplingNone();
    m_pipeline.EnableBlendingAlphablend();
    m_pipeline.DisableDepthtest();
    m_pipeline.SetShaders(shaderStages);
    m_pipeline.SetDescriptorSets(&m_dsLayout, 1);
    m_pipeline.Create(m_renderDevice, m_renderPass);

    vkDestroyShaderModule(m_renderDevice->device, fragShaderModule, nullptr);
    vkDestroyShaderModule(m_renderDevice->device, vertShaderModule, nullptr);
}

}
