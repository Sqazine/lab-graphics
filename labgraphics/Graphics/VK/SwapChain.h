#pragma once
#include <vulkan/vulkan.h>
#include <memory>
#include <vector>
#include "SyncObject.h"
#include "ImageView.h"
#include "Math/Vector2.h"
class SwapChain
{
public:
    SwapChain(const class Device &device);
    ~SwapChain();

    const VkSwapchainKHR &GetHandle() const;
    const Format &GetFormat() const;
    const VkPresentModeKHR &GetPresentMode() const;
    Vector2u32 GetExtent() const;
    const VkExtent2D& GetVkExtent() const;

    const std::vector<VkImage> &GetImages() const;
    const VkImage &GetImage(uint32_t idx) const;
    const std::vector<std::unique_ptr<ImageView2D>> &GetImageViews() const;

    uint32_t GetImageCount() const;

    uint32_t AcquireNextImage(const Semaphore *semaphore = nullptr, const Fence *fence = nullptr) const;

private:
    VkSurfaceFormatKHR SelectSurfaceFormat();
    VkPresentModeKHR SelectPresentMode();
    VkExtent2D SelectExtent();

    Format mFormat;

    const class Device &mDevice;
    VkSwapchainKHR mHandle;
    VkPresentModeKHR mSurfacePresentMode;
    VkExtent2D mExtent;

    std::vector<VkImage> mSwapChainImages;
    std::vector<std::unique_ptr<ImageView2D>> mSwapChainImageViews;
};