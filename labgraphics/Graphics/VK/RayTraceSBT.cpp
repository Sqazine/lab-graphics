#include "RayTraceSBT.h"
#include "VK/Device.h"
#include "VK/Utils.h"
#include "Math/Math.hpp"
#include <iostream>

RayTraceSBT::RayTraceSBT(const Device& device)
	: mDevice(device)
{
}

RayTraceSBT::RayTraceSBT(const Device& device, VkPipeline pipeline, const RayTraceShaderGroup& shaderGroup)
	: mDevice(device)
{
	Build(pipeline, shaderGroup);
}
RayTraceSBT::~RayTraceSBT()
{
}

void RayTraceSBT::Build(VkPipeline pipeline, const RayTraceShaderGroup& shaderGroup)
{
	mGroupCount = 1 +
		shaderGroup.GetRayClosestHitShaderGroups().size() +
		shaderGroup.GetRayMissShaderGroups().size() +
		shaderGroup.GetRayAnyHitShaderGroups().size() +
		shaderGroup.GetRayIntersectionShaderGroups().size() +
		shaderGroup.GetRayCallableShaderGroups().size();

	mHandleSize = mDevice.GetRayTracingPipelineProps().shaderGroupHandleSize;
	mHandleAlignment = mDevice.GetRayTracingPipelineProps().shaderGroupHandleAlignment;
	mHandleSizeAligned = Math::AlignTo(mHandleSize, mHandleAlignment);
		
	mBaseSizeAligned=Math::RoundUp(mHandleSize,mDevice.GetRayTracingPipelineProps().shaderGroupBaseAlignment);

	mSize = mGroupCount * mHandleSizeAligned;

	std::vector<uint8_t> sbtResults(mSize);

	VK_CHECK(mDevice.vkGetRayTracingShaderGroupHandlesKHR(mDevice.GetHandle(), pipeline, 0, mGroupCount, mSize, sbtResults.data()));

	{
		mRayGenBuffer = mDevice.CreateCPUBuffer(sbtResults.data(), mHandleSize,BufferUsage::SHADER_BINDING_TABLE);
		mRayGenAddressRegion = {};
		mRayGenAddressRegion.deviceAddress = mRayGenBuffer->GetAddress();
		mRayGenAddressRegion.stride = mHandleSizeAligned;
		mRayGenAddressRegion.size = mHandleSizeAligned;
	}
	uint32_t offset = 1;

	{
		mRayClosestHitBuffers.resize(shaderGroup.GetRayClosestHitShaderGroups().size());
		mRayClosestHitAddressRegions.resize(mRayClosestHitBuffers.size());

		for (auto& rayClosestHitBuffer : mRayClosestHitBuffers)
			rayClosestHitBuffer = mDevice.CreateCPUBuffer(sbtResults.data() + mHandleSizeAligned * (offset++), mHandleSize,BufferUsage::SHADER_BINDING_TABLE);

		for (uint32_t i = 0; i < mRayClosestHitBuffers.size(); ++i)
		{
			VkStridedDeviceAddressRegionKHR rayClosestHitAddressRegion{};
			rayClosestHitAddressRegion.deviceAddress = mRayClosestHitBuffers[i]->GetAddress();
			rayClosestHitAddressRegion.stride = mHandleSizeAligned;
			rayClosestHitAddressRegion.size = mHandleSizeAligned;

			mRayClosestHitAddressRegions[i] = rayClosestHitAddressRegion;
		}
	}

	{
		mRayMissBuffers.resize(shaderGroup.GetRayMissShaderGroups().size());
		mRayMissAddressRegions.resize(mRayMissBuffers.size());

		for (auto& rayMissBuffer : mRayMissBuffers)
			rayMissBuffer = mDevice.CreateCPUBuffer(sbtResults.data() + mHandleSizeAligned * (offset++), mHandleSize,BufferUsage::SHADER_BINDING_TABLE);

		for (uint32_t i = 0; i < mRayMissBuffers.size(); ++i)
		{
			VkStridedDeviceAddressRegionKHR rayMissAddressRegion{};
			rayMissAddressRegion.deviceAddress = mRayMissBuffers[i]->GetAddress();
			rayMissAddressRegion.stride = mHandleSizeAligned;
			rayMissAddressRegion.size = mHandleSizeAligned;

			mRayMissAddressRegions[i] = rayMissAddressRegion;
		}
	}

	{
		mRayAnyHitBuffers.resize(shaderGroup.GetRayAnyHitShaderGroups().size());
		mRayAnyHitAddressRegions.resize(mRayAnyHitBuffers.size());

		for (auto& rayAnyHitBuffer : mRayAnyHitBuffers)
			rayAnyHitBuffer = mDevice.CreateCPUBuffer(sbtResults.data() + mHandleSizeAligned * (offset++), mHandleSize,BufferUsage::SHADER_BINDING_TABLE);

		for (uint32_t i = 0; i < mRayAnyHitBuffers.size(); ++i)
		{
			VkStridedDeviceAddressRegionKHR rayAnyHitAddressRegion{};
			rayAnyHitAddressRegion.deviceAddress = mRayAnyHitBuffers[i]->GetAddress();
			rayAnyHitAddressRegion.stride = mHandleSizeAligned;
			rayAnyHitAddressRegion.size = mHandleSizeAligned;

			mRayAnyHitAddressRegions[i] = rayAnyHitAddressRegion;
		}
	}


	{
		mRayIntersectionBuffers.resize(shaderGroup.GetRayIntersectionShaderGroups().size());
		mRayIntersectionAddressRegions.resize(mRayIntersectionBuffers.size());

		for (auto& rayIntersectionBuffer : mRayIntersectionBuffers)
			rayIntersectionBuffer = mDevice.CreateCPUBuffer(sbtResults.data() + mHandleSizeAligned * (offset++), mHandleSize, BufferUsage::SHADER_BINDING_TABLE);

		for (uint32_t i = 0; i < mRayIntersectionBuffers.size(); ++i)
		{
			VkStridedDeviceAddressRegionKHR rayIntersectionAddressRegion{};
			rayIntersectionAddressRegion.deviceAddress = mRayIntersectionBuffers[i]->GetAddress();
			rayIntersectionAddressRegion.stride = mHandleSizeAligned;
			rayIntersectionAddressRegion.size = mHandleSizeAligned;

			mRayIntersectionAddressRegions[i] = rayIntersectionAddressRegion;
		}
	}

	{
		if (shaderGroup.GetRayCallableShaderGroups().empty())
		{
			mRayCallableAddressRegions.emplace_back(VkStridedDeviceAddressRegionKHR{});
		}
		else
		{
			mRayCallableBuffers.resize(shaderGroup.GetRayCallableShaderGroups().size());
			mRayCallableAddressRegions.resize(mRayCallableBuffers.size());

			for (auto& rayCallableBuffer : mRayCallableBuffers)
				rayCallableBuffer = mDevice.CreateCPUBuffer(sbtResults.data() + mHandleSizeAligned * (offset++), mHandleSize,BufferUsage::SHADER_BINDING_TABLE);

			for (uint32_t i = 0; i < mRayCallableBuffers.size(); ++i)
			{
				VkStridedDeviceAddressRegionKHR rayCallableAddressRegion{};
				rayCallableAddressRegion.deviceAddress = mRayCallableBuffers[i]->GetAddress();
				rayCallableAddressRegion.stride = mHandleSizeAligned;
				rayCallableAddressRegion.size = mHandleSizeAligned;

				mRayCallableAddressRegions[i] = rayCallableAddressRegion;
			}
		}
	}
}

const VkStridedDeviceAddressRegionKHR& RayTraceSBT::GetRayGenAddressRegion() const
{
	return mRayGenAddressRegion;
}
const std::vector<VkStridedDeviceAddressRegionKHR>& RayTraceSBT::GetRayMissAddressRegions() const
{
	return mRayMissAddressRegions;
}
const std::vector<VkStridedDeviceAddressRegionKHR>& RayTraceSBT::GetRayClosestHitAddressRegions() const
{
	return mRayClosestHitAddressRegions;
}

const std::vector<VkStridedDeviceAddressRegionKHR>& RayTraceSBT::GetRayAnyHitAddressRegions() const
{
	return mRayAnyHitAddressRegions;
}
const std::vector<VkStridedDeviceAddressRegionKHR>& RayTraceSBT::GetRayIntersectionAddressRegions() const
{
	return mRayIntersectionAddressRegions;
}
const std::vector<VkStridedDeviceAddressRegionKHR>& RayTraceSBT::GetRayCallableAddressRegions() const
{
	return mRayCallableAddressRegions;
}

