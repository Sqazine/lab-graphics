#pragma once
#include <vulkan/vulkan.h>
#include <string_view>
#include <spirv_reflect.h>
#include "Enum.h"
struct SpirvReflectedData
{
    std::vector<SpvReflectInterfaceVariable *> inputVariables;
    std::vector<SpvReflectInterfaceVariable *> ouputVariables;
    std::vector<SpvReflectBlockVariable *> pushConstants;
    std::vector<SpvReflectDescriptorSet *> descriptorSets;
    std::vector<SpvReflectDescriptorBinding *> descriptorBindings;
};

class Shader
{
public:
    Shader(const class Device &device, ShaderStage type, std::string_view src);
    Shader(const class Device &device, ShaderStage type, const std::vector<char> &src);
    ~Shader();

    const VkShaderModule &GetHandle() const;
    const VkPipelineShaderStageCreateInfo &GetPipelineStageInfo() const;

    const SpirvReflectedData &GetReflectedData() const;

private:
    void SpirvReflect(const std::vector<uint32_t> &spvCode);
    void SpirvReflect(size_t count, const uint32_t *spvCode);

    const class Device &mDevice;
    VkShaderModule mHandle;
    VkPipelineShaderStageCreateInfo mPipelineInfo{};

    SpirvReflectedData mReflectedData;
};