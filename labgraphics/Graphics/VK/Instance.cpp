#include "Instance.h"
#include <iostream>
#include <SDL_vulkan.h>
#include "VK/Utils.h"
static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
    void *pUserData)
{
    std::string tags;

    switch (messageSeverity)
    {
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
        tags += "[ERROR]";
        break;
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
        tags += "[WARN]";
        break;
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
        tags += "[INFO]";
        break;
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
        tags += "[VERBOSE]";
        break;
    default:
        break;
    }

    switch (messageType)
    {
    case VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT:
        tags += "[GENERAL]";
        break;
    case VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT:
        tags += "[VALIDATION]";
        break;
    case VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT:
        tags += "[PERFORMANCE]";
        break;
    default:
        break;
    }

    switch (pCallbackData->pObjects->objectType)
    {
    case VK_OBJECT_TYPE_INSTANCE:
        tags += "[INSTANCE]";
        break;
    case VK_OBJECT_TYPE_PHYSICAL_DEVICE:
        tags += "[PHYSICAL_DEVICE]";
        break;
    case VK_OBJECT_TYPE_DEVICE:
        tags += "[DEVICE]";
        break;
    case VK_OBJECT_TYPE_QUEUE:
        tags += "[QUEUE]";
        break;
    case VK_OBJECT_TYPE_SEMAPHORE:
        tags += "[SEMAPHORE]";
        break;
    case VK_OBJECT_TYPE_COMMAND_BUFFER:
        tags += "[COMMAND_BUFFER]";
        break;
    case VK_OBJECT_TYPE_FENCE:
        tags += "[FENCE]";
        break;
    case VK_OBJECT_TYPE_DEVICE_MEMORY:
        tags += "[DEVICE_MEMORY]";
        break;
    case VK_OBJECT_TYPE_BUFFER:
        tags += "[BUFFER]";
        break;
    case VK_OBJECT_TYPE_IMAGE:
        tags += "[IMAGE]";
        break;
    case VK_OBJECT_TYPE_EVENT:
        tags += "[EVENT]";
        break;
    case VK_OBJECT_TYPE_QUERY_POOL:
        tags += "[QUERY_POOL]";
        break;
    case VK_OBJECT_TYPE_BUFFER_VIEW:
        tags += "[BUFFER_VIEW]";
        break;
    case VK_OBJECT_TYPE_IMAGE_VIEW:
        tags += "[IMAGE_VIEW]";
        break;
    case VK_OBJECT_TYPE_SHADER_MODULE:
        tags += "[SHADER_MODULE]";
        break;
    case VK_OBJECT_TYPE_PIPELINE_CACHE:
        tags += "[PIPELINE_CACHE]";
        break;
    case VK_OBJECT_TYPE_PIPELINE_LAYOUT:
        tags += "[PIPELINE_LAYOUT]";
        break;
    case VK_OBJECT_TYPE_RENDER_PASS:
        tags += "[RENDER_PASS]";
        break;
    case VK_OBJECT_TYPE_PIPELINE:
        tags += "[PIPELINE]";
        break;
    case VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT:
        tags += "[DESCRIPTOR_SET_LAYOUT]";
        break;
    case VK_OBJECT_TYPE_SAMPLER:
        tags += "[SAMPLER]";
        break;
    case VK_OBJECT_TYPE_DESCRIPTOR_POOL:
        tags += "[DESCRIPTOR_POOL]";
        break;
    case VK_OBJECT_TYPE_DESCRIPTOR_SET:
        tags += "[DESCRIPTOR_SET]";
        break;
    case VK_OBJECT_TYPE_FRAMEBUFFER:
        tags += "[FRAMEBUFFER]";
        break;
    case VK_OBJECT_TYPE_COMMAND_POOL:
        tags += "[COMMAND_POOL]";
        break;
    case VK_OBJECT_TYPE_SURFACE_KHR:
        tags += "[SURFACE_KHR]";
        break;
    case VK_OBJECT_TYPE_SWAPCHAIN_KHR:
        tags += "[SWAPCHAIN_KHR]";
        break;
    case VK_OBJECT_TYPE_DISPLAY_KHR:
        tags += "[DISPLAY_KHR]";
        break;
    case VK_OBJECT_TYPE_DISPLAY_MODE_KHR:
        tags += "[DISPLAY_MODE_KHR]";
        break;
    default:
        break;
    }

    std::cerr << "\033[0m\033[1;31m" << tags << ":" << pCallbackData->pMessage << "\n\033[0m" << std::endl;

    return VK_FALSE;
}

static VkResult CreateDebugUtilsMessengerEXT(
    VkInstance instance,
    const VkDebugUtilsMessengerCreateInfoEXT *pCreateInfo,
    const VkAllocationCallbacks *pAllocator,
    VkDebugUtilsMessengerEXT *pDebugMessenger)
{
    auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
    if (func != nullptr)
        return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
    else
        return VK_ERROR_EXTENSION_NOT_PRESENT;
}

static void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks *pAllocator)
{
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
    if (func != nullptr)
        func(instance, debugMessenger, pAllocator);
}

