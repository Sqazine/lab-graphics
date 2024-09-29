#include "ImageView.h"
#include "Device.h"
#include "Image.h"
#include "Utils.h"
#include <iostream>

ImageView2D::ImageView2D(const Device &device, VkImage image, Format format)
    : mDevice(device)
{
    VkImageViewCreateInfo imageViewInfo{};
    imageViewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    imageViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    imageViewInfo.format = format.ToVkHandle();
    imageViewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    imageViewInfo.subresourceRange.baseMipLevel = 0;
    imageViewInfo.subresourceRange.levelCount = 1;
    imageViewInfo.subresourceRange.baseArrayLayer = 0;
    imageViewInfo.subresourceRange.layerCount = 1;
    imageViewInfo.image = image;
    imageViewInfo.components.r = VK_COMPONENT_SWIZZLE_R;
    imageViewInfo.components.g = VK_COMPONENT_SWIZZLE_G;
    imageViewInfo.components.b = VK_COMPONENT_SWIZZLE_B;
    imageViewInfo.components.a = VK_COMPONENT_SWIZZLE_A;
    VK_CHECK(vkCreateImageView(mDevice.GetHandle(), &imageViewInfo, nullptr, &mHandle));
}

ImageView2D::ImageView2D(const Device &device, const Image2D *image, Format format)
    : ImageView2D(device, image->GetHandle(), format)
{
}

ImageView2D::~ImageView2D()
{
    vkDestroyImageView(mDevice.GetHandle(), mHandle, nullptr);
}

const VkImageView &ImageView2D::GetHandle() const
{
    return mHandle;
}

VkImageSubresourceRange ImageView2D::GetSubresourceRange() const
{
    VkImageSubresourceRange subresourceRange;
    subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    subresourceRange.baseMipLevel = 0;
    subresourceRange.levelCount = 1;
    subresourceRange.baseArrayLayer = 0;
    subresourceRange.layerCount = 1;

    return subresourceRange;
}