#include "SwapChain.h"
#include "Device.h"
#include "VK/Utils.h"
#include <iostream>
SwapChain::SwapChain(Device &device)
    : mDevice(device), mHandle(VK_NULL_HANDLE)
{
    Build();
}
SwapChain::~SwapChain()
{
    vkDestroySwapchainKHR(mDevice.GetHandle(), mHandle, nullptr);
}

const VkSwapchainKHR &SwapChain::GetHandle() const
{
    return mHandle;
}

Format SwapChain::GetFormat() const
{
    return mSurfaceFormat.format;
}

const VkSurfaceFormatKHR &SwapChain::GetSurfaceFormat() const
{
    return mSurfaceFormat;
}

const VkPresentModeKHR &SwapChain::GetPresentMode() const
{
    return mSurfacePresentMode;
}

Vector2u32 SwapChain::GetExtent() const
{
    return Vector2u32(mExtent.width, mExtent.height);
}

const VkExtent2D &SwapChain::GetVkExtent() const
{
    return mExtent;
}

const std::vector<VkImage> &SwapChain::GetImages() const
{
    return mSwapChainImages;
}
const VkImage &SwapChain::GetImage(uint32_t idx) const
{
    return mSwapChainImages[idx];
}
VkRect2D SwapChain::GetRenderArea() const
{
    return {{0, 0}, GetVkExtent()};
}
const std::vector<std::unique_ptr<ImageView2D>> &SwapChain::GetImageViews() const
{
    return mSwapChainImageViews;
}

void SwapChain::AcquireNextImage(const Semaphore *semaphore, const Fence *fence)
{
    if (semaphore && fence)
        VK_CHECK(vkAcquireNextImageKHR(mDevice.GetHandle(), mHandle, UINT64_MAX, semaphore->GetHandle(), fence->GetHandle(), &mNextImageIdx))
    else if (semaphore && !fence)
        VK_CHECK(vkAcquireNextImageKHR(mDevice.GetHandle(), mHandle, UINT64_MAX, semaphore->GetHandle(), nullptr, &mNextImageIdx))
    else if (!semaphore && fence)
        VK_CHECK(vkAcquireNextImageKHR(mDevice.GetHandle(), mHandle, UINT64_MAX, nullptr, fence->GetHandle(), &mNextImageIdx))
    else
        VK_CHECK(vkAcquireNextImageKHR(mDevice.GetHandle(), mHandle, UINT64_MAX, nullptr, nullptr, &mNextImageIdx))
}

uint32_t SwapChain::GetNextImageIdx() const
{
    return mNextImageIdx;
}

void SwapChain::ReBuild()
{
    mDevice.WaitIdle();
    vkDestroySwapchainKHR(mDevice.GetHandle(), mHandle, nullptr);
    mHandle = VK_NULL_HANDLE;
    Build();
}

void SwapChain::Present(const std::vector<Semaphore *> waitSemaphores)
{
    std::vector<VkSemaphore> rawWait(waitSemaphores.size());

    for (size_t i = 0; i < rawWait.size(); ++i)
        rawWait[i] = waitSemaphores[i]->GetHandle();

    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = rawWait.size();
    presentInfo.pWaitSemaphores = rawWait.data();
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = &mHandle;
    presentInfo.pImageIndices = &mNextImageIdx;

    mDevice.GetPresentQueue()->Present(presentInfo);
}

RenderPass *SwapChain::GetDefaultRenderPass() const
{
    return mDefaultRenderPass.get();
}

const std::vector<std::unique_ptr<Framebuffer>> &SwapChain::GetDefaultFrameBuffers()
{
    return mDefaultFrameBuffers;
}

