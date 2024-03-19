#include "Device.h"

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

template <typename T>
inline VertexBuffer::VertexBuffer(Device &device, const std::vector<T> &vertices, BufferUsage extractUsage)
    : GpuBuffer(device, sizeof(T) * vertices.size(), BufferUsage::TRANSFER_DST | BufferUsage::VERTEX | extractUsage)
{
    uint64_t bufferSize = sizeof(T) * vertices.size();
    CpuBuffer stagingBuffer = CpuBuffer(device, bufferSize, BufferUsage::TRANSFER_SRC);
    stagingBuffer.Fill(0, bufferSize, vertices.data());

    UploadDataFrom(bufferSize, stagingBuffer);
}

template <typename T>
inline IndexBuffer::IndexBuffer(Device &device, const std::vector<T> &indices, BufferUsage extractUsage)
    : GpuBuffer(device, sizeof(T) * indices.size(), BufferUsage::TRANSFER_DST | BufferUsage::INDEX | extractUsage)
{
    mDataType = DataStr2VkIndexType(typeid(T).name());

    uint64_t bufferSize = sizeof(T) * indices.size();
    CpuBuffer stagingBuffer = CpuBuffer(device, bufferSize, BufferUsage::TRANSFER_SRC);
    stagingBuffer.Fill(0, bufferSize, indices.data());

    UploadDataFrom(bufferSize, stagingBuffer);
}

inline VkIndexType IndexBuffer::GetDataType() const
{
    return mDataType;
}

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