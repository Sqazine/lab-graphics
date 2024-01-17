#pragma once
#include "Shader.h"
#include <vulkan/vulkan.h>
#include <vector>
#include <memory>

class ShaderGroup
{
public:
    ShaderGroup() = default;
    virtual ~ShaderGroup() = default;
    virtual std::vector<VkPipelineShaderStageCreateInfo> GetShaderStages() = 0;

protected:
    virtual bool CheckLink() = 0;
};

class RasterShaderGroup : public ShaderGroup
{
public:
    RasterShaderGroup() = default;
    ~RasterShaderGroup() = default;

    void SetVertexShader(Shader *shader);
    void SetTessellationControlShader(Shader *shader);
    void SetTessellationEvaluationShader(Shader *shader);
    void SetGeometryShader(Shader *shader);
    void SetFragmentShader(Shader *shader);

    std::vector<VkPipelineShaderStageCreateInfo> GetShaderStages() override;

protected:
    bool CheckLink() override;

private:
    std::unique_ptr<Shader> mVertexShader{nullptr};
    std::unique_ptr<Shader> mTessCtrlShader{nullptr};
    std::unique_ptr<Shader> mTessEvalShader{nullptr};
    std::unique_ptr<Shader> mGeometryShader{nullptr};
    std::unique_ptr<Shader> mFragmentShader{nullptr};
};

class ComputeShaderGroup : public ShaderGroup
{
public:
    ComputeShaderGroup() = default;
    ~ComputeShaderGroup() = default;
    void SetShader(Shader *shader);
    std::vector<VkPipelineShaderStageCreateInfo> GetShaderStages() override;

protected:
    bool CheckLink() override;

private:
    std::unique_ptr<Shader> mCompShader{nullptr};
};

class RayTraceShaderGroup : ShaderGroup
{
public:
    RayTraceShaderGroup() = default;
    ~RayTraceShaderGroup() = default;

    void SetRayGenShader(Shader *shader);

    void AddRayClosestHitShader(Shader *shader);
    void AddRayMissShader(Shader *shader);
    void AddRayAnyHitShader(Shader *shader);
    void AddRayIntersectionShader(Shader *shader);
    void AddRayCallableShader(Shader *shader);

    std::vector<VkPipelineShaderStageCreateInfo> GetShaderStages() override;

    const VkRayTracingShaderGroupCreateInfoKHR &GetRayGenShaderGroup() const;
    const std::vector<VkRayTracingShaderGroupCreateInfoKHR> &GetRayClosestHitShaderGroups() const;
    const std::vector<VkRayTracingShaderGroupCreateInfoKHR> &GetRayMissShaderGroups() const;
    const std::vector<VkRayTracingShaderGroupCreateInfoKHR> &GetRayAnyHitShaderGroups() const;
    const std::vector<VkRayTracingShaderGroupCreateInfoKHR> &GetRayIntersectionShaderGroups() const;
    const std::vector<VkRayTracingShaderGroupCreateInfoKHR> &GetRayCallableShaderGroups() const;

    const std::vector<VkRayTracingShaderGroupCreateInfoKHR> &GetShaderGroups();

protected:
    bool CheckLink() override;

private:
    VkRayTracingShaderGroupCreateInfoKHR CreateRayGenShaderGroup(uint32_t idx);
    VkRayTracingShaderGroupCreateInfoKHR CreateRayClosestHitShaderGroup(uint32_t idx);
    VkRayTracingShaderGroupCreateInfoKHR CreateRayMissShaderGroup(uint32_t idx);
    VkRayTracingShaderGroupCreateInfoKHR CreateRayAnyHitShaderGroup(uint32_t idx);
    VkRayTracingShaderGroupCreateInfoKHR CreateRayIntersectionShaderGroup(uint32_t idx);
    VkRayTracingShaderGroupCreateInfoKHR CreateRayCallableShaderGroup(uint32_t idx);

    std::unique_ptr<Shader> mRayGenShader;
    std::vector<std::unique_ptr<Shader>> mRayClosestHitShaderList;
    std::vector<std::unique_ptr<Shader>> mRayMissShaderList;
    std::vector<std::unique_ptr<Shader>> mRayAnyHitShaderList;
    std::vector<std::unique_ptr<Shader>> mRayIntersectionShaderList;
    std::vector<std::unique_ptr<Shader>> mRayCallableShaderList;

    std::vector<VkPipelineShaderStageCreateInfo> mShaderStages;

    VkRayTracingShaderGroupCreateInfoKHR mRayGenShaderGroup;
    std::vector<VkRayTracingShaderGroupCreateInfoKHR> mRayClosestHitShaderGroups;
    std::vector<VkRayTracingShaderGroupCreateInfoKHR> mRayMissShaderGroups;
    std::vector<VkRayTracingShaderGroupCreateInfoKHR> mRayAnyHitShaderGroups;
    std::vector<VkRayTracingShaderGroupCreateInfoKHR> mRayIntersectionShaderGroups;
    std::vector<VkRayTracingShaderGroupCreateInfoKHR> mRayCallableShaderGroups;

    std::vector<VkRayTracingShaderGroupCreateInfoKHR> mShaderGroups;
};