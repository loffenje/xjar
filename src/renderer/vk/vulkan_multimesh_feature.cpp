#pragma once

#include "vulkan_swapchain.h"
#include "vulkan_multimesh_feature.h"
#include "vulkan_render_device.h"
#include "vulkan_pipeline.h"
#include "vulkan_model.h"
#include "vulkan_ds.h"
#include "vulkan_texture.h"

#include "io.h"
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <array>
#include "world.h"
#include "window.h"
#include "material_system.h"
#include "texture_manager.h"

namespace xjar {

static u32 g_offsetAlignment;

void Vulkan_MultiMeshFeature::Init(Vulkan_RenderDevice *device, Vulkan_Swapchain *swapchain) {
    m_renderDevice = device;
    m_swapchain = swapchain;

    CreateColorAndDepthRenderPass();
    CreateDepthResources();
    CreateFramebuffers();
    CreateDescriptorPool();
    CreatePipeline();
    CreateUniformBuffers();

    m_models.resize(32);

    VkPhysicalDeviceProperties devProps;
    vkGetPhysicalDeviceProperties(m_renderDevice->physicalDevice, &devProps);

    g_offsetAlignment = static_cast<u32>(devProps.limits.minStorageBufferOffsetAlignment);
}

void Vulkan_MultiMeshFeature::Destroy() {
    vkDestroySampler(m_renderDevice->device, m_defaultSamplerLinear, nullptr);
    vkDestroySampler(m_renderDevice->device, m_defaultSamplerNearest, nullptr);

    vkDestroyRenderPass(m_renderDevice->device, m_renderPass, nullptr);

    vkDestroyImageView(m_renderDevice->device, m_depthImageView, nullptr);
    vkDestroyImage(m_renderDevice->device, m_depthImage, nullptr);
    vkFreeMemory(m_renderDevice->device, m_depthImageMemory, nullptr);
  
    u32 imageCount = m_swapchain->images.size();
    for (u32 i = 0; i < imageCount; i++) {
        vkDestroyFramebuffer(m_renderDevice->device, m_framebuffers[i], nullptr);
        m_dsAllocators[i].DestroyPools(m_renderDevice->device);

        vkDestroyBuffer(m_renderDevice->device, m_uniformBuffers[i], nullptr);
        vkFreeMemory(m_renderDevice->device, m_uniformBuffersMemory[i], nullptr);

    }

    for (u32 i = 0; i < m_modelCount; i++) {
        DestroyModelResources(m_renderDevice->device, m_models[i]);
    }

    vkDestroyDescriptorSetLayout(m_renderDevice->device, m_dsLayout, nullptr);
    m_pipeline.Destroy(m_renderDevice->device);
}

void Vulkan_MultiMeshFeature::CreateColorAndDepthRenderPass() {
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
    subpass.pDepthStencilAttachment = &depthAttachmentRef;

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
    renderPassInfo.attachmentCount = static_cast<u32>(attachments.size());
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

void Vulkan_MultiMeshFeature::OnResize(Vulkan_Swapchain *swapchain) {
    auto &window = GetWindow();
    m_swapchain = swapchain;

    vkDestroyImageView(m_renderDevice->device, m_depthImageView, nullptr);
    u32 imageCount = m_swapchain->images.size();
    for (u32 i = 0; i < imageCount; i++) {
        vkDestroyFramebuffer(m_renderDevice->device, m_framebuffers[i], nullptr);
    }

    CreateDepthResources();
    CreateFramebuffers();
}


void Vulkan_MultiMeshFeature::CreateModel(std::vector<InstanceData>  &instances,
                const std::vector<MaterialDescr> &materials,
                const std::vector<std::string>   &textureFilenames,
                Model                            &model) {

    int *handle = new int;
    *handle = m_modelID;

    model.handle = handle;
    ModelResources &res = m_models[m_modelID++];
    m_modelCount++;

    res.m_maxInstanceCount = instances.size();
    res.m_instances = std::move(instances);

    size_t vertexDataSize = model.mesh.vertexData.size() * sizeof(model.mesh.vertexData[0]);
    size_t indexDataSize = model.mesh.indexData.size() * sizeof(model.mesh.indexData[0]);

    const u32 indirectDataSize = res.m_maxInstanceCount * sizeof(VkDrawIndirectCommand);
    res.m_maxInstanceSize = res.m_maxInstanceCount * sizeof(InstanceData);
    res.m_maxMaterialSize = 1024;

    res.m_loadedTextures.reserve(textureFilenames.size());

    auto &textureManager = TextureManager::Instance();
    for (const auto &textureName : textureFilenames) {
        res.m_loadedTextures.push_back(textureName);
        textureManager.Acquire(textureName);
    }

    const size_t imageCount = m_swapchain->images.size();
    res.m_instanceBuffers.resize(imageCount);
    res.m_instanceBuffersMemory.resize(imageCount);

    res.m_indirectBuffers.resize(imageCount);
    res.m_indirectBuffersMemory.resize(imageCount);

    CreateBuffer(m_renderDevice, res.m_maxMaterialSize,
                VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                res.m_materialBuffer, res.m_materialBufferMemory);

    res.m_maxVertexBufferSize = vertexDataSize;
    res.m_maxIndexBufferSize = indexDataSize;


    if ((res.m_maxVertexBufferSize & (g_offsetAlignment - 1)) != 0) {
        int floats = (g_offsetAlignment - (res.m_maxVertexBufferSize & (g_offsetAlignment - 1))) / sizeof(f32);
        for (int ii = 0; ii < floats; ii++)
            model.mesh.vertexData.push_back(0);
        res.m_maxVertexBufferSize = (res.m_maxVertexBufferSize + g_offsetAlignment) & ~(g_offsetAlignment - 1);
    }

    CreateBuffer(m_renderDevice, res.m_maxVertexBufferSize + res.m_maxIndexBufferSize,
                 VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                 res.m_storageBuffer, res.m_storageBufferMemory);

    UploadBufferData(m_renderDevice, res.m_storageBufferMemory, 0, model.mesh.vertexData.data(), vertexDataSize);
    UploadBufferData(m_renderDevice, res.m_storageBufferMemory, res.m_maxVertexBufferSize, model.mesh.indexData.data(), indexDataSize);

    for (auto i = 0; i < imageCount; i++) {
        CreateBuffer(m_renderDevice, indirectDataSize,
                     VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT,
                     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                     res.m_indirectBuffers[i], res.m_indirectBuffersMemory[i]);

        VkDrawIndirectCommand *data = nullptr;
        vkMapMemory(m_renderDevice->device, res.m_indirectBuffersMemory[i],
            0, res.m_maxInstanceCount * sizeof(VkDrawIndirectCommand),
            0, (void **)&data);

        for (u32 i = 0; i < res.m_maxInstanceCount; i++) {
            const u32 j = res.m_instances[i].meshIndex;
            const u32 lod = res.m_instances[i].LOD;
            data[i] = {
                .vertexCount = (u32)model.mesh.meshes[j].LodSize(lod),
                .instanceCount = 1,
                .firstVertex = 0,
                .firstInstance = i};
        }

        vkUnmapMemory(m_renderDevice->device, res.m_indirectBuffersMemory[i]);

        CreateBuffer(m_renderDevice, res.m_maxInstanceSize,
                     VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                     res.m_instanceBuffers[i], res.m_instanceBuffersMemory[i]);

        UploadBufferData(m_renderDevice, res.m_instanceBuffersMemory[i], 0, res.m_instances.data(), res.m_maxInstanceSize);
    }

    AllocateDescriptorSets(res);
}

void Vulkan_MultiMeshFeature::CreateDescriptorPool() {
    
    const u32 imageCount = static_cast<u32>(m_swapchain->images.size());

    std::vector<PoolSizeRatio> poolSizes = {
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 3},
        {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 3}
    };

    m_dsAllocators.resize(imageCount);

    for (u32 i = 0; i < imageCount; i++) {
        DescriptorAllocator &dsAllocator = m_dsAllocators[i];
        dsAllocator.Init(m_renderDevice->device, 1000, poolSizes);
    }

    DescriptorLayoutBuilder dsBindings;
    dsBindings.AddBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT); // viewProjection 
    dsBindings.AddBinding(1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_VERTEX_BIT);   // vertex
    dsBindings.AddBinding(2, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_VERTEX_BIT);   // index
    dsBindings.AddBinding(3, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_VERTEX_BIT);   // instance data buffer
    dsBindings.AddBinding(4, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT); // material
    dsBindings.AddBinding(5, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 1024);

