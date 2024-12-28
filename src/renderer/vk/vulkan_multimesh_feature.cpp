#pragma once

#include "vulkan_swapchain.h"
#include "vulkan_multimesh_feature.h"
#include "vulkan_render_device.h"
#include "vulkan_pipeline.h"
#include "vulkan_model.h"
#include "vulkan_ds.h"

#include "io.h"
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <array>
#include "world.h"
#include "window.h"

namespace xjar {

static u32 g_offsetAlignment;

void Vulkan_MultiMeshFeature::Init(void *device, void *swapchain) {
    m_renderDevice = (Vulkan_RenderDevice *)device;
    m_swapchain = (Vulkan_Swapchain *)swapchain;

    CreateColorAndDepthRenderPass();
    CreateDepthResources();
    CreateFramebuffers();
    CreateDescriptorPool();
    CreatePipeline();
    CreateUniformBuffers();

    m_models.resize(1024);

    VkPhysicalDeviceProperties devProps;
    vkGetPhysicalDeviceProperties(m_renderDevice->physicalDevice, &devProps);

    g_offsetAlignment = static_cast<u32>(devProps.limits.minStorageBufferOffsetAlignment);
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

void Vulkan_MultiMeshFeature::OnResize() {

}


void Vulkan_MultiMeshFeature::LoadMesh(const char *filename, Model &model) {
    FILE *file = fopen(filename, "rb");
    if (!file) {
        fprintf(stderr, "Failed to load mesh %s\n", filename);
        exit(1);
    }

    MeshHdr hdr;
    if (fread(&hdr, 1, sizeof(hdr), file) != sizeof(hdr)) {
        fprintf(stderr, "Unable to read %s file\n", filename);
        exit(1);
    }

    const u32         meshNum = hdr.meshNum;
    model.mesh.meshes.resize(meshNum);

    if (fread(model.mesh.meshes.data(), sizeof(Mesh), meshNum, file) != meshNum) {
        fprintf(stderr, "Unable to read meshes from %s file\n", filename);
        exit(1);
    }

    const u32 indexDataSize = hdr.indexDataSize;
    const u32 vertexDataSize = hdr.vertexDataSize;
    model.mesh.indexData.resize(indexDataSize / sizeof(u32));
    model.mesh.vertexData.resize(vertexDataSize / sizeof(f32));

    if (fread(model.mesh.indexData.data(), 1, indexDataSize, file) != indexDataSize ||
        fread(model.mesh.vertexData.data(), 1, vertexDataSize, file) != vertexDataSize) {
        fprintf(stderr, "Unable to read geometry\n");
        exit(1);
    }
}

void Vulkan_MultiMeshFeature::LoadInstanceData(const char *filename, ModelResources &res) {
    FILE *file = fopen(filename, "rb");
    if (!file) {
        fprintf(stderr, "Failed to load instance %s\n", filename);
        exit(1);
    }

    fseek(file, 0, SEEK_END);
    size_t fsize = ftell(file);
    fseek(file, 0, SEEK_SET);
    res.m_maxInstanceCount = static_cast<u32>(fsize / sizeof(InstanceData));
    res.m_instances.resize(res.m_maxInstanceCount);

    if (fread(res.m_instances.data(), sizeof(InstanceData), res.m_maxInstanceCount, file) != res.m_maxInstanceCount) {
        fprintf(stderr, "Unable to read instance data\n");
        exit(1);
    }

    fclose(file);
}

void Vulkan_MultiMeshFeature::LoadModel(const char *meshFilename, const char *instanceFilename, Model &model) {
    int *handle = new int;
    *handle = m_modelID;

    model.handle = handle;
    ModelResources &res = m_models[m_modelID++];

    LoadMesh(meshFilename, model);
    LoadInstanceData(instanceFilename, res);

    size_t vertexDataSize = model.mesh.vertexData.size() * sizeof(model.mesh.vertexData[0]);
    size_t indexDataSize = model.mesh.indexData.size() * sizeof(model.mesh.indexData[0]);

    const u32 indirectDataSize = res.m_maxInstanceCount * sizeof(VkDrawIndirectCommand);
    res.m_maxInstanceSize = res.m_maxInstanceCount * sizeof(InstanceData);
    res.m_maxMaterialSize = 1024;

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

    for (u32 i = 0; i < imageCount; i++) {

        res.m_descriptorSets[i] = m_dsAllocators[i].Allocate(m_renderDevice->device, m_dsLayout);

        DescriptorWriter writer;
        writer.WriteBuffer(0, m_uniformBuffers[i], sizeof(UniformSceneObject), 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
        writer.WriteBuffer(1, res.m_storageBuffer, res.m_maxVertexBufferSize, 0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
        writer.WriteBuffer(2, res.m_storageBuffer, res.m_maxIndexBufferSize, res.m_maxVertexBufferSize, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
        writer.WriteBuffer(3, res.m_instanceBuffers[i], res.m_maxInstanceSize, 0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
        writer.WriteBuffer(4, res.m_materialBuffer, res.m_maxMaterialSize, 0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);

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
}

void Vulkan_MultiMeshFeature::DrawEntities(FrameStatus frame, const GPU_SceneData &sceneData, std::initializer_list<Entity *> entities) {

  //  m_dsAllocators[frame.currentImage].ClearPools(m_renderDevice->device);

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

    UniformSceneObject uniformScene {};
    uniformScene.projection = sceneData.projMat;
    uniformScene.view = sceneData.viewMat;

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

    vkCmdEndRenderPass(*vkcmdbuf);
}

}
