#pragma once
#include <vulkan/vulkan.h>
#include <vector>
#include "Shader.h"
#include "Sampler.h"
#include "Enum.h"

struct DescriptorBinding
{
    uint32_t bindingPoint;
    uint32_t count;
    DescriptorType type;
    ShaderStage shaderStage;
    Sampler *sampler = nullptr;

    DescriptorBinding(uint32_t bindingPoint, uint32_t count, DescriptorType type, ShaderStage shaderStage, Sampler *sampler)
        : bindingPoint(bindingPoint), count(count), type(type), shaderStage(shaderStage), sampler(sampler)
    {
    }

    VkDescriptorSetLayoutBinding ToVkDescriptorBinding() const
    {
        VkDescriptorSetLayoutBinding tmp{};
        tmp.binding = bindingPoint;
        tmp.descriptorCount = count;
        tmp.descriptorType = DESCRIPTOR_TYPE_CAST(type);
        tmp.stageFlags = SHADER_STAGE_CAST(shaderStage);
        tmp.pImmutableSamplers = sampler ? &sampler->GetHandle() : VK_NULL_HANDLE;
        return tmp;
    }
};

class DescriptorSetLayout
{
public:
    DescriptorSetLayout(const class Device &device);
    DescriptorSetLayout(const class Device &device, const std::vector<DescriptorBinding> &setLayoutBindings);
    ~DescriptorSetLayout();

    DescriptorSetLayout &AddLayoutBinding(const DescriptorBinding &binding);
    DescriptorSetLayout &AddLayoutBinding(uint32_t binding, uint32_t count, DescriptorType type, ShaderStage shaderStage, Sampler *pImmutableSamplers = nullptr);

    const VkDescriptorSetLayout &GetHandle();
    VkDescriptorSetLayoutBinding GetVkLayoutBinding(uint32_t i);
    const DescriptorBinding &GetLayoutBinding(uint32_t i) const;
    uint32_t GetBindingCount() const;

private:
    void Build();

    const class Device &mDevice;

    std::vector<DescriptorBinding> mBindings;

    VkDescriptorSetLayout mHandle;
};