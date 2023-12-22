#include "AS.h"
#include <iostream>
#include "Device.h"
#include "Utils.h"
#include "Math/Math.hpp"
AS::AS(Device &device)
	: mDevice(device), mHandle(VK_NULL_HANDLE)
{
}

const VkAccelerationStructureKHR &AS::GetHandle() const
{
	return mHandle;
}
uint64_t AS::GetAddress() const
{
	return mAddress;
}

 uint32_t BLAS::mInstanceID=0;

BLAS::BLAS(Device &device)
	: AS(device)
{
}
BLAS::~BLAS()
{
	mScratchBuffer.reset();
	mAccelStorageBuffer.reset();
	mVertexBuffer.reset();
	mIndexBuffer.reset();
	mDevice.vkDestroyAccelerationStructureKHR(mDevice.GetHandle(), mHandle, nullptr);
}

VkAccelerationStructureInstanceKHR BLAS::CreateInstance(VkTransformMatrixKHR matrix)
{
	VkAccelerationStructureInstanceKHR instance = {};
	instance.transform = matrix;
	instance.instanceCustomIndex = mInstanceID++;
	instance.mask = 0xFF;
	instance.instanceShaderBindingTableRecordOffset = 0;
	instance.flags = VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR;
	instance.accelerationStructureReference = mAddress;

	return instance;
}

VkAccelerationStructureInstanceKHR BLAS::CreateInstance()
{
	return CreateInstance({1.0f, 0.0f, 0.0f, 0.0f,
						   0.0f, 1.0f, 0.0f, 0.0f,
						   0.0f, 0.0f, 1.0f, 0.0f});
}

TLAS::TLAS(Device &device, const std::vector<VkAccelerationStructureInstanceKHR> &instances)
	: AS(device)
{
	auto staging = mDevice.CreateCPUBuffer((void *)instances.data(), sizeof(VkAccelerationStructureInstanceKHR) * instances.size(), BufferUsage::TRANSFER_SRC);

	mInstanceBuffer = mDevice.CreateGPUBuffer(staging->GetSize(), BufferUsage::TRANSFER_DST | BufferUsage::ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY);
	mInstanceBuffer->UploadDataFrom(staging->GetSize(), *staging);

	VkAccelerationStructureGeometryKHR asGeometryInfo{};
	asGeometryInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
	asGeometryInfo.flags = VK_GEOMETRY_OPAQUE_BIT_KHR;
	asGeometryInfo.geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR;
	asGeometryInfo.geometry.instances.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR;
	asGeometryInfo.geometry.instances.arrayOfPointers = VK_FALSE;
	asGeometryInfo.geometry.instances.data = mInstanceBuffer->GetVkAddress();
	asGeometryInfo.geometry.instances.arrayOfPointers = VK_FALSE;

	VkAccelerationStructureBuildGeometryInfoKHR asBuildSizeGeometryInfo{};
	asBuildSizeGeometryInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
	asBuildSizeGeometryInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
	asBuildSizeGeometryInfo.geometryCount = 1;
	asBuildSizeGeometryInfo.pGeometries = &asGeometryInfo;
	asBuildSizeGeometryInfo.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
	asBuildSizeGeometryInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
	asBuildSizeGeometryInfo.srcAccelerationStructure = nullptr;

	const uint32_t primitiveCount = instances.size();
	VkAccelerationStructureBuildSizesInfoKHR asBuildSizesInfo{};
	asBuildSizesInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;
	mDevice.vkGetAccelerationStructureBuildSizesKHR(mDevice.GetHandle(), VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR, &asBuildSizeGeometryInfo, &primitiveCount, &asBuildSizesInfo);
	asBuildSizesInfo.accelerationStructureSize = Math::RoundUp(asBuildSizesInfo.accelerationStructureSize, (uint64_t)256);
	asBuildSizesInfo.buildScratchSize = Math::RoundUp(asBuildSizesInfo.buildScratchSize, (uint64_t)mDevice.GetRayTracingAccelerationProps().minAccelerationStructureScratchOffsetAlignment);

	mAccelStorageBuffer = mDevice.CreateAccelerationStorageBuffer(asBuildSizesInfo.accelerationStructureSize);

	VkAccelerationStructureCreateInfoKHR accelerationStructureInfo{};
	accelerationStructureInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
	accelerationStructureInfo.buffer = mAccelStorageBuffer->GetHandle();
	accelerationStructureInfo.size = asBuildSizesInfo.accelerationStructureSize;
	accelerationStructureInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
	VK_CHECK(mDevice.vkCreateAccelerationStructureKHR(mDevice.GetHandle(), &accelerationStructureInfo, nullptr, &mHandle));

	mScratchBuffer = mDevice.CreateCPUStorageBuffer(asBuildSizesInfo.buildScratchSize);

	VkAccelerationStructureBuildGeometryInfoKHR asBuildGeometryInfo{};
	asBuildGeometryInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
	asBuildGeometryInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
	asBuildGeometryInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
	asBuildGeometryInfo.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
	asBuildGeometryInfo.dstAccelerationStructure = mHandle;
	asBuildGeometryInfo.geometryCount = 1;
	asBuildGeometryInfo.pGeometries = &asGeometryInfo;
	asBuildGeometryInfo.scratchData.deviceAddress = mScratchBuffer->GetAddress();

	VkAccelerationStructureBuildRangeInfoKHR asBuildRangeInfo{};
	asBuildRangeInfo.primitiveCount = primitiveCount;
	asBuildRangeInfo.primitiveOffset = 0;
	asBuildRangeInfo.firstVertex = 0;
	asBuildRangeInfo.transformOffset = 0;
	std::vector<VkAccelerationStructureBuildRangeInfoKHR *> asBuildRangeInfos{&asBuildRangeInfo};

	auto commandBuffer = mDevice.GetRayTraceCommandPool()->CreatePrimaryCommandBuffer();
	commandBuffer->ExecuteImmediately([&]()
									  { commandBuffer->BuildAccelerationStructureKHR(1, &asBuildGeometryInfo, asBuildRangeInfos.data()); });

	VkAccelerationStructureDeviceAddressInfoKHR deviceAddressInfo{};
	deviceAddressInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR;
	deviceAddressInfo.accelerationStructure = mHandle;
	mAddress = mDevice.vkGetAccelerationStructureDeviceAddressKHR(mDevice.GetHandle(), &deviceAddressInfo);

	if (mAddress == 0)
	{
		std::cout << "Invalid Handle to TLAS" << std::endl;
		abort();
	}
}

TLAS::TLAS(Device &device)
	: AS(device)
{
}

TLAS::~TLAS()
{
	mScratchBuffer.reset();
	mAccelStorageBuffer.reset();
	mInstanceBuffer.reset();

	mDevice.vkDestroyAccelerationStructureKHR(mDevice.GetHandle(), mHandle, nullptr);
}