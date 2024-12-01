#include "vulkan_swapchain.h"
#include "types.h"
#if RENDERER_BACKEND == OpenGL
#include "glfw_gl.h"
#else
#include "glfw_vk.h"
#endif

#include "window.h"

#include <array>
#include <algorithm>

namespace xjar {

VkSurfaceFormatKHR ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR> &availableFormats) {
    for (const auto &availableFormat : availableFormats) {
        if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB &&
            availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            return availableFormat;
        }
    }

    return availableFormats[0];
}

VkPresentModeKHR ChooseSwapPresentMode(const std::vector<VkPresentModeKHR> &availablePresentModes) {
    for (const auto &availablePresentMode : availablePresentModes) {
        if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
            return availablePresentMode;
        }
    }

    return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D ChooseSwapExtent(GLFWwindow *window, VkExtent2D windowExtent, const VkSurfaceCapabilitiesKHR &caps) {
    if (caps.currentExtent.width != UINT32_MAX) {
        return caps.currentExtent;
    } else {
        int width, height;
        glfwGetFramebufferSize(window, &width, &height);

        VkExtent2D actualExtent = windowExtent;

        actualExtent.width = std::clamp(actualExtent.width, caps.minImageExtent.width, caps.maxImageExtent.width);
        actualExtent.height = std::clamp(actualExtent.height, caps.minImageExtent.height, caps.maxImageExtent.height);

        return actualExtent;
    }
}

void CreateImageViews(Vulkan_Swapchain *swapchain, Vulkan_RenderDevice &rd) {
    swapchain->imageViews.resize(swapchain->images.size());

    for (size_t i = 0; i < swapchain->images.size(); i++) {
        VkImageViewCreateInfo viewInfo {};

        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = swapchain->images[i];
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = swapchain->imageFormat;
        viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = 1;

        if (vkCreateImageView(rd.device, &viewInfo, nullptr, &swapchain->imageViews[i]) !=
            VK_SUCCESS) {
            fprintf(stderr, "Failed to create image view\n");
            exit(1);
        }
    }
}
static VkFormat FindSupportedFormat(VkPhysicalDevice physicalDevice, const std::vector<VkFormat> &candidates, VkImageTiling tiling, VkFormatFeatureFlags features) {
    for (VkFormat format : candidates) {
        VkFormatProperties props;
        vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &props);

        if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features) {
            return format;
        } else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features) {
            return format;
        }
    }

    fprintf(stderr, "Failed to find supported format\n");
    exit(EXIT_FAILURE);

    return VkFormat {};
}

