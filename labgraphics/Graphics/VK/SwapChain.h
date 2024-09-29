#pragma once
#include <vulkan/vulkan.h>
#include <memory>
#include <vector>
#include "SyncObject.h"
#include "ImageView.h"
#include "Math/Vector2.h"
#include "RenderPass.h"
#include "Framebuffer.h"

struct SwapChainSupportDetails
{
    VkSurfaceCapabilitiesKHR surfaceCapabilities;
    std::vector<VkSurfaceFormatKHR> surfaceFormats;
    std::vector<VkPresentModeKHR> surfacePresentModes;
};

class SwapChain
{
public:
    SwapChain(class Device &device);
    ~SwapChain();

    const VkSwapchainKHR &GetHandle() const;
    Format GetFormat() const;
    const VkSurfaceFormatKHR &GetSurfaceFormat() const;
    const VkPresentModeKHR &GetPresentMode() const;

    Vector2u32 GetExtent() const;
    const VkExtent2D &GetVkExtent() const;
    const std::vector<VkImage> &GetImages() const;
    const VkImage &GetImage(uint32_t idx) const;
    VkRect2D GetRenderArea() const;

    const std::vector<std::unique_ptr<ImageView2D>> &GetImageViews() const;

    void AcquireNextImage(const Semaphore *semaphore = nullptr, const Fence *fence = nullptr);

    uint32_t GetNextImageIdx() const;

    void ReBuild();

    void Present(const std::vector<Semaphore *> waitSemaphores);

    RenderPass *GetDefaultRenderPass() const;
    const std::vector<std::unique_ptr<Framebuffer>> &GetDefaultFrameBuffers();

private:
    void Build();

    SwapChainSupportDetails QuerySwapChainDetails();

    VkSurfaceFormatKHR SelectSurfaceFormat();
    VkPresentModeKHR SelectPresentMode();
    VkExtent2D SelectExtent();

    class Device &mDevice;

    VkSwapchainKHR mHandle;
    VkSurfaceFormatKHR mSurfaceFormat;
    VkPresentModeKHR mSurfacePresentMode;
    VkExtent2D mExtent;

    SwapChainSupportDetails mSwapChainSupportDetails;

    std::vector<VkImage> mSwapChainImages;
    std::vector<std::unique_ptr<ImageView2D>> mSwapChainImageViews;

    std::unique_ptr<RenderPass> mDefaultRenderPass;
    std::vector<std::unique_ptr<Framebuffer>> mDefaultFrameBuffers;

    uint32_t mNextImageIdx;
};