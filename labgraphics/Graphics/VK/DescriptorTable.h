#pragma once
#include <memory>
#include "DescriptorPool.h"
#include "DescriptorSetLayout.h"
#include "Sampler.h"
class DescriptorTable
{
public:
    DescriptorTable(const class Device &device);
    ~DescriptorTable();

    DescriptorTable &AddLayoutBinding(const DescriptorBinding &binding);
    DescriptorTable &AddLayoutBinding(uint32_t binding, uint32_t count, DescriptorType type, ShaderStage shaderStage, Sampler *pImmutableSamplers = nullptr);

    DescriptorSet* AllocateDescriptorSet();
    std::vector<DescriptorSet*> AllocateDescriptorSets(uint32_t count);

    DescriptorSetLayout *GetLayout();
    DescriptorPool *GetPool();

    DescriptorBinding GetLayoutBinding(uint32_t i);
    uint32_t GetBindingCount() const;

private:
    std::unique_ptr<DescriptorPool> mDescriptorPool;
    std::unique_ptr<DescriptorSetLayout> mDescriptorLayout;
};