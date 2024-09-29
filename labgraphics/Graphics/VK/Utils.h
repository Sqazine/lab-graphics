#pragma once
#include <vulkan/vulkan.h>
#include <optional>
#include <vector>
#include <SDL.h>
#include <SDL_vulkan.h>
#include <string_view>
#include <array>
#include <deque>
#include <functional>
#include "Format.h"

inline const char *GetErrorCode(const VkResult result)
{
	switch (result)
	{
#define STR(r)   \
	case VK_##r: \
		return #r
		STR(SUCCESS);
		STR(NOT_READY);
		STR(TIMEOUT);
		STR(EVENT_SET);
		STR(EVENT_RESET);
		STR(INCOMPLETE);
		STR(ERROR_OUT_OF_HOST_MEMORY);
		STR(ERROR_OUT_OF_DEVICE_MEMORY);
		STR(ERROR_INITIALIZATION_FAILED);
		STR(ERROR_DEVICE_LOST);
		STR(ERROR_MEMORY_MAP_FAILED);
		STR(ERROR_LAYER_NOT_PRESENT);
		STR(ERROR_EXTENSION_NOT_PRESENT);
		STR(ERROR_FEATURE_NOT_PRESENT);
		STR(ERROR_INCOMPATIBLE_DRIVER);
		STR(ERROR_TOO_MANY_OBJECTS);
		STR(ERROR_FORMAT_NOT_SUPPORTED);
		STR(ERROR_FRAGMENTED_POOL);
		STR(ERROR_OUT_OF_POOL_MEMORY);
		STR(ERROR_INVALID_EXTERNAL_HANDLE);
		STR(ERROR_SURFACE_LOST_KHR);
		STR(ERROR_NATIVE_WINDOW_IN_USE_KHR);
		STR(SUBOPTIMAL_KHR);
		STR(ERROR_OUT_OF_DATE_KHR);
		STR(ERROR_INCOMPATIBLE_DISPLAY_KHR);
		STR(ERROR_VALIDATION_FAILED_EXT);
		STR(ERROR_INVALID_SHADER_NV);
		STR(ERROR_INVALID_DRM_FORMAT_MODIFIER_PLANE_LAYOUT_EXT);
		STR(ERROR_FRAGMENTATION_EXT);
		STR(ERROR_NOT_PERMITTED_EXT);
		STR(ERROR_INVALID_DEVICE_ADDRESS_EXT);
#undef STR
	default:
		return "UNKNOWN_ERROR";
	}
}

#define VK_CHECK(r)                                                                    \
	do                                                                                 \
	{                                                                                  \
		VkResult result = (r);                                                         \
		if (result != VK_SUCCESS)                                                      \
			LOG_ERROR("[{}]File:{} Line:{}", GetErrorCode(result), __FILE__, __LINE__) \
	} while (false);

#define SPIRV_REFLECT_CHECK(v)                   \
	do                                           \
	{                                            \
		assert(v == SPV_REFLECT_RESULT_SUCCESS); \
	} while (false);

#define GET_VK_INSTANCE_PFN(instance, funcName)                                                     \
	{                                                                                               \
		funcName = reinterpret_cast<PFN_##funcName>(vkGetInstanceProcAddr(instance, "" #funcName)); \
		if (funcName == nullptr)                                                                    \
		{                                                                                           \
			const std::string name = #funcName;                                                     \
			std::cout << "Failed to resolve function " << name << std::endl;                        \
		}                                                                                           \
	}

#define GET_VK_DEVICE_PFN(device, funcName)                                                     \
	{                                                                                           \
		funcName = reinterpret_cast<PFN_##funcName>(vkGetDeviceProcAddr(device, "" #funcName)); \
		if (funcName == nullptr)                                                                \
		{                                                                                       \
			const std::string name = #funcName;                                                 \
			std::cout << "Failed to resolve function " << name << std::endl;                    \
		}                                                                                       \
	}

#define SET(member, value) \
	if (member == value)   \
		return *this;      \
	member = value;        \
	mIsDirty = true;       \
	return *this;

struct QueueFamilyIndices
{
	std::optional<uint32_t> graphicsFamily;
	std::optional<uint32_t> computeFamily;
	std::optional<uint32_t> transferFamily;
	std::optional<uint32_t> presentFamily;

	bool IsComplete()
	{
		if (!presentFamily.has_value())
			return graphicsFamily.has_value() && computeFamily.has_value() && transferFamily.has_value();
		else
			return graphicsFamily.has_value() && presentFamily.has_value() && computeFamily.has_value() && transferFamily.has_value();
	}

	bool IsSameFamily() const
	{
		bool isSame = graphicsFamily.value() == computeFamily.value() &&
					  computeFamily.value() == transferFamily.value();
		if (!presentFamily.has_value())
			return isSame;
		else
			return isSame && graphicsFamily.value() == presentFamily.value();
	}

	std::vector<uint32_t> ToIndexArray()
	{
		if (!presentFamily.has_value())
			return {graphicsFamily.value(), computeFamily.value(), transferFamily.value()};
		else
			return {graphicsFamily.value(), computeFamily.value(), transferFamily.value(), presentFamily.value()};
	}
};

std::vector<VkLayerProperties> GetInstanceLayerProps();
std::vector<VkExtensionProperties> GetInstanceExtensionProps();
bool CheckValidationLayerSupport(std::vector<const char *> validationLayerNames, std::vector<VkLayerProperties> instanceLayerProps);
bool CheckExtensionSupport(std::vector<const char *> extensionNames, std::vector<VkExtensionProperties> extensionProps);

QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR surface);

VkShaderModule CreateShaderModuleFromSpirvFile(VkDevice device, std::string_view filePath);

std::string ReadFile(std::string_view filename);

std::vector<char> ReadBinary(const std::string &filename);

VkIndexType DataStr2VkIndexType(std::string_view dataStr);

VkResult GlslToSpv(const VkShaderStageFlagBits shaderStage, std::string_view shaderSrc, std::vector<uint32_t> &spv);