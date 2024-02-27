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

	{
		mRayGenBuffer = mDevice.CreateCPUBuffer(sbtResults.data() + mHandleSizeAligned * (offset), mHandleSize, BufferUsage::SHADER_BINDING_TABLE);
		mRayGenAddressRegion = {};
		mRayGenAddressRegion.deviceAddress = mRayGenBuffer->GetAddress();
		mRayGenAddressRegion.stride = mHandleSizeAligned;
		mRayGenAddressRegion.size = mHandleSizeAligned;

		offset++;
	}

	{
		mRayMissBuffer = nullptr;
		mRayMissAddressRegion = {};
		if (!shaderGroup.GetRayMissShaderGroups().empty())
		{
			mRayMissBuffer = mDevice.CreateCPUBuffer(sbtResults.data() + mHandleSizeAligned * offset, mHandleSize * shaderGroup.GetRayMissShaderGroups().size(), BufferUsage::SHADER_BINDING_TABLE);
			mRayMissAddressRegion.deviceAddress = mRayMissBuffer->GetAddress();
			mRayMissAddressRegion.stride = mHandleSizeAligned;
			mRayMissAddressRegion.size = mHandleSizeAligned * shaderGroup.GetRayMissShaderGroups().size();

			offset += shaderGroup.GetRayMissShaderGroups().size();
		}
	}

	{
		mRayClosestHitBuffer = nullptr;
		mRayClosestHitAddressRegion = {};
		if (!shaderGroup.GetRayClosestHitShaderGroups().empty())
		{
			mRayClosestHitBuffer = mDevice.CreateCPUBuffer(sbtResults.data() + mHandleSizeAligned * offset, mHandleSize * shaderGroup.GetRayClosestHitShaderGroups().size(), BufferUsage::SHADER_BINDING_TABLE);
			mRayClosestHitAddressRegion.deviceAddress = mRayClosestHitBuffer->GetAddress();
			mRayClosestHitAddressRegion.stride = mHandleSizeAligned;
			mRayClosestHitAddressRegion.size = mHandleSizeAligned * shaderGroup.GetRayClosestHitShaderGroups().size();

			offset += shaderGroup.GetRayClosestHitShaderGroups().size();
		}
	}

	{
		mRayAnyHitBuffer = nullptr;
		mRayAnyHitAddressRegion = {};
		if (!shaderGroup.GetRayClosestHitShaderGroups().empty())
		{
			mRayAnyHitBuffer = mDevice.CreateCPUBuffer(sbtResults.data() + mHandleSizeAligned * offset, mHandleSize * shaderGroup.GetRayClosestHitShaderGroups().size(), BufferUsage::SHADER_BINDING_TABLE);
			mRayAnyHitAddressRegion.deviceAddress = mRayAnyHitBuffer->GetAddress();
			mRayAnyHitAddressRegion.stride = mHandleSizeAligned;
			mRayAnyHitAddressRegion.size = mHandleSizeAligned * shaderGroup.GetRayAnyHitShaderGroups().size();

			offset += shaderGroup.GetRayAnyHitShaderGroups().size();
		}
	}

	{
		mRayIntersectionBuffer = nullptr;
		mRayIntersectionAddressRegion = {};
		if (!shaderGroup.GetRayIntersectionShaderGroups().empty())
		{
			mRayIntersectionBuffer = mDevice.CreateCPUBuffer(sbtResults.data() + mHandleSizeAligned * offset, mHandleSize * shaderGroup.GetRayIntersectionShaderGroups().size(), BufferUsage::SHADER_BINDING_TABLE);
			mRayIntersectionAddressRegion.deviceAddress = mRayAnyHitBuffer->GetAddress();
			mRayIntersectionAddressRegion.stride = mHandleSizeAligned;
			mRayIntersectionAddressRegion.size = mHandleSizeAligned * shaderGroup.GetRayIntersectionShaderGroups().size();

			offset += shaderGroup.GetRayIntersectionShaderGroups().size();
		}
	}

	{
		mRayCallableBuffer = nullptr;
		mRayCallableAddressRegion = {};
		if (!shaderGroup.GetRayCallableShaderGroups().empty())
		{
			mRayCallableBuffer = mDevice.CreateCPUBuffer(sbtResults.data() + mHandleSizeAligned * offset, mHandleSize * shaderGroup.GetRayCallableShaderGroups().size(), BufferUsage::SHADER_BINDING_TABLE);
			mRayCallableAddressRegion.deviceAddress = mRayAnyHitBuffer->GetAddress();
			mRayCallableAddressRegion.stride = mHandleSizeAligned;
			mRayCallableAddressRegion.size = mHandleSizeAligned * shaderGroup.GetRayCallableShaderGroups().size();

			offset += shaderGroup.GetRayCallableShaderGroups().size();
		}
	}
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
