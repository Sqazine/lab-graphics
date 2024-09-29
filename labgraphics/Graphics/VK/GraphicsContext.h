#pragma once
#include <memory>
#include "VK/Instance.h"
#include "VK/Device.h"
#include "VK/RenderPass.h"
#include "VK/Framebuffer.h"

const std::vector<const char *> gInstanceExtensions = {
#ifdef _DEBUG
    VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
#endif
    VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME};

const std::vector<const char *> gValidationLayers = {
    "VK_LAYER_KHRONOS_validation",
    "VK_LAYER_LUNARG_monitor"};

class GraphicsContext
{
public:
    GraphicsContext();
    ~GraphicsContext();

    Instance *GetInstance() const;
    Device *GetDevice() const;
    SwapChain *GetSwapChain();

private:
    std::unique_ptr<Instance> mInstance;
    std::unique_ptr<Device> mDevice;

    std::unique_ptr<SwapChain> mSwapChain;
};