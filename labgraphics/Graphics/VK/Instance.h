#pragma once
#include <vulkan/vulkan.h>
#include <SDL.h>
#include <vector>
#include "Window.h"


class Instance
{
public:
    Instance(const std::vector<const char *> &validationLayers = {}, const std::vector<const char *>& extensions = {});
    Instance(const Window *window, const std::vector<const char *> &validationLayers = {}, const std::vector<const char *>& extensions = {});
    ~Instance();
    const VkInstance &GetHandle() const;

    const VkSurfaceKHR &GetSurface() const;

    const Window *GetWindow() const;

    const std::vector<const char *> &GetRequiredValidationLayers() const;

    bool IsHeadless() const;
private:
    bool IsValidationLayerAvailable(const char *layerName);

    const Window *mWindow;

    VkInstance mHandle = VK_NULL_HANDLE;
    VkSurfaceKHR mSurface = VK_NULL_HANDLE;
    VkDebugUtilsMessengerEXT mDebugUtils = VK_NULL_HANDLE;
    std::vector<const char *> mRequiredValidationLayers{};
    std::vector<const char *> mRequiredExtensions{};
};