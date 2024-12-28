#include "vulkan_render_device.h"
#include "types.h"
#if RENDERER_BACKEND == OpenGL
#include "glfw_gl.h"
#else
#include "glfw_vk.h"
#endif

#include "window.h"
#include <stdio.h>
#include <vector>
#include <optional>
#include <set>
#include <iostream>
#include <cstdint>
#include <limits>
#include <algorithm>
#include <string_view>
#include <span>
#include <array>

namespace xjar {

static VkDebugUtilsMessengerEXT g_debugMessenger;
#ifdef NDEBUG
const bool ENABLE_VALIDATION = false;
#else
const bool ENABLE_VALIDATION = true;
#endif

const std::vector<const char *> VALIDATION_LAYERS = {
    "VK_LAYER_KHRONOS_validation"};

const std::vector<const char *> DEVICE_EXTENSIONS = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME,
    VK_KHR_SHADER_DRAW_PARAMETERS_EXTENSION_NAME
};

static u32 FindMemoryType(Vulkan_RenderDevice *rd, u32 typeFilter, VkMemoryPropertyFlags properties) {
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(rd->physicalDevice, &memProperties);

    for (u32 i = 0; i < memProperties.memoryTypeCount; i++) {
        if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }

    fprintf(stderr, "Failed to find suitable memory type!\n");
    exit(EXIT_FAILURE);
}

QueueFamily FindQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR surface) {
    QueueFamily families;

    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

    int i = 0;
    for (const auto &queueFamily : queueFamilies) {
        VkBool32 presentSupport = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);
        if (presentSupport) {
            families.presentFamily = i;
        }

        if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            families.graphicsFamily = i;
        }

        if (families.IsComplete()) {
            break;
        }

        i++;
    }

    return families;
}

VkFormat FindSupportedFormat(VkPhysicalDevice physicalDevice, const std::vector<VkFormat> &candidates, VkImageTiling tiling, VkFormatFeatureFlags features) {
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

VkFormat FindDepthFormat(VkPhysicalDevice physicalDevice) {
    return FindSupportedFormat(physicalDevice,
                               {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT},
                               VK_IMAGE_TILING_OPTIMAL,
                               VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
}

VkCommandBuffer BeginImmediateCommands(Vulkan_RenderDevice *rd) {
    VkCommandBufferAllocateInfo allocInfo {};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = rd->commandPool;
    allocInfo.commandBufferCount = 1;

    VkCommandBuffer commandBuffer;
    vkAllocateCommandBuffers(rd->device, &allocInfo, &commandBuffer);

    VkCommandBufferBeginInfo beginInfo {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(commandBuffer, &beginInfo);

    return commandBuffer;
}

void EndImmediateCommands(Vulkan_RenderDevice *rd, VkCommandBuffer cmdbuf) {
    vkEndCommandBuffer(cmdbuf);

    VkSubmitInfo submitInfo {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &cmdbuf;

    vkQueueSubmit(rd->graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(rd->graphicsQueue);

    vkFreeCommandBuffers(rd->device, rd->commandPool, 1, &cmdbuf);
}

void UploadBufferData(Vulkan_RenderDevice *rd, const VkDeviceMemory &memory, VkDeviceSize deviceOffset, const void *data, const size_t dataSize) {
    void *mappedData = nullptr;

    vkMapMemory(rd->device, memory, deviceOffset, dataSize, 0, &mappedData);
    memcpy(mappedData, data, dataSize);
    vkUnmapMemory(rd->device, memory);
}


static bool HasStencilComponent(VkFormat format) {
    return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
}

void TransitionImageLayout(VkCommandBuffer      cmdbuf,
                           VkImage              image,
                           VkFormat             format,
                           VkImageLayout        oldLayout,
                           VkImageLayout        newLayout,
                           u32                  layerCount) {
    VkImageMemoryBarrier barrier {};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = oldLayout;
    barrier.newLayout = newLayout;

    //set to ignored since there is no barrier queue family ownership transfer
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = image;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = layerCount;

    if (newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;

        if (HasStencilComponent(format)) {
            barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
        }
    } else {
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    }

    VkPipelineStageFlags sourceStage;
    VkPipelineStageFlags destinationStage;

    if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

        sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    } else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;

    } else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

        sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destinationStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    } else {
        fprintf(stderr, "Unsupported layout transition\n");
        exit(EXIT_FAILURE);
    }

    vkCmdPipelineBarrier(cmdbuf, sourceStage, destinationStage,
                         0,
                         0, nullptr,
                         0, nullptr,
                         1, &barrier);
}

SwapChainSupportDetails QuerySwapChainSupport(VkPhysicalDevice device, VkSurfaceKHR surface) {
    SwapChainSupportDetails details;

    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.caps);

    u32 formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);

    if (formatCount != 0) {
        details.formats.resize(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.formats.data());
    }

    u32 presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);

    if (presentModeCount != 0) {
        details.presentModes.resize(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, details.presentModes.data());
    }

    return details;
}

