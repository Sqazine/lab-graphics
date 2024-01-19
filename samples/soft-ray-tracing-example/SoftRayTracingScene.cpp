#include "SoftRayTracingScene.h"
#include "VK/Utils.h"
#include "App.h"
#include <iostream>
#ifdef _DEBUG
VKAPI_ATTR VkBool32 VKAPI_CALL LogMessage(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                                          VkDebugUtilsMessageTypeFlagsEXT messageType,
                                          const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
                                          void *)
{
    std::string ms;

    switch (messageSeverity)
    {
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
        ms = "VERBOSE";
        break;
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
        ms = "ERROR";
        break;
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
        ms = "WARNING";
        break;
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
        ms = "INFO";
        break;
    default:
        ms = "UNKNOWN";
        break;
    }

    std::string mt;

    if (messageType == 7)
        mt = "General | Validation | Performance";
    else if (messageType == 6)
        mt = "Validation | Performance";
    else if (messageType == 5)
        mt = "General | Performance";
    else if (messageType == 4)
        mt = "Performance";
    else if (messageType == 3)
        mt = "General | Validation";
    else if (messageType == 2)
        mt = "Validation";
    else if (messageType == 1)
        mt = "General";
    else
        mt = "Unknown";

    printf("[%s: %s]\n%s\n", ms.c_str(), mt.c_str(), pCallbackData->pMessage);

    return VK_FALSE;
}

VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkDebugUtilsMessengerEXT *pDebugMessenger)
{
    auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
    if (func != nullptr)
        return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
    else
        return VK_ERROR_EXTENSION_NOT_PRESENT;
}
void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks *pAllocator)
{
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
    if (func != nullptr)
        func(instance, debugMessenger, pAllocator);
}

#endif

struct alignas(8) PushConstants
{
    float time;
    float frameCounter;
    float resolution[2];
    float mouse[4];
    float mouseDown[2];
};

float GetElapseTime()
{
    static std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();
    std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();

    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count();

    return static_cast<float>(ms) / 1000.0f;
}

VkShaderModule LoadShader(const VkDevice &device, const VkShaderStageFlagBits &shaderStage, const std::string &path)
{
    auto content = ReadFile(path);

    std::vector<uint32_t> binary;

    VK_CHECK(GlslToSpv(shaderStage, content, binary));

    VkShaderModule shaderModule;

    VkShaderModuleCreateInfo shaderInfo{};
    shaderInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    shaderInfo.pNext = nullptr;
    shaderInfo.flags = 0;
    shaderInfo.codeSize = binary.size() * sizeof(uint32_t);
    shaderInfo.pCode = binary.data();

    vkCreateShaderModule(device, &shaderInfo, nullptr, &shaderModule);
    return shaderModule;
}

SoftRayTracingScene::SoftRayTracingScene()
    : mSpp(0),
      mTotalFramesElapsed(0),
      mCursorPosition{0.5f, 0.5f},
      mSwapChainImageFormat(VK_FORMAT_B8G8R8A8_UNORM),
      mPingPongImageFormat(VK_FORMAT_R32G32B32_SFLOAT)
{
}
SoftRayTracingScene::~SoftRayTracingScene()
{
    vkDeviceWaitIdle(mDevice);
}

void SoftRayTracingScene::Init()
{
    mWidth = 1024;
    mHeight = 768;
    App::Instance().GetWindow()->Resize(mWidth, mHeight);
    App::Instance().GetWindow()->SetTitle("soft-ray-tracing-example-example");
    Setup();
}
void SoftRayTracingScene::ProcessInput()
{
}

void SoftRayTracingScene::Update()
{
}

void SoftRayTracingScene::Render()
{
}

void SoftRayTracingScene::RenderUI()
{
}

void SoftRayTracingScene::CleanUp()
{
}

void SoftRayTracingScene::Resize()
{
    vkDeviceWaitIdle(mDevice);

    auto extent = App::Instance().GetWindow()->GetExtent();
    mWidth = extent.x;
    mHeight = extent.y;

    vkDestroySwapchainKHR(mDevice, mSwapChain, nullptr);

    vkDestroyRenderPass(mDevice, mRenderPass, nullptr);

    vkDestroyPipeline(mDevice, mPipelinePathTrace, nullptr);
    vkDestroyPipeline(mDevice, mPipelineComposite, nullptr);

    vkFreeMemory(mDevice, mDeviceMemoryAB, nullptr);

    vkFreeDescriptorSets(mDevice, mDescriptorPool, 1, &mDescriptorSetA);
    vkFreeDescriptorSets(mDevice, mDescriptorPool, 1, &mDescriptorSetB);

    vkDestroyImage(mDevice, mImageA, nullptr);
    vkDestroyImage(mDevice, mImageB, nullptr);

    vkDestroyImageView(mDevice, mImageViewA, nullptr);
    vkDestroyImageView(mDevice, mImageViewB, nullptr);

    InitSwapChain();
    InitRenderPass();
    InitPipelines();
    InitPingPongImages();
    InitDescriptorSets();
    InitFrameBuffers();
    ClearPingPongImages();
}

void SoftRayTracingScene::Setup()
{
    InitWindow();
    InitInstance();
    InitDevice();
    InitSurface();
    InitSwapChain();
    InitRenderPass();
    InitDescriptorSetLayout();
    InitPipelineLayout();
    InitPipelines();

    InitPingPongImages();
    InitFrameBuffers();
    InitCommandPool();
    InitCommandBuffers();
    InitSynchronizationPrimitives();
    ClearPingPongImages();

    InitDescriptorPool();
    InitSampler();
    InitDescriptorSets();

    InitQueryPool();
}

