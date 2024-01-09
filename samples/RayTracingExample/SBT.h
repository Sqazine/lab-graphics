#pragma once
#include <cstdint>
#include <vulkan/vulkan.h>
#include <vector>
#include "VkUtils.h"
#include "labgraphics.h"
struct SBT
{
public:
    SBT();
    ~SBT();

    void Init(const uint32_t numHitGroups, const uint32_t numMissGroups, const uint32_t shaderHandleSize, const uint32_t shaderGroupAlignment);
    void Destroy();

    void SetRayGenStage(const VkPipelineShaderStageCreateInfo &stage);
    void AddStageToHitGroup(const std::vector<VkPipelineShaderStageCreateInfo> &stages, const uint32_t groupIndex);
    void AddStageToMissGroup(const VkPipelineShaderStageCreateInfo &stage, const uint32_t groupIndex);

    uint32_t GetGroupsStride() const;
    uint32_t GetNumGroups() const;
    uint32_t GetRayGenOffset() const;
    uint32_t GetRayGenSize() const;
    uint32_t GetHitGroupsOffset() const;
    uint32_t GetHitGroupsSize() const;
    uint32_t GetMissGroupsOffset() const;
    uint32_t GetMissGroupsSize() const;

    uint32_t GetNumStages() const;
    const VkPipelineShaderStageCreateInfo *GetStages() const;
    const VkRayTracingShaderGroupCreateInfoKHR *GetGroups() const;

    uint32_t GetSize() const;
    void Create(Device* device, VkPipeline rtPipeline);
    VkDeviceAddress GetAddress() const;

private:
    uint32_t mShaderHandleSize;
    uint32_t mShaderGroupAlignment;
    uint32_t mNumHitGroups;
    uint32_t mNumMissGroups;
    std::vector<uint32_t> mNumHitShaders;
    std::vector<uint32_t> mNumMissShaders;
    std::vector<VkPipelineShaderStageCreateInfo> mStages;
    std::vector<VkRayTracingShaderGroupCreateInfoKHR> mGroups;
    VkUtils::Buffer mSBTBuffer;
};
