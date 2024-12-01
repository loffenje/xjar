#pragma once

#include "types.h"
#define VK_USE_PLATFORM_WIN32_KHR
#include <vulkan/vulkan.h>

#include <vector>
#include <optional>

namespace xjar {

struct QueueFamily {
    std::optional<u32> graphicsFamily;
    std::optional<u32> presentFamily;

    inline bool IsComplete() const {
        return graphicsFamily.has_value() && presentFamily.has_value();
    }
};

struct SwapChainSupportDetails {
	VkSurfaceCapabilitiesKHR		caps;
	std::vector<VkSurfaceFormatKHR> formats;
	std::vector<VkPresentModeKHR>	presentModes;
};

struct Vulkan_RenderDevice {
    VkInstance                      instance;
	VkDevice						device;
	VkPhysicalDevice				physicalDevice;
	VkQueue							graphicsQueue;
	VkQueue							presentQueue;
    VkSurfaceKHR                    surface;
	VkCommandPool					commandPool;
};

QueueFamily FindQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR surface);
SwapChainSupportDetails QuerySwapChainSupport(VkPhysicalDevice device, VkSurfaceKHR surface);

Vulkan_RenderDevice CreateRenderDevice(const char *appName, const char *engineName);
void DestroyRenderDevice(Vulkan_RenderDevice &rd);

void CreateImage(Vulkan_RenderDevice &rd,
    VkImageCreateInfo imageInfo,
    VkMemoryPropertyFlags properties,
    VkImage &image,
    VkDeviceMemory &imageMemory);
}
