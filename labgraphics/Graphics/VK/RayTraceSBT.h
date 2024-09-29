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
	const VkStridedDeviceAddressRegionKHR& GetRayMissAddressRegion() const;
	const VkStridedDeviceAddressRegionKHR& GetRayClosestHitAddressRegion() const;
	const VkStridedDeviceAddressRegionKHR& GetRayAnyHitAddressRegion() const;
	const VkStridedDeviceAddressRegionKHR& GetRayIntersectionAddressRegion() const;
	const VkStridedDeviceAddressRegionKHR& GetRayCallableAddressRegion() const;

private:
	const class Device& mDevice;

	uint32_t mGroupCount;
	uint32_t mHandleSize;
	uint32_t mHandleAlignment;
	uint32_t mHandleSizeAligned;
	uint32_t mBaseSizeAligned;
	uint32_t mSize;

	VkStridedDeviceAddressRegionKHR mRayGenAddressRegion;
	VkStridedDeviceAddressRegionKHR mRayMissAddressRegion;
	VkStridedDeviceAddressRegionKHR mRayClosestHitAddressRegion;
	VkStridedDeviceAddressRegionKHR mRayAnyHitAddressRegion;
	VkStridedDeviceAddressRegionKHR mRayIntersectionAddressRegion;
	VkStridedDeviceAddressRegionKHR mRayCallableAddressRegion;

	std::unique_ptr<Buffer> mSbtBuffer;
};