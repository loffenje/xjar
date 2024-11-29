#pragma once

#define VK_USE_PLATFORM_WIN32_KHR
#include <vulkan/vulkan.h>

#include <vector>

namespace xjar {

struct Vulkan_RenderDevice {
	VkDevice						device;
	VkPhysicalDevice				physicalDevice;
	VkQueue							graphicsQueue;
	VkQueue							presentQueue;
	VkSwapchainKHR					swapChain;
	VkFormat					    swapChainImageFormat;
	std::vector<VkSemaphore>		imageAvailableSem;
	std::vector<VkSemaphore>		renderFinishedSem;
	std::vector<VkFence>			inFlightFences;
	std::vector<VkImage>			swapChainImages;
	std::vector<VkImageView>		swapChainImageViews;
	std::vector<VkCommandBuffer>	commandBuffers;
	VkCommandPool					commandPool;
	int								framebufferWidth;
	int								framebufferHeight;
};

}
