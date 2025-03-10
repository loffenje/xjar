#include "pch.h"
#include "vulkan_backend.h"
#include "vulkan_swapchain.h"
#include "vulkan_ds.h"
#include "vulkan_texture.h"
#include "window.h"


namespace xjar {

//TODO: set it from renderSystem globally for the scene / via default pass ?

VkDescriptorSetLayout g_dsSceneLayout;

void Vulkan_Backend::OnInit() {
    m_renderDevice = CreateRenderDevice("xjar", "xjarEngine");
    RecreateSwapchain(); 
    CreateLastRenderPass();
    CreateBuffers();

    m_multiMeshFeature = new Vulkan_MultiMeshFeature();
    m_multiMeshFeature->Init(&m_renderDevice, m_swapchain.get());
   
    m_gridFeature = new Vulkan_GridFeature();
    m_gridFeature->Init(&m_renderDevice, m_swapchain.get());
}

void Vulkan_Backend::CreateBuffers() {
    m_commandBuffers.resize(MAX_FRAMES_IN_FLIGHT);

    VkCommandBufferAllocateInfo allocInfo {};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = m_renderDevice.commandPool;
    allocInfo.commandBufferCount = static_cast<u32>(m_commandBuffers.size());

    if (vkAllocateCommandBuffers(m_renderDevice.device, &allocInfo, m_commandBuffers.data()) !=
        VK_SUCCESS) {
        fprintf(stderr, "Failed to allocate buffers\n");
        exit(1);
    }
}

void Vulkan_Backend::OnDestroy() {
    vkDeviceWaitIdle(m_renderDevice.device);

    m_multiMeshFeature->Destroy();
    m_gridFeature->Destroy();
#if 0
    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        m_frameDescriptors[i].DestroyPools(m_renderDevice.device);
    }
#endif
    vkFreeCommandBuffers(
        m_renderDevice.device,
        m_renderDevice.commandPool,
        static_cast<u32>(m_commandBuffers.size()),
        m_commandBuffers.data());
    m_commandBuffers.clear();

    DestroySwapchain(m_swapchain.get(), &m_renderDevice);
    vkDestroyRenderPass(m_renderDevice.device, m_lastRenderPass, nullptr);
    DestroyRenderDevice(&m_renderDevice);

}
void Vulkan_Backend::OnResized(u32 width, u32 height) {
    auto &window = GetWindow();
    window.width = width;
    window.height = height;
    RecreateSwapchain();
    m_multiMeshFeature->OnResize(m_swapchain.get());
    m_gridFeature->OnResize(m_swapchain.get());
}
u32 BytesPerTextureFormat(VkFormat fmt) {
    switch (fmt) {
        case VK_FORMAT_R8_SINT:
        case VK_FORMAT_R8_UNORM:
            return 1;
        case VK_FORMAT_R16_SFLOAT:
            return 2;
        case VK_FORMAT_R16G16_SFLOAT:
            return 4;
        case VK_FORMAT_R16G16_SNORM:
            return 4;
        case VK_FORMAT_B8G8R8A8_UNORM:
            return 4;
        case VK_FORMAT_R8G8B8A8_UNORM:
            return 4;
        case VK_FORMAT_R16G16B16A16_SFLOAT:
            return 4 * sizeof(uint16_t);
        case VK_FORMAT_R32G32B32A32_SFLOAT:
            return 4 * sizeof(float);
        default:
            break;
    }
    return 0;
}

void Vulkan_Backend::CreateTexture(const void *pixels, Texture *texture) {

    static u32 gentexture = 0;
    const int layerCount = 1;

    Vulkan_Texture *vktexture = (Vulkan_Texture *)malloc(sizeof(Vulkan_Texture));
    texture->id = gentexture++;
    texture->handle = vktexture;
    VkFormat format = VK_FORMAT_R8G8B8A8_UNORM;

    u32 bytesPerPixel = BytesPerTextureFormat(format);

    VkDeviceSize layerSize = texture->width * texture->height * bytesPerPixel;
    VkDeviceSize imageSize = layerSize * layerCount;

    VkBuffer       stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    CreateBuffer(&m_renderDevice, imageSize,
                 VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                 stagingBuffer, stagingBufferMemory);

    UploadBufferData(&m_renderDevice, stagingBufferMemory, 0, pixels, imageSize);

    VkImageCreateInfo imageInfo {};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = texture->width;
    imageInfo.extent.height = texture->height;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.format = format;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL; // let the gpu to shuffle the data however it sees fit
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    imageInfo.flags = 0;

    CreateImage(&m_renderDevice,
        imageInfo,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        vktexture->image,
        vktexture->memory);

    VkImageViewCreateInfo viewInfo {};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = vktexture->image;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = format;
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;

    if (vkCreateImageView(m_renderDevice.device, &viewInfo, nullptr, &vktexture->view) !=
        VK_SUCCESS) {
        fprintf(stderr, "Failed to create image view\n");
        exit(1);
    }

    VkCommandBuffer tempbuf = BeginImmediateCommands(&m_renderDevice);

    TransitionImageLayout(tempbuf, vktexture->image, format,
                          VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                        layerCount);

    CopyBufferToImage(tempbuf, stagingBuffer,
        vktexture->image, texture->width, texture->height);

    TransitionImageLayout(tempbuf, vktexture->image, format,
                          VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        layerCount);

    EndImmediateCommands(&m_renderDevice, tempbuf);

    vkDestroyBuffer(m_renderDevice.device, stagingBuffer, nullptr);
    vkFreeMemory(m_renderDevice.device, stagingBufferMemory, nullptr);

    VkSamplerCreateInfo samplerInfo {};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.anisotropyEnable = VK_TRUE;
    samplerInfo.maxAnisotropy = 16;
    samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerInfo.mipLodBias = 0.0f;
    samplerInfo.minLod = 0.0f;
    samplerInfo.maxLod = 0.0f;

    if (vkCreateSampler(m_renderDevice.device, &samplerInfo, nullptr, &vktexture->sampler) != VK_SUCCESS) {
        fprintf(stderr, "Failed to create sampler\n");
        exit(1);
    }
}