static VkFormat FindDepthFormat(VkPhysicalDevice physicalDevice) {
    return FindSupportedFormat(physicalDevice,
                               {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT},
                               VK_IMAGE_TILING_OPTIMAL,
                               VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
}

void CreateRenderPass(Vulkan_Swapchain *swapchain, Vulkan_RenderDevice &rd) {
    VkAttachmentDescription depthAttachment {};
    depthAttachment.format = FindDepthFormat(rd.physicalDevice);
    depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentReference depthAttachmentRef {};
    depthAttachmentRef.attachment = 1;
    depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentReference colorAttachmentRef = {};
    colorAttachmentRef.attachment = 0;

    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    VkAttachmentDescription colorAttachment = {};
    colorAttachment.format = swapchain->imageFormat;
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
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

    if (vkCreateRenderPass(rd.device, &renderPassInfo, nullptr, &swapchain->renderPass) != VK_SUCCESS) {
        fprintf(stderr, "Failed to create render pass\n");
        exit(1);
    }
}

void CreateDepthResources(Vulkan_Swapchain *swapchain, Vulkan_RenderDevice &rd) {
    VkFormat depthFormat = FindDepthFormat(rd.physicalDevice);
    swapchain->depthFormat = depthFormat;

    u32 imageCount = swapchain->images.size();

    swapchain->depthImages.resize(imageCount);
    swapchain->depthImageMemories.resize(imageCount);
    swapchain->depthImageViews.resize(imageCount);

    for (int i = 0; i < swapchain->depthImages.size(); i++) {
        VkImageCreateInfo imageInfo {};
        imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.imageType = VK_IMAGE_TYPE_2D;
        imageInfo.extent.width = swapchain->swapchainExtent.width;
        imageInfo.extent.height = swapchain->swapchainExtent.height;
        imageInfo.extent.depth = 1;
        imageInfo.mipLevels = 1;
        imageInfo.arrayLayers = 1;
        imageInfo.format = swapchain->depthFormat;
        imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
        imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        imageInfo.flags = 0;

        CreateImage(
            rd,
            imageInfo,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            swapchain->depthImages[i],
            swapchain->depthImageMemories[i]);

        VkImageViewCreateInfo viewInfo {};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = swapchain->depthImages[i];
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = swapchain->depthFormat;
        viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = 1;

        if (vkCreateImageView(rd.device, &viewInfo, nullptr, &swapchain->depthImageViews[i]) != VK_SUCCESS) {
            fprintf(stderr, "Failed to create texture image view\n");
            exit(1);
        }
    }
}

void CreateFramebuffers(Vulkan_Swapchain *swapchain, Vulkan_RenderDevice &rd) {
    u32 imageCount = swapchain->images.size();
    swapchain->framebuffers.resize(imageCount);
    for (u32 i = 0; i < imageCount; i++) {
        std::array<VkImageView, 2> attachments = {swapchain->imageViews[i], swapchain->depthImageViews[i]};

        VkExtent2D              swapChainExtent = swapchain->swapchainExtent;
        VkFramebufferCreateInfo framebufferInfo = {};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = swapchain->renderPass;
        framebufferInfo.attachmentCount = static_cast<u32>(attachments.size());
        framebufferInfo.pAttachments = attachments.data();
        framebufferInfo.width = swapChainExtent.width;
        framebufferInfo.height = swapChainExtent.height;
        framebufferInfo.layers = 1;

        if (vkCreateFramebuffer(
                rd.device,
                &framebufferInfo,
                nullptr,
                &swapchain->framebuffers[i]) != VK_SUCCESS) {
            fprintf(stderr, "Failed to create framebuffer\n");
            exit(1);
        }
    }
}

void CreateSync(Vulkan_Swapchain *swapchain, Vulkan_RenderDevice &rd) {
    u32 imageCount = swapchain->images.size();

    swapchain->imageAvailableSems.resize(MAX_FRAMES_IN_FLIGHT);
    swapchain->renderFinishedSems.resize(MAX_FRAMES_IN_FLIGHT);
    swapchain->inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);
    swapchain->imagesInFlight.resize(imageCount, VK_NULL_HANDLE);

    VkSemaphoreCreateInfo semaphoreInfo = {};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceInfo = {};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        if (vkCreateSemaphore(rd.device, &semaphoreInfo, nullptr, &swapchain->imageAvailableSems[i]) !=
                VK_SUCCESS ||
            vkCreateSemaphore(rd.device, &semaphoreInfo, nullptr, &swapchain->renderFinishedSems[i]) !=
                VK_SUCCESS ||
            vkCreateFence(rd.device, &fenceInfo, nullptr, &swapchain->inFlightFences[i]) != VK_SUCCESS) {
            fprintf(stderr, "Failed to create sync object\n");
            exit(1);
        }
    }
}

std::unique_ptr<Vulkan_Swapchain> CreateSwapchain(Vulkan_RenderDevice &rd, VkExtent2D windowExtent, std::shared_ptr<Vulkan_Swapchain> prev) {
    SwapChainSupportDetails swapChainSupport = QuerySwapChainSupport(rd.physicalDevice, rd.surface);

    auto swapchain = std::make_unique<Vulkan_Swapchain>();

    auto               window = GetWindow();
    VkSurfaceFormatKHR surfaceFormat = ChooseSwapSurfaceFormat(swapChainSupport.formats);
    VkPresentModeKHR   presentMode = ChooseSwapPresentMode(swapChainSupport.presentModes);
    VkExtent2D         extent = ChooseSwapExtent((GLFWwindow *)window.handle, windowExtent, swapChainSupport.caps);

    swapchain->imageFormat = surfaceFormat.format;
    swapchain->swapchainExtent = extent;

    u32 imageCount = swapChainSupport.caps.minImageCount + 1;
    if (swapChainSupport.caps.maxImageCount > 0 &&
        imageCount > swapChainSupport.caps.maxImageCount) {
        imageCount = swapChainSupport.caps.maxImageCount;
    }

    VkSwapchainCreateInfoKHR createInfo {};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = rd.surface;
    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = surfaceFormat.format;
    createInfo.imageColorSpace = surfaceFormat.colorSpace;
    createInfo.imageExtent = extent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    QueueFamily families = FindQueueFamilies(rd.physicalDevice, rd.surface);
    uint32_t    queueFamilyIndices[] = {families.graphicsFamily.value(), families.presentFamily.value()};

    if (families.graphicsFamily != families.presentFamily) {
        createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = queueFamilyIndices;
    } else {
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        createInfo.queueFamilyIndexCount = 0;     // Optional
        createInfo.pQueueFamilyIndices = nullptr; // Optional
    }

    createInfo.preTransform = swapChainSupport.caps.currentTransform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode = presentMode;
    createInfo.clipped = VK_TRUE;
    createInfo.oldSwapchain = prev == nullptr ? VK_NULL_HANDLE : prev->swapchain;

    if (vkCreateSwapchainKHR(rd.device, &createInfo, nullptr, &swapchain->swapchain) != VK_SUCCESS) {
        fprintf(stderr, "Failed to create a swapchain\n");
        exit(EXIT_FAILURE);
    }

    vkGetSwapchainImagesKHR(rd.device, swapchain->swapchain, &imageCount, nullptr);
    swapchain->images.resize(imageCount);
    vkGetSwapchainImagesKHR(rd.device, swapchain->swapchain, &imageCount, swapchain->images.data());

    CreateImageViews(swapchain.get(), rd);
    CreateRenderPass(swapchain.get(), rd);
    CreateDepthResources(swapchain.get(), rd);
    CreateFramebuffers(swapchain.get(), rd);
    CreateSync(swapchain.get(), rd);

    return swapchain;
}