VkResult CreateDebugUtilsMessengerEXT(VkInstance                                instance,
                                      const VkDebugUtilsMessengerCreateInfoEXT *pCreateInfo,
                                      const VkAllocationCallbacks              *pAllocator,
                                      VkDebugUtilsMessengerEXT                 *pDebugMessenger) {
    auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
    if (func != nullptr) {
        return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
    } else {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
}

void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks *pAllocator) {
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
    if (func != nullptr) {
        func(instance, debugMessenger, pAllocator);
    }
}

static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT      messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT             messageType,
    const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
    void                                       *pUserData) {
    std::cerr << "validation layer: " << pCallbackData->pMessage << '\n';

    if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
        // Message is important enough to show
    }

    return VK_FALSE;
}

static bool CheckDeviceExtSupport(VkPhysicalDevice device) {
    u32 extensionCount;
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

    std::vector<VkExtensionProperties> availableExtensions(extensionCount);
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

    std::set<std::string> requiredExtensions(DEVICE_EXTENSIONS.begin(), DEVICE_EXTENSIONS.end());

    for (const auto &extension : availableExtensions) {
        requiredExtensions.erase(extension.extensionName);
    }

    return requiredExtensions.empty();
}

static bool IsDeviceSuitable(VkSurfaceKHR surface, VkPhysicalDevice device) {
    VkPhysicalDeviceProperties deviceProperties;
    vkGetPhysicalDeviceProperties(device, &deviceProperties);

    VkPhysicalDeviceFeatures deviceFeatures;
    vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

    bool result = deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU &&
                  deviceFeatures.geometryShader;

    QueueFamily families = FindQueueFamilies(device, surface);

    result = result && families.IsComplete();

    bool extensionsSupported = CheckDeviceExtSupport(device);

    result = result && extensionsSupported;

    bool swapChainCompatible = false;
    if (extensionsSupported) {
        SwapChainSupportDetails swapChainSupport = QuerySwapChainSupport(device, surface);
        swapChainCompatible = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
    }

    VkPhysicalDeviceFeatures supportedFeatures;
    vkGetPhysicalDeviceFeatures(device, &supportedFeatures);

    result = result && swapChainCompatible && supportedFeatures.samplerAnisotropy;

    return result;
}

static void PickPhysicalDevice(Vulkan_RenderDevice *rd) {
    u32 deviceCount;
    vkEnumeratePhysicalDevices(rd->instance, &deviceCount, nullptr);
    if (deviceCount == 0) {
        fprintf(stderr, "Failed to find GPU with Vulkan support\n");
        exit(EXIT_FAILURE);
    }

    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(rd->instance, &deviceCount, devices.data());

    for (const auto &device : devices) {
        if (IsDeviceSuitable(rd->surface, device)) {
            rd->physicalDevice = device;
            break;
        }
    }

    if (rd->physicalDevice == VK_NULL_HANDLE) {
        fprintf(stderr, "Failed to find a suitable GPU\n");
        exit(EXIT_FAILURE);
    }
}

static void InitDebugMessenger(Vulkan_RenderDevice *rd) {
    VkDebugUtilsMessengerCreateInfoEXT createInfo {};
    createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    createInfo.pfnUserCallback = DebugCallback;
    createInfo.pUserData = nullptr; // Optional

    if (CreateDebugUtilsMessengerEXT(rd->instance, &createInfo, nullptr, &g_debugMessenger) != VK_SUCCESS) {
        fprintf(stderr, "Failed to set up debug messenger!\n");
        exit(EXIT_FAILURE);
    }
}