void SoftRayTracingScene::InitWindow()
{
    App::Instance().GetWindow()->Show();
}
void SoftRayTracingScene::InitInstance()
{
    std::vector<const char *> layers;
    std::vector<const char *> extensions;

#ifdef _DEBUG
    extensions.emplace_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    layers.emplace_back("VK_LAYER_KHRONOS_validation");
#endif

    auto instanceRequiredExts = App::Instance().GetWindow()->GetRequiredVulkanExtensions();

    extensions.insert(extensions.end(), instanceRequiredExts.begin(), instanceRequiredExts.end());

    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pNext = nullptr;
    appInfo.pApplicationName = nullptr;
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = nullptr;
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_3;

    VkInstanceCreateInfo instInfo{};
    instInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instInfo.pNext = nullptr;
    instInfo.flags = 0;
    instInfo.enabledExtensionCount = extensions.size();
    instInfo.ppEnabledExtensionNames = extensions.data();
    instInfo.enabledLayerCount = layers.size();
    instInfo.ppEnabledLayerNames = layers.data();
    instInfo.pApplicationInfo = &appInfo;

    VK_CHECK(vkCreateInstance(&instInfo, nullptr, &mInstance));

#ifdef _DEBUG
    VkDebugUtilsMessengerCreateInfoEXT debugInfo{};
    debugInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    debugInfo.pNext = nullptr;
    debugInfo.flags = 0;
    debugInfo.pfnUserCallback = LogMessage;
    debugInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT;
    debugInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT;
    debugInfo.pUserData = nullptr;

    VK_CHECK(CreateDebugUtilsMessengerEXT(mInstance, &debugInfo, nullptr, &mDebugUtilsMessenger));
#endif
}
void SoftRayTracingScene::InitDevice()
{
    uint32_t phyDeviceCount = 0;
    vkEnumeratePhysicalDevices(mInstance, &phyDeviceCount, nullptr);
    std::vector<VkPhysicalDevice> phyDevices(phyDeviceCount);
    vkEnumeratePhysicalDevices(mInstance, &phyDeviceCount, phyDevices.data());

    assert(!phyDevices.empty());

    mPhysicalDevice = phyDevices[0];

    vkGetPhysicalDeviceProperties(mPhysicalDevice, &mPhysicalDeviceProperties);

    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(mPhysicalDevice, &queueFamilyCount, nullptr);
    mQueueFamilyProperties.resize(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(mPhysicalDevice, &queueFamilyCount, mQueueFamilyProperties.data());

    const float priority = 0.0f;
    auto predicate = [](const VkQueueFamilyProperties &item)
    {
        return item.queueFlags & VK_QUEUE_GRAPHICS_BIT;
    };

    VkDeviceQueueCreateInfo queueCreateInfo{};
    queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueCreateInfo.pNext = nullptr;
    queueCreateInfo.flags = 0;
    queueCreateInfo.pQueuePriorities = &priority;
    queueCreateInfo.queueCount = 1;
    queueCreateInfo.queueFamilyIndex = static_cast<uint32_t>(std::distance(mQueueFamilyProperties.begin(), std::find_if(mQueueFamilyProperties.begin(), mQueueFamilyProperties.end(), predicate)));

    mQueueFamilyIndex = queueCreateInfo.queueFamilyIndex;

    const std::vector<const char *> deviceExts = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME};

    VkDeviceCreateInfo deviceInfo{};
    deviceInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    deviceInfo.pNext = nullptr;
    deviceInfo.flags = 0;
    deviceInfo.queueCreateInfoCount = 1;
    deviceInfo.pQueueCreateInfos = &queueCreateInfo;
    deviceInfo.enabledExtensionCount = deviceExts.size();
    deviceInfo.ppEnabledExtensionNames = deviceExts.data();
    deviceInfo.enabledLayerCount = 0;
    deviceInfo.ppEnabledLayerNames = nullptr;
    deviceInfo.pEnabledFeatures = nullptr;

    vkCreateDevice(mPhysicalDevice, &deviceInfo, nullptr, &mDevice);

    const uint32_t queueIdx = 0;
    vkGetDeviceQueue(mDevice, mQueueFamilyIndex, queueIdx, &mQueue);
}
void SoftRayTracingScene::InitSurface()
{
    mSurface = App::Instance().GetWindow()->GetSurface(mInstance);
}

