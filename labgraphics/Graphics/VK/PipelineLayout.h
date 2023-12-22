#pragma once
#include <vulkan/vulkan.h>
#include <vector>
#include "DescriptorSetLayout.h"
class PipelineLayout
{
public:
    PipelineLayout(const class Device &device);
    ~PipelineLayout();

    PipelineLayout &AddDescriptorSetLayout(DescriptorSetLayout *descriptorSetLayout);
    PipelineLayout &SetDescriptorSetLayouts(const std::vector<DescriptorSetLayout *> &descriptorSetLayouts);

    const VkPipelineLayout &GetHandle();

private:
    void Build();

    std::vector<DescriptorSetLayout *> mDescriptorSetLayoutCache;

    const class Device &mDevice;
    VkPipelineLayout mHandle;
};