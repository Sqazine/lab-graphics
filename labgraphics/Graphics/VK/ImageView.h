#pragma once
#include <vulkan/vulkan.h>
#include "Format.h"
class ImageView2D
{
public:
    ImageView2D(const class Device &device, VkImage image, Format format);
    ImageView2D(const class Device &device,const class Image2D* image, Format format);
    ~ImageView2D();

    const VkImageView &GetHandle() const;

    const VkImage& GetRelatedImageHandle() const;

    VkImageSubresourceRange GetSubresourceRange() const;
private:
    const class Device &mDevice;
    VkImage mRelatedImageHandle;
    VkImageView mHandle;
};