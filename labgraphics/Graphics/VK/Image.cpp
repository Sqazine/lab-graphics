#include "Image.h"
#include "Device.h"
#include "Utils.h"
#include "CommandPool.h"
#include "App.h"
#include <iostream>
Image2D::Image2D(Device &device, uint32_t width, uint32_t height, Format format, ImageTiling tiling, ImageUsage usage)
    : mDevice(device), mFormat(format), mWidth(width), mHeight(height), mAspect(ImageAspect::COLOR)
{
    mImageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    mImageInfo.imageType = VK_IMAGE_TYPE_2D;
    mImageInfo.flags = 0;
    mImageInfo.extent = {width, height, 1};
    mImageInfo.format = format.ToVkHandle();
    mImageInfo.mipLevels = 1;
    mImageInfo.arrayLayers = 1;
    mImageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    mImageInfo.usage = IMAGE_USAGE_CAST(usage);
    mImageInfo.tiling = IMAGE_TILING_CAST(tiling);
    mImageInfo.initialLayout = IMAGE_LAYOUT_CAST(ImageLayout::UNDEFINED);

    mImageLayout = ImageLayout::UNDEFINED;

    VK_CHECK(vkCreateImage(device.GetHandle(), &mImageInfo, nullptr, &mHandle));

    VkMemoryRequirements memoryRequirements{};
    vkGetImageMemoryRequirements(device.GetHandle(), mHandle, &memoryRequirements);

    VkMemoryAllocateInfo memoryAllocateInfo{};
    memoryAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    memoryAllocateInfo.allocationSize = memoryRequirements.size;
    memoryAllocateInfo.memoryTypeIndex = mDevice.FindMemoryType(memoryRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    VK_CHECK(vkAllocateMemory(mDevice.GetHandle(), &memoryAllocateInfo, nullptr, &mMemory));

    VK_CHECK(vkBindImageMemory(mDevice.GetHandle(), mHandle, mMemory, 0));

    mImageView = mDevice.CreateImageView(mHandle, mFormat);

    mSubResource.aspectMask = IMAGE_ASPECT_CAST(mAspect);
    mSubResource.mipLevel = 0;
    mSubResource.arrayLayer = 0;
}

Image2D::Image2D(Device &device, uint32_t width, uint32_t height, Format format, ImageTiling tiling, ImageUsage usage, VkMemoryPropertyFlags memoryProp)
    : mDevice(device), mFormat(format), mWidth(width), mHeight(height), mAspect(ImageAspect::COLOR)
{
    mImageLayout = ImageLayout::UNDEFINED;

    mImageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    mImageInfo.pNext = nullptr;
    mImageInfo.imageType = VK_IMAGE_TYPE_2D;
    mImageInfo.flags = 0;
    mImageInfo.extent = {width, height, 1};
    mImageInfo.format = format.ToVkHandle();
    mImageInfo.mipLevels = 1;
    mImageInfo.arrayLayers = 1;
    mImageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    mImageInfo.usage = IMAGE_USAGE_CAST(usage);
    mImageInfo.tiling = IMAGE_TILING_CAST(tiling);
    mImageInfo.initialLayout = IMAGE_LAYOUT_CAST(ImageLayout::UNDEFINED);

    if (mDevice.GetQueueFamilyIndices().IsSameFamily())
    {
        mImageInfo.queueFamilyIndexCount = 1;
        mImageInfo.pQueueFamilyIndices = &mDevice.GetQueueFamilyIndices().graphicsFamily.value();
        mImageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    }

    VK_CHECK(vkCreateImage(mDevice.GetHandle(), &mImageInfo, nullptr, &mHandle));

    VkMemoryRequirements memoryRequirements{};
    vkGetImageMemoryRequirements(mDevice.GetHandle(), mHandle, &memoryRequirements);

    VkMemoryAllocateInfo memoryAllocateInfo{};
    memoryAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    memoryAllocateInfo.allocationSize = memoryRequirements.size;
    memoryAllocateInfo.memoryTypeIndex = mDevice.FindMemoryType(memoryRequirements.memoryTypeBits, memoryProp);
    VK_CHECK(vkAllocateMemory(mDevice.GetHandle(), &memoryAllocateInfo, nullptr, &mMemory));

    VK_CHECK(vkBindImageMemory(mDevice.GetHandle(), mHandle, mMemory, 0));

    mImageView = mDevice.CreateImageView(mHandle, mFormat);

    mSubResource.aspectMask = IMAGE_ASPECT_CAST(mAspect);
    mSubResource.mipLevel = 0;
    mSubResource.arrayLayer = 0;
}

Image2D::~Image2D()
{
    mImageView.reset(nullptr);
    vkDestroyImage(mDevice.GetHandle(), mHandle, nullptr);
    vkFreeMemory(mDevice.GetHandle(), mMemory, nullptr);
}

const VkImage &Image2D::GetHandle() const
{
    return mHandle;
}

const VkDeviceMemory &Image2D::GetMemory() const
{
    return mMemory;
}

const ImageView2D *Image2D::GetView() const
{
    return mImageView.get();
}

const Format &Image2D::GetFormat() const
{
    return mFormat;
}
uint32_t Image2D::GetWidth() const
{
    return mWidth;
}
uint32_t Image2D::GetHeight() const
{
    return mHeight;
}

ImageAspect Image2D::GetAspect() const
{
    return mAspect;
}

VkSubresourceLayout Image2D::GetSubResourceLayout() const
{
    VkSubresourceLayout subResourceLayout;
    vkGetImageSubresourceLayout(mDevice.GetHandle(), mHandle, &mSubResource, &subResourceLayout);
    return subResourceLayout;
}

VkImageCopy Image2D::GetImageCopy(uint32_t width, uint32_t height)
{
    VkImageCopy copyRegion;
    copyRegion.srcSubresource = {IMAGE_ASPECT_CAST(mAspect), 0, 0, 1};
    copyRegion.srcOffset = {0, 0, 0};
    copyRegion.dstSubresource = {IMAGE_ASPECT_CAST(mAspect), 0, 0, 1};
    copyRegion.dstOffset = {0, 0, 0};
    copyRegion.extent = {width, height, 1};

    return copyRegion;
}

ImageLayout Image2D::GetImageLayout() const
{
    return mImageLayout;
}

uint32_t Image2D::GetMipLevel() const
{
    return mImageInfo.mipLevels;
}

void Image2D::TransitionToNewLayout(ImageLayout newLayout)
{
    auto cmd = mDevice.GetTransferCommandPool()->CreatePrimaryCommandBuffer();
    cmd->ExecuteImmediately([&]()
                            { cmd->TransitionImageNewLayout(this, newLayout); });

    mImageLayout = newLayout;
}

CpuImage2D::CpuImage2D(Device &device, uint32_t width, uint32_t height, Format format, ImageTiling tiling, ImageUsage usage)
    : Image2D(device, width, height, format, tiling, usage, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)
{
}

CpuImage2D::~CpuImage2D()
{
}

GpuImage2D::GpuImage2D(Device &device, uint32_t width, uint32_t height, Format format, ImageTiling tiling, ImageUsage usage)
    : Image2D(device, width, height, format, tiling, usage, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)
{
}

GpuImage2D::~GpuImage2D()
{
}

void GpuImage2D::UploadDataFrom(uint64_t bufferSize, CpuBuffer *stagingBuffer, ImageLayout oldLayout, ImageLayout newLayout)
{
    auto cmd = mDevice.GetTransferCommandPool()->CreatePrimaryCommandBuffer();

    cmd->ExecuteImmediately([&]()
                            {
                                cmd->ImageBarrier(mHandle, GetFormat(), oldLayout, ImageLayout::TRANSFER_DST_OPTIMAL);
                                cmd->CopyImageFromBuffer(this, stagingBuffer);
                                cmd->ImageBarrier(mHandle, GetFormat(), ImageLayout::TRANSFER_DST_OPTIMAL, newLayout);
                            });
}