    m_dsLayout = dsBindings.Build(m_renderDevice->device);

}

void Vulkan_MultiMeshFeature::CreateUniformBuffers() {
    const u32 imageCount = static_cast<u32>(m_swapchain->images.size());

    m_uniformBuffers.resize(imageCount);
    m_uniformBuffersMemory.resize(imageCount);

    for (u32 i = 0; i < imageCount; i++) {
        CreateBuffer(m_renderDevice, sizeof(UniformSceneObject),
                     VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                     m_uniformBuffers[i], m_uniformBuffersMemory[i]);
    }
}

void Vulkan_MultiMeshFeature::AllocateDescriptorSets(ModelResources &res) {
    const u32 imageCount = static_cast<u32>(m_swapchain->images.size());
    res.m_descriptorSets.resize(imageCount);

    auto &textureManager = TextureManager::Instance();
    for (u32 i = 0; i < imageCount; i++) {

        res.m_descriptorSets[i] = m_dsAllocators[i].Allocate(m_renderDevice->device, m_dsLayout);

        DescriptorWriter writer;
        writer.WriteBuffer(0, m_uniformBuffers[i], sizeof(UniformSceneObject), 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
        writer.WriteBuffer(1, res.m_storageBuffer, res.m_maxVertexBufferSize, 0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
        writer.WriteBuffer(2, res.m_storageBuffer, res.m_maxIndexBufferSize, res.m_maxVertexBufferSize, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
        writer.WriteBuffer(3, res.m_instanceBuffers[i], res.m_maxInstanceSize, 0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
        writer.WriteBuffer(4, res.m_materialBuffer, res.m_maxMaterialSize, 0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);

        std::vector<VkDescriptorImageInfo> imageInfos;
        for (const auto &textureName : res.m_loadedTextures) {
            Texture texture = textureManager.Acquire(textureName);
            Vulkan_Texture *vktexture = (Vulkan_Texture *)texture.handle;

            imageInfos.emplace_back(VkDescriptorImageInfo {
                .sampler = vktexture->sampler,
                .imageView = vktexture->view,
                .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL });
        }

        VkWriteDescriptorSet write {};
        write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write.dstBinding = 5;
        write.dstSet = VK_NULL_HANDLE;
        write.descriptorCount = imageInfos.size();
        write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        write.pImageInfo = imageInfos.data();

        writer.writes.push_back(write);

        writer.UpdateSet(m_renderDevice->device, res.m_descriptorSets[i]);

    }
}

void Vulkan_MultiMeshFeature::CreateFramebuffers() {
    auto window = GetWindow(); 

    u32 imageCount = m_swapchain->images.size();
    m_framebuffers.resize(imageCount);
    for (u32 i = 0; i < imageCount; i++) {
        std::array<VkImageView, 2> attachments = {m_swapchain->imageViews[i], m_depthImageView};

        VkFramebufferCreateInfo framebufferInfo = {};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = m_renderPass;
        framebufferInfo.attachmentCount = static_cast<u32>(attachments.size());
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

void Vulkan_MultiMeshFeature::CreateDepthResources() {

        auto depthFormat = FindDepthFormat(m_renderDevice->physicalDevice);

        auto window = GetWindow(); 
        VkImageCreateInfo imageInfo {};
        imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.imageType = VK_IMAGE_TYPE_2D;
        imageInfo.extent.width = window.width;
        imageInfo.extent.height = window.height;
        imageInfo.extent.depth = 1;
        imageInfo.mipLevels = 1;
        imageInfo.arrayLayers = 1;
        imageInfo.format = depthFormat;
        imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL; // let the gpu to shuffle the data however it sees fit
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
        imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        imageInfo.flags = 0;

        CreateImage(
            m_renderDevice,
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

        if (vkCreateImageView(m_renderDevice->device, &viewInfo, nullptr, &m_depthImageView) != VK_SUCCESS) {
            fprintf(stderr, "Failed to create texture image view\n");
            exit(1);
        }

        VkCommandBuffer cmdbuf = BeginImmediateCommands(m_renderDevice);
        
        TransitionImageLayout(cmdbuf, m_depthImage, depthFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);

        EndImmediateCommands(m_renderDevice, cmdbuf);
}

void Vulkan_MultiMeshFeature::CreatePipeline() {
    auto vertShaderCode = ReadFile("shaders/basic.vert.spv");
    auto fragShaderCode = ReadFile("shaders/basic.frag.spv");

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

    //TODO: layout ?
    VkPushConstantRange push;
    push.offset = 0;
    push.size = sizeof(PushConstantData);
    push.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    //std::vector<VkVertexInputBindingDescription> bindingDescriptions(1);
    //bindingDescriptions[0].binding = 0;
    //bindingDescriptions[0].stride = sizeof(Vertex3D);
    //bindingDescriptions[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    //std::vector<VkVertexInputAttributeDescription> attributeDescriptions(2);

    m_pipeline.SetInputTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
    m_pipeline.SetPolygonMode(VK_POLYGON_MODE_FILL);
    m_pipeline.SetCullMode(VK_CULL_MODE_NONE, VK_FRONT_FACE_CLOCKWISE);
    m_pipeline.SetMultisamplingNone();
    m_pipeline.DisableBlending();
    m_pipeline.EnableDepthtest(VK_TRUE, VK_COMPARE_OP_LESS);
    m_pipeline.SetPushConstants(push, 1);
    m_pipeline.SetShaders(shaderStages);
    m_pipeline.SetDescriptorSets(&m_dsLayout, 1);
    m_pipeline.Create(m_renderDevice, m_renderPass);

    vkDestroyShaderModule(m_renderDevice->device, fragShaderModule, nullptr);
    vkDestroyShaderModule(m_renderDevice->device, vertShaderModule, nullptr);

    
	VkSamplerCreateInfo sampler{};
    sampler.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    sampler.magFilter = VK_FILTER_NEAREST;
    sampler.minFilter = VK_FILTER_NEAREST;

    vkCreateSampler(m_renderDevice->device, &sampler, nullptr, &m_defaultSamplerNearest);

    sampler.magFilter = VK_FILTER_LINEAR;
    sampler.minFilter = VK_FILTER_LINEAR;
    vkCreateSampler(m_renderDevice->device, &sampler, nullptr, &m_defaultSamplerLinear);
}

void Vulkan_MultiMeshFeature::BeginPass(FrameStatus frame) {
    VkCommandBuffer *vkcmdbuf = (VkCommandBuffer *)frame.commandBuffer;

    std::array<VkClearValue, 2> clearValues {};
    clearValues[0].color = {0.1f, 0.1f, 0.1f, 1.0f};
    clearValues[1].depthStencil = {1.0f, 0};

    VkRenderPassBeginInfo passInfo {};
    passInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    passInfo.renderPass = m_swapchain->renderPass;
    passInfo.framebuffer = m_swapchain->framebuffers[frame.currentImage];
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

void Vulkan_MultiMeshFeature::EndPass(FrameStatus frame) {
    VkCommandBuffer *vkcmdbuf = (VkCommandBuffer *)frame.commandBuffer;

    vkCmdEndRenderPass(*vkcmdbuf);
}

void Vulkan_MultiMeshFeature::DrawEntities(FrameStatus frame, const GPU_SceneData &sceneData, std::initializer_list<Entity *> entities) {

    VkCommandBuffer *vkcmdbuf = (VkCommandBuffer *)frame.commandBuffer;

    UniformSceneObject uniformScene {};
    uniformScene.projection = sceneData.projMat;
    uniformScene.view = sceneData.viewMat;

    uniformScene.projection[1][1] *= -1;

    UploadBufferData(m_renderDevice, m_uniformBuffersMemory[frame.currentImage], 0, &uniformScene, sizeof(UniformSceneObject));

    for (Entity *ent : entities) {
        int modelID = *(int *)ent->model.handle;
        ModelResources &res = m_models[modelID];

        vkCmdBindDescriptorSets(*vkcmdbuf, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline.pipelineLayout, 0, 1, &res.m_descriptorSets[frame.currentImage], 0, nullptr);

        PushConstantData constants;
        constants.model = ent->model.localTransform;

        vkCmdPushConstants(*vkcmdbuf, m_pipeline.pipelineLayout,
                           VK_SHADER_STAGE_VERTEX_BIT,
                           0, sizeof(PushConstantData), &constants);

        vkCmdDrawIndirect(*vkcmdbuf, res.m_indirectBuffers[frame.currentImage], 0, res.m_maxInstanceCount, sizeof(VkDrawIndirectCommand));
    }
}

}
