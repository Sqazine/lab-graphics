#include "DescriptorSetLayout.h"
#include "Utils.h"
#include "Device.h"
#include <iostream>

DescriptorSetLayout::DescriptorSetLayout(const Device &device)
    : mDevice(device), mHandle(VK_NULL_HANDLE)
{
}

DescriptorSetLayout::DescriptorSetLayout(const Device &device, const std::vector<DescriptorBinding> &setLayoutBindings)
    : mDevice(device), mHandle(VK_NULL_HANDLE)
{
    for (const auto &binding : setLayoutBindings)
        mBindings.emplace_back(binding);
}

DescriptorSetLayout::~DescriptorSetLayout()
{
    vkDestroyDescriptorSetLayout(mDevice.GetHandle(), mHandle, nullptr);
}

DescriptorSetLayout &DescriptorSetLayout::AddLayoutBinding(const DescriptorBinding &binding)
{
    mBindings.emplace_back(binding);
    return *this;
}

DescriptorSetLayout &DescriptorSetLayout::AddLayoutBinding(uint32_t binding, uint32_t count, DescriptorType type, ShaderStage shaderStage, Sampler *pImmutableSamplers)
{
    mBindings.emplace_back(binding, count, type, shaderStage, pImmutableSamplers);
    return *this;
}

const VkDescriptorSetLayout &DescriptorSetLayout::GetHandle()
{
    if (mHandle == VK_NULL_HANDLE)
        Build();

    return mHandle;
}

VkDescriptorSetLayoutBinding DescriptorSetLayout::GetVkLayoutBinding(uint32_t i)
{
    return mBindings[i].ToVkDescriptorBinding();
}

const DescriptorBinding &DescriptorSetLayout::GetLayoutBinding(uint32_t i) const
{
    return mBindings[i];
}

uint32_t DescriptorSetLayout::GetBindingCount() const
{
    return mBindings.size();
}

void DescriptorSetLayout::Build()
{
    std::vector<VkDescriptorSetLayoutBinding> rawDescLayouts(mBindings.size());

    for (int32_t i = 0; i < mBindings.size(); ++i)
        rawDescLayouts[i] = mBindings[i].ToVkDescriptorBinding();

    VkDescriptorSetLayoutCreateInfo descriptorSetLayoutInfo{};
    descriptorSetLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    descriptorSetLayoutInfo.pNext = nullptr;
    descriptorSetLayoutInfo.flags = 0;
    descriptorSetLayoutInfo.bindingCount = rawDescLayouts.size();
    descriptorSetLayoutInfo.pBindings = rawDescLayouts.data();

    VK_CHECK(vkCreateDescriptorSetLayout(mDevice.GetHandle(), &descriptorSetLayoutInfo, nullptr, &mHandle));
}