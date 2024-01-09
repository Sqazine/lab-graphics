#include "SBT.h"
#include <iostream>
SBT::SBT()
    : mShaderHandleSize(0),
      mShaderGroupAlignment(0),
      mNumHitGroups(0),
      mNumMissGroups(0)
{
}
SBT::~SBT() {}

void SBT::Init(const uint32_t numHitGroups, const uint32_t numMissGroups, const uint32_t shaderHandleSize, const uint32_t shaderGroupAlignment)
{
    mNumHitGroups = numHitGroups;
    mNumMissGroups = numMissGroups;
    mShaderHandleSize = shaderHandleSize;
    mShaderGroupAlignment = shaderGroupAlignment;

    mNumHitShaders.resize(numHitGroups, 0);
    mNumMissShaders.resize(numMissGroups, 0);

    mStages.clear();
    mGroups.clear();
}
void SBT::Destroy()
{
    mNumHitShaders.clear();
    mNumMissShaders.clear();
    mStages.clear();
    mGroups.clear();

    mSBTBuffer.Destroy();
}

void SBT::SetRayGenStage(const VkPipelineShaderStageCreateInfo &stage)
{
    //ray gen shader should go first
    assert(mStages.empty());

    mStages.emplace_back(stage);

    VkRayTracingShaderGroupCreateInfoKHR groupInfo{};
    groupInfo.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
    groupInfo.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
    groupInfo.generalShader = 0;
    groupInfo.closestHitShader = VK_SHADER_UNUSED_KHR;
    groupInfo.anyHitShader = VK_SHADER_UNUSED_KHR;
    groupInfo.intersectionShader = VK_SHADER_UNUSED_KHR;
    mGroups.emplace_back(groupInfo); //group 0 always for ray gen
}
void SBT::AddStageToHitGroup(const std::vector<VkPipelineShaderStageCreateInfo> &stages, const uint32_t groupIndex)
{
    assert(!mStages.empty());
    assert(groupIndex < mNumHitShaders.size());
    assert(!stages.empty() && stages.size() <= 3); //only 3 hit shaders per group(intersection any-hit and clostest-hit)
    assert(mNumHitShaders[groupIndex] == 0);

    uint32_t offset = 1;

    for (uint32_t i = 0; i < groupIndex; ++i)
        offset += mNumHitShaders[i];

    auto itStage = mStages.begin() + offset;
    mStages.insert(itStage, stages.begin(), stages.end());

    VkRayTracingShaderGroupCreateInfoKHR groupInfo{};
    groupInfo.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
    groupInfo.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR;
    groupInfo.generalShader = VK_SHADER_UNUSED_KHR;
    groupInfo.closestHitShader = VK_SHADER_UNUSED_KHR;
    groupInfo.anyHitShader = VK_SHADER_UNUSED_KHR;
    groupInfo.intersectionShader = VK_SHADER_UNUSED_KHR;

    for (size_t i = 0; i < stages.size(); ++i)
    {
        const VkPipelineShaderStageCreateInfo &stageInfo = stages[i];
        const uint32_t shaderIdx = offset + i;

        if (stageInfo.stage == VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR)
            groupInfo.closestHitShader = shaderIdx;
        else if (stageInfo.stage == VK_SHADER_STAGE_ANY_HIT_BIT_KHR)
            groupInfo.anyHitShader = shaderIdx;
    }

    mGroups.insert(mGroups.begin() + 1 + groupIndex, groupInfo);

    mNumHitShaders[groupIndex] += stages.size();
}
void SBT::AddStageToMissGroup(const VkPipelineShaderStageCreateInfo &stage, const uint32_t groupIndex)
{
    assert(!mStages.empty());
    assert(groupIndex < mNumMissShaders.size());
    assert(mNumMissShaders[groupIndex] == 0);

    uint32_t offset = 1;

    for (const uint32_t numHitShader : mNumHitShaders)
        offset += numHitShader;

    for (uint32_t i = 0; i < groupIndex; ++i)
        offset += mNumMissShaders[i];

    mStages.insert(mStages.begin() + offset, stage);

    VkRayTracingShaderGroupCreateInfoKHR groupInfo{};
    groupInfo.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
    groupInfo.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
    groupInfo.generalShader = offset;
    groupInfo.closestHitShader = VK_SHADER_UNUSED_KHR;
    groupInfo.anyHitShader = VK_SHADER_UNUSED_KHR;
    groupInfo.intersectionShader = VK_SHADER_UNUSED_KHR;

    mGroups.insert(mGroups.begin() + (groupIndex + 1 + mNumHitGroups), groupInfo);

    mNumMissShaders[groupIndex]++;
}

uint32_t SBT::GetGroupsStride() const
{
    return mShaderGroupAlignment;
}
uint32_t SBT::GetNumGroups() const
{
    return 1 + mNumHitGroups + mNumMissGroups;
}
uint32_t SBT::GetRayGenOffset() const
{
    return 0;
}
uint32_t SBT::GetRayGenSize() const
{
    return mShaderGroupAlignment;
}
uint32_t SBT::GetHitGroupsOffset() const
{
    return GetRayGenOffset() + GetRayGenSize();
}
uint32_t SBT::GetHitGroupsSize() const
{
    return mNumHitGroups * mShaderGroupAlignment;
}
uint32_t SBT::GetMissGroupsOffset() const
{
    return GetHitGroupsOffset() + GetHitGroupsSize();
}
uint32_t SBT::GetMissGroupsSize() const
{
    return mNumMissGroups * mShaderGroupAlignment;
}

uint32_t SBT::GetNumStages() const
{
    return mStages.size();
}
const VkPipelineShaderStageCreateInfo *SBT::GetStages() const
{
    return mStages.data();
}
const VkRayTracingShaderGroupCreateInfoKHR *SBT::GetGroups() const
{
    return mGroups.data();
}

uint32_t SBT::GetSize() const
{
    return GetNumGroups() * mShaderGroupAlignment;
}
void SBT::Create(Device* device, VkPipeline rtPipeline)
{
    const size_t sbtSize = GetSize();

    VK_CHECK(mSBTBuffer.Create(sbtSize,
                                VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_SHADER_BINDING_TABLE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
                                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT));

    //get shader group handles
    std::vector<uint8_t> groupHandles(GetNumGroups() * mShaderHandleSize);
    VK_CHECK(device->vkGetRayTracingShaderGroupHandlesKHR(device->GetHandle(), rtPipeline, 0, GetNumGroups(), groupHandles.size(), groupHandles.data()));

    // now we fill sbt
    uint8_t *mem = static_cast<uint8_t *>(mSBTBuffer.Map());
    for (size_t i = 0; i < GetNumGroups(); ++i)
    {
        memcpy(mem, groupHandles.data() + i * mShaderHandleSize, mShaderHandleSize);
        mem += mShaderGroupAlignment;
    }
    mSBTBuffer.Unmap();
}
VkDeviceAddress SBT::GetAddress() const
{
    return VkUtils::GetBufferDeviceAddress(mSBTBuffer).deviceAddress;
}
