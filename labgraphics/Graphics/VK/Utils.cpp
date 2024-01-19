#include "Utils.h"
#include <algorithm>
#include <iostream>
#include <fstream>
#include <sstream>
#include <glslang/Public/ShaderLang.h>
#include <glslang/SPIRV/GlslangToSpv.h>
#include "Logger.h"
#include "Format.h"

std::vector<VkLayerProperties> GetInstanceLayerProps()
{
    uint32_t availableLayerCount;
    vkEnumerateInstanceLayerProperties(&availableLayerCount, nullptr);
    std::vector<VkLayerProperties> availableInstanceLayerProps(availableLayerCount);
    vkEnumerateInstanceLayerProperties(&availableLayerCount, availableInstanceLayerProps.data());

    std::cout << "[INFO]Available Instance Layers:" << std::endl;
    for (const auto &layer : availableInstanceLayerProps)
        std::cout << "[INFO]     name: " << layer.layerName
                  << " \n\t\timpl_version: "
                  << VK_VERSION_MAJOR(layer.implementationVersion) << "."
                  << VK_VERSION_MINOR(layer.implementationVersion) << "."
                  << VK_VERSION_PATCH(layer.implementationVersion)
                  << " \n\t\tspec_version: "
                  << VK_VERSION_MAJOR(layer.specVersion) << "."
                  << VK_VERSION_MINOR(layer.specVersion) << "."
                  << VK_VERSION_PATCH(layer.specVersion)
                  << std::endl;
    return availableInstanceLayerProps;
}
std::vector<VkExtensionProperties> GetInstanceExtensionProps()
{
    uint32_t availableInstanceExtCount = 0;
    vkEnumerateInstanceExtensionProperties(nullptr, &availableInstanceExtCount, nullptr);
    std::vector<VkExtensionProperties> availableInstanceExtProps(availableInstanceExtCount);
    vkEnumerateInstanceExtensionProperties(nullptr, &availableInstanceExtCount, availableInstanceExtProps.data());

    std::cout << "[INFO]Available Instance Extensions:" << std::endl;
    for (const auto &ext : availableInstanceExtProps)
        std::cout << "[INFO]     name: " << ext.extensionName << " \n\t\tspec_version: "
                  << VK_VERSION_MAJOR(ext.specVersion) << "."
                  << VK_VERSION_MINOR(ext.specVersion) << "."
                  << VK_VERSION_PATCH(ext.specVersion) << std::endl;

    return availableInstanceExtProps;
}

bool CheckValidationLayerSupport(std::vector<const char *> validationLayerNames, std::vector<VkLayerProperties> instanceLayerProps)
{
    for (const char *layerName : validationLayerNames)
    {
        bool layerFound = false;
        for (const auto &layerProperties : instanceLayerProps)
            if (strcmp(layerName, layerProperties.layerName) == 0)
            {
                layerFound = true;
                break;
            }

        if (layerFound == false)
            return false;
    }
    return true;
}

bool CheckExtensionSupport(std::vector<const char *> extensionNames, std::vector<VkExtensionProperties> extensionProps)
{
    for (const auto &extName : extensionNames)
    {
        bool extFound = false;
        for (const auto &extProp : extensionProps)
            if (strcmp(extName, extProp.extensionName) == 0)
            {
                extFound = true;
                break;
                ;
            }
        if (extFound == false)
            return false;
    }

    return true;
}

QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR surface)
{
    QueueFamilyIndices indices;

    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);
    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

    int i = 0;
    for (const auto &queueFamily : queueFamilies)
    {
        if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
            indices.graphicsFamily = i;
        if (queueFamily.queueFlags & VK_QUEUE_COMPUTE_BIT)
            indices.computeFamily = i;
        if (queueFamily.queueFlags & VK_QUEUE_TRANSFER_BIT)
            indices.transferFamily = i;
        if (surface == VK_NULL_HANDLE)
        {
            if (indices.computeFamily.has_value() && indices.graphicsFamily.has_value() && indices.transferFamily.has_value())
                break;
        }
        else
        {
            VkBool32 present_support = false;
            vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &present_support);
            if (present_support)
                indices.presentFamily = i;

            if (indices.IsComplete())
                break;
        }

        i++;
    }

    return indices;
}

VkShaderModule CreateShaderModuleFromSpirvFile(VkDevice device, std::string_view filePath)
{
    std::ifstream file(filePath.data(), std::ios::binary);

    if (!file.is_open())
    {
        std::cout << "failed to load shader file:" << filePath << std::endl;
        exit(1);
    }

    std::stringstream sstream;

    sstream << file.rdbuf();

    std::string content = sstream.str();

    file.close();

    VkShaderModuleCreateInfo info;
    info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    info.flags = 0;
    info.pNext = nullptr;
    info.codeSize = content.size();
    info.pCode = reinterpret_cast<const uint32_t *>(content.data());

    VkShaderModule module;

    VK_CHECK(vkCreateShaderModule(device, &info, nullptr, &module));

    return module;
}