void SoftRayTracingScene::InitSwapChain()
{
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(mPhysicalDevice, mSurface, &mSurfaceCapabilities);

    uint32_t surfaceFormtCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(mPhysicalDevice, mSurface, &surfaceFormtCount, nullptr);
    mSurfaceFormats.resize(surfaceFormtCount);
    vkGetPhysicalDeviceSurfaceFormatsKHR(mPhysicalDevice, mSurface, &surfaceFormtCount, mSurfaceFormats.data());

    uint32_t surfacePresentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(mPhysicalDevice, mSurface, &surfacePresentModeCount, nullptr);
    mSurfacePresentModes.resize(surfacePresentModeCount);

    VkBool32 supportSurface = VK_FALSE;
    vkGetPhysicalDeviceSurfaceSupportKHR(mPhysicalDevice, mQueueFamilyIndex, mSurface, &supportSurface);

    mSwapChainExtent = VkExtent2D{(uint32_t)mWidth, (uint32_t)mHeight};

    VkSwapchainCreateInfoKHR swapChainInfo{};
    swapChainInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapChainInfo.pNext = nullptr;
    swapChainInfo.flags = 0;
    swapChainInfo.oldSwapchain = mSwapChain;
    swapChainInfo.imageExtent = mSwapChainExtent;
    swapChainInfo.imageFormat = mSwapChainImageFormat;
    swapChainInfo.imageArrayLayers = 1;
    swapChainInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    swapChainInfo.presentMode = VK_PRESENT_MODE_MAILBOX_KHR;
    swapChainInfo.preTransform = mSurfaceCapabilities.currentTransform;
    swapChainInfo.minImageCount = mSurfaceCapabilities.minImageCount + 1;
    swapChainInfo.surface = mSurface;
    swapChainInfo.imageColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
    swapChainInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    swapChainInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;

    VK_CHECK(vkCreateSwapchainKHR(mDevice, &swapChainInfo, nullptr, &mSwapChain));

    uint32_t swapChainImageCount = 0;
    vkGetSwapchainImagesKHR(mDevice, mSwapChain, &swapChainImageCount, nullptr);
    mSwapChainImages.resize(swapChainImageCount);
    vkGetSwapchainImagesKHR(mDevice, mSwapChain, &swapChainImageCount, mSwapChainImages.data());

    mSwapChainImageViews.clear();

    VkImageSubresourceRange subresourceRange{};
    subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    subresourceRange.baseMipLevel = 0;
    subresourceRange.levelCount = 1;
    subresourceRange.baseArrayLayer = 0;
    subresourceRange.layerCount = 1;

    for (const auto &image : mSwapChainImages)
    {
        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.pNext = nullptr;
        viewInfo.flags = 0;
        viewInfo.format = mSwapChainImageFormat;
        viewInfo.image = image;
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.subresourceRange = subresourceRange;

        VkImageView view;
        VK_CHECK(vkCreateImageView(mDevice, &viewInfo, nullptr, &view));
        mSwapChainImageViews.emplace_back(view);
    }
}
void SoftRayTracingScene::InitRenderPass()
{
    VkAttachmentDescription attachmentDescriptionPong{};
    attachmentDescriptionPong.format = mPingPongImageFormat;
    attachmentDescriptionPong.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
    attachmentDescriptionPong.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachmentDescriptionPong.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachmentDescriptionPong.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachmentDescriptionPong.initialLayout = VK_IMAGE_LAYOUT_GENERAL;
    attachmentDescriptionPong.finalLayout = VK_IMAGE_LAYOUT_GENERAL;
    attachmentDescriptionPong.samples = VK_SAMPLE_COUNT_1_BIT;

    VkAttachmentDescription attachmentDescriptionSwap{};
    attachmentDescriptionSwap.format = mSwapChainImageFormat;
    attachmentDescriptionSwap.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachmentDescriptionSwap.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachmentDescriptionSwap.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachmentDescriptionSwap.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachmentDescriptionSwap.initialLayout = VK_IMAGE_LAYOUT_GENERAL;
    attachmentDescriptionSwap.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    attachmentDescriptionSwap.samples = VK_SAMPLE_COUNT_1_BIT;

    std::vector<VkAttachmentDescription> attachmentDescs = {
        attachmentDescriptionPong,
        attachmentDescriptionSwap};

    VkAttachmentReference attachmentPongRef{};
    attachmentPongRef.attachment = 0;
    attachmentPongRef.layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    VkAttachmentReference attachmentSwapRef{};
    attachmentSwapRef.attachment = 1;
    attachmentSwapRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    // subpass 1
    std::vector<VkAttachmentReference> attachmentRefPong = {
        VkAttachmentReference{.attachment = 0, .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL},
    };

    VkSubpassDescription subPassDescPong{};
    subPassDescPong.pColorAttachments = attachmentRefPong.data();
    subPassDescPong.colorAttachmentCount = attachmentRefPong.size();

    // subpass 2
    std::vector<VkAttachmentReference> attachmentRefFinal = {
        VkAttachmentReference{.attachment = 0, .layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL},
        VkAttachmentReference{.attachment = 1, .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL},
    };

    VkSubpassDescription subPassDescFinal{};
    subPassDescFinal.colorAttachmentCount = 1;
    subPassDescFinal.pColorAttachments = &attachmentRefFinal[1];
    subPassDescFinal.inputAttachmentCount = 1;
    subPassDescFinal.pInputAttachments = &attachmentRefFinal[0];

    VkSubpassDependency subpassDependency0{};
    subpassDependency0.srcSubpass = VK_SUBPASS_EXTERNAL;
    subpassDependency0.dstSubpass = 0;
    subpassDependency0.srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
    subpassDependency0.srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
    subpassDependency0.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    subpassDependency0.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    VkSubpassDependency subpassDependency1{};
    subpassDependency1.srcSubpass = 0;
    subpassDependency1.dstSubpass = 1;
    subpassDependency1.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    subpassDependency1.srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
    subpassDependency1.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    subpassDependency1.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    std::vector<VkSubpassDescription> subPassDescs = {
        subPassDescPong,
        subPassDescFinal};

    std::vector<VkSubpassDependency> subPassDependencies = {
        subpassDependency0,
        subpassDependency1};

    VkRenderPassCreateInfo renderPassCreateInfo{};
    renderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassCreateInfo.flags = 0;
    renderPassCreateInfo.attachmentCount = attachmentDescs.size();
    renderPassCreateInfo.pAttachments = attachmentDescs.data();
    renderPassCreateInfo.dependencyCount = subPassDependencies.size();
    renderPassCreateInfo.pDependencies = subPassDependencies.data();
    renderPassCreateInfo.subpassCount = subPassDescs.size();
    renderPassCreateInfo.pSubpasses = subPassDescs.data();

    VK_CHECK(vkCreateRenderPass(mDevice, &renderPassCreateInfo, nullptr, &mRenderPass));
}
void SoftRayTracingScene::InitDescriptorSetLayout()
{
    std::vector<VkDescriptorSetLayoutBinding> descriptorSetLayoutBindings = {
        VkDescriptorSetLayoutBinding{
            .binding = 0,
            .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
            .pImmutableSamplers = nullptr},
        VkDescriptorSetLayoutBinding{
            .binding = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT,
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
            .pImmutableSamplers = nullptr},
    };

    VkDescriptorSetLayoutCreateInfo descriptorSetLayoutInfo{};
    descriptorSetLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    descriptorSetLayoutInfo.flags = 0;
    descriptorSetLayoutInfo.pNext = nullptr;
    descriptorSetLayoutInfo.bindingCount = descriptorSetLayoutBindings.size();
    descriptorSetLayoutInfo.pBindings = descriptorSetLayoutBindings.data();

    VK_CHECK(vkCreateDescriptorSetLayout(mDevice, &descriptorSetLayoutInfo, nullptr, &mDescriptorSetLayout));
}
void SoftRayTracingScene::InitPipelineLayout()
{
    VkPushConstantRange pushConstantRange{};
    pushConstantRange.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    pushConstantRange.size = sizeof(PushConstants);
    pushConstantRange.offset = 0;

    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.flags = 0;
    pipelineLayoutInfo.pNext = nullptr;
    pipelineLayoutInfo.pushConstantRangeCount = 1;
    pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;
    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.pSetLayouts = &mDescriptorSetLayout;

    VK_CHECK(vkCreatePipelineLayout(mDevice, &pipelineLayoutInfo, nullptr, &mPipelineLayout));
}
void SoftRayTracingScene::InitPipelines()
{
    auto vsQuadShaderModule = LoadShader(mDevice, VK_SHADER_STAGE_VERTEX_BIT, std::string(ASSET_DIR) + "quad.vert");
    auto fsPathTraceShaderModule = LoadShader(mDevice, VK_SHADER_STAGE_FRAGMENT_BIT, std::string(ASSET_DIR) + "pathtrace.frag");
    auto fsCompositeShaderModule = LoadShader(mDevice, VK_SHADER_STAGE_FRAGMENT_BIT, std::string(ASSET_DIR) + "composite.frag");

    VkPipelineShaderStageCreateInfo vsQuadCreateInfo{};
    vsQuadCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vsQuadCreateInfo.pNext = nullptr;
    vsQuadCreateInfo.flags = 0;
    vsQuadCreateInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vsQuadCreateInfo.module = vsQuadShaderModule;
    vsQuadCreateInfo.pName = "main";
    vsQuadCreateInfo.pSpecializationInfo = nullptr;

    VkPipelineShaderStageCreateInfo fsPathTraceCreateInfo{};
    fsPathTraceCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fsPathTraceCreateInfo.pNext = nullptr;
    fsPathTraceCreateInfo.flags = 0;
    fsPathTraceCreateInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fsPathTraceCreateInfo.module = fsPathTraceShaderModule;
    fsPathTraceCreateInfo.pName = "main";
    fsPathTraceCreateInfo.pSpecializationInfo = nullptr;

    VkPipelineShaderStageCreateInfo fsCompositeCreateInfo{};
    fsCompositeCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fsCompositeCreateInfo.pNext = nullptr;
    fsCompositeCreateInfo.flags = 0;
    fsCompositeCreateInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fsCompositeCreateInfo.module = fsCompositeShaderModule;
    fsCompositeCreateInfo.pName = "main";
    fsCompositeCreateInfo.pSpecializationInfo = nullptr;

    std::vector<VkPipelineShaderStageCreateInfo> pathTraceShaderStages = {
        vsQuadCreateInfo,
        fsPathTraceCreateInfo};

    std::vector<VkPipelineShaderStageCreateInfo> compositeShaderStages = {
        vsQuadCreateInfo,
        fsCompositeCreateInfo};

    VkPipelineVertexInputStateCreateInfo vertexInputStateCreateInfo{};
    vertexInputStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputStateCreateInfo.pNext = nullptr;
    vertexInputStateCreateInfo.flags = 0;
    vertexInputStateCreateInfo.vertexBindingDescriptionCount = 0;
    vertexInputStateCreateInfo.pVertexBindingDescriptions = nullptr;
    vertexInputStateCreateInfo.vertexAttributeDescriptionCount = 0;
    vertexInputStateCreateInfo.pVertexAttributeDescriptions = nullptr;

    VkPipelineInputAssemblyStateCreateInfo inputAssemblyCreateInfo{};
    inputAssemblyCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssemblyCreateInfo.pNext = nullptr;
    inputAssemblyCreateInfo.flags = 0;
    inputAssemblyCreateInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssemblyCreateInfo.primitiveRestartEnable = VK_FALSE;

    constexpr float minDepth = 0.0f;
    constexpr float maxDepth = 1.0f;

    VkViewport viewPort = {
        .x = 0.0f,
        .y = 0.0f,
        .width = (float)mWidth,
        .height = (float)mHeight,
        .minDepth = minDepth,
        .maxDepth = maxDepth};

    VkRect2D scissor = {
        .offset = {0, 0},
        .extent = mSwapChainExtent,
    };

    VkPipelineViewportStateCreateInfo viewportCreateInfo{};
    viewportCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportCreateInfo.pNext = nullptr;
    viewportCreateInfo.flags = 0;
    viewportCreateInfo.viewportCount = 1;
    viewportCreateInfo.pViewports = &viewPort;
    viewportCreateInfo.scissorCount = 1;
    viewportCreateInfo.pScissors = &scissor;

    VkPipelineRasterizationStateCreateInfo rasterInfo{};
    rasterInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterInfo.pNext = nullptr;
    rasterInfo.flags = 0;
    rasterInfo.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterInfo.frontFace = VK_FRONT_FACE_CLOCKWISE;
    rasterInfo.lineWidth = 1.0f;
    rasterInfo.polygonMode = VK_POLYGON_MODE_FILL;
    rasterInfo.rasterizerDiscardEnable = VK_FALSE;
    rasterInfo.depthBiasEnable = VK_FALSE;
    rasterInfo.depthClampEnable = VK_FALSE;
    rasterInfo.depthBiasSlopeFactor = 0.0f;
    rasterInfo.depthBiasClamp = 0.0f;
    rasterInfo.depthBiasConstantFactor = 0.0f;

    VkPipelineMultisampleStateCreateInfo pipelineMultisampleInfo{};
    pipelineMultisampleInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    pipelineMultisampleInfo.pNext = nullptr;
    pipelineMultisampleInfo.flags = 0;
    pipelineMultisampleInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    pipelineMultisampleInfo.sampleShadingEnable = VK_FALSE;
    pipelineMultisampleInfo.minSampleShading = 1.0f;
    pipelineMultisampleInfo.pSampleMask = nullptr;
    pipelineMultisampleInfo.alphaToOneEnable = VK_FALSE;
    pipelineMultisampleInfo.alphaToCoverageEnable = VK_FALSE;

    VkPipelineColorBlendAttachmentState blendAttachmentState{};
    blendAttachmentState.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

    VkPipelineColorBlendStateCreateInfo blendStateCreateInfo{};
    blendStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    blendStateCreateInfo.pNext = nullptr;
    blendStateCreateInfo.flags = 0;
    blendStateCreateInfo.attachmentCount = 1;
    blendStateCreateInfo.pAttachments = &blendAttachmentState;
    blendStateCreateInfo.logicOpEnable = VK_FALSE;
    blendStateCreateInfo.logicOp = VK_LOGIC_OP_CLEAR;

    VkGraphicsPipelineCreateInfo graphicsPipelineInfo{};
    graphicsPipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    graphicsPipelineInfo.flags = 0;
    graphicsPipelineInfo.stageCount = pathTraceShaderStages.size();
    graphicsPipelineInfo.pStages = pathTraceShaderStages.data();
    graphicsPipelineInfo.pVertexInputState = &vertexInputStateCreateInfo;
    graphicsPipelineInfo.pInputAssemblyState = &inputAssemblyCreateInfo;
    graphicsPipelineInfo.pRasterizationState = &rasterInfo;
    graphicsPipelineInfo.pViewportState = &viewportCreateInfo;
    graphicsPipelineInfo.pDepthStencilState = nullptr;
    graphicsPipelineInfo.pColorBlendState = &blendStateCreateInfo;
    graphicsPipelineInfo.renderPass = mRenderPass;
    graphicsPipelineInfo.subpass = 0;
    graphicsPipelineInfo.layout = mPipelineLayout;

    VK_CHECK(vkCreateGraphicsPipelines(mDevice, VK_NULL_HANDLE, 1, &graphicsPipelineInfo, nullptr, &mPipelinePathTrace));

    graphicsPipelineInfo.stageCount = compositeShaderStages.size();
    graphicsPipelineInfo.pStages = compositeShaderStages.data();
    graphicsPipelineInfo.subpass = 1;
    VK_CHECK(vkCreateGraphicsPipelines(mDevice, VK_NULL_HANDLE, 1, &graphicsPipelineInfo, nullptr, &mPipelineComposite));
}
void SoftRayTracingScene::InitPingPongImages()
{
    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.pNext = nullptr;
    imageInfo.flags = 0;
    imageInfo.format = mPingPongImageFormat;
    imageInfo.extent = {.width = (uint32_t)mWidth, .height = (uint32_t)mHeight, .depth = 1};
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.queueFamilyIndexCount = 0;
    imageInfo.pQueueFamilyIndices = nullptr;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    imageInfo.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;

    VK_CHECK(vkCreateImage(mDevice, &imageInfo, nullptr, &mImageA));
    VK_CHECK(vkCreateImage(mDevice, &imageInfo, nullptr, &mImageB));

    VkMemoryRequirements memoryRequirementA;
    vkGetImageMemoryRequirements(mDevice, mImageA, &memoryRequirementA);

    VkMemoryRequirements memoryRequirementB;
    vkGetImageMemoryRequirements(mDevice, mImageB, &memoryRequirementB);

    VkPhysicalDeviceMemoryProperties memoryProps{};
    vkGetPhysicalDeviceMemoryProperties(mPhysicalDevice, &memoryProps);
    auto chosenMemoryTypeIndex = UINT32_MAX;

    for (size_t i = 0; i < memoryProps.memoryTypeCount; ++i)
    {
        if ((memoryRequirementA.memoryTypeBits & (1 << i)) &&
            (memoryRequirementB.memoryTypeBits & (1 << i)) &&
            (memoryProps.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT))
            chosenMemoryTypeIndex = i;
    }

    if (chosenMemoryTypeIndex == UINT32_MAX)
        std::cout << "Could not find suitable memory type for allocation" << std::endl;

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.pNext = nullptr;
    allocInfo.memoryTypeIndex = chosenMemoryTypeIndex;
    allocInfo.allocationSize = memoryRequirementA.size + memoryRequirementB.size;
    vkAllocateMemory(mDevice, &allocInfo, nullptr, &mDeviceMemoryAB);

    vkBindImageMemory(mDevice, mImageA, mDeviceMemoryAB, 0);
    vkBindImageMemory(mDevice, mImageB, mDeviceMemoryAB, memoryRequirementA.size);

    VkImageSubresourceRange subresourceRange{};
    subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    subresourceRange.levelCount = 1;
    subresourceRange.baseMipLevel = 0;
    subresourceRange.layerCount = 1;
    subresourceRange.baseArrayLayer = 0;

    VkImageViewCreateInfo imageViewCreateInfo{};
    imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    imageViewCreateInfo.pNext = nullptr;
    imageViewCreateInfo.flags = 0;
    imageViewCreateInfo.subresourceRange = subresourceRange;
    imageViewCreateInfo.format = mPingPongImageFormat;
    imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    imageViewCreateInfo.image = mImageA;
    imageViewCreateInfo.components = {
        .r = VK_COMPONENT_SWIZZLE_IDENTITY,
        .g = VK_COMPONENT_SWIZZLE_IDENTITY,
        .b = VK_COMPONENT_SWIZZLE_IDENTITY,
        .a = VK_COMPONENT_SWIZZLE_IDENTITY};

    VK_CHECK(vkCreateImageView(mDevice, &imageViewCreateInfo, nullptr, &mImageViewA));

    imageViewCreateInfo.image = mImageB;
    VK_CHECK(vkCreateImageView(mDevice, &imageViewCreateInfo, nullptr, &mImageViewB));
}
void SoftRayTracingScene::InitFrameBuffers()
{
    mPingPongFramebuffers.clear();

    for (size_t i = 0; i < mSwapChainImageViews.size(); ++i)
    {
        std::vector<VkImageView> attachmentAB = {
            mImageViewA,
            mSwapChainImageViews[i],
        };

        VkFramebufferCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        createInfo.pNext = nullptr;
        createInfo.flags = 0;
        createInfo.width = mWidth;
        createInfo.height = mHeight;
        createInfo.layers = 1;
        createInfo.renderPass = mRenderPass;
        createInfo.attachmentCount = attachmentAB.size();
        createInfo.pAttachments = attachmentAB.data();

        VkFramebuffer fbAB;
        VK_CHECK(vkCreateFramebuffer(mDevice, &createInfo, nullptr, &fbAB));
        mPingPongFramebuffers.emplace_back(fbAB);

        std::vector<VkImageView> attachmentBA = {
            mImageViewB,
            mSwapChainImageViews[i],
        };
        createInfo.attachmentCount = attachmentBA.size();
        createInfo.pAttachments = attachmentBA.data();

        VkFramebuffer fbBA;
        VK_CHECK(vkCreateFramebuffer(mDevice, &createInfo, nullptr, &fbBA));
        mPingPongFramebuffers.emplace_back(fbBA);
    }
}
void SoftRayTracingScene::InitCommandPool()
{
    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT | VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
    poolInfo.queueFamilyIndex = mQueueFamilyIndex;

    VK_CHECK(vkCreateCommandPool(mDevice, &poolInfo, nullptr, &mCommandPool));
}
void SoftRayTracingScene::InitCommandBuffers()
{
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.pNext = nullptr;
    allocInfo.commandBufferCount = mPingPongFramebuffers.size() / 2;
    allocInfo.commandPool = mCommandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

    vkAllocateCommandBuffers(mDevice, &allocInfo, mCommandBuffers.data());
}
void SoftRayTracingScene::InitSynchronizationPrimitives()
{
    VkSemaphoreCreateInfo semaInfo{};
    semaInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    semaInfo.pNext = nullptr;
    semaInfo.flags = 0;

    vkCreateSemaphore(mDevice, &semaInfo, nullptr, &mSemaphoreImageAvailable);
    vkCreateSemaphore(mDevice, &semaInfo, nullptr, &mSemaphoreRenderFinished);

    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.pNext = nullptr;
    fenceInfo.flags = 0;
    for (size_t i = 0; i < mCommandBuffers.size(); ++i)
    {
        // Create each fence in a signaled state, so that the first call to `waitForFences` in the draw loop doesn't throw any errors
        VkFence fence;
        vkCreateFence(mDevice, &fenceInfo, nullptr, &fence);
        mFences.emplace_back(fence);
    }
}
void SoftRayTracingScene::InitDescriptorPool()
{
    VkDescriptorPoolSize descriptorPoolSizes[2] = {
        {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 2},
        {VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 2}};

    // We need a unique descriptor set for each ping-pong image, although they will share the same layout
    VkDescriptorPoolCreateInfo poolCreateInfo{};
    poolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolCreateInfo.pNext = nullptr;
    poolCreateInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    poolCreateInfo.poolSizeCount = 2;
    poolCreateInfo.pPoolSizes = descriptorPoolSizes;
    poolCreateInfo.maxSets = 2;

    vkCreateDescriptorPool(mDevice, &poolCreateInfo, nullptr, &mDescriptorPool);
}

