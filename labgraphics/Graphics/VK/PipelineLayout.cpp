#include "PipelineLayout.h"
#include "Device.h"
PipelineLayout::PipelineLayout(const Device &device)
    : mDevice(device), mHandle(VK_NULL_HANDLE)
{
}

PipelineLayout::~PipelineLayout()
{
    vkDestroyPipelineLayout(mDevice.GetHandle(), mHandle, nullptr);
}

PipelineLayout &PipelineLayout::AddDescriptorSetLayout(DescriptorSetLayout *descriptorSetLayout)
{
    mDescriptorSetLayoutCache.emplace_back(descriptorSetLayout);

    return *this;
}

PipelineLayout &PipelineLayout::SetDescriptorSetLayouts(const std::vector<DescriptorSetLayout *> &descriptorSetLayouts)
{
    mDescriptorSetLayoutCache = descriptorSetLayouts;

    return *this;
}

const VkPipelineLayout &PipelineLayout::GetHandle()
{
    if (mHandle == VK_NULL_HANDLE)
        Build();
    return mHandle;
}

void PipelineLayout::Build()
{
    std::vector<VkDescriptorSetLayout> rawLayout(mDescriptorSetLayoutCache.size());

    for (int32_t i = 0; i < rawLayout.size(); ++i)
        rawLayout[i] = mDescriptorSetLayoutCache[i]->GetHandle();

    VkPipelineLayoutCreateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    info.pNext = nullptr;
    info.flags = 0;
    info.setLayoutCount = rawLayout.size();
    info.pSetLayouts = rawLayout.data();
    info.pushConstantRangeCount = 0;
    info.pPushConstantRanges = nullptr;

    VK_CHECK(vkCreatePipelineLayout(mDevice.GetHandle(), &info, nullptr, &mHandle));

    mDescriptorSetLayoutCache.clear();
}