#include "Buffer.h"
#include "Device.h"
#include <iostream>
#include "Utils.h"
#include "CommandPool.h"
#include "CommandBuffer.h"
Buffer::Buffer(Device &device,
               uint64_t size,
               BufferUsage usage,
               VkMemoryPropertyFlags properties)
    : Buffer(device, nullptr, size, usage, properties)
{
}

Buffer::Buffer(Device &device,
               void *srcData,
               uint64_t size,
               BufferUsage usage,
               VkMemoryPropertyFlags properties)
    : mDevice(device), mSize(size)
{
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.pNext = nullptr;
    bufferInfo.flags = 0;
    bufferInfo.size = mSize;

    if (mDevice.GetRequiredFeature() & DeviceFeature::BUFFER_ADDRESS)
        bufferInfo.usage = BUFFER_USAGE_CAST(usage | BufferUsage::SHADER_DEVICE_ADDRESS);
    else
        bufferInfo.usage = BUFFER_USAGE_CAST(usage);

    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VK_CHECK(vkCreateBuffer(mDevice.GetHandle(), &bufferInfo, nullptr, &mHandle));

    VkMemoryRequirements memRequirements;
    memRequirements = GetMemoryRequirements();

    mAlignedMemorySize = memRequirements.size;

    VkMemoryAllocateFlagsInfo flagsInfo{};
    flagsInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO;
    flagsInfo.pNext = nullptr;
    flagsInfo.deviceMask = -1;
    if (mDevice.GetRequiredFeature() & DeviceFeature::BUFFER_ADDRESS)
        flagsInfo.flags = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT;
    else
        flagsInfo.flags = 0;

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.pNext = &flagsInfo;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = mDevice.FindMemoryType(memRequirements.memoryTypeBits, properties);

    VK_CHECK(vkAllocateMemory(mDevice.GetHandle(), &allocInfo, nullptr, &mMemory));
    VK_CHECK(vkBindBufferMemory(mDevice.GetHandle(), mHandle, mMemory, 0));

    VkBufferDeviceAddressInfoKHR bufferAddressInfo = {};
    bufferAddressInfo.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO_KHR;
    bufferAddressInfo.buffer = mHandle;

    if (srcData)
    {
        void *dstData = nullptr;
        VK_CHECK(vkMapMemory(mDevice.GetHandle(), mMemory, 0, mSize, 0, &dstData));
        memcpy(dstData, srcData, mSize);
        vkUnmapMemory(mDevice.GetHandle(), mMemory);
    }

    if (mDevice.GetRequiredFeature() & DeviceFeature::BUFFER_ADDRESS)
        mAddress = mDevice.vkGetBufferDeviceAddressKHR(mDevice.GetHandle(), &bufferAddressInfo);
    else
        mAddress = -1;
}
Buffer::~Buffer()
{
    vkFreeMemory(mDevice.GetHandle(), mMemory, nullptr);
    vkDestroyBuffer(mDevice.GetHandle(), mHandle, nullptr);
}

VkMemoryRequirements Buffer::GetMemoryRequirements() const
{
    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(mDevice.GetHandle(), mHandle, &memRequirements);
    return memRequirements;
}

const VkBuffer &Buffer::GetHandle() const
{
    return mHandle;
}
const VkDeviceMemory &Buffer::GetMemory() const
{
    return mMemory;
}

uint64_t Buffer::GetSize() const
{
    return mSize;
}

uint64_t Buffer::GetAlignedMemorySize() const
{
    return mAlignedMemorySize;
}

uint64_t Buffer::GetAddress() const
{
    if ((mDevice.GetRequiredFeature() & DeviceFeature::BUFFER_ADDRESS) != DeviceFeature::BUFFER_ADDRESS)
        LOG_ERROR("Vulkan Device not support or not open VkPhysicalDeviceBufferDeviceAddressFeatures");
    return mAddress;
}

VkDeviceOrHostAddressConstKHR Buffer::GetVkAddress() const
{
    VkDeviceOrHostAddressConstKHR address{};
    address.deviceAddress = GetAddress();
    return address;
}

void Buffer::Unmap()
{
    vkUnmapMemory(mDevice.GetHandle(), mMemory);
}

void Buffer::FillWhole(const void *data)
{
    Fill(0, mSize, data);
}

void Buffer::Fill(size_t offset, size_t size, const void *data)
{
    void *mappedMemory = nullptr;
    VK_CHECK(vkMapMemory(mDevice.GetHandle(), mMemory, offset, size, 0, &mappedMemory));
    std::memset(mappedMemory, 0, size);
    std::memcpy(mappedMemory, data, size);
    vkUnmapMemory(mDevice.GetHandle(), mMemory);
}

CpuBuffer::CpuBuffer(Device &device, void *srcData, uint64_t size, BufferUsage usage)
    : Buffer(device, srcData, size, usage, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)
{
}
CpuBuffer::CpuBuffer(Device &device, uint64_t size, BufferUsage usage)
    : CpuBuffer(device, nullptr, size, usage)
{
}

GpuBuffer::GpuBuffer(Device &device, uint64_t size, BufferUsage usage)
    : Buffer(device, size, usage, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)
{
}

void GpuBuffer::UploadDataFrom(uint64_t bufferSize, const CpuBuffer &stagingBuffer)
{
    auto cmd = mDevice.GetTransferCommandPool()->CreatePrimaryCommandBuffer();
    cmd->ExecuteImmediately([&]()
                            {
                                VkBufferCopy copyRegion{};
                                copyRegion.srcOffset = 0;
                                copyRegion.dstOffset = 0;
                                copyRegion.size = bufferSize;

                                cmd->CopyBuffer(*this, stagingBuffer, copyRegion);
                            });
}