void SoftRayTracingScene::InitSampler()
{
    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.flags = 0;
    samplerInfo.pNext = nullptr;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.compareOp = VK_COMPARE_OP_NEVER;
    samplerInfo.anisotropyEnable = VK_FALSE;
    samplerInfo.maxAnisotropy = 0;

    vkCreateSampler(mDevice, &samplerInfo, nullptr, &mSampler);
}
void SoftRayTracingScene::InitDescriptorSets()
{
    VkDescriptorSetAllocateInfo descriptorSetAllocInfo{};
    descriptorSetAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    descriptorSetAllocInfo.pNext = nullptr;
    descriptorSetAllocInfo.descriptorPool = mDescriptorPool;
    descriptorSetAllocInfo.descriptorSetCount = 1;
    descriptorSetAllocInfo.pSetLayouts = &mDescriptorSetLayout;

    vkAllocateDescriptorSets(mDevice, &descriptorSetAllocInfo, &mDescriptorSetA);
    vkAllocateDescriptorSets(mDevice, &descriptorSetAllocInfo, &mDescriptorSetB);

    // These are hard-coded bindings in the shader and therefore do not change between A / B
    const uint32_t cisBinding = 0;
    const uint32_t inputBinding = 1;

    VkDescriptorImageInfo descImgInfoA{};
    descImgInfoA.sampler = mSampler;
    descImgInfoA.imageView = mImageViewA;
    descImgInfoA.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    VkDescriptorImageInfo descImgInfoB{};
    descImgInfoA.sampler = mSampler;
    descImgInfoA.imageView = mImageViewB;
    descImgInfoA.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    // Write to descriptor set A, where A is the intermediate render target
    {
        VkWriteDescriptorSet cisDescSetWrite{};
        cisDescSetWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        cisDescSetWrite.pNext = nullptr;
        cisDescSetWrite.descriptorCount = 1;
        cisDescSetWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        cisDescSetWrite.dstSet = mDescriptorSetA;
        cisDescSetWrite.pImageInfo = &descImgInfoB;
        cisDescSetWrite.dstBinding = cisBinding;

        VkWriteDescriptorSet inputDescSetWrite{};
        inputDescSetWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        inputDescSetWrite.pNext = nullptr;
        inputDescSetWrite.descriptorCount = 1;
        inputDescSetWrite.descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
        inputDescSetWrite.dstSet = mDescriptorSetA;
        inputDescSetWrite.pImageInfo = &descImgInfoA;
        inputDescSetWrite.dstBinding = inputBinding;

        std::vector<VkWriteDescriptorSet> descWrites{cisDescSetWrite, inputDescSetWrite};

        vkUpdateDescriptorSets(mDevice, descWrites.size(), descWrites.data(), 0, nullptr);
    }

    // Write to descriptor set B, where B is the intermediate render target
    {
         VkWriteDescriptorSet cisDescSetWrite{};
        cisDescSetWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        cisDescSetWrite.pNext = nullptr;
        cisDescSetWrite.descriptorCount = 1;
        cisDescSetWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        cisDescSetWrite.dstSet = mDescriptorSetB;
        cisDescSetWrite.pImageInfo = &descImgInfoA;
        cisDescSetWrite.dstBinding = cisBinding;

        VkWriteDescriptorSet inputDescSetWrite{};
        inputDescSetWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        inputDescSetWrite.pNext = nullptr;
        inputDescSetWrite.descriptorCount = 1;
        inputDescSetWrite.descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
        inputDescSetWrite.dstSet = mDescriptorSetB;
        inputDescSetWrite.pImageInfo = &descImgInfoB;
        inputDescSetWrite.dstBinding = inputBinding;

        std::vector<VkWriteDescriptorSet> descWrites{cisDescSetWrite, inputDescSetWrite};

        vkUpdateDescriptorSets(mDevice, descWrites.size(), descWrites.data(), 0, nullptr);
    }
}

