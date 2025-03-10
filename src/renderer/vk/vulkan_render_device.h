#pragma once


namespace xjar {


struct QueueFamily {
    std::optional<u32> graphicsFamily;
    std::optional<u32> presentFamily;

    inline bool IsComplete() const {
        return graphicsFamily.has_value() && presentFamily.has_value();
    }
};

struct SwapChainSupportDetails {
    VkSurfaceCapabilitiesKHR        caps;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR>   presentModes;
};

struct Vulkan_RenderDevice {
    VkInstance       instance;
    VkDevice         device;
    VkPhysicalDevice physicalDevice;
    VkQueue          graphicsQueue;
    VkQueue          presentQueue;
    VkSurfaceKHR     surface;
    VkCommandPool    commandPool;
    VkFormat         swapchainImageFormat;
};

QueueFamily             FindQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR surface);
SwapChainSupportDetails QuerySwapChainSupport(VkPhysicalDevice device, VkSurfaceKHR surface);

VkFormat FindSupportedFormat(VkPhysicalDevice physicalDevice, const std::vector<VkFormat> &candidates, VkImageTiling tiling, VkFormatFeatureFlags features);

VkFormat FindDepthFormat(VkPhysicalDevice physicalDevice);

Vulkan_RenderDevice CreateRenderDevice(const char *appName, const char *engineName);
void                DestroyRenderDevice(Vulkan_RenderDevice *rd);

void CreateImage(Vulkan_RenderDevice  *rd,
                 VkImageCreateInfo     imageInfo,
                 VkMemoryPropertyFlags properties,
                 VkImage              &image,
                 VkDeviceMemory       &imageMemory);

void CopyImage2Image(VkCommandBuffer cmdbuf, VkImage src, VkImage dest, VkExtent2D srcExtent, VkExtent2D destExtent);

void CreateBuffer(Vulkan_RenderDevice  *rd,
                  VkDeviceSize          size,
                  VkBufferUsageFlags    usage,
                  VkMemoryPropertyFlags properties,
                  VkBuffer             &buffer,
                  VkDeviceMemory       &bufferMemory);

VkCommandBuffer BeginImmediateCommands(Vulkan_RenderDevice *rd);
void            EndImmediateCommands(Vulkan_RenderDevice *rd, VkCommandBuffer cmdbuf);
void            UploadBufferData(Vulkan_RenderDevice *rd, const VkDeviceMemory &memory, VkDeviceSize deviceOffset, const void *data, const size_t dataSize);

    void TransitionImageLayout(VkCommandBuffer      cmdbuf,
                           VkImage              image,
                           VkFormat             format,
                           VkImageLayout        oldLayout,
                           VkImageLayout        newLayout,
                           u32                  layerCount = 1);

VkShaderModule CreateShaderModule(Vulkan_RenderDevice *rd, std::span<char> code);

void CopyBufferToImage(VkCommandBuffer cmdbuf, VkBuffer buffer, VkImage image, u32 width, u32 height);

}