void Vulkan_Backend::CreateModel(std::vector<InstanceData>      &instances,
                std::vector<MaterialDescr> &materials,
                const std::vector<std::string>   &textureFilenames,
                Model                            &model) {
    m_multiMeshFeature->CreateModel(instances, materials, textureFilenames, model);
}

void Vulkan_Backend::DestroyTexture(Texture *texture) {
    Vulkan_Texture *vktexture = (Vulkan_Texture *)texture->handle;

    vkDestroyImageView(m_renderDevice.device, vktexture->view, nullptr);
    vkDestroyImage(m_renderDevice.device, vktexture->image, nullptr);
    vkFreeMemory(m_renderDevice.device, vktexture->memory, nullptr);
    //TODO: sampler can be shared between textures, do smart delete
    vkDestroySampler(m_renderDevice.device, vktexture->sampler, nullptr);

     free(vktexture);
}

void Vulkan_Backend::RecreateSwapchain() {
    auto       window = GetWindow();
    VkExtent2D extent = {window.width, window.height};
    while (extent.width == 0 || extent.height == 0) {
        extent = {window.width, window.height};
        glfwWaitEvents();
    }

    vkDeviceWaitIdle(m_renderDevice.device);

    if (m_swapchain == nullptr) {
        m_swapchain = CreateSwapchain(&m_renderDevice, extent, nullptr);
    } else {
        std::shared_ptr<Vulkan_Swapchain> oldSwapchain = std::move(m_swapchain);
        m_swapchain = CreateSwapchain(&m_renderDevice, extent, oldSwapchain);
        DestroySwapchain(oldSwapchain.get(), &m_renderDevice);
    }
}

VkResult Vulkan_Backend::AcquireNextImage(u32 *imageIndex) {
    vkWaitForFences(
        m_renderDevice.device,
        1,
        &m_swapchain->inFlightFences[m_swapchain->currentFrame],
        VK_TRUE,
        UINT64_MAX);


    //m_frameDescriptors[m_currentFrameIndex].ClearPools();

    VkResult result = vkAcquireNextImageKHR(
        m_renderDevice.device,
        m_swapchain->swapchain,
        UINT64_MAX,
        m_swapchain->imageAvailableSems[m_swapchain->currentFrame],
        VK_NULL_HANDLE,
        imageIndex);

    return result;
}

FrameStatus Vulkan_Backend::BeginFrame() {
    VkResult result = AcquireNextImage(&m_currentImageIndex);
    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        RecreateSwapchain();
        FrameStatus status{.success = false, .commandBuffer=nullptr};
        return status;
    }

    if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        fprintf(stderr, "Failed to acquire swapchain image\n");
        exit(1);
    }

    auto *cmdbuf = GetCurrentCommandBuffer();

    VkCommandBufferBeginInfo beginInfo {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

    if (vkBeginCommandBuffer(*cmdbuf, &beginInfo) != VK_SUCCESS) {
        fprintf(stderr, "Failed to begin recording\n");
        exit(1);
    }

    FrameStatus status {
        .success = true,
        .commandBuffer = cmdbuf,
        .currentImage = m_currentImageIndex};

    return status;
}

void Vulkan_Backend::EndFrame() {
    auto *cmdbuf = GetCurrentCommandBuffer();


    auto &window = GetWindow();

    const VkRect2D screenRect = {
        .offset = {0, 0},
        .extent = {.width = window.width, .height = window.height}};

    VkRenderPassBeginInfo passInfo {};
    passInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    passInfo.renderPass = m_lastRenderPass;
    passInfo.framebuffer = m_swapchain->framebuffers[m_currentImageIndex];
    passInfo.renderArea = screenRect;

    vkCmdBeginRenderPass(*cmdbuf, &passInfo, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdEndRenderPass(*cmdbuf); // transition to swapchain KHR layout

    if (vkEndCommandBuffer(*cmdbuf) != VK_SUCCESS) {
        fprintf(stderr, "Failed to end recording\n");
        exit(1);
    }

    VkResult result = SubmitCommandBuffers(m_swapchain.get(), &m_renderDevice, cmdbuf, &m_currentImageIndex);
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
        //window.resized = false;

        RecreateSwapchain();
        m_multiMeshFeature->OnResize(m_swapchain.get());
        m_gridFeature->OnResize(m_swapchain.get());
    } else if (result != VK_SUCCESS) {
        fprintf(stderr, "Failed to present swapchain image\n");
        exit(1);
    }

    m_currentFrameIndex = (m_currentFrameIndex + 1) % MAX_FRAMES_IN_FLIGHT;
}