static bool CheckValidationSupport() {
    u32 layerCount;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

    std::vector<VkLayerProperties> availableLayers(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

    for (const char *layerName : VALIDATION_LAYERS) {
        bool layerFound = false;

        for (const auto &layerProperties : availableLayers) {
            if (strcmp(layerName, layerProperties.layerName) == 0) {
                layerFound = true;
                break;
            }
        }

        if (!layerFound) {
            return false;
        }
    }

    return true;
}

std::vector<const char *> GetRequiredExtensions() {
    u32          glfwExtensionCount = 0;
    const char **glfwExtensions;
    glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    std::vector<const char *> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

    if (ENABLE_VALIDATION) {
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }

    return extensions;
}

static void CreateSurface(Vulkan_RenderDevice *rd) {
    VkWin32SurfaceCreateInfoKHR createInfo {};
    createInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
    createInfo.hwnd = (HWND)GetWindow().nativeHandle;
    createInfo.hinstance = GetModuleHandle(nullptr);

    if (vkCreateWin32SurfaceKHR(rd->instance, &createInfo, nullptr, &rd->surface) != VK_SUCCESS) {
        fprintf(stderr, "Failed to create surface\n");
        exit(EXIT_FAILURE);
    }
}

static void CreateDevice(Vulkan_RenderDevice *rd) {
    QueueFamily families = FindQueueFamilies(rd->physicalDevice, rd->surface);

    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    std::set<u32>                        uniqueQueueFamilies = {
        families.graphicsFamily.value(),
        families.presentFamily.value()};

    float queuePriority = 1.0f;
    for (uint32_t queueFamily : uniqueQueueFamilies) {
        VkDeviceQueueCreateInfo queueCreateInfo {};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = queueFamily;
        queueCreateInfo.queueCount = 1;
        // it influences the scheduling of command buffer execution
        queueCreateInfo.pQueuePriorities = &queuePriority;
        queueCreateInfos.push_back(queueCreateInfo);
    }

    VkPhysicalDeviceFeatures deviceFeatures {};
    deviceFeatures.samplerAnisotropy = VK_TRUE;

    VkDeviceCreateInfo createInfo {};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.pQueueCreateInfos = queueCreateInfos.data();
    createInfo.queueCreateInfoCount = static_cast<u32>(queueCreateInfos.size());
    createInfo.pEnabledFeatures = &deviceFeatures;
    createInfo.enabledExtensionCount = static_cast<u32>(DEVICE_EXTENSIONS.size());
    createInfo.ppEnabledExtensionNames = DEVICE_EXTENSIONS.data();

    if (ENABLE_VALIDATION) {
        createInfo.enabledLayerCount = static_cast<uint32_t>(VALIDATION_LAYERS.size());
        createInfo.ppEnabledLayerNames = VALIDATION_LAYERS.data();
    } else {
        createInfo.enabledLayerCount = 0;
    }

    if (vkCreateDevice(rd->physicalDevice, &createInfo, nullptr, &rd->device) != VK_SUCCESS) {
        fprintf(stderr, "Failed to create a logical device\n");
        exit(EXIT_FAILURE);
    }

    vkGetDeviceQueue(rd->device, families.graphicsFamily.value(), 0, &rd->graphicsQueue);
    vkGetDeviceQueue(rd->device, families.presentFamily.value(), 0, &rd->presentQueue);
}

void CreateCommandPool(Vulkan_RenderDevice *rd) {
    QueueFamily families = FindQueueFamilies(rd->physicalDevice, rd->surface);

    VkCommandPoolCreateInfo poolInfo {};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT; // alow to be rerecorded individually
    poolInfo.queueFamilyIndex = families.graphicsFamily.value();

    if (vkCreateCommandPool(rd->device, &poolInfo, nullptr, &rd->commandPool) != VK_SUCCESS) {
        fprintf(stderr, "Failed to create command pool.\n");
        exit(EXIT_FAILURE);
    }
}

void CreateImage(Vulkan_RenderDevice  *rd,
                 VkImageCreateInfo     imageInfo,
                 VkMemoryPropertyFlags properties,
                 VkImage              &image,
                 VkDeviceMemory       &imageMemory) {
    if (vkCreateImage(rd->device, &imageInfo, nullptr, &image) != VK_SUCCESS) {
        fprintf(stderr, "Failed to create image\n");
        exit(1);
    }

    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(rd->device, image, &memRequirements);

    VkMemoryAllocateInfo allocInfo {};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = FindMemoryType(rd, memRequirements.memoryTypeBits, properties);

    if (vkAllocateMemory(rd->device, &allocInfo, nullptr, &imageMemory) != VK_SUCCESS) {
        fprintf(stderr, "Failed to allocate memory\n");
        exit(1);
    }

    if (vkBindImageMemory(rd->device, image, imageMemory, 0) != VK_SUCCESS) {
        fprintf(stderr, "Failed to bind image memory\n");
        exit(1);
    }
}

Vulkan_RenderDevice CreateRenderDevice(const char *appName, const char *engineName) {
    Vulkan_RenderDevice rd {};

    // Create Instance
    VkApplicationInfo appInfo {};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = appName;
    appInfo.applicationVersion = 1;
    appInfo.pEngineName = engineName;
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_0;

    VkInstanceCreateInfo createInfo {};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;

    auto extensions = GetRequiredExtensions();
    createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
    createInfo.ppEnabledExtensionNames = extensions.data();

    if (ENABLE_VALIDATION) {
        createInfo.enabledLayerCount = static_cast<uint32_t>(VALIDATION_LAYERS.size());
        createInfo.ppEnabledLayerNames = VALIDATION_LAYERS.data();
    } else {
        createInfo.enabledLayerCount = 0;
    }
    if (vkCreateInstance(&createInfo, nullptr, &rd.instance) != VK_SUCCESS) {
        fprintf(stderr, "Failed to create instance\n");
        exit(EXIT_FAILURE);
    }

    if (ENABLE_VALIDATION) {
        InitDebugMessenger(&rd);
    }

    CreateSurface(&rd);
    PickPhysicalDevice(&rd);
    CreateDevice(&rd);
    CreateCommandPool(&rd);

    return rd;
}

void DestroyRenderDevice(Vulkan_RenderDevice *rd) {
    vkDestroyCommandPool(rd->device, rd->commandPool, nullptr);
    vkDestroyDevice(rd->device, nullptr);

    if (ENABLE_VALIDATION) {
        DestroyDebugUtilsMessengerEXT(rd->instance, g_debugMessenger, nullptr);
    }

    vkDestroySurfaceKHR(rd->instance, rd->surface, nullptr);
    vkDestroyInstance(rd->instance, nullptr);
}

void CopyImage2Image(VkCommandBuffer cmdbuf, VkImage src, VkImage dest, VkExtent2D srcExtent, VkExtent2D destExtent) {
    VkImageBlit2 blitRegion {};
    blitRegion.sType = VK_STRUCTURE_TYPE_IMAGE_BLIT_2;
    blitRegion.pNext = nullptr;

    blitRegion.srcOffsets[1].x = srcExtent.width;
    blitRegion.srcOffsets[1].y = srcExtent.height;
    blitRegion.srcOffsets[1].z = 1;

    blitRegion.dstOffsets[1].x = destExtent.width;
    blitRegion.dstOffsets[1].y = destExtent.height;
    blitRegion.dstOffsets[1].z = 1;

    blitRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    blitRegion.srcSubresource.baseArrayLayer = 0;
    blitRegion.srcSubresource.layerCount = 1;
    blitRegion.srcSubresource.mipLevel = 0;

    blitRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    blitRegion.dstSubresource.baseArrayLayer = 0;
    blitRegion.dstSubresource.layerCount = 1;
    blitRegion.dstSubresource.mipLevel = 0;

    VkBlitImageInfo2 blitInfo {};
    blitInfo.sType = VK_STRUCTURE_TYPE_BLIT_IMAGE_INFO_2;
    blitInfo.pNext = nullptr;
    blitInfo.dstImage = dest;
    blitInfo.dstImageLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    blitInfo.srcImage = src;
    blitInfo.srcImageLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    blitInfo.filter = VK_FILTER_LINEAR;
    blitInfo.regionCount = 1;
    blitInfo.pRegions = &blitRegion;

    vkCmdBlitImage2(cmdbuf, &blitInfo);
}

void CreateBuffer(Vulkan_RenderDevice *rd, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer &buffer, VkDeviceMemory &bufferMemory) {
    VkBufferCreateInfo bufferInfo {};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateBuffer(rd->device, &bufferInfo, nullptr, &buffer) != VK_SUCCESS) {
        fprintf(stderr, "Failed to create buffer!\n");
        exit(EXIT_FAILURE);
    }

    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(rd->device, buffer, &memRequirements);

    VkMemoryAllocateInfo allocInfo {};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = FindMemoryType(rd, memRequirements.memoryTypeBits, properties);

    if (vkAllocateMemory(rd->device, &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS) {
        fprintf(stderr, "Failed to allocate memory for buffer\n");
        exit(EXIT_FAILURE);
    }

    vkBindBufferMemory(rd->device, buffer, bufferMemory, 0);
}

VkShaderModule CreateShaderModule(Vulkan_RenderDevice *rd, std::span<char> code) {
    VkShaderModuleCreateInfo createInfo {};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = code.size();
    createInfo.pCode = reinterpret_cast<const u32 *>(code.data());

    VkShaderModule shaderModule;
    if (vkCreateShaderModule(rd->device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
        fprintf(stderr, "Failed to create shader module.\n");
        exit(EXIT_FAILURE);
    }

    return shaderModule;
}

void CopyBufferToImage(VkCommandBuffer cmdbuf, VkBuffer buffer, VkImage image, u32 width, u32 height) {

    VkBufferImageCopy region {};
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;

    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;

    region.imageOffset = {0, 0, 0};
    region.imageExtent = {width, height, 1};

    vkCmdCopyBufferToImage(cmdbuf,
                           buffer, image,
                           VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
}
}
