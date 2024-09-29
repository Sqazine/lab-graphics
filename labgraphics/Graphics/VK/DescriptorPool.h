#pragma once
#include <vulkan/vulkan.h>
#include <vector>
#include <memory>
#include <unordered_map>
#include "DescriptorSetLayout.h"
#include "DescriptorSet.h"

class DescriptorPool
{
public:
    DescriptorPool(const class Device &device);
    ~DescriptorPool();

    void AddPoolDesc(DescriptorType type,uint32_t count);

    const VkDescriptorPool &GetHandle();

    DescriptorSet* AllocateDescriptorSet(DescriptorSetLayout* descLayout);
    std::vector<DescriptorSet*> AllocateDescriptorSets(DescriptorSetLayout* descLayout,uint32_t count);
private:
    void Build();

    const class Device &mDevice;

    std::vector<std::unique_ptr<DescriptorSet>> mDescriptorSetCache;
    std::unordered_map<DescriptorType, uint32_t> mPoolDescs;
    VkDescriptorPool mHandle;
};