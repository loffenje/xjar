#pragma once

#include "types.h"
#define VK_USE_PLATFORM_WIN32_KHR
#include <vulkan/vulkan.h>

namespace xjar {

struct Vulkan_Texture {
    VkImage        image;
    VkDeviceMemory memory;
    VkImageView    view;
    VkSampler      sampler;
};

}