VkSurfaceFormatKHR SwapChain::SelectSurfaceFormat()
{
    std::vector<VkSurfaceFormatKHR> surfaceFormats = mSwapChainSupportDetails.surfaceFormats;
    if (surfaceFormats.size() == 1 && surfaceFormats[0].format == VK_FORMAT_UNDEFINED)
        return {VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR};

    for (const auto &surfaceFormat : surfaceFormats)
    {
        if (surfaceFormat.format == VK_FORMAT_B8G8R8A8_UNORM && surfaceFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
            return surfaceFormat;
    }

    return surfaceFormats[0];
}
VkPresentModeKHR SwapChain::SelectPresentMode()
{
    VkPresentModeKHR result = VK_PRESENT_MODE_IMMEDIATE_KHR;
    for (const auto &presentMode : mSwapChainSupportDetails.surfacePresentModes)
    {
        if (presentMode == VK_PRESENT_MODE_MAILBOX_KHR)
            return presentMode;
        else if (presentMode == VK_PRESENT_MODE_FIFO_KHR)
            result = presentMode;
    }
    return result;
}
VkExtent2D SwapChain::SelectExtent()
{
    VkSurfaceCapabilitiesKHR surfaceCapabilities = mSwapChainSupportDetails.surfaceCapabilities;

    if (surfaceCapabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
        return surfaceCapabilities.currentExtent;
    else
    {
        VkExtent2D actualExtent = {(uint32_t)mDevice.GetInstance().GetWindow()->GetExtent().x, (uint32_t)mDevice.GetInstance().GetWindow()->GetExtent().y};
        actualExtent.width = std::max(surfaceCapabilities.minImageExtent.width, std::min(surfaceCapabilities.maxImageExtent.width, actualExtent.width));
        actualExtent.height = std::max(surfaceCapabilities.minImageExtent.height, std::min(surfaceCapabilities.maxImageExtent.height, actualExtent.height));

        return actualExtent;
    }
}

void SwapChain::Build()
{
    mSwapChainSupportDetails = QuerySwapChainDetails();

    mSurfaceFormat = SelectSurfaceFormat();

    mSurfacePresentMode = SelectPresentMode();
    mExtent = SelectExtent();

    VkSwapchainCreateInfoKHR swapChainInfo{};
    swapChainInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapChainInfo.surface = mDevice.GetInstance().GetSurface();
    swapChainInfo.minImageCount = mSwapChainSupportDetails.surfaceCapabilities.minImageCount + 1;
    swapChainInfo.imageFormat = mSurfaceFormat.format;
    swapChainInfo.imageColorSpace = mSurfaceFormat.colorSpace;
    swapChainInfo.imageExtent = mExtent;
    swapChainInfo.imageArrayLayers = 1;
    swapChainInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    swapChainInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    swapChainInfo.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
    swapChainInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    swapChainInfo.presentMode = mSurfacePresentMode;
    swapChainInfo.clipped = VK_TRUE;
    if (mHandle)
        swapChainInfo.oldSwapchain = mHandle;
    else
        swapChainInfo.oldSwapchain = VK_NULL_HANDLE;

    VK_CHECK(vkCreateSwapchainKHR(mDevice.GetHandle(), &swapChainInfo, nullptr, &mHandle));

    uint32_t swapChainImageCount = 0;
    vkGetSwapchainImagesKHR(mDevice.GetHandle(), mHandle, &swapChainImageCount, nullptr);
    mSwapChainImages.resize(swapChainImageCount);
    vkGetSwapchainImagesKHR(mDevice.GetHandle(), mHandle, &swapChainImageCount, mSwapChainImages.data());

    mSwapChainImageViews.resize(swapChainImageCount);
    for (uint32_t i = 0; i < swapChainImageCount; ++i)
        mSwapChainImageViews[i] = mDevice.CreateImageView(mSwapChainImages[i], mSurfaceFormat.format);

    mDefaultRenderPass = std::make_unique<RenderPass>(mDevice, GetFormat());
    mDefaultFrameBuffers.resize(GetImageViews().size());

    for (size_t i = 0; i < GetImageViews().size(); ++i)
    {
        mDefaultFrameBuffers[i] = std::make_unique<Framebuffer>(mDevice);
        mDefaultFrameBuffers[i]->AttachRenderPass(mDefaultRenderPass.get())
                                .SetExtent(GetExtent().x, GetExtent().y)
                                .BindAttachment(0, GetImageViews()[i].get());
    }
}

SwapChainSupportDetails SwapChain::QuerySwapChainDetails()
{
    SwapChainSupportDetails result;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(mDevice.GetPhysicalHandle(), mDevice.GetInstance().GetSurface(), &result.surfaceCapabilities);

    uint32_t surfaceFormatCount = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(mDevice.GetPhysicalHandle(), mDevice.GetInstance().GetSurface(), &surfaceFormatCount, nullptr);
    result.surfaceFormats.resize(surfaceFormatCount);
    vkGetPhysicalDeviceSurfaceFormatsKHR(mDevice.GetPhysicalHandle(), mDevice.GetInstance().GetSurface(), &surfaceFormatCount, result.surfaceFormats.data());

    uint32_t presentModeCount = 0;
    vkGetPhysicalDeviceSurfacePresentModesKHR(mDevice.GetPhysicalHandle(), mDevice.GetInstance().GetSurface(), &presentModeCount, nullptr);
    result.surfacePresentModes.resize(presentModeCount);
    vkGetPhysicalDeviceSurfacePresentModesKHR(mDevice.GetPhysicalHandle(), mDevice.GetInstance().GetSurface(), &presentModeCount, result.surfacePresentModes.data());

    return result;
}