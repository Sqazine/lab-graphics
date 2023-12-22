#pragma once
#include <cstdint>
#include <vector>
#include <vulkan/vulkan.h>
#include <memory>
#include "Buffer.h"
#include "Shader.h"
#include "ShaderGroup.h"

class RayTraceSBT
{
public:
	RayTraceSBT(const class Device& device);
	RayTraceSBT(const class Device& device, VkPipeline pipeline, const RayTraceShaderGroup& shaderGroup);
	~RayTraceSBT();

	void Build(VkPipeline pipeline, const RayTraceShaderGroup& shaderGroup);

	const VkStridedDeviceAddressRegionKHR& GetRayGenAddressRegion() const;
	const std::vector<VkStridedDeviceAddressRegionKHR>& GetRayMissAddressRegions() const;
	const std::vector<VkStridedDeviceAddressRegionKHR>& GetRayClosestHitAddressRegions() const;
	const std::vector<VkStridedDeviceAddressRegionKHR>& GetRayAnyHitAddressRegions() const;
	const std::vector<VkStridedDeviceAddressRegionKHR>& GetRayIntersectionAddressRegions() const;
	const std::vector<VkStridedDeviceAddressRegionKHR>& GetRayCallableAddressRegions() const;

private:
	const class Device& mDevice;

	uint32_t mGroupCount;
	uint32_t mHandleSize;
	uint32_t mHandleAlignment;
	uint32_t mHandleSizeAligned;
	uint32_t mBaseSizeAligned;
	uint32_t mSize;

	VkStridedDeviceAddressRegionKHR mRayGenAddressRegion;
	std::vector<VkStridedDeviceAddressRegionKHR> mRayMissAddressRegions;
	std::vector<VkStridedDeviceAddressRegionKHR> mRayClosestHitAddressRegions;
	std::vector<VkStridedDeviceAddressRegionKHR> mRayAnyHitAddressRegions;
	std::vector<VkStridedDeviceAddressRegionKHR> mRayIntersectionAddressRegions;
	std::vector<VkStridedDeviceAddressRegionKHR> mRayCallableAddressRegions;

	std::unique_ptr<Buffer> mRayGenBuffer;
	std::vector<std::unique_ptr<Buffer>> mRayMissBuffers;
	std::vector<std::unique_ptr<Buffer>> mRayClosestHitBuffers;
	std::vector<std::unique_ptr<Buffer>> mRayAnyHitBuffers;
	std::vector<std::unique_ptr<Buffer>> mRayIntersectionBuffers;
	std::vector<std::unique_ptr<Buffer>> mRayCallableBuffers;
};