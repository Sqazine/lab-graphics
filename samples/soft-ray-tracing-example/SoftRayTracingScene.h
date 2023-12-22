#pragma once
#include "Scene/Scene.h"
#include <vulkan/vulkan.h>
#include <string_view>
#include <memory>
#include <vector>
#include "Platform/Window.h"
class SoftRayTracingScene : public Scene
{
public:
    SoftRayTracingScene();
    ~SoftRayTracingScene() override;

    void Init() override;
    void ProcessInput() override;
    void Update() override;
    void Render() override;
    void RenderUI() override;
    void CleanUp() override;

private:
    void Resize();

    void Setup();

    void InitWindow();
    void InitInstance();
    void InitDevice();
    void InitSurface();
    void InitSwapChain();
    void InitRenderPass();
    void InitDescriptorSetLayout();
    void InitPipelineLayout();
    void InitPipelines();
    void InitPingPongImages();
    void InitFrameBuffers();
    void InitCommandPool();
    void InitCommandBuffers();
    void InitSynchronizationPrimitives();
    void InitDescriptorPool();
    void InitSampler();
    void InitDescriptorSets();
    void InitQueryPool();

    void RecordCommandBuffer(uint32_t idx);

    void OneTimeCommands(std::function<void(VkCommandBuffer const &)> func);
    void SingleTimeCommands(std::function<void(VkCommandBuffer)> func);

    void ClearPingPongImages();

    int32_t mWidth;
    int32_t mHeight;
    std::string_view mName;
    uint32_t mSpp;
    uint32_t mTotalFramesElapsed;
    float mCursorPosition[4];

    VkSurfaceCapabilitiesKHR mSurfaceCapabilities;
    std::vector<VkSurfaceFormatKHR> mSurfaceFormats;
    std::vector<VkPresentModeKHR> mSurfacePresentModes;

    VkFormat mPingPongImageFormat;
    VkFormat mSwapChainImageFormat;

    VkExtent2D mSwapChainExtent;

    VkPhysicalDevice mPhysicalDevice;
    VkPhysicalDeviceProperties mPhysicalDeviceProperties;
    std::vector<VkQueueFamilyProperties> mQueueFamilyProperties;
    VkDebugUtilsMessengerEXT mDebugUtilsMessenger;
    VkQueue mQueue;
    uint32_t mQueueFamilyIndex;

    VkInstance mInstance;
    VkDevice mDevice;
    VkSurfaceKHR mSurface;
    VkSwapchainKHR mSwapChain{VK_NULL_HANDLE};
    VkRenderPass mRenderPass;
    VkPipelineLayout mPipelineLayout;
    VkPipeline mPipelinePathTrace;
    VkPipeline mPipelineComposite;

    VkCommandPool mCommandPool;
    VkSemaphore mSemaphoreImageAvailable;
    VkSemaphore mSemaphoreRenderFinished;

    std::vector<VkImage> mSwapChainImages;
    std::vector<VkImageView> mSwapChainImageViews;

    std::vector<VkCommandBuffer> mCommandBuffers;
    std::vector<VkFence> mFences;

    VkImage mImageA;
    VkImage mImageB;

    VkDeviceMemory mDeviceMemoryAB;
    VkImageView mImageViewA;
    VkImageView mImageViewB;

    VkDescriptorSetLayout mDescriptorSetLayout;
    VkDescriptorPool mDescriptorPool;
    VkDescriptorSet mDescriptorSetA;
    VkDescriptorSet mDescriptorSetB;
    VkSampler mSampler;

    std::vector<VkFramebuffer> mPingPongFramebuffers;

    VkQueryPool mQueryPool;
};