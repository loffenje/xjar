#pragma once

#include "types.h"
#define VK_USE_PLATFORM_WIN32_KHR
#include <vulkan/vulkan.h>
#include <memory>
#include <vector>

namespace xjar {

struct Vulkan_RenderDevice;

struct Vulkan_Swapchain {
    VkFormat                          imageFormat;
    VkFormat                          depthFormat;
    VkExtent2D                        swapchainExtent;
    VkExtent2D                        windowExtent;
    std::vector<VkFramebuffer>        framebuffers;
    VkRenderPass                      renderPass;
    std::vector<VkImage>              depthImages;
    std::vector<VkDeviceMemory>       depthImageMemories;
    std::vector<VkImageView>          depthImageViews;
    std::vector<VkImage>              images;
    std::vector<VkImageView>          imageViews;
    VkSwapchainKHR                    swapchain;
    std::shared_ptr<Vulkan_Swapchain> oldSwapchain;
    std::vector<VkSemaphore>          imageAvailableSems;
    std::vector<VkSemaphore>          renderFinishedSems;
    std::vector<VkFence>              inFlightFences;
    std::vector<VkFence>              imagesInFlight;
    size_t                            currentFrame = 0;
};

std::unique_ptr<Vulkan_Swapchain> CreateSwapchain(Vulkan_RenderDevice *rd, VkExtent2D windowExtent, std::shared_ptr<Vulkan_Swapchain> prev);
void                              DestroySwapchain(Vulkan_Swapchain *swapchain, Vulkan_RenderDevice *rd);

VkResult SubmitCommandBuffers(Vulkan_Swapchain *swapchain, Vulkan_RenderDevice *rd, const VkCommandBuffer *buffers, u32 *imageIndex);

}
