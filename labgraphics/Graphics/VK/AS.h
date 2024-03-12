#pragma once
#include <memory>
#include <vulkan/vulkan.h>
#include "Buffer.h"
#include "CommandPool.h"
#include <iostream>

class AS
{
public:
    AS(class Device &device);
    virtual ~AS() = default;
    const VkAccelerationStructureKHR &GetHandle() const;
    uint64_t GetAddress() const;

    AS(AS &&other)
        : mDevice(other.mDevice)
    {
        this->mAccelStorageBuffer = std::move(other.mAccelStorageBuffer);
        this->mScratchBuffer = std::move(other.mScratchBuffer);
        this->mHandle = other.mHandle;
        this->mAddress = other.mAddress;
    }

protected:
    class Device &mDevice;

    std::unique_ptr<Buffer> mAccelStorageBuffer;
    std::unique_ptr<Buffer> mScratchBuffer;
    VkAccelerationStructureKHR mHandle;
    uint64_t mAddress = 0;
};

class BLAS : public AS
{
public:
    template <typename T1, typename T2>
    BLAS(Device &device, const std::vector<T1> &vertices, const std::vector<T2> &indices);
    BLAS(class Device &device);
    ~BLAS() override;

    BLAS(BLAS &&other)
        : AS(other.mDevice)
    {
        this->mVertexBuffer = std::move(other.mVertexBuffer);
        this->mIndexBuffer = std::move(other.mIndexBuffer);
    }

    template <typename vType, typename iType>
    void SetData(const std::vector<vType> &vertices, const std::vector<iType> &indices);

    VkAccelerationStructureInstanceKHR CreateInstance(VkTransformMatrixKHR matrix);
    VkAccelerationStructureInstanceKHR CreateInstance();

private:
    static uint32_t mInstanceID;

    std::unique_ptr<Buffer> mVertexBuffer;
    std::unique_ptr<IndexBuffer> mIndexBuffer;
};

template <typename T1, typename T2>
BLAS::BLAS(Device &device, const std::vector<T1> &vertices, const std::vector<T2> &indices)
    : AS(device)
{
    SetData(vertices, indices);
}

template <typename vType, typename iType>
inline void BLAS::SetData(const std::vector<vType> &vertices, const std::vector<iType> &indices)
{
    mVertexBuffer = mDevice.CreateRayTraceVertexBuffer(vertices);

    if (!indices.empty())
        mIndexBuffer = mDevice.CreateRayTraceIndexBuffer(indices);

    VkAccelerationStructureGeometryKHR asGeometryInfo = {};
    asGeometryInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
    asGeometryInfo.flags = VK_GEOMETRY_OPAQUE_BIT_KHR;
    asGeometryInfo.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR;
    asGeometryInfo.geometry.triangles.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;
    asGeometryInfo.geometry.triangles.vertexData = mVertexBuffer->GetVkAddress();
    asGeometryInfo.geometry.triangles.vertexFormat = VK_FORMAT_R32G32B32_SFLOAT;
    asGeometryInfo.geometry.triangles.maxVertex = (uint32_t)vertices.size();
    asGeometryInfo.geometry.triangles.vertexStride = sizeof(vType);

    uint32_t primitiveCount = (uint32_t)vertices.size() / 3;
    if (!indices.empty())
    {
        asGeometryInfo.geometry.triangles.indexData = mIndexBuffer->GetVkAddress();
        asGeometryInfo.geometry.triangles.indexType = DataStr2VkIndexType(typeid(iType).name());
        primitiveCount = (uint32_t)indices.size() / 3;
    }

    VkAccelerationStructureBuildGeometryInfoKHR asBuildSizeGeometryInfo{};
    asBuildSizeGeometryInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
    asBuildSizeGeometryInfo.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
    asBuildSizeGeometryInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
    asBuildSizeGeometryInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
    asBuildSizeGeometryInfo.geometryCount = 1;
    asBuildSizeGeometryInfo.pGeometries = &asGeometryInfo;
    asBuildSizeGeometryInfo.srcAccelerationStructure = nullptr;

    VkAccelerationStructureBuildSizesInfoKHR asBuildSizeInfo{};
    asBuildSizeInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;
    mDevice.vkGetAccelerationStructureBuildSizesKHR(mDevice.GetHandle(), VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR, &asBuildSizeGeometryInfo, &primitiveCount, &asBuildSizeInfo);
    asBuildSizeInfo.accelerationStructureSize = Math::RoundUp(asBuildSizeInfo.accelerationStructureSize, (uint64_t)256);
    asBuildSizeInfo.buildScratchSize = Math::RoundUp(asBuildSizeInfo.buildScratchSize, (uint64_t)mDevice.GetRayTracingAccelerationProps().minAccelerationStructureScratchOffsetAlignment);

    mAccelStorageBuffer = mDevice.CreateAccelerationStorageBuffer(asBuildSizeInfo.accelerationStructureSize);

    VkAccelerationStructureCreateInfoKHR accelerationStructureInfo{};
    accelerationStructureInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
    accelerationStructureInfo.buffer = mAccelStorageBuffer->GetHandle();
    accelerationStructureInfo.size = asBuildSizeInfo.accelerationStructureSize;
    accelerationStructureInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;

    VK_CHECK(mDevice.vkCreateAccelerationStructureKHR(mDevice.GetHandle(), &accelerationStructureInfo, nullptr, &mHandle));

    mScratchBuffer = mDevice.CreateCPUStorageBuffer(asBuildSizeInfo.buildScratchSize);

    VkAccelerationStructureBuildGeometryInfoKHR asBuildGeometryInfo{};
    asBuildGeometryInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
    asBuildGeometryInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
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
    std::vector<VkAccelerationStructureBuildRangeInfoKHR *> asBuildRangeInfos = {
        &asBuildRangeInfo};

    auto commandBuffer = mDevice.GetRayTraceCommandPool()->CreatePrimaryCommandBuffer();

    commandBuffer->ExecuteImmediately([&]()
                                      { commandBuffer->BuildAccelerationStructureKHR(1, &asBuildGeometryInfo, asBuildRangeInfos.data()); });

    VkAccelerationStructureDeviceAddressInfoKHR asDeviceAddressInfo{};
    asDeviceAddressInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR;
    asDeviceAddressInfo.accelerationStructure = mHandle;
    mAddress = mDevice.vkGetAccelerationStructureDeviceAddressKHR(mDevice.GetHandle(), &asDeviceAddressInfo);

    if (mAddress == 0)
    {
        std::cout << "Invalid Handle to BLAS" << std::endl;
        abort();
    }
}

class TLAS : public AS
{
public:
    TLAS(class Device &device, const std::vector<VkAccelerationStructureInstanceKHR> &instances);
    TLAS(class Device &device);
    ~TLAS() override;

    TLAS(TLAS &&other)
        : AS(other.mDevice)
    {
        this->mInstanceBuffer = std::move(other.mInstanceBuffer);
    }

private:
    std::unique_ptr<GpuBuffer> mInstanceBuffer;
};