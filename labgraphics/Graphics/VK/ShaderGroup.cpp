#include "ShaderGroup.h"
#include "Logger.h"
void RasterShaderGroup::SetVertexShader(Shader *shader)
{
    mVertexShader.reset(shader);
}

void RasterShaderGroup::SetTessellationControlShader(Shader *shader)
{
    mTessCtrlShader.reset(shader);
}

void RasterShaderGroup::SetTessellationEvaluationShader(Shader *shader)
{
    mTessEvalShader.reset(shader);
}

void RasterShaderGroup::SetGeometryShader(Shader *shader)
{
    mGeometryShader.reset(shader);
}

void RasterShaderGroup::SetFragmentShader(Shader *shader)
{
    mFragmentShader.reset(shader);
}

std::vector<VkPipelineShaderStageCreateInfo> RasterShaderGroup::GetShaderStages()
{
    if (!mVertexShader)
        LOG_ERROR("Vertex shader is necessary in rasterization");

    if (!mFragmentShader)
        LOG_ERROR("Fragment shader is necessary in rasterization");

    CheckLink();

    std::vector<VkPipelineShaderStageCreateInfo> result;

    result.emplace_back(mVertexShader->GetPipelineStageInfo());

    if (mTessCtrlShader)
        result.emplace_back(mTessCtrlShader->GetPipelineStageInfo());

    if (mTessEvalShader)
        result.emplace_back(mTessEvalShader->GetPipelineStageInfo());

    if (mGeometryShader)
        result.emplace_back(mGeometryShader->GetPipelineStageInfo());

    result.emplace_back(mFragmentShader->GetPipelineStageInfo());

    return result;
}

bool RasterShaderGroup::CheckLink()
{
    auto vertReflData = mVertexShader->GetReflectedData();
    auto fragReflData = mFragmentShader->GetReflectedData();
    return true;
}

void ComputeShaderGroup::SetShader(Shader *shader)
{
    mCompShader.reset(shader);
}
std::vector<VkPipelineShaderStageCreateInfo> ComputeShaderGroup::GetShaderStages()
{
    CheckLink();
    return {mCompShader->GetPipelineStageInfo()};
}

bool ComputeShaderGroup::CheckLink()
{
    return true;
}

void RayTraceShaderGroup::SetRayGenShader(Shader *shader)
{
    mRayGenShader.reset(shader);
}

void RayTraceShaderGroup::AddRayClosestHitShader(Shader *shader)
{
    mRayClosestHitShaderList.emplace_back(shader);
}

void RayTraceShaderGroup::AddRayMissShader(Shader *shader)
{
    mRayMissShaderList.emplace_back(shader);
}

void RayTraceShaderGroup::AddRayAnyHitShader(Shader *shader)
{
    mRayAnyHitShaderList.emplace_back(shader);
}

void RayTraceShaderGroup::AddRayIntersectionShader(Shader *shader)
{
    mRayIntersectionShaderList.emplace_back(shader);
}

void RayTraceShaderGroup::AddRayCallableShader(Shader *shader)
{
    mRayCallableShaderList.emplace_back(shader);
}

std::vector<VkPipelineShaderStageCreateInfo> RayTraceShaderGroup::GetShaderStages()
{
    mShaderStages.clear();

    mShaderStages.emplace_back(mRayGenShader->GetPipelineStageInfo());

    for (const auto &rayMiss : mRayMissShaderList)
        mShaderStages.emplace_back(rayMiss->GetPipelineStageInfo());

    for (const auto &rayClosestHit : mRayClosestHitShaderList)
        mShaderStages.emplace_back(rayClosestHit->GetPipelineStageInfo());

    for (const auto &rayAnyHit : mRayAnyHitShaderList)
        mShaderStages.emplace_back(rayAnyHit->GetPipelineStageInfo());

    for (const auto &rayIntersection : mRayIntersectionShaderList)
        mShaderStages.emplace_back(rayIntersection->GetPipelineStageInfo());

    for (const auto &rayCallable : mRayCallableShaderList)
        mShaderStages.emplace_back(rayCallable->GetPipelineStageInfo());

    return mShaderStages;
}