Instance::Instance(const std::vector<const char *> &validationLayers, const std::vector<const char *> &extensions)
    : mWindow(nullptr), mSurface(VK_NULL_HANDLE)
{
    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = nullptr;
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = nullptr;
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_3;

    VkInstanceCreateInfo instInfo{};
    instInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instInfo.pApplicationInfo = &appInfo;
    instInfo.enabledLayerCount = 0;
    instInfo.ppEnabledLayerNames = nullptr;
    instInfo.enabledExtensionCount = 0;
    instInfo.ppEnabledExtensionNames = nullptr;

    std::vector<VkExtensionProperties> instanceExtensionProps = GetInstanceExtensionProps();

    bool extSatisfied = CheckExtensionSupport(extensions, instanceExtensionProps);

    if (extSatisfied)
    {
        instInfo.enabledExtensionCount = extensions.size();
        instInfo.ppEnabledExtensionNames = extensions.data();
    }

#if _DEBUG
    instInfo.enabledLayerCount = mRequiredValidationLayers.size();
    instInfo.ppEnabledLayerNames = mRequiredValidationLayers.data();
    if (!CheckValidationLayerSupport(mRequiredValidationLayers, GetInstanceLayerProps()))
        std::cout << "Lack of necessary validation layer" << std::endl;
#endif

    VK_CHECK(vkCreateInstance(&instInfo, nullptr, &mHandle));

    VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
    debugCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    debugCreateInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT;
    debugCreateInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT;
    debugCreateInfo.pfnUserCallback = debugCallback;
    debugCreateInfo.pUserData = nullptr;
    debugCreateInfo.flags = 0;
    debugCreateInfo.pNext = nullptr;

    VK_CHECK(CreateDebugUtilsMessengerEXT(mHandle, &debugCreateInfo, nullptr, &mDebugUtils));
}

Instance::Instance(const Window *window, const std::vector<const char *> &validationLayers, const std::vector<const char *> &extensions)
    : mWindow(window), mRequiredValidationLayers(validationLayers)
{
    std::vector<const char *> requiredInstanceExtensions = window->GetRequiredVulkanExtensions();
    requiredInstanceExtensions.insert(requiredInstanceExtensions.end(),extensions.begin(),extensions.end());

    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = nullptr;
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = nullptr;
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_3;

    VkInstanceCreateInfo instInfo{};
    instInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instInfo.pApplicationInfo = &appInfo;
    instInfo.enabledLayerCount = 0;
    instInfo.ppEnabledLayerNames = nullptr;
    instInfo.enabledExtensionCount = 0;
    instInfo.ppEnabledExtensionNames = nullptr;

    std::vector<VkExtensionProperties> instanceExtensionProps = GetInstanceExtensionProps();

    bool extSatisfied = CheckExtensionSupport(requiredInstanceExtensions, instanceExtensionProps);

    if (extSatisfied)
    {
        instInfo.enabledExtensionCount = requiredInstanceExtensions.size();
        instInfo.ppEnabledExtensionNames = requiredInstanceExtensions.data();
    }

#if _DEBUG
    instInfo.enabledLayerCount = mRequiredValidationLayers.size();
    instInfo.ppEnabledLayerNames = mRequiredValidationLayers.data();
    if (!CheckValidationLayerSupport(mRequiredValidationLayers, GetInstanceLayerProps()))
        std::cout << "Lack of necessary validation layer" << std::endl;
#endif

    VK_CHECK(vkCreateInstance(&instInfo, nullptr, &mHandle));

    mSurface = window->GetSurface(mHandle);

#if _DEBUG

    VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
    debugCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    debugCreateInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT;
    debugCreateInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT;
    debugCreateInfo.pfnUserCallback = debugCallback;
    debugCreateInfo.pUserData = nullptr;
    debugCreateInfo.flags = 0;
    debugCreateInfo.pNext = nullptr;

    VK_CHECK(CreateDebugUtilsMessengerEXT(mHandle, &debugCreateInfo, nullptr, &mDebugUtils));
#endif
}
Instance::~Instance()
{
#if _DEBUG
    DestroyDebugUtilsMessengerEXT(mHandle, mDebugUtils, nullptr);
#endif
    vkDestroySurfaceKHR(mHandle, mSurface, nullptr);
    vkDestroyInstance(mHandle, nullptr);
}

const VkInstance &Instance::GetHandle() const
{
    return mHandle;
}

const VkSurfaceKHR &Instance::GetSurface() const
{
    return mSurface;
}

const Window *Instance::GetWindow() const
{
    return mWindow;
}

const std::vector<const char *> &Instance::GetRequiredValidationLayers() const
{
    return mRequiredValidationLayers;
}

bool Instance::IsHeadless() const
{
    return mWindow == nullptr;
}

bool Instance::IsValidationLayerAvailable(const char *layerName)
{
    uint32_t propertyCount = 0;
    VK_CHECK(vkEnumerateInstanceLayerProperties(&propertyCount, nullptr));
    std::vector<VkLayerProperties> properties(propertyCount);
    VK_CHECK(vkEnumerateInstanceLayerProperties(&propertyCount, properties.data()));
    for (uint32_t i = 0; i < properties.size(); ++i)
    {
        if (strcmp(layerName, properties[i].layerName) == 0)
            return true;
    }

    return false;
}