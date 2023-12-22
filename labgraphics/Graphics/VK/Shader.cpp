#include "Shader.h"
#include <vector>
#include <iostream>
#include <cassert>
#include "Device.h"
#include "Utils.h"

Shader::Shader(const Device &device, ShaderStage type, std::string_view src)
	: mDevice(device)
{
	std::vector<uint32_t> spv;
	VK_CHECK(GlslToSpv(SHADER_STAGE_CAST(type), src, spv));

	SpirvReflect(spv);

	VkShaderModuleCreateInfo shaderModuleInfo{};
	shaderModuleInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	shaderModuleInfo.codeSize = spv.size() * sizeof(uint32_t);
	shaderModuleInfo.pCode = spv.data();

	VK_CHECK(vkCreateShaderModule(mDevice.GetHandle(), &shaderModuleInfo, nullptr, &mHandle));

	mPipelineInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	mPipelineInfo.pNext = nullptr;
	mPipelineInfo.flags = 0;
	mPipelineInfo.stage = SHADER_STAGE_CAST(type);
	mPipelineInfo.module = mHandle;
	mPipelineInfo.pName = "main";
	mPipelineInfo.pSpecializationInfo = nullptr;
}

Shader::Shader(const Device &device, ShaderStage type, const std::vector<char> &src)
	: mDevice(device)
{
	SpirvReflect(src.size()/sizeof(uint32_t),reinterpret_cast<const uint32_t*>(src.data()));

	VkShaderModuleCreateInfo shaderModuleInfo{};
	shaderModuleInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	shaderModuleInfo.codeSize = src.size();
	shaderModuleInfo.pCode = reinterpret_cast<const uint32_t*>(src.data());

	VK_CHECK(vkCreateShaderModule(mDevice.GetHandle(), &shaderModuleInfo, nullptr, &mHandle));

	mPipelineInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	mPipelineInfo.pNext = nullptr;
	mPipelineInfo.flags = 0;
	mPipelineInfo.stage = SHADER_STAGE_CAST(type);
	mPipelineInfo.module = mHandle;
	mPipelineInfo.pName = "main";
	mPipelineInfo.pSpecializationInfo = nullptr;
}

Shader::~Shader()
{
	vkDestroyShaderModule(mDevice.GetHandle(), mHandle, nullptr);
}

const VkShaderModule &Shader::GetHandle() const
{
	return mHandle;
}
const VkPipelineShaderStageCreateInfo &Shader::GetPipelineStageInfo() const
{
	return mPipelineInfo;
}

const SpirvReflectedData &Shader::GetReflectedData() const
{
	return mReflectedData;
}

 void Shader::SpirvReflect(const std::vector<uint32_t>& spvCode)
 {
	SpirvReflect(spvCode.size(),spvCode.data());
 }

void Shader::SpirvReflect(size_t count,const uint32_t* spvCode)
{
	SpvReflectShaderModule module;
	SPIRV_REFLECT_CHECK(spvReflectCreateShaderModule((size_t)(sizeof(uint32_t) * count), (const void *)spvCode, &module));

	uint32_t varCount = 0;
	SPIRV_REFLECT_CHECK(spvReflectEnumerateInputVariables(&module, &varCount, nullptr));
	mReflectedData.inputVariables.resize(varCount);
	SPIRV_REFLECT_CHECK(spvReflectEnumerateInputVariables(&module, &varCount, mReflectedData.inputVariables.data()));

	varCount = 0;
	SPIRV_REFLECT_CHECK(spvReflectEnumerateOutputVariables(&module, &varCount, nullptr));
	mReflectedData.ouputVariables.resize(varCount);
	SPIRV_REFLECT_CHECK(spvReflectEnumerateOutputVariables(&module, &varCount, mReflectedData.ouputVariables.data()));

	varCount = 0;
	SPIRV_REFLECT_CHECK(spvReflectEnumerateDescriptorBindings(&module, &varCount, nullptr));
	mReflectedData.descriptorBindings.resize(varCount);
	SPIRV_REFLECT_CHECK(spvReflectEnumerateDescriptorBindings(&module, &varCount, mReflectedData.descriptorBindings.data()));

	varCount = 0;
	SPIRV_REFLECT_CHECK(spvReflectEnumerateDescriptorSets(&module, &varCount, nullptr));
	mReflectedData.descriptorSets.resize(varCount);
	SPIRV_REFLECT_CHECK(spvReflectEnumerateDescriptorSets(&module, &varCount, mReflectedData.descriptorSets.data()));

	varCount = 0;
	SPIRV_REFLECT_CHECK(spvReflectEnumeratePushConstantBlocks(&module, &varCount, nullptr));
	mReflectedData.pushConstants.resize(varCount);
	SPIRV_REFLECT_CHECK(spvReflectEnumeratePushConstantBlocks(&module, &varCount, mReflectedData.pushConstants.data()));
}