const VkRayTracingShaderGroupCreateInfoKHR &RayTraceShaderGroup::GetRayGenShaderGroup() const
{
    return mRayGenShaderGroup;
}

const std::vector<VkRayTracingShaderGroupCreateInfoKHR> &RayTraceShaderGroup::GetRayClosestHitShaderGroups() const
{
    return mRayClosestHitShaderGroups;
}

const std::vector<VkRayTracingShaderGroupCreateInfoKHR> &RayTraceShaderGroup::GetRayMissShaderGroups() const
{
    return mRayMissShaderGroups;
}

const std::vector<VkRayTracingShaderGroupCreateInfoKHR> &RayTraceShaderGroup::GetRayAnyHitShaderGroups() const
{
    return mRayAnyHitShaderGroups;
}

const std::vector<VkRayTracingShaderGroupCreateInfoKHR> &RayTraceShaderGroup::GetRayIntersectionShaderGroups() const
{
    return mRayIntersectionShaderGroups;
}

const std::vector<VkRayTracingShaderGroupCreateInfoKHR> &RayTraceShaderGroup::GetRayCallableShaderGroups() const
{
    return mRayCallableShaderGroups;
}

const std::vector<VkRayTracingShaderGroupCreateInfoKHR> &RayTraceShaderGroup::GetShaderGroups()
{
    mRayMissShaderGroups.clear();
    mRayClosestHitShaderGroups.clear();
    mRayAnyHitShaderGroups.clear();
    mRayIntersectionShaderGroups.clear();
    mRayCallableShaderGroups.clear();

    mShaderGroups.clear();

    int32_t idx = 0;

    mRayGenShaderGroup = CreateRayGenShaderGroup(idx++);

    for (int32_t i = 0; i < mRayMissShaderList.size(); ++i)
        mRayMissShaderGroups.emplace_back(CreateRayMissShaderGroup(idx++));

    for (int32_t i = 0; i < mRayClosestHitShaderList.size(); ++i)
        mRayClosestHitShaderGroups.emplace_back(CreateRayClosestHitShaderGroup(idx++));

    for (int32_t i = 0; i < mRayAnyHitShaderList.size(); ++i)
        mRayAnyHitShaderGroups.emplace_back(CreateRayAnyHitShaderGroup(idx++));

    for (int32_t i = 0; i < mRayIntersectionShaderList.size(); ++i)
        mRayIntersectionShaderGroups.emplace_back(CreateRayIntersectionShaderGroup(idx++));

    for (int32_t i = 0; i < mRayCallableShaderList.size(); ++i)
        mRayCallableShaderGroups.emplace_back(CreateRayCallableShaderGroup(idx++));

    mShaderGroups.emplace_back(mRayGenShaderGroup);
    mShaderGroups.insert(mShaderGroups.end(), mRayMissShaderGroups.begin(), mRayMissShaderGroups.end());
    mShaderGroups.insert(mShaderGroups.end(), mRayClosestHitShaderGroups.begin(), mRayClosestHitShaderGroups.end());
    mShaderGroups.insert(mShaderGroups.end(), mRayAnyHitShaderGroups.begin(), mRayAnyHitShaderGroups.end());
    mShaderGroups.insert(mShaderGroups.end(), mRayIntersectionShaderGroups.begin(), mRayIntersectionShaderGroups.end());
    mShaderGroups.insert(mShaderGroups.end(), mRayCallableShaderGroups.begin(), mRayCallableShaderGroups.end());

    return mShaderGroups;
}

bool RayTraceShaderGroup::CheckLink()
{
    return true;
}

