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
    Buffer(class Device &device,
           uint64_t size,
           BufferUsage usage,
           VkMemoryPropertyFlags properties);

    Buffer(class Device &device,
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

template <typename T>
inline T *Buffer::Map(size_t offset, size_t size) const
{
    T *data;
    VK_CHECK(vkMapMemory(mDevice.GetHandle(), mMemory, offset, size, 0, (void **)&data));
    return data;
}

template <typename T>
inline T *Buffer::MapWhole() const
{
    T *data;
    VK_CHECK(vkMapMemory(mDevice.GetHandle(), mMemory, 0, mSize, 0, (void **)&data));
    return data;
}

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

template <typename T>
class VertexBuffer : public GpuBuffer
{
public:
    VertexBuffer(class Device &device, const std::vector<T> &vertices);
};

template <typename T>
inline VertexBuffer<T>::VertexBuffer(class Device &device, const std::vector<T> &vertices)
    : GpuBuffer(device, sizeof(T) * vertices.size(), VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)
{
    uint64_t bufferSize = sizeof(T) * vertices.size();
    CpuBuffer stagingBuffer = CpuBuffer(bufferSize, BufferUsage::TRANSFER_SRC);
    stagingBuffer.Fill(bufferSize, vertices.data());

    UploadDataFrom(bufferSize, stagingBuffer);
}

template <typename T>
class IndexBuffer : public GpuBuffer
{
public:
    IndexBuffer(const std::vector<T> &indices);
    VkIndexType GetDataType() const;

private:
    VkIndexType mDataType;
};

template <typename T>
inline IndexBuffer<T>::IndexBuffer(const std::vector<T> &indices)
    : GpuBuffer(sizeof(T) * indices.size(), VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)
{
    mDataType = DataStr2VkIndexType(typeid(T).name());

    uint64_t bufferSize = sizeof(T) * indices.size();
    CpuBuffer stagingBuffer = CpuBuffer(bufferSize, BufferUsage::TRANSFER_SRC);
    stagingBuffer.Fill(bufferSize, indices.data());

    UploadDataFrom(bufferSize, stagingBuffer);
}
template <typename T>
inline VkIndexType IndexBuffer<T>::GetDataType() const
{
    return mDataType;
}

template <typename T>
class UniformBuffer : public CpuBuffer
{
public:
    UniformBuffer(class Device &device);

    void Set(const T &data);
};

template <typename T>
inline UniformBuffer<T>::UniformBuffer(Device &device)
    : CpuBuffer(device, sizeof(T), BufferUsage::UNIFORM)
{
}

template <typename T>
inline void UniformBuffer<T>::Set(const T &data)
{
    this->Fill(0, sizeof(T), (void *)&data);
}