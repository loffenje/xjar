#include "pch.h"
#include "vulkan_shadow_technique.h"
#include "vulkan_render_device.h"
#include "vulkan_swapchain.h"
#include "vulkan_ds.h"
#include "window.h"
#include "io.h"

namespace xjar {

static void CreateShaderStages(Vulkan_RenderDevice *rd, const char *vs, const char *fs, VkPipelineShaderStageCreateInfo *stages) {
    auto vertShaderCode = ReadFile("shaders/shadow_depth.vert.spv");
    auto fragShaderCode = ReadFile("shaders/shadow_depth.frag.spv");

    VkShaderModule vertShaderModule = CreateShaderModule(rd, vertShaderCode);
    VkShaderModule fragShaderModule = CreateShaderModule(rd, fragShaderCode);

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

    stages[0] = vertShaderStageInfo;
    stages[1] = fragShaderStageInfo;
}

void Vulkan_ShadowTechnique::SetupDescriptorLayout(Vulkan_RenderDevice *rd) {
    DescriptorLayoutBuilder dsBindings;
    dsBindings.AddBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT);
    dsBindings.AddBinding(1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_VERTEX_BIT);           // vertices of model
    dsBindings.AddBinding(2, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_VERTEX_BIT);           // indices of model
    dsBindings.AddBinding(3, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_VERTEX_BIT);           // instance data buffer
    dsBindings.AddBinding(4, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT); // shadowmap

    m_dsLayout = dsBindings.Build(rd->device);
}

void Vulkan_ShadowTechnique::CreateShadowDepthPipeline(Vulkan_RenderDevice *rd, u32 imageCount) {
    VkPipelineShaderStageCreateInfo shaderStages[1];

    auto vertShaderCode = ReadFile("shaders/shadow_depth.vert.spv");

    VkShaderModule vertShaderModule = CreateShaderModule(rd, vertShaderCode);

    VkPipelineShaderStageCreateInfo vertShaderStageInfo {};
    vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertShaderStageInfo.module = vertShaderModule;
    vertShaderStageInfo.pName = "main";

    shaderStages[0] = vertShaderStageInfo;

    m_offscreenPipeline.SetInputTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
    m_offscreenPipeline.SetPolygonMode(VK_POLYGON_MODE_FILL);
    m_offscreenPipeline.SetCullMode(VK_CULL_MODE_NONE, VK_FRONT_FACE_COUNTER_CLOCKWISE);
    m_offscreenPipeline.SetMultisamplingNone();
    m_offscreenPipeline.DisableBlending();
    m_offscreenPipeline.EnableDepthtest(VK_TRUE, VK_COMPARE_OP_LESS_OR_EQUAL);
    m_offscreenPipeline.SetShaders(shaderStages);
    m_offscreenPipeline.SetDescriptorSets(&m_dsLayout, 1);
    m_offscreenPipeline.Create(rd, m_renderPass);

    vkDestroyShaderModule(rd->device, shaderStages[0].module, nullptr);
}

void Vulkan_ShadowTechnique::CreateShadowMap(Vulkan_RenderDevice *rd, u32 imageCount) {
    auto depthFormat = FindDepthFormat(rd->physicalDevice);

    m_framebuffers.resize(imageCount);
    m_uniformsDepth.resize(imageCount);
    m_uniformsMemoryDepth.resize(imageCount);

    VkImageCreateInfo imageInfo {};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = m_width;
    imageInfo.extent.height = m_height;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.format = depthFormat;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL; // let the gpu to shuffle the data however it sees fit
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;

    CreateImage(
        rd,
        imageInfo,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        m_depthImage,
        m_depthImageMemory);

    VkImageViewCreateInfo viewInfo {};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = m_depthImage;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = depthFormat;
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;

    if (vkCreateImageView(rd->device, &viewInfo, nullptr, &m_depthImageView) != VK_SUCCESS) {
        fprintf(stderr, "Failed to create texture image view\n");
        exit(1);
    }
}

void Vulkan_ShadowTechnique::CreateFramebuffers(Vulkan_RenderDevice *rd, u32 imageCount) {
    for (size_t i = 0; i < imageCount; i++) {
        VkFramebufferCreateInfo framebufferInfo = {};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = m_renderPass;
        framebufferInfo.attachmentCount = 1;
        framebufferInfo.pAttachments = &m_depthImageView;
        framebufferInfo.width = m_width;
        framebufferInfo.height = m_height;
        framebufferInfo.layers = 1;

        if (vkCreateFramebuffer(
                rd->device,
                &framebufferInfo,
                nullptr,
                &m_framebuffers[i]) != VK_SUCCESS) {
            fprintf(stderr, "Failed to create shadow map\n");
            exit(1);
        }
    }
}