void Vulkan_Backend::DrawGrid(FrameStatus frame, GPU_SceneData *sceneData) {
    m_gridFeature->Draw(frame, sceneData);
}

void Vulkan_Backend::DrawEntities(FrameStatus frame, GPU_SceneData *sceneData, std::initializer_list<Entity *> entities) {
    if (m_multiMeshFeature->IsShadowsEnabled()) {
        m_multiMeshFeature->BeginShadowPass(frame);
        m_multiMeshFeature->DrawEntities(frame, sceneData, entities);
        m_multiMeshFeature->EndShadowPass(frame);
    }

    m_multiMeshFeature->BeginDefaultPass(frame);
    m_multiMeshFeature->DrawEntities(frame, sceneData, entities);
    m_multiMeshFeature->EndDefaultPass(frame);
}

void Vulkan_Backend::ClearColor(FrameStatus frame, f32 r, f32 g, f32 b, f32 a) {
    VkCommandBuffer *cmdbuf = (VkCommandBuffer *)frame.commandBuffer;

    std::array<VkClearValue, 2> clearValues {};
    clearValues[0].color = {r, g, b, a};
    clearValues[1].depthStencil = {1.0f, 0};

    VkRenderPassBeginInfo passInfo {};
    passInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    passInfo.renderPass = m_swapchain->renderPass;
    passInfo.framebuffer = m_swapchain->framebuffers[m_currentImageIndex];
    passInfo.renderArea.offset = {0, 0};
    passInfo.renderArea.extent = m_swapchain->swapchainExtent;
    passInfo.clearValueCount = static_cast<u32>(clearValues.size());
    passInfo.pClearValues = clearValues.data();

    vkCmdBeginRenderPass(*cmdbuf, &passInfo, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdEndRenderPass(*cmdbuf); // transition to optimal

    
    VkViewport viewport {};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<f32>(m_swapchain->swapchainExtent.width);
    viewport.height = static_cast<f32>(m_swapchain->swapchainExtent.height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    VkRect2D scissor {{0, 0}, m_swapchain->swapchainExtent};

    vkCmdSetViewport(*cmdbuf, 0, 1, &viewport);
    vkCmdSetScissor(*cmdbuf, 0, 1, &scissor);
}

void Vulkan_Backend::BeginGridPass(FrameStatus frame) {
    m_gridFeature->BeginPass(frame);
}

void Vulkan_Backend::EndGridPass(FrameStatus frame) {
    VkCommandBuffer *cmdbuf = (VkCommandBuffer *)frame.commandBuffer;

    vkCmdEndRenderPass(*cmdbuf);
}

void Vulkan_Backend::UpdateGlobalState(const GPU_SceneData &sceneData) {
    VkBuffer       sceneBuffer;
    VkDeviceMemory sceneBufferMemory;
    CreateBuffer(&m_renderDevice, sizeof(GPU_SceneData),
                 VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                 sceneBuffer, sceneBufferMemory);
    
    UploadBufferData(&m_renderDevice, sceneBufferMemory, 0, &sceneData, sizeof(GPU_SceneData));
   
#if 0
    VkDescriptorSet globalDescriptor = GetCurrentFrameDescriptor()->Allocate(m_renderDevice.device, g_dsSceneLayout);
    DescriptorWriter writer;
    writer.WriteBuffer(0, sceneBuffer, sizeof(GPU_SceneData), 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
    writer.UpdateSet(m_renderDevice.device, globalDescriptor);
#endif
}

void Vulkan_Backend::CreateLastRenderPass() {

        VkAttachmentDescription depthAttachment {};
        depthAttachment.format = FindDepthFormat(m_renderDevice.physicalDevice);
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
        colorAttachment.format = m_swapchain->imageFormat;
        colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
        colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
        colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        colorAttachment.initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

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

        if (vkCreateRenderPass(m_renderDevice.device, &renderPassInfo, nullptr, &m_lastRenderPass) != VK_SUCCESS) {
            fprintf(stderr, "Failed to create render pass\n");
            exit(1);
        }
}

VkCommandBuffer *Vulkan_Backend::GetCurrentCommandBuffer() {
    return &m_commandBuffers[m_currentFrameIndex];
}

void *Vulkan_Backend::GetDefaultRenderPass() {
    return &m_swapchain->renderPass;
}

void *Vulkan_Backend::GetRenderDevice() {
    return &m_renderDevice;
}

void *Vulkan_Backend::GetSwapchain() {
    return m_swapchain.get();
}

}
