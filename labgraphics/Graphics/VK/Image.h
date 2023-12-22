#pragma once
#include <vulkan/vulkan.h>
#include <memory>
#include "ImageView.h"
#include "Buffer.h"
#include "Format.h"
#include "Enum.h"

class Image2D
{
public:
    Image2D(class Device &device, uint32_t width, uint32_t height, Format format, ImageTiling tiling, ImageUsage usage);
    Image2D(class Device &device, uint32_t width, uint32_t height, Format format, ImageTiling tiling, ImageUsage usage, VkMemoryPropertyFlags memoryProp);
    virtual ~Image2D();

    const VkImage &GetHandle() const;
    const VkDeviceMemory &GetMemory() const;

    const ImageView2D *GetView() const;
    const Format &GetFormat() const;
    uint32_t GetWidth() const;
    uint32_t GetHeight() const;

    ImageAspect GetAspect() const;

    VkSubresourceLayout GetSubResourceLayout() const;
    VkImageCopy GetImageCopy(uint32_t width, uint32_t height);

    ImageLayout GetImageLayout() const;

    uint32_t GetMipLevel() const;

    void TransitionToNewLayout(ImageLayout newLayout);

    template <typename T>
    std::vector<T> GetRawData(const ImageAspect &aspect);

protected:
    class Device &mDevice;

    VkImageCreateInfo mImageInfo;

    VkImage mHandle;
    VkDeviceMemory mMemory;
    std::unique_ptr<ImageView2D> mImageView;

    uint32_t mWidth, mHeight;
    Format mFormat;

    ImageAspect mAspect;

    VkImageSubresource mSubResource;

    friend class RasterCommandBuffer;
    friend class ComputeCommandBuffer;
    friend class TransferCommandBuffer;

    ImageLayout mImageLayout;
};

template <typename T>
std::vector<T> Image2D::GetRawData(const ImageAspect &aspect)
{
    auto srcImg = this;

    auto stagingBuffer = std::make_unique<VKBuffer>(mImageInfo.extent.width * mImageInfo.extent.height * sizeof(T), VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

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

class CpuImage2D : public Image2D
{
public:
    CpuImage2D(class Device &device, uint32_t width, uint32_t height, Format format, ImageTiling tiling, ImageUsage usage);
    ~CpuImage2D() override;
};

class GpuImage2D : public Image2D
{
public:
    GpuImage2D(class Device &device, uint32_t width, uint32_t height, Format format, ImageTiling tiling, ImageUsage usage);
    ~GpuImage2D() override;

    void UploadDataFrom(uint64_t bufferSize, CpuBuffer *stagingBuffer, ImageLayout oldLayout, ImageLayout newLayout);
};