void Vulkan_ShadowTechnique::Create(Vulkan_RenderDevice *rd, Vulkan_Swapchain *swapchain) {

    u32 imageCount = static_cast<u32>(swapchain->images.size());

    CreateShadowMap(rd, imageCount);

    VkSamplerCreateInfo sampler {};
    sampler.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO; 
    sampler.magFilter = VK_FILTER_LINEAR;
    sampler.minFilter = VK_FILTER_LINEAR;
    sampler.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    sampler.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    sampler.addressModeV = sampler.addressModeU;
    sampler.addressModeW = sampler.addressModeU;
    sampler.mipLodBias = 0.0f;
    sampler.maxAnisotropy = 1.0f;
    sampler.minLod = 0.0f;
    sampler.maxLod = 1.0f;
    sampler.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;

    if (vkCreateSampler(rd->device, &sampler, nullptr, &m_depthSampler) != VK_SUCCESS) {
        fprintf(stderr, "Failed to create sampler for shadow pass\n");
        exit(1);
    }

    VkAttachmentDescription attachment {};
    attachment.format = FindDepthFormat(rd->physicalDevice);
    attachment.samples = VK_SAMPLE_COUNT_1_BIT;
    attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    attachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;

    VkAttachmentReference depthAttachmentRef {};
    depthAttachmentRef.attachment = 0;
    depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass = {};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 0;
    subpass.pDepthStencilAttachment = &depthAttachmentRef;

    std::array<VkSubpassDependency, 2> dependencies;

    dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
    dependencies[0].dstSubpass = 0;
    dependencies[0].srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    dependencies[0].dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependencies[0].srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
    dependencies[0].dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

    dependencies[1].srcSubpass = 0;
    dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
    dependencies[1].srcStageMask = VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
    dependencies[1].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    dependencies[1].srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    dependencies[1].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

    VkRenderPassCreateInfo renderPassInfo = {};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = 1;
    renderPassInfo.pAttachments = &attachment;
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = static_cast<u32>(dependencies.size());
    renderPassInfo.pDependencies = dependencies.data();

    if (vkCreateRenderPass(rd->device, &renderPassInfo, nullptr, &m_renderPass) != VK_SUCCESS) {
        fprintf(stderr, "Failed to create shadow render pass\n");
        exit(1);
    }
    
    CreateFramebuffers(rd, imageCount);
    SetupUniforms(rd, imageCount);
    SetupDescriptorLayout(rd);
    CreateShadowDepthPipeline(rd, imageCount);
}

void Vulkan_ShadowTechnique::Destroy(Vulkan_RenderDevice *rd, u32 imageCount) {
    vkDestroyImageView(rd->device, m_depthImageView, nullptr);
    for (size_t i = 0; i < imageCount; i++) {
        vkDestroyFramebuffer(rd->device, m_framebuffers[i], nullptr);
    }

    vkDestroySampler(rd->device, m_depthSampler, nullptr);
    vkDestroyRenderPass(rd->device, m_renderPass, nullptr);

    vkDestroyImage(rd->device, m_depthImage, nullptr);
    vkFreeMemory(rd->device, m_depthImageMemory, nullptr);

    vkDestroyDescriptorSetLayout(rd->device, m_dsLayout, nullptr);
    m_offscreenPipeline.Destroy(rd->device);
}

void Vulkan_ShadowTechnique::SetupUniforms(Vulkan_RenderDevice *rd, u32 imageCount) {
    for (u32 i = 0; i < imageCount; i++) {
        CreateBuffer(rd, sizeof(GPU_ShadowDepth),
                     VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                     m_uniformsDepth[i], m_uniformsMemoryDepth[i]);
    }
}


void Vulkan_ShadowTechnique::Update(Vulkan_RenderDevice *rd, int currentImage, const glm::vec3 &lightPos) {
    f32 nearPlane = 1.0f;
    f32 farPlane = 7.5f;
    glm::mat4 lightProj = glm::ortho(-10.0f, 10.0f, -10.0f, 10.0f, nearPlane, farPlane);
    glm::mat4 lightView = glm::lookAt(lightPos, glm::vec3(0.0f), glm::vec3(0.0, 1.0, 0.0));
    m_lightSpaceMatrix = lightProj * lightView;

    GPU_ShadowDepth shadowDepth {};
    shadowDepth.m_depthMVP = m_lightSpaceMatrix;

    UploadBufferData(rd, m_uniformsMemoryDepth[currentImage], 0, &shadowDepth, sizeof(GPU_ShadowDepth));
}

void Vulkan_ShadowTechnique::BeginPass(VkCommandBuffer cmdbuf, VkExtent2D extent, int currentImage) {

    std::array<VkClearValue, 2> clearValues {};
    clearValues[0].depthStencil = {1.0f, 0};
    // first pass: render the scene from light's POV
    
    VkRenderPassBeginInfo passInfo {};
    passInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    passInfo.renderPass = m_renderPass;
    passInfo.framebuffer = m_framebuffers[currentImage];
    passInfo.renderArea.offset = {0, 0};
    passInfo.renderArea.extent = extent;
    passInfo.clearValueCount = static_cast<u32>(clearValues.size());
    passInfo.pClearValues = clearValues.data();

    vkCmdBeginRenderPass(cmdbuf, &passInfo, VK_SUBPASS_CONTENTS_INLINE);

    VkViewport viewport {};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<f32>(m_width);
    viewport.height = static_cast<f32>(m_height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    VkRect2D scissor {{0, 0}, extent};

    vkCmdSetViewport(cmdbuf, 0, 1, &viewport);
    vkCmdSetScissor(cmdbuf, 0, 1, &scissor);
}

void Vulkan_ShadowTechnique::EndPass(VkCommandBuffer cmdbuf) {
    vkCmdEndRenderPass(cmdbuf);
}

}
