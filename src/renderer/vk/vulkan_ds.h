#pragma once

#include "types.h"
#define VK_USE_PLATFORM_WIN32_KHR
#include <vulkan/vulkan.h>

#include <vector>

namespace xjar {

#if 0
struct DescriptorLayoutBuilder {
    std::vector<VkDescriptorSetLayoutBinding> bindings;

    void AddBinding(u32 binding, VkDescriptorType type);
    void Clear();

    VkDescriptorSetLayout Build(VkDevice device, VkShaderStageFlags shaderStages, VkDescriptorSetLayoutCreateFlags flags = 0, void *next = nullptr);
};

class DescriptorAllocator {
public:
    struct PoolSizeRatio {
        VkDescriptorType type;
        f32              ratio;
    };

    void            Init(VkDevice device, u32 initSets, std::span<PoolSizeRatio> poolRatios);
    void            ClearPools(VkDevice device);
    void            DestroyPools(VkDevice device);
    VkDescriptorSet Allocate(VkDevice, VkDescriptorSetLayout layout, void *next = nullptr);

private:
    VkDescriptorPool GetPool(VkDevice device);
    VkDescriptorPool CreatePool(VkDevice device, u32 setCount, std::span<PoolSizeRatio> poolRatios);

    std::vector<PoolSizeRatio>    m_ratios;
    std::vector<VkDescriptorPool> m_fullPools;
    std::vector<VkDescriptorPool> m_readyPools;
    u32                           m_setsPerPool;
};


struct DescriptorWriter {
    std::deque<VkDescriptorImageInfo> imageInfos;
    std::deque<VkDescriptorBufferInfo> bufferInfos;
    std::vector<VkWriteDescriptorSet> writes;

    void WriteImage(int binding, VkImageView imageView, VkSampler sampler, VkImageLayout imageLayout, VkDescriptorType type);
    void WriteBuffer(int binding, VkBuffer buffer, size_t size, size_t offset, VkDescriptorType type);
    void Clear();
    void UpdateSet(VkDevice device, VkDescriptorSet ds);
};

#endif
}
