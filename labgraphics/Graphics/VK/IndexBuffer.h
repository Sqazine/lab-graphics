#pragma once
#include <vulkan/vulkan.h>
#include <vector>
#include "Utils.h"

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
    VKBuffer stagingBuffer = VKBuffer(bufferSize,
                                      VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                                      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    stagingBuffer.Fill(bufferSize, indices.data());

    UploadDataFrom(bufferSize, stagingBuffer);
}
template <typename T>
inline VkIndexType IndexBuffer<T>::GetDataType() const
{
    return mDataType;
}