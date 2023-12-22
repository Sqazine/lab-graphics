#pragma once
#include <vulkan/vulkan.h>
#include "Camera.h"
#include <vector>
#include "Math/Vector2.h"
#include <memory>
#include "SBT.h"
#include "Scene.h"

struct VertexAttribute
{
    Vector4f normal;
    Vector4f uv;
};

struct UniformParams
{
    Vector4f camPos;
    Vector4f camDir;
    Vector4f camUp;
    Vector4f camSide;
    Vector4f camNearFarFov;
    uint32_t currentSamplesCount=0;
};

class RtxApp
{
public:
    RtxApp();
    ~RtxApp();

    void Run();

protected:
    void InitApp();
    void FreeResources();
    void FillCommandBuffer(VkCommandBuffer commandBuffer, const size_t imageIndex);
    void Update();

private:
    void Init();
    void ProcessInput();
    void Draw();
    void Shutdown();

    void InitVulkan();

    void InitFencesAndCommandPool();
    void InitOffscreenImage();
    void InitCommandBuffers();
    void InitSynchronization();
    void FillCommandBuffers();

    void FreeVulkan();

    void LoadSceneGeometry();
    void CreateScene();
    void CreateCamera();
    void UpdateCameraParams(struct UniformParams *params, const float dt);
    void CreateDescriptorSetLayouts();
    void CreateRayTracingPipelineAndSBT();
    void UpdateDescriptorSets();

    bool mIsRunning;

    Timer mTimer;

    std::unique_ptr<Window> mWindow;
    std::unique_ptr<Instance> mInstance{VK_NULL_HANDLE};
    std::unique_ptr<Device> mDevice{VK_NULL_HANDLE};
    std::unique_ptr<SwapChain> mSwapChain{VK_NULL_HANDLE};

    std::vector<std::unique_ptr<Fence>> mWaitForFrameFences;
    VkCommandPool mCommandPool{VK_NULL_HANDLE};
    VkUtils::Image mOffscreenImage;
    std::vector<VkCommandBuffer> mCommandBuffers;
    VkSemaphore mSemaphoreImageAcquired{VK_NULL_HANDLE};
    VkSemaphore mSemaphoreRenderFinished{VK_NULL_HANDLE};

    std::vector<VkDescriptorSetLayout> mDescriptorSetsLayouts;
    VkPipelineLayout mPipelineLayout;
    VkPipeline mPipeline;
    VkDescriptorPool mDescriptorPool;
    std::vector<VkDescriptorSet> mDescriptorSets;

    SBT mSBT;

    _Scene mScene;
    VkUtils::Image mEnvTexture;
    VkDescriptorImageInfo mEnvTextureDescInfo;

    _Camera mCamera;
    VkUtils::Buffer mCameraBuffer;

    Vector2f mCursorPos;
};