VkRayTracingShaderGroupCreateInfoKHR RayTraceShaderGroup::CreateRayGenShaderGroup(uint32_t idx)
{
    VkRayTracingShaderGroupCreateInfoKHR rayGenGroup{};
    rayGenGroup.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
    rayGenGroup.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
    rayGenGroup.generalShader = idx;
    rayGenGroup.closestHitShader = VK_SHADER_UNUSED_KHR;
    rayGenGroup.anyHitShader = VK_SHADER_UNUSED_KHR;
    rayGenGroup.intersectionShader = VK_SHADER_UNUSED_KHR;

    return rayGenGroup;
}

VkRayTracingShaderGroupCreateInfoKHR RayTraceShaderGroup::CreateRayClosestHitShaderGroup(uint32_t idx)
{
    VkRayTracingShaderGroupCreateInfoKHR rayClosestHitGroup{};
    rayClosestHitGroup.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
    rayClosestHitGroup.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR;
    rayClosestHitGroup.generalShader = VK_SHADER_UNUSED_KHR;
    rayClosestHitGroup.closestHitShader = idx;
    rayClosestHitGroup.anyHitShader = VK_SHADER_UNUSED_KHR;
    rayClosestHitGroup.intersectionShader = VK_SHADER_UNUSED_KHR;
    return rayClosestHitGroup;
}

VkRayTracingShaderGroupCreateInfoKHR RayTraceShaderGroup::CreateRayMissShaderGroup(uint32_t idx)
{
    VkRayTracingShaderGroupCreateInfoKHR rayMissGroup{};
    rayMissGroup.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
    rayMissGroup.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
    rayMissGroup.generalShader = idx;
    rayMissGroup.closestHitShader = VK_SHADER_UNUSED_KHR;
    rayMissGroup.anyHitShader = VK_SHADER_UNUSED_KHR;
    rayMissGroup.intersectionShader = VK_SHADER_UNUSED_KHR;
    return rayMissGroup;
}

VkRayTracingShaderGroupCreateInfoKHR RayTraceShaderGroup::CreateRayAnyHitShaderGroup(uint32_t idx)
{
    VkRayTracingShaderGroupCreateInfoKHR rayAnyHitGroup{};
    rayAnyHitGroup.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
    rayAnyHitGroup.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR;
    rayAnyHitGroup.generalShader = VK_SHADER_UNUSED_KHR;
    rayAnyHitGroup.closestHitShader = VK_SHADER_UNUSED_KHR;
    rayAnyHitGroup.anyHitShader = idx;
    rayAnyHitGroup.intersectionShader = VK_SHADER_UNUSED_KHR;
    return rayAnyHitGroup;
}

VkRayTracingShaderGroupCreateInfoKHR RayTraceShaderGroup::CreateRayIntersectionShaderGroup(uint32_t idx)
{
    VkRayTracingShaderGroupCreateInfoKHR rayIntersectionGroup{};
    rayIntersectionGroup.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
    rayIntersectionGroup.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR;
    rayIntersectionGroup.generalShader = VK_SHADER_UNUSED_KHR;
    rayIntersectionGroup.closestHitShader = VK_SHADER_UNUSED_KHR;
    rayIntersectionGroup.anyHitShader = VK_SHADER_UNUSED_KHR;
    rayIntersectionGroup.intersectionShader = idx;
    return rayIntersectionGroup;
}

VkRayTracingShaderGroupCreateInfoKHR RayTraceShaderGroup::CreateRayCallableShaderGroup(uint32_t idx)
{
    VkRayTracingShaderGroupCreateInfoKHR rayCallableGroup{};
    rayCallableGroup.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
    rayCallableGroup.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
    rayCallableGroup.generalShader = idx;
    rayCallableGroup.closestHitShader = VK_SHADER_UNUSED_KHR;
    rayCallableGroup.anyHitShader = VK_SHADER_UNUSED_KHR;
    rayCallableGroup.intersectionShader = VK_SHADER_UNUSED_KHR;
    return rayCallableGroup;
}