void SoftRayTracingScene::InitQueryPool()
{
    uint32_t timeStampValidBits=mQueueFamilyProperties[0].timestampValidBits;
    if(timeStampValidBits==0)
        throw std::runtime_error("Timestamp queries are not supported on this queue");
    else
        std::cout<<"Valid timestamp bits: " << timeStampValidBits<<std::endl;

    VkQueryPoolCreateInfo poolCreateInfo{};
    poolCreateInfo.sType=VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
    poolCreateInfo.pNext=nullptr;
    poolCreateInfo.flags=0;
    poolCreateInfo.queryCount=2;
    poolCreateInfo.queryType=VK_QUERY_TYPE_TIMESTAMP;
    
    vkCreateQueryPool(mDevice,&poolCreateInfo,nullptr,&mQueryPool);

    SingleTimeCommands([&](auto cmd){
        vkResetQueryPool(mDevice,mQueryPool,0,2);
    });
}

void SoftRayTracingScene::RecordCommandBuffer(uint32_t idx)
{
    const VkRect2D renderArea{{0, 0}, mSwapChainExtent};

    int32_t x = 0, y = 0;
    auto mouseButton = SDL_GetMouseState(&x, &y);

    VkClearValue clearValues[2];
    clearValues[0].color = {0.0f, 0.0f, 0.0f, 1.0f};
    clearValues[1].color = {0.0f, 0.0f, 0.0f, 1.0f};

    int32_t frameOffset = (mTotalFramesElapsed % 2 == 0) ? 0 : 1;

    VkCommandBufferBeginInfo cmdBeginInfo{};
    cmdBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    cmdBeginInfo.pNext = nullptr;
    cmdBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
    cmdBeginInfo.pInheritanceInfo = nullptr;

    vkBeginCommandBuffer(mCommandBuffers[idx], &cmdBeginInfo);
    vkCmdResetQueryPool(mCommandBuffers[idx], mQueryPool, 0, 2);

    VkRenderPassBeginInfo renderPassBeginInfo{};
    renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassBeginInfo.framebuffer = mPingPongFramebuffers[idx * 2 + frameOffset];
    renderPassBeginInfo.renderArea = renderArea;
    renderPassBeginInfo.renderPass = mRenderPass;
    renderPassBeginInfo.pClearValues = clearValues;
    renderPassBeginInfo.pNext = nullptr;

    vkCmdBeginRenderPass(mCommandBuffers[idx], &renderPassBeginInfo, VkSubpassContents::VK_SUBPASS_CONTENTS_INLINE);
    vkCmdWriteTimestamp(mCommandBuffers[idx], VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, mQueryPool, 0);

    auto lMouseDown = mouseButton & SDL_BUTTON_LEFT;
    auto rMouseDown = mouseButton & SDL_BUTTON_RIGHT;

    if (lMouseDown || rMouseDown)
        mSpp = 0;

    if (lMouseDown)
    {
        mCursorPosition[0] = float(x) / mWidth;
        mCursorPosition[1] = float(y) / mHeight;
    }
    else if (rMouseDown)
    {
        mCursorPosition[2] = float(x) / mWidth;
        mCursorPosition[3] = float(y) / mHeight;
    }

    PushConstants pushConstants = {
        GetElapseTime(),
        static_cast<float>(mSpp),
        static_cast<float>(mWidth),
        static_cast<float>(mHeight),
        mCursorPosition[0],
        mCursorPosition[1],
        mCursorPosition[2],
        mCursorPosition[3],
        float(lMouseDown),
        float(rMouseDown)};

    vkCmdPushConstants(mCommandBuffers[idx], mPipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(pushConstants), &pushConstants);

    if (frameOffset == 0)
        vkCmdBindDescriptorSets(mCommandBuffers[idx], VK_PIPELINE_BIND_POINT_GRAPHICS, mPipelineLayout, 0, 1, &mDescriptorSetA, 0, nullptr); // bind set A
    else
        vkCmdBindDescriptorSets(mCommandBuffers[idx], VK_PIPELINE_BIND_POINT_GRAPHICS, mPipelineLayout, 0, 1, &mDescriptorSetB, 0, nullptr); // bind set B

    // Bind the pathtracer pipeline object and render a full-screen quad
    vkCmdBindPipeline(mCommandBuffers[idx], VK_PIPELINE_BIND_POINT_GRAPHICS, mPipelinePathTrace);
    vkCmdDraw(mCommandBuffers[idx], 6, 1, 0, 0);

    // Begin the second subpass
    vkCmdNextSubpass(mCommandBuffers[idx], VK_SUBPASS_CONTENTS_INLINE);

    // Bind the composite pipeline object and render another full-screen quad
    vkCmdBindPipeline(mCommandBuffers[idx], VK_PIPELINE_BIND_POINT_GRAPHICS, mPipelineComposite);
    vkCmdDraw(mCommandBuffers[idx], 6, 1, 0, 0);

    // End the renderpass
    vkCmdWriteTimestamp(mCommandBuffers[idx], VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, mQueryPool, 1);
    vkCmdEndRenderPass(mCommandBuffers[idx]);
    vkEndCommandBuffer(mCommandBuffers[idx]);
}

