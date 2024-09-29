#include "RayTraceSBT.h"
#include "VK/Device.h"
#include "VK/Utils.h"
#include "Math/Math.hpp"
#include <iostream>

RayTraceSBT::RayTraceSBT(const Device &device)
	: mDevice(device)
{
}

RayTraceSBT::RayTraceSBT(const Device &device, VkPipeline pipeline, const RayTraceShaderGroup &shaderGroup)
	: mDevice(device)
{
	Build(pipeline, shaderGroup);
}
RayTraceSBT::~RayTraceSBT()
{
}

void RayTraceSBT::Build(VkPipeline pipeline, const RayTraceShaderGroup &shaderGroup)
{
	mGroupCount = 1 +
				  shaderGroup.GetRayMissShaderGroups().size() +
				  shaderGroup.GetRayClosestHitShaderGroups().size() +
				  shaderGroup.GetRayAnyHitShaderGroups().size() +
				  shaderGroup.GetRayIntersectionShaderGroups().size() +
				  shaderGroup.GetRayCallableShaderGroups().size();

	mHandleSize = mDevice.GetRayTracingPipelineProps().shaderGroupHandleSize;
	mHandleAlignment = mDevice.GetRayTracingPipelineProps().shaderGroupHandleAlignment;
	mHandleSizeAligned = Math::AlignTo(mHandleSize, mHandleAlignment);

	mBaseSizeAligned = Math::RoundUp(mHandleSize, mDevice.GetRayTracingPipelineProps().shaderGroupBaseAlignment);

	mSize = mGroupCount * mHandleSizeAligned;

	std::vector<uint8_t> sbtResults(mSize);

	VK_CHECK(mDevice.vkGetRayTracingShaderGroupHandlesKHR(mDevice.GetHandle(), pipeline, 0, mGroupCount, mSize, sbtResults.data()));

	uint32_t offset = 0;

	mSbtBuffer = mDevice.CreateCPUBuffer(mBaseSizeAligned * mGroupCount, BufferUsage::SHADER_BINDING_TABLE);
	auto addr = mSbtBuffer->MapWhole<uint8_t>();

	// ray gen

	mRayGenAddressRegion = {};
	mRayGenAddressRegion.deviceAddress = mSbtBuffer->GetAddress() + offset * mBaseSizeAligned;
	mRayGenAddressRegion.stride = mBaseSizeAligned;
	mRayGenAddressRegion.size = mBaseSizeAligned;

	std::memcpy(addr, sbtResults.data() + (offset++) * mHandleSize, mHandleSize);
	addr += mBaseSizeAligned;

	// ray miss
	mRayMissAddressRegion = {};
	if (!shaderGroup.GetRayMissShaderGroups().empty())
	{
		mRayMissAddressRegion.deviceAddress = mSbtBuffer->GetAddress() + offset * mBaseSizeAligned;
		mRayMissAddressRegion.stride = mBaseSizeAligned;
		mRayMissAddressRegion.size = mBaseSizeAligned * shaderGroup.GetRayMissShaderGroups().size();

		for (size_t i = 0; i < shaderGroup.GetRayMissShaderGroups().size(); ++i)
		{
			std::memcpy(addr, sbtResults.data() + (offset++) * mHandleSize, mHandleSize);
			addr += mBaseSizeAligned;
		}
	}

	// ray closest hit
	mRayClosestHitAddressRegion = {};
	if (!shaderGroup.GetRayClosestHitShaderGroups().empty())
	{
		mRayClosestHitAddressRegion.deviceAddress = mSbtBuffer->GetAddress() + offset * mBaseSizeAligned;
		mRayClosestHitAddressRegion.stride = mBaseSizeAligned;
		mRayClosestHitAddressRegion.size = mBaseSizeAligned * shaderGroup.GetRayClosestHitShaderGroups().size();

		for (size_t i = 0; i < shaderGroup.GetRayClosestHitShaderGroups().size(); ++i)
		{
			std::memcpy(addr, sbtResults.data() + (offset++) * mHandleSize, mHandleSize);
			addr += mBaseSizeAligned;
		}
	}

	// ray any hit
	mRayAnyHitAddressRegion = {};
	if (!shaderGroup.GetRayClosestHitShaderGroups().empty())
	{
		mRayAnyHitAddressRegion.deviceAddress = mSbtBuffer->GetAddress() + offset * mBaseSizeAligned;
		mRayAnyHitAddressRegion.stride = mHandleSizeAligned;
		mRayAnyHitAddressRegion.size = mHandleSizeAligned * shaderGroup.GetRayAnyHitShaderGroups().size();

		for (size_t i = 0; i < shaderGroup.GetRayAnyHitShaderGroups().size(); ++i)
		{
			std::memcpy(addr, sbtResults.data() + (offset++) * mHandleSize, mHandleSize);
			addr += mBaseSizeAligned;
		}
	}

	// ray intersection
	mRayIntersectionAddressRegion = {};
	if (!shaderGroup.GetRayIntersectionShaderGroups().empty())
	{
		mRayIntersectionAddressRegion.deviceAddress = mSbtBuffer->GetAddress() + offset * mBaseSizeAligned;
		mRayIntersectionAddressRegion.stride = mHandleSizeAligned;
		mRayIntersectionAddressRegion.size = mHandleSizeAligned * shaderGroup.GetRayIntersectionShaderGroups().size();

		for (size_t i = 0; i < shaderGroup.GetRayIntersectionShaderGroups().size(); ++i)
		{
			std::memcpy(addr, sbtResults.data() + (offset++) * mHandleSize, mHandleSize);
			addr += mBaseSizeAligned;
		}
	}

	// ray callable
	mRayCallableAddressRegion = {};
	if (!shaderGroup.GetRayCallableShaderGroups().empty())
	{
		mRayCallableAddressRegion.deviceAddress = mSbtBuffer->GetAddress() + offset * mBaseSizeAligned;
		mRayCallableAddressRegion.stride = mHandleSizeAligned;
		mRayCallableAddressRegion.size = mHandleSizeAligned * shaderGroup.GetRayCallableShaderGroups().size();

		for (size_t i = 0; i < shaderGroup.GetRayCallableShaderGroups().size(); ++i)
		{
			std::memcpy(addr, sbtResults.data() + (offset++) * mHandleSize, mHandleSize);
			addr += mBaseSizeAligned;
		}
	}

	mSbtBuffer->Unmap();
}

const VkStridedDeviceAddressRegionKHR &RayTraceSBT::GetRayGenAddressRegion() const
{
	return mRayGenAddressRegion;
}

const VkStridedDeviceAddressRegionKHR &RayTraceSBT::GetRayMissAddressRegion() const
{
	return mRayMissAddressRegion;
}

const VkStridedDeviceAddressRegionKHR &RayTraceSBT::GetRayClosestHitAddressRegion() const
{
	return mRayClosestHitAddressRegion;
}

const VkStridedDeviceAddressRegionKHR &RayTraceSBT::GetRayAnyHitAddressRegion() const
{
	return mRayAnyHitAddressRegion;
}
const VkStridedDeviceAddressRegionKHR &RayTraceSBT::GetRayIntersectionAddressRegion() const
{
	return mRayIntersectionAddressRegion;
}
const VkStridedDeviceAddressRegionKHR &RayTraceSBT::GetRayCallableAddressRegion() const
{
	return mRayCallableAddressRegion;
}
