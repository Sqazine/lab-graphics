#pragma once
#include <vector>
#include "Buffer.h"
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