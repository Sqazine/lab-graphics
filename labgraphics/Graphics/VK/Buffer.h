#pragma once
#include <vulkan/vulkan.h>
#include <vector>
#include <string_view>
#include "Utils.h"
#include "Enum.h"

class Buffer
{
public:
    Buffer( class Device &device,
           uint64_t size,
           BufferUsage usage,
           VkMemoryPropertyFlags properties);

    Buffer( class Device &device,
           void *srcData,
           uint64_t size,
           BufferUsage usage,
           VkMemoryPropertyFlags properties);

    ~Buffer();

    const VkBuffer &GetHandle() const;
    const VkDeviceMemory &GetMemory() const;
    uint64_t GetSize() const;
    uint64_t GetAlignedMemorySize() const;
    uint64_t GetAddress() const;

    VkDeviceOrHostAddressConstKHR GetVkAddress() const;

    void Fill(const void *data);
    void Fill(size_t offset,size_t size, const void *data);

    template <typename T>
    T *Map(size_t offset, size_t size) const;
    void Unmap();
protected:
    class Device &mDevice;

private:
    VkMemoryRequirements GetMemoryRequirements() const;

    VkBuffer mHandle;
    VkDeviceMemory mMemory;
    uint64_t mSize;
    uint64_t mAlignedMemorySize;
    uint64_t mAddress;
};

template <typename T>
inline T *Buffer::Map(size_t offset, size_t size) const
{
    T *data;
    VK_CHECK(vkMapMemory(mDevice.GetHandle(), mMemory, offset, size, 0, (void **)&data));
    return data;
}

class CpuBuffer:public Buffer
{
public:
    CpuBuffer(class Device &device, void *srcData, uint64_t size, BufferUsage usage);
    CpuBuffer(class Device &device, uint64_t size, BufferUsage usage);
};

class GpuBuffer:public Buffer
{
public:
    GpuBuffer( class Device &device, uint64_t size, BufferUsage usage);

    void UploadDataFrom(uint64_t bufferSize, const CpuBuffer &stagingBuffer);
};