std::string ReadFile(std::string_view filename)
{
    std::ifstream file(filename.data());

    if (!file.is_open())
        throw std::runtime_error("Could not open file");

    std::stringstream sstream;
    sstream << file.rdbuf();
    std::string content = sstream.str();
    file.close();

    return content;
}

std::vector<char> ReadBinary(const std::string &filename)
{
    std::ifstream file{filename, std::ios::binary | std::ios::ate};
    if (!file.is_open())
        throw std::runtime_error("Could not open file: " + filename);

    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);

    std::vector<char> buffer(size);
    file.read(buffer.data(), size);
    return buffer;
}

VkIndexType DataStr2VkIndexType(std::string_view dataStr)
{
    if (dataStr.compare("uint32_t") == 0 || dataStr.compare("unsigned int") == 0)
        return VK_INDEX_TYPE_UINT32;
    else if (dataStr.compare("uint8_t") == 0 || dataStr.compare("unsigned char") == 0)
        return VK_INDEX_TYPE_UINT16;
    else if (dataStr.compare("uint16_t") == 0 || dataStr.compare("unsigned short") == 0)
        return VK_INDEX_TYPE_UINT8_EXT;
    return VK_INDEX_TYPE_UINT32;
}

TBuiltInResource Initresources() noexcept
{
    TBuiltInResource resources;

    resources.maxLights = 32;
    resources.maxClipPlanes = 6;
    resources.maxTextureUnits = 32;
    resources.maxTextureCoords = 32;
    resources.maxVertexAttribs = 64;
    resources.maxVertexUniformComponents = 4096;
    resources.maxVaryingFloats = 64;
    resources.maxVertexTextureImageUnits = 32;
    resources.maxCombinedTextureImageUnits = 80;
    resources.maxTextureImageUnits = 32;
    resources.maxFragmentUniformComponents = 4096;
    resources.maxDrawBuffers = 32;
    resources.maxVertexUniformVectors = 128;
    resources.maxVaryingVectors = 8;
    resources.maxFragmentUniformVectors = 16;
    resources.maxVertexOutputVectors = 16;
    resources.maxFragmentInputVectors = 15;
    resources.minProgramTexelOffset = -8;
    resources.maxProgramTexelOffset = 7;
    resources.maxClipDistances = 8;
    resources.maxComputeWorkGroupCountX = 65535;
    resources.maxComputeWorkGroupCountY = 65535;
    resources.maxComputeWorkGroupCountZ = 65535;
    resources.maxComputeWorkGroupSizeX = 1024;
    resources.maxComputeWorkGroupSizeY = 1024;
    resources.maxComputeWorkGroupSizeZ = 64;
    resources.maxComputeUniformComponents = 1024;
    resources.maxComputeTextureImageUnits = 16;
    resources.maxComputeImageUniforms = 8;
    resources.maxComputeAtomicCounters = 8;
    resources.maxComputeAtomicCounterBuffers = 1;
    resources.maxVaryingComponents = 60;
    resources.maxVertexOutputComponents = 64;
    resources.maxGeometryInputComponents = 64;
    resources.maxGeometryOutputComponents = 128;
    resources.maxFragmentInputComponents = 128;
    resources.maxImageUnits = 8;
    resources.maxCombinedImageUnitsAndFragmentOutputs = 8;
    resources.maxCombinedShaderOutputResources = 8;
    resources.maxImageSamples = 0;
    resources.maxVertexImageUniforms = 0;
    resources.maxTessControlImageUniforms = 0;
    resources.maxTessEvaluationImageUniforms = 0;
    resources.maxGeometryImageUniforms = 0;
    resources.maxFragmentImageUniforms = 8;
    resources.maxCombinedImageUniforms = 8;
    resources.maxGeometryTextureImageUnits = 16;
    resources.maxGeometryOutputVertices = 256;
    resources.maxGeometryTotalOutputComponents = 1024;
    resources.maxGeometryUniformComponents = 1024;
    resources.maxGeometryVaryingComponents = 64;
    resources.maxTessControlInputComponents = 128;
    resources.maxTessControlOutputComponents = 128;
    resources.maxTessControlTextureImageUnits = 16;
    resources.maxTessControlUniformComponents = 1024;
    resources.maxTessControlTotalOutputComponents = 4096;
    resources.maxTessEvaluationInputComponents = 128;
    resources.maxTessEvaluationOutputComponents = 128;
    resources.maxTessEvaluationTextureImageUnits = 16;
    resources.maxTessEvaluationUniformComponents = 1024;
    resources.maxTessPatchComponents = 120;
    resources.maxPatchVertices = 32;
    resources.maxTessGenLevel = 64;
    resources.maxViewports = 16;
    resources.maxVertexAtomicCounters = 0;
    resources.maxTessControlAtomicCounters = 0;
    resources.maxTessEvaluationAtomicCounters = 0;
    resources.maxGeometryAtomicCounters = 0;
    resources.maxFragmentAtomicCounters = 8;
    resources.maxCombinedAtomicCounters = 8;
    resources.maxAtomicCounterBindings = 1;
    resources.maxVertexAtomicCounterBuffers = 0;
    resources.maxTessControlAtomicCounterBuffers = 0;
    resources.maxTessEvaluationAtomicCounterBuffers = 0;
    resources.maxGeometryAtomicCounterBuffers = 0;
    resources.maxFragmentAtomicCounterBuffers = 1;
    resources.maxCombinedAtomicCounterBuffers = 1;
    resources.maxAtomicCounterBufferSize = 16384;
    resources.maxTransformFeedbackBuffers = 4;
    resources.maxTransformFeedbackInterleavedComponents = 64;
    resources.maxCullDistances = 8;
    resources.maxCombinedClipAndCullDistances = 8;
    resources.maxSamples = 4;
    resources.maxMeshOutputVerticesNV = 256;
    resources.maxMeshOutputPrimitivesNV = 512;
    resources.maxMeshWorkGroupSizeX_NV = 32;
    resources.maxMeshWorkGroupSizeY_NV = 1;
    resources.maxMeshWorkGroupSizeZ_NV = 1;
    resources.maxTaskWorkGroupSizeX_NV = 32;
    resources.maxTaskWorkGroupSizeY_NV = 1;
    resources.maxTaskWorkGroupSizeZ_NV = 1;
    resources.maxMeshViewCountNV = 4;

    resources.limits.nonInductiveForLoops = false;
    resources.limits.whileLoops = 1;
    resources.limits.doWhileLoops = 1;
    resources.limits.generalUniformIndexing = 1;
    resources.limits.generalAttributeMatrixVectorIndexing = 1;
    resources.limits.generalVaryingIndexing = 1;
    resources.limits.generalSamplerIndexing = 1;
    resources.limits.generalVariableIndexing = 1;
    resources.limits.generalConstantMatrixVectorIndexing = 1;

    return resources;
}