void DestroySwapchain(Vulkan_Swapchain *swapchain, Vulkan_RenderDevice &rd) {
    for (auto imageView : swapchain->imageViews) {
        vkDestroyImageView(rd.device, imageView, nullptr);
    }
    swapchain->imageViews.clear();

    vkDestroySwapchainKHR(rd.device, swapchain->swapchain, nullptr);

    for (int i = 0; i < swapchain->depthImages.size(); i++) {
        vkDestroyImageView(rd.device, swapchain->depthImageViews[i], nullptr);
        vkDestroyImage(rd.device, swapchain->depthImages[i], nullptr);
        vkFreeMemory(rd.device, swapchain->depthImageMemories[i], nullptr);
    }

    for (auto framebuffer : swapchain->framebuffers) {
        vkDestroyFramebuffer(rd.device, framebuffer, nullptr);
    }

    vkDestroyRenderPass(rd.device, swapchain->renderPass, nullptr);

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        vkDestroySemaphore(rd.device, swapchain->renderFinishedSems[i], nullptr);
        vkDestroySemaphore(rd.device, swapchain->imageAvailableSems[i], nullptr);
        vkDestroyFence(rd.device, swapchain->inFlightFences[i], nullptr);
    }
}

VkResult AcquireNextImage(Vulkan_Swapchain *swapchain, Vulkan_RenderDevice &rd, u32 *imageIndex) {
    vkWaitForFences(
        rd.device,
        1,
        &swapchain->inFlightFences[swapchain->currentFrame],
        VK_TRUE,
        UINT64_MAX);

    VkResult result = vkAcquireNextImageKHR(
        rd.device,
        swapchain->swapchain,
        UINT64_MAX,
        swapchain->imageAvailableSems[swapchain->currentFrame],
        VK_NULL_HANDLE,
        imageIndex);

    return result;
}

VkResult SubmitCommandBuffers(Vulkan_Swapchain *swapchain, Vulkan_RenderDevice &rd, const VkCommandBuffer *buffers, u32 *imageIndex) {
    if (swapchain->imagesInFlight[*imageIndex] != VK_NULL_HANDLE) {
        vkWaitForFences(rd.device, 1, &swapchain->imagesInFlight[*imageIndex], VK_TRUE, UINT64_MAX);
    }

    swapchain->imagesInFlight[*imageIndex] = swapchain->inFlightFences[swapchain->currentFrame];

    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    VkSemaphore          waitSemaphores[] = {swapchain->imageAvailableSems[swapchain->currentFrame]};
    VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;

    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = buffers;

    VkSemaphore signalSemaphores[] = {swapchain->renderFinishedSems[swapchain->currentFrame]};
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    vkResetFences(rd.device, 1, &swapchain->inFlightFences[swapchain->currentFrame]);
    if (vkQueueSubmit(rd.graphicsQueue, 1, &submitInfo, swapchain->inFlightFences[swapchain->currentFrame]) !=
        VK_SUCCESS) {
        fprintf(stderr, "Failed to submit to graphics queue\n");
        exit(1);
    }

    VkPresentInfoKHR presentInfo = {};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;

    VkSwapchainKHR swapChains[] = {swapchain->swapchain};
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapChains;
    presentInfo.pImageIndices = imageIndex;

    VkResult result = vkQueuePresentKHR(rd.presentQueue, &presentInfo);
    swapchain->currentFrame = (swapchain->currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;

    return result;
}

}
