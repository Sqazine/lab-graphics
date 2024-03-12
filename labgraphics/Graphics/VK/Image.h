#pragma once
#include <vulkan/vulkan.h>
#include <memory>
#include <vector>
#include "ImageView.h"
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

    void UploadDataFrom(uint64_t bufferSize,class CpuBuffer *stagingBuffer, ImageLayout oldLayout, ImageLayout newLayout);
};