VkResult GlslToSpv(const VkShaderStageFlagBits shaderStage, std::string_view shaderSrc, std::vector<uint32_t> &spv)
{
    glslang::InitializeProcess();

    glslang::TProgram program;

    EShLanguage stage;
    switch (shaderStage)
    {
    case VK_SHADER_STAGE_VERTEX_BIT:
        stage = EShLangVertex;
        break;
    case VK_SHADER_STAGE_FRAGMENT_BIT:
        stage = EShLangFragment;
        break;
    case VK_SHADER_STAGE_COMPUTE_BIT:
        stage = EShLangCompute;
        break;
    case VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT:
        stage = EShLangTessControl;
        break;
    case VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT:
        stage = EShLangTessEvaluation;
        break;
    case VK_SHADER_STAGE_GEOMETRY_BIT:
        stage = EShLangGeometry;
        break;
    case VK_SHADER_STAGE_RAYGEN_BIT_KHR:
        stage = EShLangRayGen;
        break;
    case VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR:
        stage = EShLangClosestHit;
        break;
    case VK_SHADER_STAGE_ANY_HIT_BIT_KHR:
        stage = EShLangAnyHit;
        break;
    case VK_SHADER_STAGE_INTERSECTION_BIT_KHR:
        stage = EShLangIntersect;
        break;
    case VK_SHADER_STAGE_CALLABLE_BIT_KHR:
        stage = EShLangCallable;
        break;
    case VK_SHADER_STAGE_MISS_BIT_KHR:
        stage = EShLangMiss;
        break;
    default:
        break;
    }

    glslang::TShader *shader = new glslang::TShader(stage);

    std::vector<const char *> pTempShaderSrc{shaderSrc.data()};
    shader->setStrings(pTempShaderSrc.data(), static_cast<int>(pTempShaderSrc.size()));
    shader->setEnvInput(glslang::EShSource::EShSourceGlsl, stage, glslang::EShClient::EShClientVulkan, 460);
    shader->setEnvClient(glslang::EShClient::EShClientVulkan, glslang::EShTargetClientVersion::EShTargetVulkan_1_3);
    shader->setEnvTarget(glslang::EShTargetLanguage::EShTargetSpv, glslang::EShTargetLanguageVersion::EShTargetSpv_1_4);

    EShMessages messages = (EShMessages)(EShMsgSpvRules | EShMsgVulkanRules);

    TBuiltInResource resource = Initresources();

    if (!shader->parse(&resource, 460, false, messages))
    {
        std::cout << shader->getInfoLog() << std::endl;
        std::cout << shader->getInfoDebugLog() << std::endl;
        return VK_NOT_READY;
    }

    program.addShader(shader);

    if (!program.link(messages))
    {
        std::cout << program.getInfoLog() << std::endl;
        std::cout << program.getInfoDebugLog() << std::endl;
        return VK_NOT_READY;
    }

    glslang::GlslangToSpv(*program.getIntermediate(stage), spv);

    glslang::FinalizeProcess();

    delete shader;

    return VK_SUCCESS;
}