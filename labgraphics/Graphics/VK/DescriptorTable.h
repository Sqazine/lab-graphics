#pragma once
#include <memory>
#include <vector>
#include "Sampler.h"
class DescriptorTable
{
public:
    DescriptorTable(const class Device &device);
    ~DescriptorTable();

    DescriptorTable &AddLayoutBinding(const struct DescriptorBinding &binding);
    DescriptorTable &AddLayoutBinding(uint32_t binding, uint32_t count, DescriptorType type, ShaderStage shaderStage, Sampler *pImmutableSamplers = nullptr);

    class DescriptorSet *AllocateDescriptorSet();
    std::vector<class DescriptorSet *> AllocateDescriptorSets(uint32_t count);

    class DescriptorSetLayout *GetLayout();
    class DescriptorPool *GetPool();

    struct DescriptorBinding GetLayoutBinding(uint32_t i);
    uint32_t GetBindingCount() const;

private:
    std::unique_ptr<class DescriptorPool> mDescriptorPool;
    std::unique_ptr<class DescriptorSetLayout> mDescriptorLayout;
};