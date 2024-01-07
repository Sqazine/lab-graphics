#include "DescriptorPool.h"
#include <iostream>
#include "VK/Utils.h"
#include "VK/Device.h"

DescriptorPool::DescriptorPool(const Device &device)
    : mDevice(device), mHandle(VK_NULL_HANDLE)
{
}
DescriptorPool::~DescriptorPool()
{
    mDescriptorSetCache.clear();
    vkDestroyDescriptorPool(mDevice.GetHandle(), mHandle, nullptr);
}

void DescriptorPool::AddPoolDesc(DescriptorType type, uint32_t count)
{
    auto iter = mPoolDescs.find(type);
    if (iter != mPoolDescs.end())
        mPoolDescs[type] += count;
    else
        mPoolDescs[type] = count;
}

const VkDescriptorPool &DescriptorPool::GetHandle()
{
    if (mHandle == VK_NULL_HANDLE)
        Build();
    return mHandle;
}

DescriptorSet *DescriptorPool::AllocateDescriptorSet(DescriptorSetLayout *descLayout)
{
    mDescriptorSetCache.emplace_back(std::move(std::make_unique<DescriptorSet>(mDevice, this, descLayout)));
    return mDescriptorSetCache.back().get();
}

std::vector<DescriptorSet *> DescriptorPool::AllocateDescriptorSets(DescriptorSetLayout *descLayout, uint32_t count)
{
    std::vector<DescriptorSet *> result(count);
    for (int32_t i = 0; i < count; ++i)
        result[i] = AllocateDescriptorSet(descLayout);
    return result;
}

void DescriptorPool::Build()
{
    std::vector<VkDescriptorPoolSize> poolSizes;
    for (const auto &poolDesc : mPoolDescs)
    {
        VkDescriptorPoolSize poolSize{};
        poolSize.type = DESCRIPTOR_TYPE_CAST(poolDesc.first);
        poolSize.descriptorCount = poolDesc.second;
        poolSizes.emplace_back(poolSize);
    }

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.pNext = nullptr;
    poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    poolInfo.maxSets = 256 * poolSizes.size();
    poolInfo.poolSizeCount = poolSizes.size();
    poolInfo.pPoolSizes = poolSizes.data();

    VK_CHECK(vkCreateDescriptorPool(mDevice.GetHandle(), &poolInfo, nullptr, &mHandle));
}