#include "pch.h"
#include "vulkan_ds.h"

namespace xjar {

void DescriptorLayoutBuilder::AddBinding(u32 binding, VkDescriptorType type, VkShaderStageFlags shaderStages, u32 dsCount) {
    VkDescriptorSetLayoutBinding b {};
    b.binding = binding;
    b.descriptorCount = dsCount;
    b.descriptorType = type;
    b.stageFlags = shaderStages;

    bindings.push_back(b);
}

void DescriptorLayoutBuilder::Clear() {
    bindings.clear();
}

VkDescriptorSetLayout DescriptorLayoutBuilder::Build(VkDevice device, VkDescriptorSetLayoutCreateFlags flags, void *next) {

    VkDescriptorSetLayoutCreateInfo info {};
    info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    info.pNext = next;
    info.pBindings = bindings.data();
    info.bindingCount = static_cast<u32>(bindings.size());
    info.flags = flags;

    VkDescriptorSetLayout layout;
    if (vkCreateDescriptorSetLayout(device, &info, nullptr, &layout) != VK_SUCCESS) {
        fprintf(stderr, "failed to create descriptor set layout\n");
        exit(1);
    }

    return layout;
}


void DescriptorWriter::WriteImage(int binding, VkImageView imageView, VkSampler sampler, VkImageLayout imageLayout, VkDescriptorType type, u32 descriptorCount) {
    VkDescriptorImageInfo &info = imageInfos.emplace_back(VkDescriptorImageInfo {
        .sampler = sampler,
        .imageView = imageView,
        .imageLayout = imageLayout});

    VkWriteDescriptorSet write {};
    write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write.dstBinding = binding;
    write.dstSet = VK_NULL_HANDLE;
    write.descriptorCount = descriptorCount;
    write.descriptorType = type;
    write.pImageInfo = &info;

    writes.push_back(write);
}

void DescriptorWriter::WriteBuffer(int binding, VkBuffer buffer, size_t size, size_t offset, VkDescriptorType type) {
    VkDescriptorBufferInfo &info = bufferInfos.emplace_back(VkDescriptorBufferInfo {
        .buffer = buffer,
        .offset = offset,
        .range = size});

    VkWriteDescriptorSet write {};
    write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write.dstBinding = binding;
    write.dstSet = VK_NULL_HANDLE;
    write.descriptorCount = 1;
    write.descriptorType = type;
    write.pBufferInfo = &info;

    writes.push_back(write);
}
void DescriptorWriter::Clear() {
    imageInfos.clear();
    bufferInfos.clear();
    writes.clear();
}

void DescriptorWriter::UpdateSet(VkDevice device, VkDescriptorSet ds) {
    for (VkWriteDescriptorSet &write : writes) {
        write.dstSet = ds;
    }

    vkUpdateDescriptorSets(device, (u32)writes.size(), writes.data(), 0, nullptr);
}

void DescriptorAllocator::Init(VkDevice device, u32 initSets, std::span<PoolSizeRatio> poolRatios) {
    m_ratios.clear();

    for (auto poolRatio : poolRatios) {
        m_ratios.push_back(poolRatio);
    }

    VkDescriptorPool newPool = CreatePool(device, initSets, poolRatios);
    m_setsPerPool = initSets << 1;
    m_readyPools.push_back(newPool);
}

void DescriptorAllocator::ClearPools(VkDevice device) {
    
    for (auto poolRatio : m_readyPools) {
        vkResetDescriptorPool(device, poolRatio, 0);
    }

    for (auto poolRatio : m_fullPools) {
        vkResetDescriptorPool(device, poolRatio, 0);
        m_readyPools.push_back(poolRatio);
    }

    m_fullPools.clear();
}

void DescriptorAllocator::DestroyPools(VkDevice device) {
    for (auto poolRatio : m_readyPools) {
        vkDestroyDescriptorPool(device, poolRatio, nullptr);
    }

    m_readyPools.clear();


    for (auto poolRatio : m_fullPools) {
        vkDestroyDescriptorPool(device, poolRatio, nullptr);
    }

    m_fullPools.clear();
}

VkDescriptorSet DescriptorAllocator::Allocate(VkDevice device, VkDescriptorSetLayout layout, void *next) {
    VkDescriptorPool poolToUse = GetPool(device);

    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.pNext = next;
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = poolToUse;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &layout;

    VkDescriptorSet ds;
    VkResult result = vkAllocateDescriptorSets(device, &allocInfo, &ds);
    if (result == VK_ERROR_OUT_OF_POOL_MEMORY || result == VK_ERROR_FRAGMENTED_POOL) {
        m_fullPools.push_back(poolToUse);
        poolToUse = GetPool(device);
        allocInfo.descriptorPool = poolToUse;

        if (vkAllocateDescriptorSets(device, &allocInfo, &ds) != VK_SUCCESS) {
            fprintf(stderr, "failed to allocate descriptor set\n");
            exit(1);
        }
    }

    m_readyPools.push_back(poolToUse);

    return ds;
}

VkDescriptorPool DescriptorAllocator::GetPool(VkDevice device) {
    VkDescriptorPool newPool;
    if (!m_readyPools.empty()) {
        newPool = m_readyPools.back();
        m_readyPools.pop_back();
    } else {
        newPool = CreatePool(device, m_setsPerPool, m_ratios);

        m_setsPerPool = m_setsPerPool << 1;
        if (m_setsPerPool > 4092) {
            m_setsPerPool = 4092;
        }
    }

    return newPool;
}

VkDescriptorPool DescriptorAllocator::CreatePool(VkDevice device, u32 setCount, std::span<PoolSizeRatio> poolRatios) {
    std::vector<VkDescriptorPoolSize> poolSizes;
    for (PoolSizeRatio ratio : poolRatios) {
        poolSizes.push_back(VkDescriptorPoolSize{
            .type = ratio.type,
            .descriptorCount = u32(ratio.ratio * setCount)
        });
    }

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.flags = 0;
    poolInfo.maxSets = setCount;
    poolInfo.poolSizeCount = (u32)poolSizes.size();
    poolInfo.pPoolSizes = poolSizes.data();

    VkDescriptorPool newPool;
    if (vkCreateDescriptorPool(device, &poolInfo, nullptr, &newPool) != VK_SUCCESS) {
        fprintf(stderr, "Failed to create ds pool\n");
        exit(1);
    }

    return newPool;
}

}
