#pragma once

#include "renderer/renderer_types.h"

#define VK_USE_PLATFORM_WIN32_KHR
#include <vulkan/vulkan.h>

namespace xjar {

struct Vulkan_RenderDevice;

struct Vulkan_Model {
    VkBuffer             vertexBuffer;
    VkDeviceMemory       vertexBufferMemory;
    u32                  vertexCount;
    Vulkan_RenderDevice *rd;

    void Init(Vulkan_RenderDevice *_rd, const std::vector<Vertex3D> &vertices);
    void Bind(VkCommandBuffer cmdbuf);
    void Draw(VkCommandBuffer cmdbuf);
};

};
