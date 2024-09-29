#include <vector>
#include <vulkan/vulkan.h>
#include "Buffer.h"
template <typename T>
std::vector<T> Image2D::GetRawData(const ImageAspect &aspect)
{
    auto srcImg = this;

    auto stagingBuffer = std::make_unique<CpuBuffer>(mDevice, mImageInfo.extent.width * mImageInfo.extent.height * sizeof(T), BufferUsage::TRANSFER_DST);

    GraphicsContext::GetDevice()->GetGraphicsCommandPool()->SubmitOnce([&](CommandBuffer *cmd)
                                                                       {
                                                                           auto oldLayout = srcImg->GetImageLayout();
                                                                           cmd->TransitionImageNewLayout(srcImg, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);

                                                                           VkBufferImageCopy copyRegion{};
                                                                           copyRegion.bufferOffset = 0;
                                                                           copyRegion.bufferRowLength = 0;
                                                                           copyRegion.bufferImageHeight = 0;
                                                                           copyRegion.imageExtent = {mImageInfo.extent.width, mImageInfo.extent.height, 1};
                                                                           copyRegion.imageOffset = {0, 0, 0};
                                                                           copyRegion.imageSubresource = {(VkImageAspectFlags)aspect, 0, 0, 1};

                                                                           vkCmdCopyImageToBuffer(cmd->Get(), srcImg->Get(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, stagingBuffer->GetBuffer(), 1, &copyRegion);

                                                                           cmd->TransitionImageNewLayout(srcImg, oldLayout);
                                                                       });

    std::vector<T> result(mImageInfo.extent.width * mImageInfo.extent.height);

    T *data = nullptr;
    vkMapMemory(GraphicsContext::GetDevice()->GetLogicalDevice(), stagingBuffer->GetMemory(), 0, VK_WHOLE_SIZE, 0, (void **)&data);

    for (int32_t i = 0; i < mImageInfo.extent.width * mImageInfo.extent.height; ++i)
        result[i] = data[i];

    return result;
}