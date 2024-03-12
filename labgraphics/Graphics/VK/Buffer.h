#pragma once
#include <vulkan/vulkan.h>
#include <vector>
#include <string_view>
#include "Utils.h"
#include "Enum.h"
#include "Logger.h"

class Buffer
{
public:
    Buffer(class Device &device, uint64_t size, BufferUsage usage, VkMemoryPropertyFlags properties);
    Buffer(class Device &device, void *srcData, uint64_t size, BufferUsage usage, VkMemoryPropertyFlags properties);
    ~Buffer();

    const VkBuffer &GetHandle() const;
    const VkDeviceMemory &GetMemory() const;
    uint64_t GetSize() const;
    uint64_t GetAlignedMemorySize() const;
    uint64_t GetAddress() const;

    VkDeviceOrHostAddressConstKHR GetVkAddress() const;

    void FillWhole(const void *data);
    void Fill(size_t offset, size_t size, const void *data);

    template <typename T>
    T *Map(size_t offset, size_t size) const;

    template <typename T>
    T *MapWhole() const;

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

class CpuBuffer : public Buffer
{
public:
    CpuBuffer(class Device &device, void *srcData, uint64_t size, BufferUsage usage);
    CpuBuffer(class Device &device, uint64_t size, BufferUsage usage);
};

class GpuBuffer : public Buffer
{
public:
    GpuBuffer(class Device &device, uint64_t size, BufferUsage usage);
    void UploadDataFrom(uint64_t bufferSize, const CpuBuffer &stagingBuffer);
};

class VertexBuffer : public GpuBuffer
{
public:
    template <typename T>
    VertexBuffer(class Device &device, const std::vector<T> &vertices, BufferUsage extractUsage = BufferUsage::NONE);
};

class IndexBuffer : public GpuBuffer
{
public:
    template <typename T>
    IndexBuffer(class Device &device, const std::vector<T> &indices, BufferUsage extractUsage = BufferUsage::NONE);
    VkIndexType GetDataType() const;
private:
    VkIndexType mDataType;
};

template <typename T>
class UniformBuffer : public CpuBuffer
{
public:
    UniformBuffer(class Device &device);
    void Set(const T &data);
};

#include "Buffer.inl"