void SoftRayTracingScene::OneTimeCommands(std::function<void(VkCommandBuffer const &)> func)
{
    VkCommandBuffer mOneTimeCommandBuffer;

    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.pNext = nullptr;
    allocInfo.commandBufferCount = 1;
    allocInfo.commandPool = mCommandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

    vkAllocateCommandBuffers(mDevice, &allocInfo, &mOneTimeCommandBuffer);

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;

    // Record user-defined commands in between begin/end
    vkBeginCommandBuffer(mOneTimeCommandBuffer, &beginInfo);

    func(mOneTimeCommandBuffer);

    vkEndCommandBuffer(mOneTimeCommandBuffer);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.pNext = nullptr;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &mOneTimeCommandBuffer;
    submitInfo.pSignalSemaphores = nullptr;
    submitInfo.signalSemaphoreCount = 0;
    submitInfo.waitSemaphoreCount = 0;
    submitInfo.pWaitSemaphores = nullptr;
    submitInfo.pWaitDstStageMask = nullptr;

    // One time submit, so wait idle here for the work to complete
    vkQueueSubmit(mQueue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(mQueue);

    vkFreeCommandBuffers(mDevice, mCommandPool, 1, &mOneTimeCommandBuffer);
}
void SoftRayTracingScene::SingleTimeCommands(std::function<void(VkCommandBuffer)> func)
{
    VkCommandBuffer mOneTimeCommandBuffer;

    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.pNext = nullptr;
    allocInfo.commandBufferCount = 1;
    allocInfo.commandPool = mCommandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

    vkAllocateCommandBuffers(mDevice, &allocInfo, &mOneTimeCommandBuffer);

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    // Record user-defined commands in between begin/end
    vkBeginCommandBuffer(mOneTimeCommandBuffer, &beginInfo);

    func(mOneTimeCommandBuffer);

    vkEndCommandBuffer(mOneTimeCommandBuffer);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.pNext = nullptr;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &mOneTimeCommandBuffer;
    submitInfo.pSignalSemaphores = nullptr;
    submitInfo.signalSemaphoreCount = 0;
    submitInfo.waitSemaphoreCount = 0;
    submitInfo.pWaitSemaphores = nullptr;
    submitInfo.pWaitDstStageMask = nullptr;

    // One time submit, so wait idle here for the work to complete
    vkQueueSubmit(mQueue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(mQueue);

    vkFreeCommandBuffers(mDevice, mCommandPool, 1, &mOneTimeCommandBuffer);
}

void SoftRayTracingScene::ClearPingPongImages()
{
    VkClearColorValue black = {0.0f, 0.0f, 0.0f, 1.0f};

    VkImageSubresourceRange subresourceRange{};
    subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    subresourceRange.baseMipLevel = 0;
    subresourceRange.layerCount = 1;
    subresourceRange.baseArrayLayer = 0;
    subresourceRange.layerCount = 1;

    // The memory barrier that will be used to transition each image from `vk::ImageLayout::eUndefined` to `vk::ImageLayout::eGeneral`,
    // which, per the spec, is required before attempting to clear the image
    VkImageMemoryBarrier imgBarrier{};
    imgBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    imgBarrier.pNext = nullptr;
    imgBarrier.image = mImageA;
    imgBarrier.subresourceRange = subresourceRange;
    imgBarrier.srcAccessMask = VK_ACCESS_NONE;
    imgBarrier.dstAccessMask = VK_ACCESS_NONE;
    imgBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    imgBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    imgBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imgBarrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;

    // Clear both A and B
    OneTimeCommands([&](const auto &cmd)
                    { 
                        vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, 0, 0, nullptr, 0, nullptr, 1, &imgBarrier); 
                        imgBarrier.image=mImageB;
                        vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, 0, 0, nullptr, 0, nullptr, 1, &imgBarrier); 

                        // Both images are now in `vk::ImageLayout::eGeneral`, due to the pipeline barrier above
                        vkCmdClearColorImage(cmd,mImageA,VK_IMAGE_LAYOUT_GENERAL,&black,1,&subresourceRange);
                        vkCmdClearColorImage(cmd,mImageB,VK_IMAGE_LAYOUT_GENERAL,&black,1,&subresourceRange); });
}