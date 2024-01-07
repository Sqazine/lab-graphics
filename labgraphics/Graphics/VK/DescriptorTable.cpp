#include "DescriptorTable.h"
#include "Device.h"
#include "DescriptorPool.h"
#include "DescriptorSet.h"
#include "DescriptorSetLayout.h"

DescriptorTable::DescriptorTable(const Device &device)
    : mDescriptorPool(std::make_unique<DescriptorPool>(device)), mDescriptorLayout(std::make_unique<DescriptorSetLayout>(device))
{
}

DescriptorTable::~DescriptorTable()
{
}

DescriptorTable &DescriptorTable::AddLayoutBinding(const DescriptorBinding &binding)
{
    mDescriptorPool->AddPoolDesc(binding.type, binding.count);
    mDescriptorLayout->AddLayoutBinding(binding);
    return *this;
}

DescriptorTable &DescriptorTable::AddLayoutBinding(uint32_t binding, uint32_t count, DescriptorType type, ShaderStage shaderStage, Sampler *pImmutableSamplers)
{
    mDescriptorPool->AddPoolDesc(type, count);
    mDescriptorLayout->AddLayoutBinding(binding, count, type, shaderStage, pImmutableSamplers);
    return *this;
}

DescriptorSet *DescriptorTable::AllocateDescriptorSet()
{
    return mDescriptorPool->AllocateDescriptorSet(mDescriptorLayout.get());
}

std::vector<DescriptorSet *> DescriptorTable::AllocateDescriptorSets(uint32_t count)
{
    return mDescriptorPool->AllocateDescriptorSets(mDescriptorLayout.get(), count);
}

DescriptorSetLayout *DescriptorTable::GetLayout()
{
    return mDescriptorLayout.get();
}

DescriptorPool *DescriptorTable::GetPool()
{
    return mDescriptorPool.get();
}

DescriptorBinding DescriptorTable::GetLayoutBinding(uint32_t i)
{
    return mDescriptorLayout->GetLayoutBinding(i);
}

uint32_t DescriptorTable::GetBindingCount() const
{
    return mDescriptorLayout->GetBindingCount();
}